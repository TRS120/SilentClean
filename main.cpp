#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// --- Memory Cleaning Logic (Empty Standby List) ---
void EmptyStandbyList() {
    // Requires SE_PROF_SINGLE_PROCESS_NAME privilege for some memory operations
    // but SetProcessWorkingSetSize works for the current process.
    // For a global standby list flush, we mimic the behavior of memory management tools.
    
    // 1. Clear Working Sets
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    // 2. Attempt to flush system-wide standby list (requires admin)
    // This is a simplified version of what EmptyStandbyList.exe does
    typedef LONG (WINAPI *NtSetSystemInformation)(INT, PVOID, ULONG);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        auto setInfo = (NtSetSystemInformation)GetProcAddress(ntdll, "NtSetSystemInformation");
        if (setInfo) {
            // SystemMemoryListInformation = 80
            // Command to flush: 4 (StandbyList)
            SYSTEM_MEMORY_LIST_COMMAND command = MemoryFlushStandbyList;
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
    std::string cmdLine = lpCmdLine;

    // Check for "-et" argument (Empty Standby List Only)
    if (cmdLine.find("-et") != std::string::npos) {
        EmptyStandbyList();
        return 0;
    }

    // Default: Full Cleanup
    // 1. Memory
    EmptyStandbyList();

    // 2. Files
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

    // 3. Registry
    CleanRegistry(HKEY_CLASSES_ROOT, L"Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");
    CleanRegistry(HKEY_CURRENT_USER, L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");

    return 0;
}
