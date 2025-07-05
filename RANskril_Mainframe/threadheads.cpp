#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
#include "threadheads.h"
#include "kernelcommunicator.h"
#include "commons.h"
#include "utils.h"
#include "registryhandlerv2.h"
#include "directoryhandlerv2.h"
#include "decoyhandlerv2.h"
#include "TRIUNEJDG.h"
#include "logger.h"

NotifierSignal notifierSender;
BOOLEAN armed = TRUE;

void KernelDecoyMonListenerEntrypoint() {
	std::thread notifierThread = std::thread(NotifierPipeThreadFunction);
	std::thread receiverThread = std::thread(ReceiverPipeThreadFunction);
	std::thread statusPingThread = std::thread(StatusPingPipeThreadFunction);

	Logger& logger = Logger::GetInstance();
	try {
		HandleMessageLoop();
	}
	catch (std::exception& exception) {
		WCHAR logMessage[256];

		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FATAL ERROR IN MAIN LOOP: %hs", exception.what());
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, GetLastError(), WINEVENT_LEVEL_ERROR);
	}

	notifierSender.isShuttingDown = true;
	CancelSynchronousIo(notifierThread.native_handle());
	CancelSynchronousIo(receiverThread.native_handle());
	CancelSynchronousIo(statusPingThread.native_handle());

	notifierThread.join();
	receiverThread.join();
	statusPingThread.join();
}

void HandleMessageLoop() {
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	Logger& logger = Logger::GetInstance();

	while (true) {
		DECOYMON_SEND_MESSAGE_STRUCT message{};
		DWORD operationStatus = kernelCommunicator.GetMessageFromDecoyMon(&message);
		if (!kernelCommunicator.IsPortOnline())
			break;

		if (!armed)
			continue;

		if (message.info.operationType & OPERATION_TYPE::DECOY_DIR_ADD) {
			HandleDecoyAdditionIntoNewFolder(message);
		}
		else {
			HandleReceivedCommand(message);
		}
	}
}

