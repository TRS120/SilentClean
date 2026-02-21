#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// --- Native API Definitions ---
typedef enum _SYSTEM_MEMORY_LIST_COMMAND {
    MemoryFlushWorkingSets = 2,
    MemoryFlushModifiedList = 3,
    MemoryFlushStandbyList = 4,
    MemoryPurgeLowPriorityStandbyList = 5,
    MemoryPurgeStandbyList = 6
} SYSTEM_MEMORY_LIST_COMMAND;

typedef LONG (WINAPI *NtSetSystemInformation)(
    INT SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength
);

// --- 1. RAM Cleaning Logic ---
void EmptyStandbyList() {
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        auto setInfo = (NtSetSystemInformation)GetProcAddress(ntdll, "NtSetSystemInformation");
        if (setInfo) {
            SYSTEM_MEMORY_LIST_COMMAND command;
            command = MemoryFlushStandbyList;
            setInfo(80, &command, sizeof(command));
            command = MemoryPurgeLowPriorityStandbyList;
            setInfo(80, &command, sizeof(command));
        }
    }
}

// --- 2. Safe File Cleaning ---
void CleanDirectory(const std::wstring& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) return;
    auto options = fs::directory_options::skip_permission_denied;
    for (auto it = fs::directory_iterator(path, options, ec); it != fs::end(it); it.increment(ec)) {
        if (ec) break;
        try {
            fs::remove_all(it->path(), ec);
        } catch (...) {}
    }
}

// --- 3. Registry Cleanup ---
void CleanRegistry(HKEY hKeyRoot, const std::wstring& subKey) {
    RegDeleteTreeW(hKeyRoot, subKey.c_str());
}

// --- Main Entry ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::string cmdLine = lpCmdLine ? lpCmdLine : "";
    if (cmdLine.find("-et") != std::string::npos) {
        EmptyStandbyList();
        return 0;
    }

    EmptyStandbyList();
    
DWORD bufferSize = GetTempPathW(0, NULL);
if (bufferSize > 0) {
    std::wstring tempPath(bufferSize, L'\0');
    DWORD len = GetTempPathW(bufferSize, &tempPath[0]);
    if (len > 0 && len < bufferSize) {
        tempPath.resize(len);
        std::vector<std::wstring> targets = {
            tempPath,
            L"C:\\Windows\\Temp",
            L"C:\\Windows\\Prefetch",
        };
        for (const auto& path : targets) {
            CleanDirectory(path);
        }
    }
}
    CleanRegistry(HKEY_CLASSES_ROOT, L"Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");
    CleanRegistry(HKEY_CURRENT_USER, L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");
    return 0;
}
