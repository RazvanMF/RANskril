// https://developercommunity.visualstudio.com/t/Access-violation-with-std::mutex::lock-a/10664660
// on god, the WORST C++ experience i ever had.
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
#include "TRIUNEJDG.h"
#include "logger.h"
#include "utils.h"

ETWGlobalData gETWesqueGlobalData;

TRIUNEJDGsystem::TRIUNEJDGsystem() {
	amsiModule = new AMSImodule();
	etwModule = new ETWmodule();
	wvtModule = new WVTmodule();
}

TRIUNEJDGsystem::~TRIUNEJDGsystem() {
	delete amsiModule;
	delete etwModule;
	delete wvtModule;
}

TRIUNEJDGsystem& TRIUNEJDGsystem::getInstance() {
	static TRIUNEJDGsystem instance;
	return instance;
}

DWORD TRIUNEJDGsystem::GetProcessBuffer(ULONG_PTR processPID, PVOID buffer, SIZE_T inputSize, SIZE_T& outputSize)
{
	WCHAR logMessage[256];

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processPID);
	if (hProcess == NULL) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT OPENING PROCESS %llu FOR BUFFER", processPID);
		Utils::LogEvent(std::wstring{logMessage}, SERVICE_IDENTIFIER, TRIUNEJDG_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return errorCode;
	}

	HMODULE hProcessModules[256];
	DWORD noModules;

	DWORD result = EnumProcessModulesEx(hProcess, hProcessModules, sizeof(hProcessModules), &noModules, LIST_MODULES_ALL);
	if (result == 0) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT ENUMERATING PROCESS MODULES FOR %llu", processPID);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		CloseHandle(hProcess);
		return errorCode;
	}

	MODULEINFO moduleInfo = { 0 };
	result = GetModuleInformation(hProcess, hProcessModules[0], &moduleInfo, sizeof(moduleInfo));
	if (result == 0) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT GETTING MAIN MODULE INFORMATION FOR %llu", processPID);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		CloseHandle(hProcess);
		return errorCode;
	}

	SIZE_T bytesRead = 0;
	LPVOID baseAddress = moduleInfo.lpBaseOfDll;
	SIZE_T imageSize = moduleInfo.SizeOfImage;
	BOOL readResult = ReadProcessMemory(hProcess, baseAddress, buffer, min(inputSize, imageSize), &bytesRead);
	if (!readResult) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT READING PROCESS MEMORY FOR %llu", processPID);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		CloseHandle(hProcess);
		return errorCode;
	}

	CloseHandle(hProcess);
	return ERROR_SUCCESS;
}

DWORD TRIUNEJDGsystem::GetProcessImageName(ULONG_PTR processPID, LPCWSTR buffer, ULONG inputSize)
{
	WCHAR logMessage[256];

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processPID);
	if (hProcess == NULL) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT OPENING PROCESS %llu FOR IMAGE NAME", processPID);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return errorCode;
	}

	SIZE_T bytesRead;
	BOOL result = QueryFullProcessImageNameW(hProcess, 0, (LPWSTR)buffer, &inputSize);
	if (!result) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT QUERYING PROCESS IMAGE NAME FOR %llu", processPID);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		CloseHandle(hProcess);
		return errorCode;
	}

	CloseHandle(hProcess);
	return ERROR_SUCCESS;
}


AMSImodule::AMSImodule() {
	WCHAR logMessage[256];

	amsiModuleName = L"RANskrilAMSI";
	HRESULT result = AmsiInitialize(amsiModuleName.c_str(), &amsiContext);
	if (FAILED(result)) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT INITIALIZING AMSI MODULE");
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_AMSI_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return;
	}
	result = AmsiOpenSession(amsiContext, &amsiSession);
	if (FAILED(result)) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT OPENING AMSI SESSION");
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_AMSI_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return;
	}
}

AMSImodule::~AMSImodule()
{
	AmsiCloseSession(amsiContext, amsiSession);
	AmsiUninitialize(amsiContext);
}

AMSI_RESULT AMSImodule::ScanBuffer(PVOID buffer, UINT32 bufferSize, LPCWSTR filePath)
{
	AMSI_RESULT result;
	AmsiScanBuffer(amsiContext, buffer, bufferSize, filePath, amsiSession, &result);
	return result;
}

AMSI_RESULT AMSImodule::ScanString(LPCWSTR buffer, LPCWSTR filePath)
{
	AMSI_RESULT result;
	AmsiScanString(amsiContext, buffer, filePath, amsiSession, &result);
	return result;
}

BOOL AMSImodule::ResultIsMalware(AMSI_RESULT result)
{
	return AmsiResultIsMalware(result);
}


