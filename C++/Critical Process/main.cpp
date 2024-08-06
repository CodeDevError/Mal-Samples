#include <iostream>
#include <string>
#include <windows.h>
#include <tlhelp32.h>

#pragma comment(lib, "ntdll.lib")

extern "C" NTSTATUS NTAPI RtlAdjustPrivilege(
    ULONG Privilege,
    BOOLEAN Enable,
    BOOLEAN CurrentThread,
    PBOOLEAN Enabled
);

extern "C" NTSTATUS NTAPI NtSetInformationProcess(
    HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength
);

#define ProcessBreakOnTermination 0x1D

DWORD GetProcessIdByName(const std::wstring& processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        do {
            std::wstring exeFileName(processEntry.szExeFile, processEntry.szExeFile + strlen(processEntry.szExeFile));
            if (processName == exeFileName) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    return 0;
}

int main() {
    std::wstring processName;
    std::wcout << L"Enter the process name (e.g., notepad.exe): ";
    std::getline(std::wcin, processName);

    DWORD processId = GetProcessIdByName(processName);
    if (processId == 0) {
        std::wcout << L"Process not found." << std::endl;
        std::wcout << L"Press Enter to close.";
        std::wcin.get();
        return 1;
    }

    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!processHandle) {
        std::wcout << L"Failed to open process." << std::endl;
        std::wcout << L"Press Enter to close.";
        std::wcin.get();
        return 1;
    }

    BOOLEAN enabled;
    NTSTATUS status = RtlAdjustPrivilege(20, TRUE, FALSE, &enabled);
    if (status != 0) {
        std::wcout << L"Failed to adjust privilege." << std::endl;
        CloseHandle(processHandle);
        std::wcout << L"Press Enter to close.";
        std::wcin.get();
        return 1;
    }

    ULONG breakOnTermination = 1;
    status = NtSetInformationProcess(processHandle, ProcessBreakOnTermination, &breakOnTermination, sizeof(ULONG));
    if (status != 0) {
        std::wcout << L"Failed to set process information." << std::endl;
        CloseHandle(processHandle);
        std::wcout << L"Press Enter to close.";
        std::wcin.get();
        return 1;
    }

    std::wcout << L"Process made critical. Terminating this process will cause a BSOD." << std::endl;
    std::wcout << L"Press Enter to close.";
    std::wcin.get();

    CloseHandle(processHandle);
    return 0;
}
