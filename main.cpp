#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Function to run commands hidden
void RunHiddenCommand(const std::wstring& cmd) {
    std::wstring fullCmd = L"/c " + cmd;
    ShellExecuteW(NULL, L"open", L"cmd.exe", fullCmd.c_str(), NULL, SW_HIDE);
}

// Function to take ownership silently
bool SecurePermissions(const std::wstring& path) {
    std::wstring takeownCmd = L"takeown /f \"" + path + L"\" /r /d y >nul 2>&1";
    std::wstring icaclsCmd = L"icacls \"" + path + L"\" /grant administrators:F /t /q >nul 2>&1";

    RunHiddenCommand(takeownCmd);
    RunHiddenCommand(icaclsCmd);
    return true;
}

// Safely delete a directory's contents silently
void CleanDirectory(const std::wstring& path) {
    try {
        if (!fs::exists(path)) return;
        SecurePermissions(path);
        
        // Brief pause to allow permissions to propagate
        Sleep(500);

        for (const auto& entry : fs::directory_iterator(path)) {
            try {
                fs::remove_all(entry.path());
            } catch (...) {
                // Ignore errors (files in use)
            }
        }
    } catch (...) {
        // Ignore access errors
    }
}

// Clean Registry Key silently
void CleanRegistry(HKEY hKeyRoot, const std::wstring& subKey) {
    RegDeleteTreeW(hKeyRoot, subKey.c_str());
}

// WINAPI WinMain ensures NO console window pops up
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Get Environment Paths
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath)) {
        std::wstring userTemp = tempPath;
        
        // 2. Define target folders
        std::vector<std::wstring> targets = {
            userTemp,
            L"C:\\Windows\\Temp",
            L"C:\\Windows\\Prefetch",
            L"C:\\ProgramData\\NVIDIA Corporation\\NV_Cache"
        };

        // 3. Process Directories
        for (const auto& path : targets) {
            CleanDirectory(path);
        }
    }

    // 4. Process Registry
    CleanRegistry(HKEY_CLASSES_ROOT, L"Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");
    CleanRegistry(HKEY_CURRENT_USER, L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");

    return 0;
}
