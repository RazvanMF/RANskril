#pragma once
#include <Windows.h>
#include <string>
#include "commons.h"

static class Utils {
public:
	static std::wstring NtToDosPath(std::wstring ntPath);
	static int ImpersonateCurrentlyLoggedInUser();
	static void LogEvent(const std::wstring& message, const WCHAR identifier[], const WCHAR subidentifier[], DWORD error = 0x0, DWORD winlevel = WINEVENT_LEVEL_INFO);
	static DWORD RestartExplorerForUser();
	static BOOL HasInteractiveFileDialog(DWORD targetPID);
};