#pragma once
#include <Windows.h>
#include <string>
#include <map>
#include <vector>

typedef struct _RegistryKey {
	HKEY root;
	std::wstring path;
} RegistryKey, *PRegistryKey;

typedef struct _RegistryValue {
	RegistryKey key;
	std::wstring name;
	DWORD type;
	LPVOID pValueBuffer;
	DWORD valueSize;
} RegistryValue, * PRegistryValue;


class RegistryHandler {
private:
	std::vector<RegistryKey> keyStructures = {
		RegistryKey{HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\RANskril")},
		RegistryKey{HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\RANskril\\DecoyMon")},
		RegistryKey{HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths")}
	};

	std::vector<RegistryValue> valueStructures = {
		RegistryValue{keyStructures[1], std::wstring(L"Initialized"), REG_DWORD, nullptr},
		RegistryValue{keyStructures[1], std::wstring(L"PathsCount"), REG_DWORD, nullptr}
	};

	std::map<HKEY, std::wstring> rootStringTranslation = { {HKEY_LOCAL_MACHINE, std::wstring(L"HKEY_LOCAL_MACHINE")}, {HKEY_CURRENT_USER, std::wstring(L"HKEY_CURRENT_USER")} };

	// constructor. other than creating the value structure initial values, is has no other point.
	RegistryHandler();
	~RegistryHandler();
public:
	static RegistryHandler& GetInstance();

	// function that runs at service boot, to check if RANskril's registries remain unaffected.
	DWORD AssureRegistryIntegrity();

	// function that checks if a key exists inside the registry.
	bool KeyExists(RegistryKey key);

	// function that checks if a value exists inside a given key.
	bool ValueExists(RegistryValue value);

	// function that creates a key inside the registry.
	bool CreateKeyIfMissing(RegistryKey key);

	// function that creates a value inside the given key of a registry
	bool CreateValueIfMissing(RegistryValue value);

	DWORD OpenKey(RegistryKey key, PHKEY outputKey);
	DWORD RetrieveValue(RegistryValue value, PVOID resultBuffer, DWORD resultBufferSize);
	DWORD SetValue(RegistryValue value);
	DWORD DeleteKey(RegistryKey key);
	DWORD DeleteValue(RegistryValue value);

	std::vector<std::wstring> EnumerateKeysFromKey(RegistryKey masterKey);
	std::vector<std::wstring> EnumerateValuesFromKey(RegistryKey masterKey);
};
