#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#include <iostream>
#include <string>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "advapi32.lib")

bool RunFileWithTaskScheduler(const std::wstring& filePath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM library. Error code: " << hr << std::endl;
        return false;
    }

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM security. Error code: " << hr << std::endl;
        CoUninitialize();
        return false;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create an instance of ITaskService. Error code: " << hr << std::endl;
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        std::wcerr << L"ITaskService::Connect failed. Error code: " << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get Root Folder pointer. Error code: " << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Delete the task if it exists
    pRootFolder->DeleteTask(_bstr_t(L"RunFileWithElevatedPrivileges"), 0);

    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create a task definition. Error code: " << hr << std::endl;
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IRegistrationInfo* pRegInfo = NULL;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get identification pointer. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pRegInfo->put_Author(_bstr_t(L"Administrator"));
    pRegInfo->Release();
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put identification info. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get principal pointer. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put principal info. Error code: " << hr << std::endl;
        pPrincipal->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
    pPrincipal->Release();
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put run level. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskSettings* pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get settings pointer. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
    pSettings->Release();
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put setting info. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IActionCollection* pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get task collection pointer. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IAction* pAction = NULL;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    pActionCollection->Release();
    if (FAILED(hr)) {
        std::wcerr << L"Cannot create the action. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IExecAction* pExecAction = NULL;
    hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    pAction->Release();
    if (FAILED(hr)) {
        std::wcerr << L"QueryInterface call failed for IExecAction. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pExecAction->put_Path(_bstr_t(filePath.c_str()));
    pExecAction->Release();
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put the path of executable. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(_bstr_t(L"RunFileWithElevatedPrivileges"), pTask, TASK_CREATE_OR_UPDATE, _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &pRegisteredTask);
    if (FAILED(hr)) {
        std::wcerr << L"Error saving the Task : " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Run the task
    IRunningTask* pRunningTask = NULL;
    hr = pRegisteredTask->Run(_variant_t(), &pRunningTask);
    if (FAILED(hr)) {
        std::wcerr << L"Error running the Task : " << hr << std::endl;
        pRegisteredTask->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    std::wcout << L"Task successfully registered and executed." << std::endl;

    // Wait for a reasonable amount of time for the task to complete
    Sleep(10000); // Wait for 10 seconds

    pRunningTask->Release();

    pRegisteredTask->Release();
    pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    // Delete the task
    hr = pRootFolder->DeleteTask(_bstr_t(L"RunFileWithElevatedPrivileges"), 0);
    if (FAILED(hr)) {
        std::wcerr << L"Error deleting the Task : " << hr << std::endl;
    }

    return true;
}

int wmain() {
    std::wstring filePath;
    std::wcout << L"Enter the file path to run as admin: ";
    std::getline(std::wcin, filePath);

    if (!RunFileWithTaskScheduler(filePath)) {
        std::wcerr << L"Failed to run the file as admin.\n";
    }
    else {
        std::wcout << L"File is running as admin.\n";
    }

    return 0;
}
