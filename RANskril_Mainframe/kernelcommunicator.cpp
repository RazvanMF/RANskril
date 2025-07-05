#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
#include "kernelcommunicator.h"
#include "commons.h"
#include "../K_DecoyMon/decoymon_public.h"
#include "registryhandlerv2.h"
#include "directoryhandlerv2.h"
#include "decoyhandlerv2.h"
#include "TRIUNEJDG.h"
#include "logger.h"
#include "utils.h"

KernelCommunicator& KernelCommunicator::GetInstance()
{
	static KernelCommunicator instance;
	return instance;
}

KernelCommunicator::KernelCommunicator() {
	OpenDecoyMonDevice();
	OpenDecoyMonPort();
}

KernelCommunicator::~KernelCommunicator() {
	CloseDecoyMonPort();
	CloseDecoyMonDevice();
}

DWORD KernelCommunicator::OpenDecoyMonDevice()
{
	if (hDecoyMonDevice != INVALID_HANDLE_VALUE)
		return ERROR_ALREADY_INITIALIZED;

	hDecoyMonDevice = CreateFile(L"\\\\.\\DecoyMon", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDecoyMonDevice == INVALID_HANDLE_VALUE) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT OPENING DECOYMON"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}

	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::OpenDecoyMonPort()
{
	HRESULT result = FilterConnectCommunicationPort(L"\\DecoyMonProcessIDBridge", 0, nullptr, 0, NULL, &hDecoyMonPort);
	if (FAILED(result)) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT CONNECTING TO FILTER PORT IN DECOYMON"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::CloseDecoyMonDevice()
{
	if (hDecoyMonDevice == INVALID_HANDLE_VALUE)
		return ERROR_NOT_CONNECTED;

	BOOL operationStatus = CloseHandle(hDecoyMonDevice);
	hDecoyMonDevice = INVALID_HANDLE_VALUE;
	if (!operationStatus) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT CLOSING DECOYMON DEVICE"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::CloseDecoyMonPort()
{
	BOOL operationStatus = CloseHandle(hDecoyMonPort);
	if (!operationStatus) {
		hDecoyMonPort = INVALID_HANDLE_VALUE;
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT DISCONNECTING FROM FILTER PORT IN DECOYMON"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendAddDecoyIOCTLToDecoyMon(std::wstring decoyPath, DWORD decoyPathSize)
{
	DWORD returnedBytes = 0;
	BOOL result;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_ADD_DECOY_FILE, (VOID*)decoyPath.c_str(), decoyPathSize, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING DECOY FILE PATH FOR ADDITION TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendAddDirectoryIOCTLToDecoyMon(std::wstring directoryPath, DWORD directoryPathSize)
{
	DWORD returnedBytes = 0;
	BOOL result;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_ADD_DECOY_DIRECTORY, (VOID*)directoryPath.c_str(), directoryPathSize, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING DIRECTORY PATH CONTAINING DECOYS FOR ADDITION TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendRemoveDecoyIOCTLToDecoyMon(std::wstring decoyPath, DWORD decoyPathSize)
{
	DWORD returnedBytes = 0;
	BOOL result;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_REMOVE_DECOY_FILE, (VOID*)decoyPath.c_str(), decoyPathSize, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING DECOY FILE PATH FOR REMOVAL TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendRemoveDirectoryIOCTLToDecoyMon(std::wstring directoryPath, DWORD directoryPathSize)
{
	DWORD returnedBytes = 0;
	BOOL result;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_REMOVE_DECOY_DIRECTORY, (VOID*)directoryPath.c_str(), directoryPathSize, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING DIRECTORY PATH CONTAINING DECOYS FOR REMOVAL TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendAddExcludedDirectoryIOCTLToDecoyMon(std::wstring directoryPath, DWORD directoryPathSize)
{
	DWORD returnedBytes = 0;
	BOOL result;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_ADD_EXCLUDED_DIRECTORY, (VOID*)directoryPath.c_str(), directoryPathSize, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING EXCLUDED DIRECTORY PATH FOR ADDITION TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendTurnOffIOCTLToDecoyMon()
{
	DWORD returnedBytes = 0;
	BOOL result;
	ULONG off = 0;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_RANSKRILOVERRIDE_TURNOFF, (VOID*)&off, 4, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING TURN OFF SIGNAL TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendTurnOnIOCTLToDecoyMon()
{
	DWORD returnedBytes = 0;
	BOOL result;
	ULONG on = 1;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_RANSKRILOVERRIDE_TURNON, (VOID*)&on, 4, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING TURN ON SIGNAL TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendNukeFileDBIOCTLToDecoyMon()
{
	DWORD returnedBytes = 0;
	BOOL result;
	ULONG on = 1;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_EMPTY_DECOY_FILE_MEMORY, (VOID*)&on, 4, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING DELETE DECOY FILE TABLE SIGNAL TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendNukeDirectoryDBIOCTLToDecoyMon()
{
	DWORD returnedBytes = 0;
	BOOL result;
	ULONG on = 1;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_EMPTY_DECOY_DIRECTORY_MEMORY, (VOID*)&on, 4, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING DELETE DECOY DIRECTORY TABLE SIGNAL TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendNukeExcludedDBIOCTLToDecoyMon()
{
	DWORD returnedBytes = 0;
	BOOL result;
	ULONG on = 1;
	result = DeviceIoControl(hDecoyMonDevice, IOCTL_EMPTY_EXCLUDED_DIRECTORY_MEMORY, (VOID*)&on, 4, NULL, 0, &returnedBytes, nullptr);
	if (!result) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"ERROR AT SENDING DELETE EXCLUDED DIRECTORY LIST SIGNAL TO DECOYMON VIA IOCTL"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendAllDecoysToDecoyMon()
{
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();
	WCHAR logMessage[256];

	std::vector<std::wstring> decoyPaths;
	std::vector<std::wstring> directoryPaths;
	HKEY hSubkeyDecoyMonPaths;
	RegistryKey masterKey{ HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths") };
	registryHandler.OpenKey(masterKey, &hSubkeyDecoyMonPaths);
	for (auto& pathKey : registryHandler.EnumerateKeysFromKey(masterKey)) {
		RegistryKey hPathSubkey{ hSubkeyDecoyMonPaths, pathKey.c_str() };
		directoryPaths.push_back(pathKey.substr(0, pathKey.size() - 1));
		for (auto& decoyPath : registryHandler.EnumerateValuesFromKey(hPathSubkey))
			decoyPaths.push_back(decoyPath);
	}
	RegCloseKey(hSubkeyDecoyMonPaths);

	for (auto& decoyPath : decoyPaths) {
		std::wstring dosName = decoyPath.substr(0, 2);
		WCHAR deviceName[32];
		QueryDosDeviceW(dosName.c_str(), deviceName, 32);
		std::wstring devicePath = deviceName + decoyPath.substr(2);
		DWORD inputSize = (DWORD)((wcslen(devicePath.c_str()) + 1) * sizeof(WCHAR));


		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ATTEMPTING TO SEND DECOY FILE PATH %s", devicePath.c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, 0, WINEVENT_LEVEL_INFO);

		SendAddDecoyIOCTLToDecoyMon(devicePath, inputSize);
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendAllDirectoriesToDecoyMon()
{
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();
	WCHAR logMessage[256];

	std::vector<std::wstring> decoyPaths;
	std::vector<std::wstring> directoryPaths;
	HKEY hSubkeyDecoyMonPaths;
	RegistryKey masterKey{ HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths") };
	registryHandler.OpenKey(masterKey, &hSubkeyDecoyMonPaths);
	for (auto& pathKey : registryHandler.EnumerateKeysFromKey(masterKey)) {
		RegistryKey hPathSubkey{ hSubkeyDecoyMonPaths, pathKey.c_str() };
		directoryPaths.push_back(pathKey.substr(0, pathKey.size() - 1));
		for (auto& decoyPath : registryHandler.EnumerateValuesFromKey(hPathSubkey))
			decoyPaths.push_back(decoyPath);
	}
	RegCloseKey(hSubkeyDecoyMonPaths);

	for (auto& directoryPath : directoryPaths) {
		std::wstring backslashedPath{ directoryPath };
		std::replace(backslashedPath.begin(), backslashedPath.end(), L'/', L'\\');
		std::wstring dosName = backslashedPath.substr(0, 2);
		WCHAR deviceName[32];
		QueryDosDeviceW(dosName.c_str(), deviceName, 32);
		std::wstring devicePath = deviceName + backslashedPath.substr(2);
		DWORD inputSize = (DWORD)((wcslen(devicePath.c_str()) + 1) * sizeof(WCHAR));

		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ATTEMPTING TO SEND DIRECTORY PATH %s", devicePath.c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, 0, WINEVENT_LEVEL_INFO);

		SendAddDirectoryIOCTLToDecoyMon(devicePath, inputSize);
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::SendAllExcludedDirectoriesToDecoyMon()
{
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();
	DirectoryHandler& directoryHandler = DirectoryHandler::GetInstance();
	WCHAR logMessage[256];

	for (auto& excludedDirectory : directoryHandler.GetBlacklistedFolders()) {
		std::wstring backslashedPath{ excludedDirectory };
		std::replace(backslashedPath.begin(), backslashedPath.end(), L'/', L'\\');
		std::wstring dosName = backslashedPath.substr(0, 2);
		WCHAR deviceName[32];
		QueryDosDeviceW(dosName.c_str(), deviceName, 32);
		std::wstring devicePath = deviceName + backslashedPath.substr(2) + L"*";
		DWORD inputSize = (DWORD)((wcslen(devicePath.c_str()) + 1) * sizeof(WCHAR));

		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ATTEMPTING TO SEND EXCLUDED DIRECTORY PATH %s", devicePath.c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, 0, WINEVENT_LEVEL_INFO);

		SendAddExcludedDirectoryIOCTLToDecoyMon(devicePath, inputSize);
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::GetMessageFromDecoyMon(PDECOYMON_SEND_MESSAGE_STRUCT message)
{
	HRESULT operationStatus = FilterGetMessage(hDecoyMonPort, &message->header, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION) + sizeof(FILTER_MESSAGE_HEADER), nullptr);
	if (FAILED(operationStatus)) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"FAILURE AT FILTER PORT MESSAGE RECEIVE"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

DWORD KernelCommunicator::ReplyToMessageFromDecoyMon(PDECOYMON_REPLY_MESSAGE_STRUCT message)
{
	HRESULT operationStatus = FilterReplyMessage(hDecoyMonPort, &message->header, sizeof(FILTER_REPLY_HEADER) + sizeof(DECOYMON_REPLY_MESSAGE_INFORMATION));
	if (FAILED(operationStatus)) {
		DWORD errorCode = GetLastError();
		Utils::LogEvent(std::wstring(L"FAILURE AT FILTER PORT MESSAGE REPLY"), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		return errorCode;
	}
	return ERROR_SUCCESS;
}

bool KernelCommunicator::IsDeviceOnline()
{
	return hDecoyMonDevice != INVALID_HANDLE_VALUE;
}

bool KernelCommunicator::IsPortOnline()
{
	return hDecoyMonPort != INVALID_HANDLE_VALUE;
}