void HandleReceivedCommand(DECOYMON_SEND_MESSAGE_STRUCT& message) {
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	Logger& logger = Logger::GetInstance();
	WCHAR logMessage[256];

	std::tuple<BOOLEAN, std::vector<DWORD>, BOOLEAN, DWORD> verdictVerbose;
	verdictVerbose = GenerateProcessTreeVerdict(message.info.pid);

	BOOLEAN verdict = std::get<0>(verdictVerbose);
	std::vector<DWORD> pidChain = std::get<1>(verdictVerbose);
	BOOLEAN explorerPresent = std::get<2>(verdictVerbose);
	DWORD explorerPID = std::get<3>(verdictVerbose);

	RtlZeroMemory(logMessage, sizeof(logMessage));
	swprintf(logMessage, 256, L"%llu'S ACCESS TO DECOY FILES IS CONSIDERED %s", message.info.pid, verdict == TRUE ? L"MALICIOUS" : L"SAFE");
	Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, verdict == TRUE ? WINEVENT_LEVEL_WARNING : WINEVENT_LEVEL_INFO);

	if (message.info.pid == 0xFFFFFFFF) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"%llu'S PREVIOUS VERDICT HAS BEEN OVERRIDDEN. OPERATION CONSIDERED SAFE BY DEFAULT", message.info.pid);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
		verdict = FALSE;
	}

	if (Utils::HasInteractiveFileDialog(message.info.pid)) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"%llu TRIGGERED AN INTERACTIVE FILE DIALOG. OPERATION WILL BE ALLOWED BY DEFAULT", message.info.pid);
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
		verdict = FALSE;
	}

	verdict == TRUE ? HandleVerdictCaseTrue(pidChain, explorerPresent, explorerPID) : HandleVerdictCaseFalse(pidChain, explorerPresent, message);

	DECOYMON_REPLY_MESSAGE_STRUCT sendMessage{};
	sendMessage.header.Status = ERROR_SUCCESS;
	sendMessage.header.MessageId = message.header.MessageId;
	sendMessage.info.isPIDRejected = verdict;

	DWORD operationStatus = kernelCommunicator.ReplyToMessageFromDecoyMon(&sendMessage);
	if (operationStatus == ERROR_SUCCESS)
		Utils::LogEvent(std::wstring(L"SUCCESSFULLY SENT VERDICT TO KERNEL"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
}

std::tuple<BOOLEAN, std::vector<DWORD>, BOOLEAN, DWORD>
GenerateProcessTreeVerdict(DWORD masterPID) {
	TRIUNEJDGsystem& triuneJDGsystem = TRIUNEJDGsystem::getInstance();
	Logger& logger = Logger::GetInstance();

	std::vector<DWORD> pids{};
	DWORD arrowPID = masterPID;
	pids.push_back(masterPID);
	while (arrowPID != 0) {
		arrowPID = triuneJDGsystem.etwModule->GetExecutorPID(arrowPID);
		pids.push_back(arrowPID);
	}

	WCHAR logMessage[256];
	RtlZeroMemory(logMessage, sizeof(logMessage));
	swprintf(logMessage, 256, L"BEGINNING ANALYSIS FOR PROCESS %lu", masterPID);
	Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

	BOOLEAN generalVerdict = FALSE;
	BOOLEAN isExplorerPresent = FALSE;
	std::wstring explorer = std::wstring(L"explorer.exe");
	DWORD explorerPID = 0;
	for (DWORD pid : pids) {
		if (pid == 0)
			continue;

		DWORD wvtVerdict = triuneJDGsystem.wvtModule->RunWinVerifyTrust(pid);
		BOOLEAN isMicrosoft = triuneJDGsystem.wvtModule->IsAMicrosoftProcess(pid);

		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"WINVERIFYTRUST VERDICT FOR %lu: (0x%X) | MICROSOFT VERDICT FOR %lu: %ws", pid, wvtVerdict, pid, isMicrosoft == TRUE ? L"TRUE" : L"FALSE");
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, (wvtVerdict == ERROR_SUCCESS && isMicrosoft == TRUE) ? WINEVENT_LEVEL_INFO : WINEVENT_LEVEL_WARNING);

		WCHAR* processImageName = new WCHAR[32768];
		BYTE* buffer = new BYTE[32768];
		SIZE_T bufferSize = 0;

		DWORD processMemoryResult = triuneJDGsystem.GetProcessBuffer(pid, buffer, 32768, bufferSize);
		DWORD imageNameResult = triuneJDGsystem.GetProcessImageName(pid, processImageName, 32768);

		AMSI_RESULT bufferResult = (processMemoryResult == ERROR_SUCCESS) ? triuneJDGsystem.amsiModule->ScanBuffer(buffer, (UINT32)bufferSize, processImageName) : AMSI_RESULT_BLOCKED_BY_ADMIN_START;
		AMSI_RESULT stringResult = (imageNameResult == ERROR_SUCCESS) ? triuneJDGsystem.amsiModule->ScanString(processImageName, processImageName) : AMSI_RESULT_BLOCKED_BY_ADMIN_START;

		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"AMSI BUFFER VERDICT FOR %lu: (0x%X) | AMSI STRING VERDICT FOR %lu: (0x%X)", pid, triuneJDGsystem.amsiModule->ResultIsMalware(bufferResult), pid, triuneJDGsystem.amsiModule->ResultIsMalware(stringResult));
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, (triuneJDGsystem.amsiModule->ResultIsMalware(bufferResult) == FALSE && triuneJDGsystem.amsiModule->ResultIsMalware(stringResult) == FALSE) ? WINEVENT_LEVEL_INFO : WINEVENT_LEVEL_WARNING);

		std::wstring processNameString = std::wstring(processImageName);
		std::size_t found = processNameString.find(explorer);
		if (found != std::wstring::npos && isMicrosoft) {
			isExplorerPresent = TRUE;
			explorerPID = pid;

			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"explorer.exe WAS FOUND IN %lu's PROCESS TREE. PROCESS WILL BE RESTARTED.", masterPID);
			Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_WARNING);
		}

		delete[] processImageName;
		delete[] buffer;

		BOOLEAN localVerdict = (wvtVerdict == ERROR_SUCCESS && isMicrosoft == TRUE
			&& triuneJDGsystem.amsiModule->ResultIsMalware(bufferResult) == FALSE && triuneJDGsystem.amsiModule->ResultIsMalware(stringResult) == FALSE) ? FALSE : TRUE;
		generalVerdict = generalVerdict | localVerdict;
	}

	return std::tuple<BOOLEAN, std::vector<DWORD>, BOOLEAN, DWORD>(generalVerdict, pids, isExplorerPresent, explorerPID);
}

