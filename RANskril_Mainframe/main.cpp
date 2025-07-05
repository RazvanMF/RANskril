#pragma comment(lib, "fltlib.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Amsi.lib")
#pragma comment(lib, "tdh.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Userenv.lib")
// #pragma comment(lib, "crypt32.lib")

#include "commons.h"
#include "../K_DecoyMon/decoymon_public.h"
#include "registryhandlerv2.h"
#include "kernelcommunicator.h"
#include "directoryhandlerv2.h"
#include "decoyhandlerv2.h"
#include "TRIUNEJDG.h"
#include "logger.h"
#include "threadheads.h"
#include "utils.h"

WCHAR gServiceName[] = L"RANskril CORE";
SERVICE_STATUS_HANDLE gService = NULL;
SERVICE_STATUS gServiceStatus = { 0 };

void SetStatus(DWORD status) {
	gServiceStatus.dwCurrentState = status;
	if (status == SERVICE_RUNNING) {
		gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	}
	else {
		gServiceStatus.dwControlsAccepted = 0;
	}
	SetServiceStatus(gService, &gServiceStatus);
}

void WINAPI ServiceControlHandler(DWORD controlCode) {
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	switch (controlCode) {
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		SetStatus(SERVICE_STOP_PENDING);

		kernelCommunicator.CloseDecoyMonPort();
		Utils::LogEvent(std::wstring{ L"RANskril CORE SERVICE IS STOPPING" }, SERVICE_IDENTIFIER, SERVICE_SUBIDENTIFIER_END, 0, WINEVENT_LEVEL_INFO);

		SetStatus(SERVICE_STOPPED);
		break;
	default:
		break;
	}
}

void WINAPI ServiceMain(DWORD dwNumServicesArgs, WCHAR** lpServiceArgVectors) {
	gServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gServiceStatus.dwWaitHint = 3000;

	gService = RegisterServiceCtrlHandler(gServiceName, ServiceControlHandler);
	if (!gService)
	{
		Utils::LogEvent(std::wstring{ L"FAILED TO REGISTER SERVICE CONTROL HANDLER" }, SERVICE_IDENTIFIER, SERVICE_SUBIDENTIFIER_INIT, GetLastError(), WINEVENT_LEVEL_ERROR);
		return;
	}
	SetStatus(SERVICE_START_PENDING);
	srand(time(NULL));

	RegistryHandler& registryHandler = RegistryHandler::GetInstance();
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	DirectoryHandler& directoryHandler = DirectoryHandler::GetInstance();
	DecoyHandler& decoyHandler = DecoyHandler::GetInstance();
	TRIUNEJDGsystem& triuneJDGsystem = TRIUNEJDGsystem::getInstance();
	Logger& logger = Logger::GetInstance();


	/* * * * * * * * * * * * * * * */
	/* ASSURING REGISTRY INTEGRITY */
	/* * * * * * * * * * * * * * * */

	DWORD operationStatus = registryHandler.AssureRegistryIntegrity();
	if (operationStatus != ERROR_SUCCESS) {
		Utils::LogEvent(std::wstring{ L"FAILED TO ASSURE REGISTRY INTEGRITY" }, SERVICE_IDENTIFIER, SERVICE_SUBIDENTIFIER_INIT, operationStatus, WINEVENT_LEVEL_ERROR);
		SetStatus(SERVICE_STOPPED);
		return;
	}


	/* * * * * * * * * * * * * * */
	/* FIRST TIME INITIALIZATION */
	/* * * * * * * * * * * * * * */

	RegistryKey subkeyDecoyMonStruct{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\RANskril\\DecoyMon" };
	DWORD initialized = 0;
	DWORD size = sizeof(DWORD);
	RegistryValue initializedStruct{ subkeyDecoyMonStruct, std::wstring(L"Initialized"), REG_DWORD, nullptr, 0 };
	operationStatus = registryHandler.RetrieveValue(initializedStruct, &initialized, size);
	if (operationStatus != ERROR_SUCCESS) {
		Utils::LogEvent(std::wstring{ L"FAILED TO RETRIEVE INITIALIZATION STATUS FROM REGISTRY" }, SERVICE_IDENTIFIER, SERVICE_SUBIDENTIFIER_INIT, operationStatus, WINEVENT_LEVEL_ERROR);
		SetStatus(SERVICE_STOPPED);
		return;
	}
	if (initialized == 0)
		decoyHandler.ArmSystem();


	/* * * * * * * *  */
	/* SYSTEM REFRESH */
	/* * * * * * * *  */

	decoyHandler.RefreshSystem();

	// post initialization
	Utils::LogEvent(std::wstring{ L"SYSTEM IS ARMED AND REFRESHED" }, SERVICE_IDENTIFIER, SERVICE_SUBIDENTIFIER_INIT, 0, WINEVENT_LEVEL_INFO);


	/* * * * * * * * * * *  */
	/* SUPPLYING THE DRIVER */
	/* * * * * * * * * * *  */

	// don't attempt to send to decoymon if offline
	if (!kernelCommunicator.IsDeviceOnline()) {
		Utils::LogEvent(std::wstring{ L"FAILED TO ESTABLISH CONNECTION TO DECOYMON. SHUTTING DOWN" }, SERVICE_IDENTIFIER, SERVICE_SUBIDENTIFIER_INIT, 0, WINEVENT_LEVEL_ERROR);
		SetStatus(SERVICE_STOPPED);
	}

	kernelCommunicator.SendAllDecoysToDecoyMon();
	kernelCommunicator.SendAllDirectoriesToDecoyMon();
	kernelCommunicator.SendAllExcludedDirectoriesToDecoyMon();

	Utils::LogEvent(std::wstring{ L"SENT PATH INFORMATION TO DECOYMON" }, SERVICE_IDENTIFIER, SERVICE_SUBIDENTIFIER_INIT, 0, WINEVENT_LEVEL_INFO);


	/* * * * * * * * * * * * *  */
	/* LISTENING TO FILTER PORT */
	/* * * * * * * * * * * * *  */

	std::thread(KernelDecoyMonListenerEntrypoint).detach();
	SetStatus(SERVICE_RUNNING);
}

int main(int argc, char** argv) {
	SERVICE_TABLE_ENTRY serviceTable[] = {
		{ gServiceName, ServiceMain },
		{ nullptr, nullptr }
	};

	if (!StartServiceCtrlDispatcher(serviceTable)) {
		DWORD error = GetLastError();
		Utils::LogEvent(std::wstring{ L"FAILED TO START SERVICE CONTROL DISPATCHER" }, SERVICE_IDENTIFIER, SERVICE_SUBIDENTIFIER_INIT, GetLastError(), WINEVENT_LEVEL_ERROR);
		return error;
	}
}