#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// --- Manual Definitions for Native API ---
// These are required because they are not in standard windows.h
typedef enum _SYSTEM_MEMORY_LIST_COMMAND {
    MemoryCaptureFilledProcessWorkingSets = 1,
    MemoryFlushModifiedList = 2,
    MemoryFlushStandbyList = 3, // Command used to empty standby list
    MemoryPurgeLowPriorityStandbyList = 4,
    MemoryPurgeStandbyList = 5
} SYSTEM_MEMORY_LIST_COMMAND;

typedef LONG (WINAPI *NtSetSystemInformation)(
    INT SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength
);

// --- Memory Cleaning Logic ---
void EmptyStandbyList() {
    // 1. Clear current process working set
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    // 2. Flush System Standby List
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        auto setInfo = (NtSetSystemInformation)GetProcAddress(ntdll, "NtSetSystemInformation");
        if (setInfo) {
            SYSTEM_MEMORY_LIST_COMMAND command = MemoryFlushStandbyList;
            // SystemMemoryListInformation class is 80
            setInfo(80, &command, sizeof(command));
        }
    }
}

// --- File & Registry Cleanup ---
void RunHiddenCommand(const std::wstring& cmd) {
    std::wstring fullCmd = L"/c " + cmd;
    ShellExecuteW(NULL, L"open", L"cmd.exe", fullCmd.c_str(), NULL, SW_HIDE);
}

void SecurePermissions(const std::wstring& path) {
    std::wstring takeownCmd = L"takeown /f \"" + path + L"\" /r /d y >nul 2>&1";
    std::wstring icaclsCmd = L"icacls \"" + path + L"\" /grant administrators:F /t /q >nul 2>&1";
    RunHiddenCommand(takeownCmd);
    RunHiddenCommand(icaclsCmd);
}

void CleanDirectory(const std::wstring& path) {
    try {
        if (!fs::exists(path)) return;
        SecurePermissions(path);
        Sleep(300);
        for (const auto& entry : fs::directory_iterator(path)) {
            try { fs::remove_all(entry.path()); } catch (...) {}
        }
    } catch (...) {}
}

void CleanRegistry(HKEY hKeyRoot, const std::wstring& subKey) {
    RegDeleteTreeW(hKeyRoot, subKey.c_str());
}

// --- Main Entry ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::string cmdLine = lpCmdLine ? lpCmdLine : "";

    // Check for "-et" argument (Empty Standby List Only)
    if (cmdLine.find("-et") != std::string::npos) {
        EmptyStandbyList();
        return 0;
    }

    // Default: Full Cleanup
    EmptyStandbyList();

    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath)) {
        std::vector<std::wstring> targets = {
            tempPath,
            L"C:\\Windows\\Temp",
            L"C:\\Windows\\Prefetch",
            L"C:\\ProgramData\\NVIDIA Corporation\\NV_Cache"
        };
        for (const auto& path : targets) CleanDirectory(path);
    }

    CleanRegistry(HKEY_CLASSES_ROOT, L"Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");
    CleanRegistry(HKEY_CURRENT_USER, L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");

    return 0;
}