void HandleVerdictCaseTrue(std::vector<DWORD> pidChain, BOOLEAN explorerPresent, DWORD explorerPID) 
{
	TRIUNEJDGsystem& triunejdg = TRIUNEJDGsystem::getInstance();
	Logger& logger = Logger::GetInstance();
	WCHAR logMessage[256];

	for (DWORD pid : pidChain) {
		if (pid == 0)
			continue;
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess) {
			WCHAR* processImageName = new WCHAR[32768];
			SIZE_T bufferSize = 0;
			DWORD imageNameResult = triunejdg.GetProcessImageName(pid, processImageName, 32768);
			std::wstring processName = std::wstring{ processImageName };
			delete[] processImageName;
			WriteInfringingProcessToTempLog(processName);

			if (explorerPresent && explorerPID == pid)
				continue;
			else
				TerminateProcess(hProcess, -1);
			CloseHandle(hProcess);
			
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"%lu HAS BEEN TERMINATED. THE SYSTEM MAY HAVE BEEN TRIPPED!", pid);
			Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_WARNING);
		}
	}

	if (explorerPresent) {
		std::thread([]() {
			Utils::RestartExplorerForUser();
		}).detach();
	}

	{
		std::lock_guard<std::mutex> lock(notifierSender.mutex);
		notifierSender.shouldNotify = true;
	}
	notifierSender.cv.notify_one();

	WCHAR title[] = L"RANskril";
	WCHAR msg[] = L"RANskril has detected a malicious process in your system. The process has been terminated. Please check the RANskril logs for more information and take caution.";
	DWORD response;
	WTSSendMessage(nullptr, WTSGetActiveConsoleSessionId(),
		title, sizeof(title), msg, sizeof(msg),
		MB_OK | MB_ICONEXCLAMATION, 0, &response, FALSE);
}

void HandleVerdictCaseFalse(std::vector<DWORD> pidChain, BOOLEAN explorerPresent, DECOYMON_SEND_MESSAGE_STRUCT& message)
{
	std::map<int, std::wstring> opcodeTable{ {0x1, L"DECOY_DELETED"}, {0x2, L"DECOY_RENAMED"},
	{0x4, L"DECOY_COPIED"}, {0x8, L"DECOY_READ"}, {0x10, L"DECOY_MEMORYMAPPED"},
	{0x20, L"DECOY_DIR_DELETED"}, {0x40, L"DECOY_DIR_RENAMED"}, {0x1000, L"DECOY_DIR_ADD"} };

	Logger& logger = Logger::GetInstance();
	WCHAR logMessage[256];

	RtlZeroMemory(logMessage, sizeof(logMessage));
	swprintf(logMessage, 256, L"%llu HAS BEEN ALLOWED TO CONTINUE. OPERATION IS 0x%X (%ws)", message.info.pid, message.info.operationType, opcodeTable[message.info.operationType].c_str());
	Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

	switch (message.info.operationType) {
	case DECOY_DELETED:
	case DECOY_RENAMED:
		HandleDecoyDisarmament(message);
		break;
	case DECOY_DIR_DELETED:
	case DECOY_DIR_RENAMED:
		HandleFolderDisarmament(message);
		break;
	}
}

