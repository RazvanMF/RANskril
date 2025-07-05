#include "directoryhandlerv2.h"
#include "utils.h"

void DirectoryHandler::PrepareBlacklist()
{
	WCHAR logMessage[256];

	/* * * * * * * * * * * * * * *  */
	/* GENERAL-PURPOSE ROOT FOLDERS */
	/* * * * * * * * * * * * * * *  */


	const GUID folderIDs[] = {
	FOLDERID_Windows,
	FOLDERID_RoamingAppData,
	FOLDERID_ProgramData,
	FOLDERID_ProgramFiles,
	FOLDERID_ProgramFilesX86,
	FOLDERID_LocalAppDataLow,
	FOLDERID_LocalAppData,
	FOLDERID_Public
	};

	for (auto& refs : folderIDs) {
		LPWSTR buffer;
		HRESULT result = SHGetKnownFolderPath(refs, 0, NULL, &buffer);
		if (FAILED(result)) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"COULDN'T GET KNOWN FOLDER PATH");
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DIRECTORY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			return;
		}
		blacklistedFolders.emplace_back(buffer);
		CoTaskMemFree(buffer);
	}

	WCHAR rootLocation[MAX_PATH];
	GetSystemDirectoryW(rootLocation, MAX_PATH);
	std::wstring dosRootLetter = std::wstring(rootLocation).substr(0, 3);

	blacklistedFolders.push_back(dosRootLetter + L"Recovery");
	blacklistedFolders.push_back(dosRootLetter + L"inetpub");
	blacklistedFolders.push_back(dosRootLetter + L"Microsoft Shared");
	blacklistedFolders.push_back(dosRootLetter + L"PerfLogs");
	blacklistedFolders.push_back(dosRootLetter + L"$WinREAgent");
	blacklistedFolders.push_back(dosRootLetter + L"Users\\Default");
	blacklistedFolders.push_back(dosRootLetter + L"Documents and Settings");
	blacklistedFolders.push_back(dosRootLetter + L"$Recycle.Bin");
	blacklistedFolders.push_back(dosRootLetter + L"$RECYCLE.BIN");
	blacklistedFolders.push_back(dosRootLetter + L"System Volume Information");


	/* * * * * * * * * * * * * * */
	/* DRIVE-DEPENDENT LOCATIONS */
	/* * * * * * * * * * * * * * */

	std::vector<std::wstring> driveLetters = RetrieveLogicalDosDrives();

	for (auto& driveLetter : driveLetters) {
		if (driveLetter == dosRootLetter)
			continue;

		blacklistedFolders.push_back(driveLetter + L"$Recycle.Bin");
		blacklistedFolders.push_back(driveLetter + L"$RECYCLE.BIN");
		blacklistedFolders.push_back(driveLetter + L"System Volume Information");
	}

}

DirectoryHandler& DirectoryHandler::GetInstance()
{
	static DirectoryHandler instance;
	return instance;
}

DirectoryHandler::DirectoryHandler()
{
	blacklistedFolders = {};
}

DirectoryHandler::~DirectoryHandler()
{
}

std::vector<std::wstring> DirectoryHandler::RetrieveLogicalDosDrives()
{
	std::vector<std::wstring> driveLetters;
	DWORD bitResult = GetLogicalDrives();
	for (int i = 0; i < 26; i++) {
		if (bitResult & 1) {
			const wchar_t path[] = { L'A' + i, L':', L'\\', L'\0' };
			WCHAR filesystem[8];
			BOOL result = GetVolumeInformationW(path, NULL, NULL, NULL, NULL, 0, filesystem, 8);
			if (result && wcscmp(filesystem, L"NTFS") == 0)
				driveLetters.emplace_back(path);
		}
		bitResult >>= 1;
	}
	return driveLetters;
}

std::vector<std::wstring> DirectoryHandler::GetDirectories(std::wstring startPoint)
{
	auto _ = blacklistedFolders.empty();
	PrepareBlacklist();

	std::queue<std::wstring> fileTree;
	WIN32_FIND_DATAW fileAttributeData;
	std::wstring augmentedStartPoint = startPoint + L"*";
	HANDLE hRootHandle = FindFirstFileExW(augmentedStartPoint.c_str(), FindExInfoBasic, &fileAttributeData, FindExSearchLimitToDirectories, NULL, FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY);
	WCHAR logMessage[256];

	std::vector<std::wstring> foldersToProtect;

	fileTree.push(startPoint);
	while (!fileTree.empty()) {
		std::wstring currentPath = fileTree.front();
		std::wstring augmentedCurrentPath = currentPath + L"*";
		foldersToProtect.push_back(currentPath);
		fileTree.pop();

		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"QUERYING %ws", currentPath.c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DIRECTORY_SUBIDENTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

		HANDLE hFileHandle = FindFirstFileExW(augmentedCurrentPath.c_str(), FindExInfoBasic, &fileAttributeData, FindExSearchLimitToDirectories, NULL, FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY);
		if (hFileHandle != INVALID_HANDLE_VALUE)
			do {
				// skip . and ..
				if (wcscmp(fileAttributeData.cFileName, L".") == 0 || wcscmp(fileAttributeData.cFileName, L"..") == 0)
					continue;
				// skip hard shortcuts
				if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
					continue;
				// skip SYSTEM directories
				if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
					continue;

				// only for directories:
				if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					bool childIsBlacklisted = false;
					std::wstring fullPath = currentPath + fileAttributeData.cFileName;
					for (auto& path : blacklistedFolders) {
						if (fullPath == path) {
							childIsBlacklisted = true;
							break;
						}
					}

					if (!childIsBlacklisted) {
						fullPath += L"\\";
						fileTree.push(fullPath);
					}
				}
			} while (FindNextFileW(hFileHandle, &fileAttributeData));
		else {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO QUERY %ws", currentPath.c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DIRECTORY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		}
	}

	return foldersToProtect;
}

std::vector<std::wstring> DirectoryHandler::GetBlacklistedFolders()
{
	return blacklistedFolders;
}

std::unordered_map<std::wstring, DWORD> DirectoryHandler::GetFileRatios(std::wstring startPoint)
{
	std::unordered_map<std::wstring, DWORD> fileRatios;
	fileRatios.insert(std::pair<std::wstring, DWORD>(L"all", 0));

	std::wstring augmentedStartPoint = startPoint + L"*";
	WIN32_FIND_DATAW fileAttributeData;
	HANDLE hDirectory = FindFirstFileExW(augmentedStartPoint.c_str(), FindExInfoBasic, &fileAttributeData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH | FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY);
	if (hDirectory != INVALID_HANDLE_VALUE) {
		do {
			// skip . and ..
			if (wcscmp(fileAttributeData.cFileName, L".") == 0 || wcscmp(fileAttributeData.cFileName, L"..") == 0)
				continue;
			// skip hard shortcuts
			if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
				continue;
			// skip SYSTEM directories
			if (fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
				continue;

			if (!(fileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				std::wstring fileName(fileAttributeData.cFileName);
				std::wstring extension = fileName.substr(fileName.rfind(L"."));
				++fileRatios[L"all"];
				if (fileRatios.find(extension) == fileRatios.end())
					fileRatios[extension] = 1;
				else
					++fileRatios[extension];
			}
		} while (FindNextFileW(hDirectory, &fileAttributeData));
	}

	return fileRatios;
}
