#pragma once
#include "pch.h"
#include <TraceLoggingProvider.h>
#include <winmeta.h>

constexpr auto DRIVER_PREFIX = "DecoyMon";
constexpr auto DRIVER_TAG = 'Mycd';

extern const TraceLoggingHProvider g_hKernelProvider;

NTSTATUS CompleteRequest(_In_ PIRP pIRP, _In_ NTSTATUS operationStatus = STATUS_SUCCESS, _In_ ULONG_PTR information = 0);
NTSTATUS DecoyMonCreateClose(_In_ PDEVICE_OBJECT pDeviceObject, _In_ PIRP pIRP);
NTSTATUS DecoyMonDeviceControl(_In_ PDEVICE_OBJECT pDeviceObject, _In_ PIRP pIRP);


typedef NTSTATUS(*QUERY_INFO_PROCESS)(
	__in HANDLE                                      ProcessHandle,
	__in PROCESSINFOCLASS                            ProcessInformationClass,
	__out_bcount_opt(ProcessInformationLength) PVOID ProcessInformation,
	__in UINT32                                      ProcessInformationLength,
	__out_opt PUINT32                                ReturnLength
	);

extern "C" NTSYSCALLAPI NTSTATUS NTAPI ZwQueryInformationProcess(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength
);

#define PsProtectedTypeNone      0
#define PsProtectedTypeProtected 1
#define PsProtectedTypeLight     2

typedef union _PS_PROTECTION {
	UCHAR Level;
	struct {
		UCHAR Type : 3;
		UCHAR Audit : 1;
		UCHAR Signer : 4;
	} Flags;
} PS_PROTECTION, * PPS_PROTECTION;

extern "C" NTSYSAPI PS_PROTECTION NTAPI PsGetProcessProtection(PEPROCESS Process);
