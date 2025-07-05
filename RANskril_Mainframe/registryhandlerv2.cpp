#include "registryhandlerv2.h"
#include "commons.h"
#include "logger.h"
#include "utils.h"

RegistryHandler::RegistryHandler()
{
	LPVOID pInitialized = new DWORD;
	LPVOID pPathsCount = new DWORD;
	*(LPDWORD)pInitialized = 0;
	*(LPDWORD)pPathsCount = 0;

	valueStructures[0].pValueBuffer = pInitialized;
	valueStructures[0].valueSize = sizeof(DWORD);
	valueStructures[1].pValueBuffer = pPathsCount;
	valueStructures[1].valueSize = sizeof(DWORD);
}

DWORD RegistryHandler::AssureRegistryIntegrity()
{
	WCHAR logMessage[256];
	for (auto& keyStruct : keyStructures) {
		if (!CreateKeyIfMissing(keyStruct)) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"ERROR AT CREATING SUBKEY %ws\\%ws", rootStringTranslation[keyStruct.root].c_str(), keyStruct.path.c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			return errorCode;
		}
	}

	for (auto& valueStruct : valueStructures) {
		if (ValueExists(valueStruct))
			continue;
		if (!SetValue(valueStruct)) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"ERROR AT CREATING VALUE %ws\\%ws -> %ws", rootStringTranslation[valueStruct.key.root].c_str(), valueStruct.key.path.c_str(), valueStruct.name.c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, KERNELCOMM_COMPONENT_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			return errorCode;
		}
	}

	return ERROR_SUCCESS;
}

bool RegistryHandler::KeyExists(RegistryKey key)
{
	HKEY hKey = nullptr;
	DWORD operationStatus = RegOpenKeyExW(key.root, key.path.c_str(), 0, KEY_READ, &hKey);
	if (operationStatus == ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return true;
	}
	return false;
}

bool RegistryHandler::ValueExists(RegistryValue value)
{
	HKEY hKey = nullptr;
	DWORD operationStatus = RegOpenKeyExW(value.key.root, value.key.path.c_str(), 0, KEY_READ, &hKey);
	if (operationStatus != ERROR_SUCCESS)
		return false;

	DWORD type = 0;
	operationStatus = RegQueryValueExW(hKey, value.name.c_str(), nullptr, &type, nullptr, nullptr);
	RegCloseKey(hKey);
	return (operationStatus == ERROR_SUCCESS);
}

bool RegistryHandler::CreateKeyIfMissing(RegistryKey key)
{
	HKEY hKey = nullptr;
	DWORD disposition;
	LSTATUS operationStatus = RegCreateKeyExW(key.root, key.path.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &disposition);
	if (operationStatus == ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return true;
	}
	return false;
}

bool RegistryHandler::CreateValueIfMissing(RegistryValue value)
{
	if (ValueExists(value)) 
		return true;

	HKEY hKey = nullptr;
	DWORD operationStatus = RegOpenKeyExW(value.key.root, value.key.path.c_str(), 0, KEY_WRITE, &hKey);
	if (operationStatus != ERROR_SUCCESS) 
		return false;

	operationStatus = RegSetKeyValueW(hKey, value.name.c_str(), value.name.c_str(), value.type, &value.pValueBuffer, value.valueSize);
	RegCloseKey(hKey);
	return (operationStatus == ERROR_SUCCESS);
}

DWORD RegistryHandler::OpenKey(RegistryKey key, PHKEY outputKey)
{
	return RegOpenKeyExW(key.root, key.path.c_str(), 0, KEY_READ | KEY_WRITE, outputKey);
}

DWORD RegistryHandler::RetrieveValue(RegistryValue value, PVOID resultBuffer, DWORD resultBufferSize)
{
	switch (value.type) {
	case REG_DWORD:
		return RegGetValueW(value.key.root, value.key.path.c_str(), value.name.c_str(), RRF_RT_DWORD, NULL, resultBuffer, &resultBufferSize);
	default:
		return ERROR_INVALID_PARAMETER;
	}
}

DWORD RegistryHandler::SetValue(RegistryValue value)
{
	switch (value.type) {
	case REG_DWORD:
		return RegSetKeyValueW(value.key.root, value.key.path.c_str(), value.name.c_str(), value.type, value.pValueBuffer, value.valueSize);
	default:
		return ERROR_INVALID_PARAMETER;
	}
}

DWORD RegistryHandler::DeleteKey(RegistryKey key)
{
	return RegDeleteKeyW(key.root, key.path.c_str());
}

DWORD RegistryHandler::DeleteValue(RegistryValue value)
{
	return RegDeleteKeyValueW(value.key.root, value.key.path.c_str(), value.name.c_str());
}

std::vector<std::wstring> RegistryHandler::EnumerateKeysFromKey(RegistryKey masterKey)
{
	std::vector<std::wstring> keys;
	HKEY hTargetKey;
	OpenKey(masterKey, &hTargetKey);
	for (DWORD i = 0; ; i++) {
		WCHAR keyName[MAX_PATH];
		DWORD keyNameSize = ARRAYSIZE(keyName);
		DWORD expectedType;

		DWORD result = RegEnumKeyW(hTargetKey, i, keyName, keyNameSize);
		if (result == ERROR_NO_MORE_ITEMS)
			break;

		keys.push_back(std::wstring(keyName));
	}
	RegCloseKey(hTargetKey);
	return keys;
}

std::vector<std::wstring> RegistryHandler::EnumerateValuesFromKey(RegistryKey masterKey)
{
	std::vector<std::wstring> values;
	HKEY hSubkeyPath;
	OpenKey(masterKey, &hSubkeyPath);

	for (DWORD j = 0; ; j++) {
		WCHAR valueName[MAX_PATH];
		DWORD valueNameSize = ARRAYSIZE(valueName);
		DWORD expectedType;

		DWORD result = RegEnumValueW(hSubkeyPath, j, valueName, &valueNameSize, 0, &expectedType, nullptr, nullptr);
		if (result == ERROR_NO_MORE_ITEMS)
			break;

		values.push_back(std::wstring(valueName));
	}

	RegCloseKey(hSubkeyPath);
	return values;
}

RegistryHandler::~RegistryHandler()
{
	for (auto& registryValue : valueStructures)
		delete registryValue.pValueBuffer;
}

RegistryHandler& RegistryHandler::GetInstance()
{
	static RegistryHandler instance;
	return instance;
}
