#pragma once
#include "commons.h"
#include "utils.h"
#include <Windows.h>
#include <fltUser.h>
#include <string>
#include <mutex>
#include <map>
#include <algorithm>

class KernelCommunicator {
private:
	HANDLE hDecoyMonDevice = INVALID_HANDLE_VALUE;
	HANDLE hDecoyMonPort = INVALID_HANDLE_VALUE;

	DWORD OpenDecoyMonDevice();
	DWORD OpenDecoyMonPort();
	DWORD CloseDecoyMonDevice();

	KernelCommunicator();
	~KernelCommunicator();
public:
	KernelCommunicator(const KernelCommunicator&) = delete;
	KernelCommunicator& operator=(const KernelCommunicator&) = delete;

	static KernelCommunicator& GetInstance();

	DWORD SendAddDecoyIOCTLToDecoyMon(std::wstring decoyPath, DWORD decoyPathSize);
	DWORD SendAddDirectoryIOCTLToDecoyMon(std::wstring directoryPath, DWORD directoryPathSize);

	DWORD SendRemoveDecoyIOCTLToDecoyMon(std::wstring decoyPath, DWORD decoyPathSize);
	DWORD SendRemoveDirectoryIOCTLToDecoyMon(std::wstring directoryPath, DWORD directoryPathSize);

	DWORD SendAddExcludedDirectoryIOCTLToDecoyMon(std::wstring directoryPath, DWORD directoryPathSize);

	DWORD SendTurnOffIOCTLToDecoyMon();
	DWORD SendTurnOnIOCTLToDecoyMon();

	DWORD SendNukeFileDBIOCTLToDecoyMon();
	DWORD SendNukeDirectoryDBIOCTLToDecoyMon();
	DWORD SendNukeExcludedDBIOCTLToDecoyMon();

	DWORD SendAllDecoysToDecoyMon();
	DWORD SendAllDirectoriesToDecoyMon();
	DWORD SendAllExcludedDirectoriesToDecoyMon();

	DWORD GetMessageFromDecoyMon(PDECOYMON_SEND_MESSAGE_STRUCT message);
	DWORD ReplyToMessageFromDecoyMon(PDECOYMON_REPLY_MESSAGE_STRUCT message);

	DWORD CloseDecoyMonPort();

	bool IsDeviceOnline();
	bool IsPortOnline();
};
