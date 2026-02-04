#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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

void EmptyStandbyList() {
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        auto setInfo = (NtSetSystemInformation)GetProcAddress(ntdll, "NtSetSystemInformation");
        if (setInfo) {
            SYSTEM_MEMORY_LIST_COMMAND command;

            command = MemoryFlushWorkingSets;
            setInfo(80, &command, sizeof(command));

            command = MemoryFlushModifiedList;
            setInfo(80, &command, sizeof(command));

            command = MemoryFlushStandbyList;
            setInfo(80, &command, sizeof(command));

            command = MemoryPurgeLowPriorityStandbyList;
            setInfo(80, &command, sizeof(command));
        }
    }
}

void CleanDirectory(const std::wstring& path) {
    try {
        if (!fs::exists(path)) return;

        for (const auto& entry : fs::directory_iterator(path)) {
            try {
                fs::remove_all(entry.path());
            } catch (...) {ে
            }
        }
    } catch (...) {}
}

void CleanRegistry(HKEY hKeyRoot, const std::wstring& subKey) {
    RegDeleteTreeW(hKeyRoot, subKey.c_str());
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    std::string cmdLine = lpCmdLine ? lpCmdLine : "";
ে
    if (cmdLine.find("-et") != std::string::npos) {
        EmptyStandbyList();
        return 0;
    }

    EmptyStandbyList();

    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath)) {
        std::vector<std::wstring> targets = {
            tempPath,
            L"C:\\Windows\\Temp",
            L"C:\\Windows\\Prefetch",
            L"C:\\ProgramData\\NVIDIA Corporation\\NV_Cache"
        };
        for (const auto& path : targets) {
            CleanDirectory(path);
        }
    }

    CleanRegistry(HKEY_CLASSES_ROOT, L"Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");
    CleanRegistry(HKEY_CURRENT_USER, L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");

    return 0;
}
