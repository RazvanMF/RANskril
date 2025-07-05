#include "utils.h"
#include "commons.h"
#include "logger.h"
#include "resource.h"

std::wstring Utils::NtToDosPath(std::wstring ntPath)
{
	auto desiredStopperDevice = ntPath.find(L'\\', 8);
	std::wstring device = ntPath.substr(0, desiredStopperDevice);
	DWORD drivesBitmask = GetLogicalDrives();
	std::wstring dosDevice = L"";
	for (DWORD i = 0; i < 26; i++) {
		if (drivesBitmask & 1) {
			const wchar_t path[] = { L'A' + i, L':', L'\0' };
			WCHAR deviceName[32];
			QueryDosDeviceW(path, deviceName, 32);
			if (device == deviceName) {
				dosDevice = std::wstring(path);
				break;
			}
		}
		drivesBitmask >>= 1;
	}
	return dosDevice + ntPath.substr(desiredStopperDevice);
}

int Utils::ImpersonateCurrentlyLoggedInUser()
{
	HANDLE hUserToken = NULL;
	DWORD sessionId = WTSGetActiveConsoleSessionId();

	if (!WTSQueryUserToken(sessionId, &hUserToken)) {
		DWORD error = GetLastError();
		Utils::LogEvent(std::wstring(L"IMPERSONATION: WTSQueryUserToken FAILED"), SERVICE_IDENTIFIER, L"UTILS", error, WINEVENT_LEVEL_ERROR);
		return 1;
	}

	HANDLE hUserImpersonationToken = NULL;
	if (!DuplicateTokenEx(hUserToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hUserImpersonationToken)) {
		DWORD error = GetLastError();
		CloseHandle(hUserToken);
		Utils::LogEvent(std::wstring(L"IMPERSONATION: DuplicateTokenEx FAILED"), SERVICE_IDENTIFIER, L"UTILS", error, WINEVENT_LEVEL_ERROR);
		return 1;
	}

	if (!ImpersonateLoggedOnUser(hUserImpersonationToken)) {
		DWORD error = GetLastError();
		CloseHandle(hUserToken);
		CloseHandle(hUserImpersonationToken);
		Utils::LogEvent(std::wstring(L"IMPERSONATION: ImpersonateLoggedOnUser FAILED"), SERVICE_IDENTIFIER, L"UTILS", error, WINEVENT_LEVEL_ERROR);
		return 1;
	}

	CloseHandle(hUserToken);
	CloseHandle(hUserImpersonationToken);
	return 0;
}

void Utils::LogEvent(const std::wstring& message, const WCHAR identifier[], const WCHAR subidentifier[], DWORD error, DWORD winlevel)
{
	Logger& logger = Logger::GetInstance();
	WCHAR errorMessage[256];
	RtlZeroMemory(errorMessage, sizeof(errorMessage));
	swprintf(errorMessage, 256, L"[e] %ws:%ws - %ws (0x%X)", identifier, subidentifier, message.c_str(), error);
	OutputDebugString(errorMessage);

	RtlZeroMemory(errorMessage, sizeof(errorMessage));
	swprintf(errorMessage, 256, L"%ws (0x%X)", message.c_str(), error);
	logger.Write(std::wstring(identifier) + L":" + std::wstring(subidentifier), std::wstring(errorMessage), winlevel);
}

DWORD Utils::RestartExplorerForUser()
{
	auto tempPath = std::filesystem::temp_directory_path();
	std::wstring tempExecPath = std::wstring{ tempPath.c_str() } + L"\\RANskrilExplorerRestartHelper.exe";
	WCHAR logMessage[256];

	HANDLE hExplorerExec = CreateFileW(tempExecPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hExplorerExec == INVALID_HANDLE_VALUE) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT CREATING HELPER FILE %ws", tempExecPath.c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return errorCode;
	}

	bool status = true;
	do {
		HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(EXPLORER_HELPER), RT_RCDATA);
		if (!hRes) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO FIND EXPLORER HELPER");
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			status = false;
			break;
		}

		HGLOBAL hLoadedRes = LoadResource(NULL, hRes);
		if (!hLoadedRes) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO LOAD EXPLORER HELPER");
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			status = false;
			break;
		}

		DWORD size = SizeofResource(NULL, hRes);
		void* pData = LockResource(hLoadedRes);
		if (!pData) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO LOCK EXPLORER HELPER");
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			status = false;
			break;
		}

		DWORD written = 0;
		BOOL ok = WriteFile(hExplorerExec, pData, size, &written, NULL);
		if (!ok) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO WRITE FROM EXPLORER HELPER TO %ws", tempExecPath.c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			status = false;
			break;
		}

	} while (false);

	if (status == false) {
		CloseHandle(hExplorerExec);
		return GetLastError();
	}

	CloseHandle(hExplorerExec);
	DWORD sessionId = WTSGetActiveConsoleSessionId();
	if (sessionId == 0xFFFFFFFF)
		return ERROR_INVALID_PARAMETER;

	HANDLE hUserToken = NULL;
	if (!WTSQueryUserToken(sessionId, &hUserToken))
		return GetLastError();

	HANDLE hPrimaryToken = NULL;
	if (!DuplicateTokenEx(hUserToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &hPrimaryToken)) {
		CloseHandle(hUserToken);
		return GetLastError();
	}

	LPVOID pEnv = NULL;
	if (!CreateEnvironmentBlock(&pEnv, hPrimaryToken, FALSE)) {
		DWORD err = GetLastError();
		CloseHandle(hUserToken);
		CloseHandle(hPrimaryToken);
		return err;
	}

	STARTUPINFOW si = { sizeof(si) };
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.lpDesktop = const_cast<LPWSTR>(L"winsta0\\default");

	PROCESS_INFORMATION pi = { 0 };

	BOOL result = CreateProcessAsUserW(
		hPrimaryToken,
		tempExecPath.c_str(),
		NULL,
		NULL,
		NULL,
		FALSE,
		CREATE_UNICODE_ENVIRONMENT,
		pEnv,
		NULL,
		&si,
		&pi
	);

	if (result) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	std::this_thread::sleep_for(std::chrono::seconds(3));
	DeleteFileW(tempExecPath.c_str());
	DestroyEnvironmentBlock(pEnv);
	CloseHandle(hUserToken);
	CloseHandle(hPrimaryToken);
	return result;
}

