#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

DWORD GetProcessIdByName(const char* processName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot of processes." << std::endl;
        return 0;
    }

    if (Process32First(snapshot, &entry) == TRUE) {
        do {
            if (_wcsicmp(entry.szExeFile, std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(processName).c_str()) == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &entry) == TRUE);
    }

    CloseHandle(snapshot);
    return 0;
}

bool InjectDLL(DWORD processId, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Failed to open target process." << std::endl;
        return false;
    }

    LPVOID pDllPath = VirtualAllocEx(hProcess, 0, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (pDllPath == NULL) {
        std::cerr << "Failed to allocate memory in target process." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (WriteProcessMemory(hProcess, pDllPath, (LPVOID)dllPath, strlen(dllPath) + 1, NULL) == 0) {
        std::cerr << "Failed to write to target process memory." << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pDllPath, 0, NULL);
    if (hThread == NULL) {
        std::cerr << "Failed to create remote thread in target process." << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: Injector <target_process_name> <path_to_dll>" << std::endl;
        return 1;
    }

    const char* targetProcessName = argv[1];
    const char* dllPath = argv[2];

    DWORD processId = GetProcessIdByName(targetProcessName);
    if (processId == 0) {
        std::cerr << "Could not find target process." << std::endl;
        return 1;
    }

    if (InjectDLL(processId, dllPath)) {
        std::cout << "DLL injected successfully." << std::endl;
    } else {
        std::cerr << "DLL injection failed." << std::endl;
    }

    return 0;
}