void HandleDecoyDisarmament(DECOYMON_SEND_MESSAGE_STRUCT& message) {
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();
	Logger& logger = Logger::GetInstance();

	std::wstring givenPath, dosPath, filelessPath, dosPathBackSlashed;
	DWORD inputSize, stopperFile;
	RegistryKey regPathKey, regPathSubKey;
	RegistryValue regValueKey;
	WCHAR logMessage[256];

	givenPath = std::wstring(message.info.path);
	inputSize = (DWORD)((wcslen(givenPath.c_str()) + 1) * sizeof(WCHAR));
	kernelCommunicator.SendRemoveDecoyIOCTLToDecoyMon(givenPath, inputSize);

	dosPath = Utils::NtToDosPath(givenPath);
	stopperFile = dosPath.rfind(L'\\');
	filelessPath = dosPath.substr(0, stopperFile + 1);
	std::replace(filelessPath.begin(), filelessPath.end(), L'\\', L'/');

	regPathKey = RegistryKey{ HKEY_LOCAL_MACHINE, (std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths\\") + filelessPath.c_str()) };
	regValueKey = RegistryValue{ regPathKey, dosPath, REG_DWORD, nullptr, 0 };
	std::wprintf(L"\t%ws; %ws; %ws->%ws\n", givenPath.c_str(), dosPath.c_str(), regValueKey.key.path.c_str(), regValueKey.name.c_str());
	registryHandler.DeleteValue(regValueKey);

	RtlZeroMemory(logMessage, sizeof(logMessage));
	swprintf(logMessage, 256, L"%llu'S OPERATION 0x%X RESULTED IN DECOY DISARMAMENT AT %ws", message.info.pid, message.info.operationType, dosPath.c_str());
	Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
}

void HandleFolderDisarmament(DECOYMON_SEND_MESSAGE_STRUCT& message) {
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();
	Logger& logger = Logger::GetInstance();

	std::wstring givenPath, dosPath, filelessPath, dosPathBackSlashed;
	DWORD inputSize;
	RegistryKey regPathKey, regPathSubKey;
	RegistryValue regValueKey;
	WCHAR logMessage[256];

	givenPath = std::wstring(message.info.path);
	inputSize = (DWORD)((wcslen(givenPath.c_str()) + 1) * sizeof(WCHAR));

	dosPath = Utils::NtToDosPath(givenPath) + L"\\";
	std::replace(dosPath.begin(), dosPath.end(), L'\\', L'/');
	regPathKey = RegistryKey{ HKEY_LOCAL_MACHINE, (std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths\\") + dosPath.c_str()) };

	for (auto valueToDisarm : registryHandler.EnumerateValuesFromKey(regPathKey)) {
		std::wstring dosName = valueToDisarm.substr(0, 2);
		WCHAR deviceName[32];
		QueryDosDeviceW(dosName.c_str(), deviceName, 32);
		std::wstring devicePath = deviceName + valueToDisarm.substr(2);
		DWORD inputSize = (DWORD)((wcslen(devicePath.c_str()) + 1) * sizeof(WCHAR));
		kernelCommunicator.SendRemoveDecoyIOCTLToDecoyMon(devicePath, inputSize);

		DeleteFileW(valueToDisarm.c_str());
	}
	inputSize = (DWORD)((wcslen(givenPath.c_str()) + 1) * sizeof(WCHAR));
	kernelCommunicator.SendRemoveDirectoryIOCTLToDecoyMon(givenPath, inputSize);
	registryHandler.DeleteKey(regPathKey);

	RtlZeroMemory(logMessage, sizeof(logMessage));
	swprintf(logMessage, 256, L"%llu'S OPERATION 0x%X RESULTED IN FOLDER-WIDE, RECURSIVE DECOY DISARMAMENT AT %ws", message.info.pid, message.info.operationType, dosPath.c_str());
	Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
}

void HandleDecoyAdditionIntoNewFolder(DECOYMON_SEND_MESSAGE_STRUCT& message)
{
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	DirectoryHandler& directoryHandler = DirectoryHandler::GetInstance();
	Logger& logger = Logger::GetInstance();
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();

	std::wstring givenPath = std::wstring(message.info.path);
	DWORD inputSize = (DWORD)((wcslen(givenPath.c_str()) + 1) * sizeof(WCHAR));
	std::wstring dosPath = Utils::NtToDosPath(givenPath) + L"\\";

	bool isBlacklisted = false;
	for (auto& blacklistedDirectory : directoryHandler.GetBlacklistedFolders()) {
		if (dosPath.find(blacklistedDirectory) != std::wstring::npos) {
			isBlacklisted = true;
			break;
		}
	}
	if (isBlacklisted)
		return;

	std::wstring logMessageStr = L"FOLDER CREATION/RENAME AT \"" + dosPath + L"\" HAS BEEN ISSUED. PREPARING BASELINE DECOY SEEDING.";
	Utils::LogEvent(logMessageStr, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

	std::wstring forwardSlashDosPath = dosPath;
	std::replace(forwardSlashDosPath.begin(), forwardSlashDosPath.end(), L'\\', L'/');
	RegistryKey regPathKey = RegistryKey{ HKEY_LOCAL_MACHINE, (std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths\\") + forwardSlashDosPath.c_str()) };
	registryHandler.CreateKeyIfMissing(regPathKey);

	logMessageStr = L"SENDING DECOY AND DIRECTORY DATA TO DECOYMON FOR DIRECTORY \"" + dosPath + L"\"";
	Utils::LogEvent(logMessageStr, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_MAIN, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

	HandleDecoyCreation(true, dosPath, regPathKey);
	HandleDecoyCreation(false, dosPath, regPathKey);
	kernelCommunicator.SendAddDirectoryIOCTLToDecoyMon(givenPath, inputSize);
}

void HandleDecoyCreation(bool isHead, std::wstring dosPath, RegistryKey regPathKey) 
{
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	DecoyHandler& decoyHandler = DecoyHandler::GetInstance();
	Logger& logger = Logger::GetInstance();

	DecoyFile decoy;
	if (isHead)
		decoy = decoyHandler.PrepareHeadDecoy(dosPath, L".pdf", regPathKey);
	else
		decoy = decoyHandler.PrepareTailDecoy(dosPath, L".jpg", regPathKey);

	std::wstring decoyName = decoy.path + decoy.name;

	decoyHandler.CreateDecoyInFilesystem(decoy);
	decoyHandler.PlaceDecoyInsideRegistry(decoy);

	std::wstring dosName = decoyName.substr(0, 2);
	WCHAR deviceName[32];
	QueryDosDeviceW(dosName.c_str(), deviceName, 32);
	std::wstring deviceDecoyPath = deviceName + decoyName.substr(2);
	DWORD inputSize = (DWORD)((wcslen(deviceDecoyPath.c_str()) + 1) * sizeof(WCHAR));

	kernelCommunicator.SendAddDecoyIOCTLToDecoyMon(deviceDecoyPath, inputSize);
}

void NotifierPipeThreadFunction()
{
	WCHAR logMessage[256];
	try {
		Utils::LogEvent(std::wstring(L"SEPARATE NOTIFIER THREAD STARTED"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_NOTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
		int value = 1;
		DWORD bytesWritten;

		while (true) {
			HANDLE pipe = CreateNamedPipe(PIPE_OUT, PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, sizeof(int), sizeof(int), 0, NULL);

			if (pipe == INVALID_HANDLE_VALUE) {
				RtlZeroMemory(logMessage, sizeof(logMessage));
				swprintf(logMessage, 256, L"FAILED TO CREATE PIPE FOR NOTIFIER");
				Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_NOTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);
				return;
			}

			BOOL connected = ConnectNamedPipe(pipe, NULL);
			if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
				RtlZeroMemory(logMessage, sizeof(logMessage));
				swprintf(logMessage, 256, L"FAILED TO CONNECT CLIENT TO PIPE");
				Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_NOTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);
				CloseHandle(pipe);
				continue;
			}

			Utils::LogEvent(std::wstring(L"CLIENT CONNECTED TO NOTIFIER PIPE"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_NOTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

			while (true) {
				std::unique_lock<std::mutex> lock(notifierSender.mutex);
				notifierSender.cv.wait(lock, [] {
					return notifierSender.shouldNotify || notifierSender.isShuttingDown;
					});

				if (notifierSender.isShuttingDown) {
					RtlZeroMemory(logMessage, sizeof(logMessage));
					swprintf(logMessage, 256, L"STOPPING NOTIFIER THREAD");
					Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_NOTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					CloseHandle(pipe);
					return;
				}

				notifierSender.shouldNotify = false;

				lock.unlock();

				BOOL result = WriteFile(pipe, &value, sizeof(value), &bytesWritten, NULL);
				if (!result) {
					RtlZeroMemory(logMessage, sizeof(logMessage));
					swprintf(logMessage, 256, L"CONNECTION TO CLIENT HAS BEEN BROKEN");
					Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_NOTIFIER, GetLastError(), WINEVENT_LEVEL_ERROR);

					CloseHandle(pipe);
					break;
				}

				Utils::LogEvent(std::wstring{ L"VERDICT SENT TO GUI"}, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_NOTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
			}
		}
	}
	catch (std::exception& exception) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FATAL ERROR IN NOTIFIER LOOP (%hs)", exception.what());
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_NOTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_ERROR);
	}
}

void ReceiverPipeThreadFunction() {
	KernelCommunicator& kernelCommunicator = KernelCommunicator::GetInstance();
	DecoyHandler& decoyHandler = DecoyHandler::GetInstance();
	Logger& logger = Logger::GetInstance();

	WCHAR logMessage[256];
	try {
		Utils::LogEvent(std::wstring(L"SEPARATE RECEIVER THREAD STARTED"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
		while (true) {
			HANDLE pipe = CreateNamedPipe(PIPE_IN, PIPE_ACCESS_INBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, sizeof(int) * 2, sizeof(int) * 2, 0, NULL);
			if (pipe == INVALID_HANDLE_VALUE) {
				Utils::LogEvent(std::wstring(L"FAILED TO CREATE PIPE FOR RECEIVER"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, GetLastError(), WINEVENT_LEVEL_ERROR);
				return;
			}

			BOOL connected = ConnectNamedPipe(pipe, NULL);
			Utils::LogEvent(std::wstring(L"CLIENT CONNECTED TO RECEIVER PIPE"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
			if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
				CloseHandle(pipe);
				continue;
			}

			DWORD bytesRead;
			while (true) {
				int orders[2] = { 0, 0 };
				BOOL result = ReadFile(pipe, &orders, sizeof(orders), &bytesRead, NULL);
				if (result == FALSE) {
					if (GetLastError() == ERROR_BROKEN_PIPE) {
						Utils::LogEvent(std::wstring(L"CLIENT CONNECTED TO NOTIFIER PIPE"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_BROKEN_PIPE, WINEVENT_LEVEL_ERROR);
						CloseHandle(pipe);
						break;
					}
				}

				if (notifierSender.isShuttingDown)
				{
					RtlZeroMemory(logMessage, sizeof(logMessage));
					swprintf(logMessage, 256, L"STOPPING RECEIVER THREAD");
					Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					CloseHandle(pipe);
					return;
				}

				switch (orders[0]) {
				case MacroOpCode::DecoyRegeneration:
					armed = false;

					kernelCommunicator.SendTurnOffIOCTLToDecoyMon();
					kernelCommunicator.SendNukeFileDBIOCTLToDecoyMon();
					kernelCommunicator.SendNukeDirectoryDBIOCTLToDecoyMon();
					kernelCommunicator.SendNukeExcludedDBIOCTLToDecoyMon();

					decoyHandler.DisarmSystem();
					decoyHandler.ArmSystem();

					kernelCommunicator.SendAllDecoysToDecoyMon();
					kernelCommunicator.SendAllDirectoriesToDecoyMon();
					kernelCommunicator.SendAllExcludedDirectoriesToDecoyMon();
					kernelCommunicator.SendTurnOnIOCTLToDecoyMon();

					armed = true;
					Utils::LogEvent(std::wstring(L"FINISHED DECOY REGENERATION COMMAND"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					break;
				case MacroOpCode::DecoyMetaReset:
					decoyHandler.RefreshSystem();
					Utils::LogEvent(std::wstring(L"FINISHED DECOY REFRESH COMMAND"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					break;
				case MacroOpCode::DecoyDisarmSystem:
					armed = false;
					kernelCommunicator.SendTurnOffIOCTLToDecoyMon();
					kernelCommunicator.SendNukeFileDBIOCTLToDecoyMon();
					kernelCommunicator.SendNukeDirectoryDBIOCTLToDecoyMon();
					kernelCommunicator.SendNukeExcludedDBIOCTLToDecoyMon();
					decoyHandler.DisarmSystem();

					armed = true;
					kernelCommunicator.SendTurnOnIOCTLToDecoyMon();
					Utils::LogEvent(std::wstring(L"FINISHED DECOY DISARMAMENT COMMAND"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					break;
				case MacroOpCode::GeneralSafeRestart:
					HandleRestartToSafeMode();
					Utils::LogEvent(std::wstring(L"FINISHED RESTART TO SAFE MODE COMMAND"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					break;
				case MacroOpCode::GeneralRearmSystem:
					break;
				case MacroOpCode::RANskrilStatus:
					armed = orders[1] == 0 ? FALSE : TRUE;
					wprintf(L"[i] RANskril HAS BEEN TURNED %ws\n", orders[1] == 0 ? L"OFF" : L"ON");
					if (orders[1] == 0)
						kernelCommunicator.SendTurnOffIOCTLToDecoyMon();
					else if (orders[1] == 1)
						kernelCommunicator.SendTurnOnIOCTLToDecoyMon();

					if (armed == TRUE)
						Utils::LogEvent(std::wstring(L"RANskril HAS BEEN TURNED ON."), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					else
						Utils::LogEvent(std::wstring(L"RANskril HAS BEEN TURNED OFF. PLEASE REENABLE IT AS SOON AS POSSIBLE TO KEEP YOUR SYSTEM PROTECTED."), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_WARNING);
					break;
				}
			}
		}
	}
	catch (std::exception& exception) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FATAL ERROR IN RECEIVER LOOP (%hs)", exception.what());
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_RECEIVER, ERROR_SUCCESS, WINEVENT_LEVEL_ERROR);
	}
}

std::wstring GetUserDesktopPath() {
	PWSTR path = NULL;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &path);
	std::wstring result = path;
	CoTaskMemFree(path);
	return result;
}

void HandleRestartToSafeMode() {
	Utils::ImpersonateCurrentlyLoggedInUser();
	std::wstring path = GetUserDesktopPath() + L"\\RANskril README - Safe Mode Instructions.txt";
	RevertToSelf();

	std::wofstream file(path);
	file << L"You have been rebooted into Safe Mode by RANskril.\n";
	file << L"You can investigate further inside this mode.\n";
	file << L"In order to disable Safe Mode or change its settings, you can do the following:\n\n";
	file << L"------ CHANGE NETWORKING SETTINGS ------\n";
	file << L"Enable Networking: ADMINISTRATOR cmd.exe => bcdedit /set {current} safeboot network\n";
	file << L"Disable Networking (default): ADMINISTRATOR cmd.exe => bcdedit /set {current} safeboot minimal\n\n";
	file << L"------ DISABLE SAFEMODE ------\n";
	file << L"ADMINISTRATOR cmd.exe => bcdedit /deletevalue {current} safeboot\n";
	file.close();

	system("bcdedit /set {current} safeboot minimal");
	system("shutdown /r /f /t 0");
}

void WriteInfringingProcessToTempLog(std::wstring path) {
	Utils::ImpersonateCurrentlyLoggedInUser();
	auto tempPath = std::filesystem::temp_directory_path();
	std::wstring tempFilePath = std::wstring{ tempPath.c_str() } + L"\\ranskril_CRlogs.txt";

	std::wofstream file;
	file.open(tempFilePath, std::ios_base::app);
	if (!file)
		file.open(tempFilePath);
	SYSTEMTIME logtime;
	GetSystemTime(&logtime);

	WCHAR timestring[256];
	RtlZeroMemory(timestring, sizeof(timestring));
    swprintf(timestring, 256, L"[%02d-%02d-%d][%02d:%02d:%02d]", logtime.wDay, logtime.wMonth, logtime.wYear, logtime.wHour, logtime.wMinute, logtime.wSecond);

	file << timestring << " " << path << L"\n";
	file.close();
	
	RevertToSelf();
}

void StatusPingPipeThreadFunction() {
	WCHAR logMessage[256];

	try {
		Utils::LogEvent(std::wstring(L"SEPARATE STATUS PING THREAD STARTED"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_STATUSPING, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
		while (true) {
			HANDLE pipe = CreateNamedPipe(PIPE_DUPLEX, PIPE_ACCESS_DUPLEX,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, sizeof(int) * 2, sizeof(int) * 2, 0, NULL);
			if (pipe == INVALID_HANDLE_VALUE) return;
			ConnectNamedPipe(pipe, NULL);
			Utils::LogEvent(std::wstring(L"CLIENT CONNECTED TO STATUS PING PIPE"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_STATUSPING, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

			DWORD bytesRead;
			while (true) {
				int orders[1] = { 0 };
				BOOL result = ReadFile(pipe, &orders, sizeof(orders), &bytesRead, NULL);
				if (result == FALSE) {
					if (GetLastError() == ERROR_BROKEN_PIPE) {
						Utils::LogEvent(std::wstring(L"CLIENT CONNECTED TO NOTIFIER PIPE"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_STATUSPING, ERROR_BROKEN_PIPE, WINEVENT_LEVEL_ERROR);
						CloseHandle(pipe);
						break;
					}
				}

				if (notifierSender.isShuttingDown) 
				{
					RtlZeroMemory(logMessage, sizeof(logMessage));
					swprintf(logMessage, 256, L"STOPPING STATUS PING THREAD");
					Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_STATUSPING, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					CloseHandle(pipe);
					return;
				}

				DWORD bytesWritten = 0;
				orders[0] = armed;
				result = WriteFile(pipe, &orders, sizeof(orders), &bytesWritten, NULL);
				if (!result) {
					DWORD err = GetLastError();
					Utils::LogEvent(std::wstring(L"CLIENT CONNECTED TO NOTIFIER PIPE"), SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_STATUSPING, GetLastError(), WINEVENT_LEVEL_ERROR);
					CloseHandle(pipe);
					break;
				}

				if (notifierSender.isShuttingDown)
				{
					RtlZeroMemory(logMessage, sizeof(logMessage));
					swprintf(logMessage, 256, L"STOPPING STATUS THREAD");
					Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_STATUSPING, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);
					CloseHandle(pipe);
					return;
				}
			}
		}
	}
	catch (std::exception& exception) {
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"FATAL ERROR IN STATUS PING LOOP (%hs)", exception.what());
		Utils::LogEvent(std::wstring{ logMessage }, SERVICE_IDENTIFIER, SWITCH_SUBIDENTIFIER_STATUSPING, ERROR_SUCCESS, WINEVENT_LEVEL_ERROR);
	}
}