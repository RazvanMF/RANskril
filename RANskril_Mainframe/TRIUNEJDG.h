#pragma once
#include "commons.h"
#include <Windows.h>
#include <amsi.h>
#include <string>
#include <Psapi.h>
#include <tdh.h>
#include <evntrace.h>
#include <evntcons.h>
#include <evntprov.h>
#include <unordered_map>
#include <mutex>
#include <strsafe.h>

class AMSImodule;
class ETWmodule;
class WVTmodule;

struct ETWGlobalData {
	std::unordered_map<DWORD, std::pair<DWORD, DWORD>> table{};
	std::mutex mutex{};
};
extern ETWGlobalData gETWesqueGlobalData;

class TRIUNEJDGsystem {
private:
	TRIUNEJDGsystem();
	~TRIUNEJDGsystem();

public:
	AMSImodule* amsiModule = nullptr;
	ETWmodule* etwModule = nullptr;
	WVTmodule* wvtModule = nullptr;

	TRIUNEJDGsystem(const TRIUNEJDGsystem&) = delete;
	TRIUNEJDGsystem& operator=(const TRIUNEJDGsystem&) = delete;

	static TRIUNEJDGsystem& getInstance();

	DWORD GetProcessBuffer(ULONG_PTR processPID, PVOID buffer, SIZE_T inputSize, SIZE_T& outputSize);
	DWORD GetProcessImageName(ULONG_PTR processPID, LPCWSTR buffer, ULONG inputSize);
};

class AMSImodule {
private:
	std::wstring amsiModuleName;
	HAMSICONTEXT amsiContext = nullptr;
	HAMSISESSION amsiSession = nullptr;
public:
	AMSImodule();
	~AMSImodule();

	AMSI_RESULT ScanBuffer(PVOID buffer, UINT32 bufferSize, LPCWSTR filePath);
	AMSI_RESULT ScanString(LPCWSTR buffer, LPCWSTR filePath);
	BOOL ResultIsMalware(AMSI_RESULT result);
};

class ETWmodule {
private:
	std::thread listener;

	TRACEHANDLE hSession = 0;
	TRACEHANDLE hTrace = INVALID_PROCESSTRACE_HANDLE;
	GUID KernelProcessProvider = { 0x22fb2cd6, 0x0e7b, 0x422b, {0xa0, 0xc7, 0x2f, 0xad, 0x1f, 0xd0, 0xe7, 0x16} };

	BYTE buffer[sizeof(EVENT_TRACE_PROPERTIES) + 1024 * 2] = {};
	PEVENT_TRACE_PROPERTIES props;

	static VOID WINAPI ProcessEventCallback(PEVENT_RECORD pEventRecord);
	VOID SetupTrace_STA();
	VOID StartListening();
	VOID StopListening();
public:
	ETWmodule();
	~ETWmodule();

	DWORD GetExecutorPID(DWORD childPID);
};

class WVTmodule {
public: 	
	DWORD RunWinVerifyTrust(ULONG_PTR pid);
	BOOLEAN IsAMicrosoftProcess(ULONG_PTR pid);
};