BOOL Utils::HasInteractiveFileDialog(DWORD targetPID) {
	WCHAR logMessage[256];
	auto tempPath = std::filesystem::temp_directory_path();
	std::wstring helperPath = std::wstring{ tempPath.c_str() } + L"\\RANskrilForegroundHelper.exe";

	const std::wstring pipeName = L"\\\\.\\pipe\\RANskrilForegroundPipe";

	HANDLE hForegroundExec = CreateFileW(helperPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hForegroundExec == INVALID_HANDLE_VALUE) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT CREATING HELPER FILE %ws", helperPath.c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

		return errorCode;
	}

	bool status = true;
	do {
		HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(FOREGROUND_HELPER), RT_RCDATA);
		if (!hRes) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO FIND FOREGROUND HELPER");
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			status = false;
			break;
		}

		HGLOBAL hLoadedRes = LoadResource(NULL, hRes);
		if (!hLoadedRes) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO LOAD FOREGROUND HELPER");
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			status = false;
			break;
		}

		DWORD size = SizeofResource(NULL, hRes);
		void* pData = LockResource(hLoadedRes);
		if (!pData) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO LOCK FOREGROUND HELPER");
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			status = false;
			break;
		}

		DWORD written = 0;
		BOOL ok = WriteFile(hForegroundExec, pData, size, &written, NULL);
		if (!ok) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO WRITE FROM FOREGROUND HELPER TO %ws", helperPath.c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			status = false;
			break;
		}

	} while (false);

	if (status == false) {
		CloseHandle(hForegroundExec);
		return GetLastError();
	}

	CloseHandle(hForegroundExec);

	SECURITY_DESCRIPTOR sd = {};
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

	SECURITY_ATTRIBUTES sa = {};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;

	WCHAR buffer[64] = { 0 };
	HANDLE hPipe = CreateNamedPipeW(
		pipeName.c_str(),
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1, sizeof(buffer), sizeof(buffer), 1000, &sa
	);

	if (hPipe == INVALID_HANDLE_VALUE) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO CREATE PIPE FOR %lu", targetPID);
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);
		DeleteFileW(helperPath.c_str());
		return false;
	}

	DWORD sessionId = WTSGetActiveConsoleSessionId();
	HANDLE hUserToken = NULL;
	if (!WTSQueryUserToken(sessionId, &hUserToken)) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO QUERY USER TOKEN FOR %lu", targetPID);
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);
		CloseHandle(hPipe);
		DeleteFileW(helperPath.c_str());
		return false;
	}

	HANDLE hPrimaryToken = NULL;
	if (!DuplicateTokenEx(hUserToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hPrimaryToken)) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO DUPLICATE USER TOKEN FOR %lu", targetPID);
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);
		CloseHandle(hUserToken);
		CloseHandle(hPipe);
		DeleteFileW(helperPath.c_str());
		return false;
	}

	WCHAR cmdLine[MAX_PATH + 16] = { 0 };
	swprintf(cmdLine, ARRAYSIZE(cmdLine), L"\"%s\"", helperPath.c_str());

	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi = {};

	BOOL started = CreateProcessAsUserW(
		hPrimaryToken, NULL, cmdLine, NULL, NULL, FALSE,
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi
	);

	CloseHandle(hPrimaryToken);
	CloseHandle(hUserToken);

	if (!started) {
		CloseHandle(hPipe);
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO START HELPER PROCESS IN SESSION 1 FOR %lu", targetPID);
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);
		DeleteFileW(helperPath.c_str());
		return false;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	if (!ConnectNamedPipe(hPipe, NULL)) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO CONNECT TO PIPE FOR %lu", targetPID);
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);

		CloseHandle(hPipe);
		DeleteFileW(helperPath.c_str());
		return false;
	}

	DWORD bytesRead = 0;
	if (!ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(WCHAR), &bytesRead, NULL)) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FAILED TO READ FROM PIPE FOR %lu", targetPID);
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);
		CloseHandle(hPipe);
		DeleteFileW(helperPath.c_str());
		return false;
	}

	CloseHandle(hPipe);
	DeleteFileW(helperPath.c_str());

	std::wstring msg(buffer);
	RtlZeroMemory(logMessage, sizeof(logMessage));
	swprintf(logMessage, 256, L"RETRIEVED %ws FOR %lu", msg.c_str(), targetPID);
	Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, UTILS_SUBIDENTIFIER, 0x0, WINEVENT_LEVEL_INFO);
	if (msg.substr(0, 2) == L"OK") {
		DWORD fgPid = std::wcstoul(msg.substr(3).c_str(), nullptr, 10);
		return (fgPid == targetPID);
	}

	return false;
}
