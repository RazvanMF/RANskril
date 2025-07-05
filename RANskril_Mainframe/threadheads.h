#pragma once
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
#include <Windows.h>
#include <fltUser.h>
#include <string>
#include <mutex>
#include <map>
#include <algorithm>
#include <vector>
#include <shellapi.h>
#include <Wtsapi32.h>

#include "kernelcommunicator.h"
#include "commons.h"
#include "utils.h"
#include "registryhandlerv2.h"
#include "directoryhandlerv2.h"
#include "decoyhandlerv2.h"
#include "TRIUNEJDG.h"
#include "logger.h"

#define PIPE_IN  L"\\\\.\\pipe\\RANskrilPipeIn"
#define PIPE_OUT L"\\\\.\\pipe\\RANskrilPipeOut"
#define PIPE_DUPLEX L"\\\\.\\pipe\\RANskrilPipeDuplex"

struct NotifierSignal {
	std::mutex mutex;
	std::condition_variable cv;
	bool shouldNotify = false;
	bool isShuttingDown = false;
};

enum MacroOpCode {
	DecoyRatio = 0x1,
	DecoyRegeneration = 0x2,
	DecoyMetaReset = 0x4,
	DecoyDisarmSystem = 0x8,
	GeneralSafeRestart = 0x1000,
	GeneralRearmSystem = 0x2000,
	RANskrilStatus = 0xFFFF
};

void KernelDecoyMonListenerEntrypoint();
void HandleMessageLoop();
void HandleReceivedCommand(DECOYMON_SEND_MESSAGE_STRUCT& message);
std::tuple<BOOLEAN, std::vector<DWORD>, BOOLEAN, DWORD> GenerateProcessTreeVerdict(DWORD masterPID);
void HandleVerdictCaseTrue(std::vector<DWORD> pidChain, BOOLEAN explorerPresent, DWORD explorerPID);
void WriteInfringingProcessToTempLog(std::wstring path);
void HandleVerdictCaseFalse(std::vector<DWORD> pidChain, BOOLEAN explorerPresent, DECOYMON_SEND_MESSAGE_STRUCT& message);
void HandleDecoyDisarmament(DECOYMON_SEND_MESSAGE_STRUCT& message);
void HandleFolderDisarmament(DECOYMON_SEND_MESSAGE_STRUCT& message);
void HandleDecoyAdditionIntoNewFolder(DECOYMON_SEND_MESSAGE_STRUCT& message);
void HandleDecoyCreation(bool isHead, std::wstring dosPath, RegistryKey regPathKey);

void NotifierPipeThreadFunction();
void ReceiverPipeThreadFunction();
std::wstring GetUserDesktopPath();
void HandleRestartToSafeMode();
void StatusPingPipeThreadFunction();
