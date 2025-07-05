#pragma once
#include "commons.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <Shlobj.h>
#include <PathCch.h>

class DirectoryHandler {
private:
	std::vector<std::wstring> blacklistedFolders;
	void PrepareBlacklist();

	DirectoryHandler();
	~DirectoryHandler();
public:
	DirectoryHandler(const DirectoryHandler&) = delete;
	DirectoryHandler& operator=(const DirectoryHandler&) = delete;

	static DirectoryHandler& GetInstance();

	std::vector<std::wstring> RetrieveLogicalDosDrives();
	std::vector<std::wstring> GetDirectories(std::wstring startPoint);
	std::vector<std::wstring> GetBlacklistedFolders();
	std::unordered_map<std::wstring, DWORD> GetFileRatios(std::wstring startPoint);
};