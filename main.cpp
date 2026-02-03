#include <windows.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Function to take ownership and grant full access (Mimics takeown + icacls)
bool SecurePermissions(const std::wstring& path) {
    // Note: In C++, we can use the Shell to run takeown/icacls silently
    // or use SetNamedSecurityInfo. For simplicity and reliability in a 
    // maintenance tool, calling the system utilities hidden is often preferred.
    
    std::wstring takeownCmd = L"takeown /f \"" + path + L"\" /r /d y >nul 2>&1";
    std::wstring icaclsCmd = L"icacls \"" + path + L"\" /grant administrators:F /t /q >nul 2>&1";

    _wsystem(takeownCmd.c_str());
    _wsystem(icaclsCmd.c_str());
    return true;
}

// Safely delete a directory's contents
void CleanDirectory(const std::wstring& path) {
    try {
        if (!fs::exists(path)) return;

        std::wcout << L"[...] Cleaning: " << path << std::endl;
        SecurePermissions(path);

        for (const auto& entry : fs::directory_iterator(path)) {
            try {
                fs::remove_all(entry.path());
            } catch (const std::exception& e) {
                // Files in use (like logs) will naturally fail; we skip them
            }
        }
        std::wcout << L"[OK] Cleaned: " << path << std::endl;
    } catch (...) {
        std::wcerr << L"[ERROR] Could not access: " << path << std::endl;
    }
}

// Clean Registry Key (Mimics reg delete)
void CleanRegistry(HKEY hKeyRoot, const std::wstring& subKey) {
    LSTATUS status = RegDeleteTreeW(hKeyRoot, subKey.c_str());
    if (status == ERROR_SUCCESS) {
        std::wcout << L"[OK] Registry Cleaned: " << subKey << std::endl;
    } else if (status != ERROR_FILE_NOT_FOUND) {
        std::wcerr << L"[!] Registry Key busy or access denied: " << subKey << std::endl;
    }
}

int main() {
    // 1. Get Environment Paths
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring userTemp = tempPath;
    
    // 2. Define target folders
    std::vector<std::wstring> targets = {
        userTemp,
        L"C:\\Windows\\Temp",
        L"C:\\Windows\\Prefetch",
        L"C:\\ProgramData\\NVIDIA Corporation\\NV_Cache"
    };

    std::cout << "Starting System Deep Clean..." << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    // 3. Process Directories
    for (const auto& path : targets) {
        CleanDirectory(path);
    }

    // 4. Process Registry
    std::cout << "Cleaning Shell MuiCache..." << std::endl;
    CleanRegistry(HKEY_CLASSES_ROOT, L"Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");
    CleanRegistry(HKEY_CURRENT_USER, L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");

    std::cout << "---------------------------------------" << std::endl;
    std::cout << "Cleanup Complete!" << std::endl;
    
    return 0;
}