DWORD WVTmodule::RunWinVerifyTrust(ULONG_PTR pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (hProcess == NULL) {
		int errorCode = GetLastError();
		WCHAR logMessage[256];
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT OPENING PROCESS %llu FOR WINVERIFYTRUST", pid);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_WVT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	WCHAR processName[MAX_PATH];
	DWORD processNameSize = MAX_PATH;
	BOOL result = QueryFullProcessImageNameW(hProcess, 0, processName, &processNameSize);
	if (!result) {
		int errorCode = GetLastError();
		WCHAR logMessage[256];
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT QUERYING PROCESS IMAGE NAME FOR %llu", pid);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_WVT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		CloseHandle(hProcess);
		return errorCode;
	}
	CloseHandle(hProcess);

	WINTRUST_FILE_INFO winTrustFileData = { 0 };
	memset(&winTrustFileData, 0, sizeof(WINTRUST_FILE_INFO));
	winTrustFileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
	winTrustFileData.pcwszFilePath = processName;
	winTrustFileData.hFile = NULL;
	winTrustFileData.pgKnownSubject = NULL;

	WINTRUST_DATA winTrustData = { 0 };
	memset(&winTrustData, 0, sizeof(WINTRUST_DATA));
	winTrustData.cbStruct = sizeof(WINTRUST_DATA);
	winTrustData.pPolicyCallbackData = NULL;
	winTrustData.pSIPClientData = NULL;
	winTrustData.dwUIChoice = WTD_UI_NONE;
	winTrustData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
	winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
	winTrustData.pFile = &winTrustFileData;
	winTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
	winTrustData.hWVTStateData = NULL;
	winTrustData.pwszURLReference = NULL;
	winTrustData.dwProvFlags = WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT | WTD_USE_DEFAULT_OSVER_CHECK;

	GUID policyGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	DWORD verdict = WinVerifyTrust(NULL, &policyGuid, &winTrustData);

	winTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust(NULL, &policyGuid, &winTrustData);

	return verdict;
}

BOOLEAN WVTmodule::IsAMicrosoftProcess(ULONG_PTR pid)
{
	WCHAR logMessage[256];

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (hProcess == NULL) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT OPENING PROCESS %llu FOR WVT Microsoft Check", pid);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_WVT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return FALSE;
	}
	WCHAR processName[MAX_PATH];
	DWORD processNameSize = MAX_PATH;

	BOOL result = QueryFullProcessImageNameW(hProcess, 0, processName, &processNameSize);
	if (!result) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT QUERYING PROCESS IMAGE NAME FOR %llu", pid);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_WVT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		CloseHandle(hProcess);
		return FALSE;
	}
	CloseHandle(hProcess);

	// get the version info's size
	DWORD versionInfoSize = GetFileVersionInfoSizeW(processName, NULL);
	if (versionInfoSize == 0) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO GET FILE VERSION INFO SIZE");
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_WVT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return FALSE;
	}

	// get the version info
	LPVOID versionInfo = malloc(versionInfoSize);
	if (versionInfo == NULL) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO ALLOCATE MEMORY FOR VERSION INFO");
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_WVT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return FALSE;
	}

	if (GetFileVersionInfoW(processName, 0, versionInfoSize, versionInfo) == 0) {
		free(versionInfo);

		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO GET FILE VERSION INFO");
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_WVT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return FALSE;
	}

	// get the company name
	LPVOID companyName;
	UINT companyNameSize;
	if (VerQueryValueW(versionInfo, L"\\StringFileInfo\\040904b0\\CompanyName", &companyName, &companyNameSize) == 0) {
		free(versionInfo);

		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO QUERY FILE VERSION VALUE");
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, TRIUNEJDG_WVT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return FALSE;
	}

	// check if the company name is Microsoft
	BOOLEAN isMicrosoft = FALSE;
	if (_wcsicmp((WCHAR*)companyName, L"Microsoft Corporation") == 0) {
		isMicrosoft = TRUE;
	}

	free(versionInfo);
	return isMicrosoft;
}


ETWmodule::ETWmodule()
{
	StartListening();
}

ETWmodule::~ETWmodule()
{
	StopListening();
}

