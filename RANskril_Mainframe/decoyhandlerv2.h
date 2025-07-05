#pragma once
#include "registryhandlerv2.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdlib.h>
#include <algorithm>


typedef struct _DecoyFile {
	std::wstring path;
	std::wstring name;
	std::wstring extension;
	std::pair<std::vector<BYTE>, DWORD> signature;
	RegistryKey registryLocationKey;
} DecoyFile, *PDecoyFile;

class DecoyBuilder {
private:
	DecoyFile decoyFile{};
	static std::unordered_map<std::wstring, std::pair<std::vector<BYTE>, DWORD>> signatures;
public:
	DecoyBuilder();
	~DecoyBuilder();

	void Reset();
	DecoyBuilder* SetPath(std::wstring path);
	DecoyBuilder* SetName(std::wstring name);
	DecoyBuilder* SetExtension(std::wstring extension);
	DecoyBuilder* SetRegistryLocation(RegistryKey registryLocationKey);
	DecoyFile Build();
};

class DecoyFileActualizer {
public:
	DWORD CreateDecoyInFilesystem(DecoyFile decoyFile);
	DWORD PlaceDecoyInsideRegistry(DecoyFile decoyFile);
};

class DecoyHandler {
private:
	DecoyBuilder* decoyBuilder = nullptr;
	DecoyFileActualizer* decoyFileActualizer = nullptr;
	std::wstring GenerateDecoyName(std::wstring extension, DWORD prefixLocation);

	DecoyHandler();
	~DecoyHandler();
public:
	DecoyHandler(const DecoyHandler&) = delete;
	DecoyHandler& operator=(const DecoyHandler&) = delete;

	static DecoyHandler& GetInstance();
	DecoyFile PrepareHeadDecoy(std::wstring path, std::wstring extension, RegistryKey registryLocationKey);
	DecoyFile PrepareTailDecoy(std::wstring path, std::wstring extension, RegistryKey registryLocationKey);
	DecoyFile PrepareMiddleDecoy(std::wstring path, std::wstring extension, RegistryKey registryLocationKey);
	DWORD CreateDecoyInFilesystem(DecoyFile decoyFile);
	DWORD PlaceDecoyInsideRegistry(DecoyFile decoyFile);
	DWORD ArmSystem();
	DWORD DisarmSystem();
	DWORD RefreshSystem();
};