VOID __stdcall ETWmodule::ProcessEventCallback(PEVENT_RECORD pEventRecord)
{
	if (!pEventRecord || !pEventRecord->UserData || pEventRecord->UserDataLength == 0)
		return;

	DWORD childPID = 0, parentPID = 0;
	DWORD executorPID = pEventRecord->EventHeader.ProcessId;
	USHORT eventId = pEventRecord->EventHeader.EventDescriptor.Id;

	// ID 1 IS "create process", ID 2 IS "terminate process"
	if (eventId != 1 && eventId != 2)
		return;

	// GET EVENT INFORMATION FOR THE EVENT THAT OCCURED
	ULONG bufferSize = 0;
	TdhGetEventInformation(pEventRecord, 0, nullptr, nullptr, &bufferSize);
	auto pInfo = (PTRACE_EVENT_INFO)malloc(bufferSize);
	if (!pInfo) return;

	if (TdhGetEventInformation(pEventRecord, 0, nullptr, pInfo, &bufferSize) != ERROR_SUCCESS) {
		free(pInfo);
		return;
	}

	// PARSE EVENT PROPERTIES
	for (ULONG i = 0; i < pInfo->TopLevelPropertyCount; i++) {
		auto& prop = pInfo->EventPropertyInfoArray[i];
		auto name = (LPWSTR)((PBYTE)pInfo + prop.NameOffset);

		if (_wcsicmp(name, L"ProcessID") != 0 && _wcsicmp(name, L"ParentProcessID") != 0)
			continue;

		PROPERTY_DATA_DESCRIPTOR desc{};
		desc.PropertyName = (ULONGLONG)name;
		desc.ArrayIndex = ULONG_MAX;

		DWORD value = 0;
		ULONG size = sizeof(value);
		if (TdhGetProperty(pEventRecord, 0, nullptr, 1, &desc, size, (PBYTE)&value) == ERROR_SUCCESS) {
			if (_wcsicmp(name, L"ProcessID") == 0)
				childPID = value;
			else if (_wcsicmp(name, L"ParentProcessID") == 0)
				parentPID = value;
		}
	}

	free(pInfo);

	if (eventId == 1 && childPID != 0) {
		//std::lock_guard<std::mutex> lock(gETWesqueGlobalData.mutex);
		gETWesqueGlobalData.mutex.lock();
		gETWesqueGlobalData.table[childPID] = std::make_pair(executorPID, parentPID);
		gETWesqueGlobalData.mutex.unlock();
	}
	else if (eventId == 2) {
		//std::lock_guard<std::mutex> lock(gETWesqueGlobalData.mutex);
		gETWesqueGlobalData.mutex.lock();
		gETWesqueGlobalData.table.erase(pEventRecord->EventHeader.ProcessId);
		gETWesqueGlobalData.mutex.unlock();
	}
}

VOID ETWmodule::SetupTrace_STA()
{
	PEVENT_TRACE_PROPERTIES props = (PEVENT_TRACE_PROPERTIES)buffer;

	props->Wnode.BufferSize = sizeof(buffer);
	props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	props->Wnode.ClientContext = 1;
	props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

	StringCchCopy((LPWSTR)((PBYTE)props + props->LoggerNameOffset), 128, L"RANskrilProcessTrace");

	// START TRACE SESSION
	ULONG status = StartTrace(&hSession, L"RANskrilProcessTrace", props);
	if (status == ERROR_ALREADY_EXISTS) {
		ControlTrace(0, L"RANskrilProcessTrace", props, EVENT_TRACE_CONTROL_STOP);
		StartTrace(&hSession, L"RANskrilProcessTrace", props);
	}

	// ENABLE KERNEL PROCESS PROVIDER
	ENABLE_TRACE_PARAMETERS params = {};
	params.Version = ENABLE_TRACE_PARAMETERS_VERSION;

	EnableTraceEx2(
		hSession, &KernelProcessProvider, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
		TRACE_LEVEL_INFORMATION, 0x10, 0, 0, &params);

	// SETUP CONSUMER FOR THE TRACE
	EVENT_TRACE_LOGFILE trace = {};
	trace.LoggerName = (LPWSTR)L"RANskrilProcessTrace";
	trace.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
	trace.EventRecordCallback = ProcessEventCallback;

	hTrace = OpenTrace(&trace);
	if (hTrace == INVALID_PROCESSTRACE_HANDLE) {
		int errorCode = GetLastError();
		Utils::LogEvent(std::wstring{ L"ERROR AT OPENING TRACE" }, SERVICE_IDENTIFIER, TRIUNEJDG_ETW_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return;
	}


	Utils::LogEvent(std::wstring{ L"BEGINNING TRACING PROCESS EVENTS ON SEPARATE THREAD" }, SERVICE_IDENTIFIER, TRIUNEJDG_ETW_SUBIDENTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
	ProcessTrace(&hTrace, 1, nullptr, nullptr);
}

VOID ETWmodule::StartListening()
{
	listener = std::thread(&ETWmodule::SetupTrace_STA, this);
}

VOID ETWmodule::StopListening()
{
	CloseTrace(hTrace);
	ControlTrace(hSession, L"RANskrilProcessTrace", props, EVENT_TRACE_CONTROL_STOP);

	if (listener.joinable())
		listener.join();
}

DWORD ETWmodule::GetExecutorPID(DWORD childPID)
{
	if (childPID == 4 || childPID == 0)
		return 0;

	//std::lock_guard<std::mutex> lock(gETWesqueGlobalData.mutex);
	gETWesqueGlobalData.mutex.lock();
	auto it = gETWesqueGlobalData.table.find(childPID);
	if (it == gETWesqueGlobalData.table.end()) {
		gETWesqueGlobalData.mutex.unlock();
		return 0;
	}
	gETWesqueGlobalData.mutex.unlock();
	return it->second.first;
}
