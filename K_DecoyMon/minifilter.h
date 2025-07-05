#pragma once
#include <fltKernel.h>
#include "STRUCT_ERESOURCE.h"
#include "UNICODE_STRING_HASH_TABLE.h"
#include "UNICODE_STRING_ITERABLE_LIST.h"

/// <summary>
/// Contains all the important global variables for the filter driver.
/// </summary>
struct FilterState {
	PFLT_PORT pFilterPort;
	PFLT_PORT pClientPort;
	PFLT_FILTER pFilterObject;
	PUNICODE_STRING_HASH_TABLE fileHashTable;
	STRUCT_ERESOURCE executiveResourceFiles;
	PUNICODE_STRING_HASH_TABLE directoryHashTable;
	STRUCT_ERESOURCE executiveResourceDirectories;
	PUNICODE_STRING_ITERABLE_LIST excludedDirectoryList;
	STRUCT_ERESOURCE executiveResourceExcluded;
	PDRIVER_OBJECT pDriverObject;

	BOOLEAN armed;
	STRUCT_ERESOURCE executiveResourceArmedCheck;
};

/// <summary>
/// Defines an enumeration for various operation types related to decoy file and directories encompassing decoys, as well as folder creation.
/// </summary>
enum OPERATION_TYPE {
	DECOY_DELETED = 0x1,
	DECOY_RENAMED = 0x2,
	DECOY_COPIED = 0x4,
	DECOY_READ = 0x8,
	DECOY_MEMORYMAPPED = 0x10,
	DECOY_DIR_DELETED = 0x20,
	DECOY_DIR_RENAMED = 0x40,
	DECOY_DIR_ADD = 0x1000
};

/// <summary>
/// Defines a structure for sending message information related to an attempt to access a decoy file or perform a destructive operation on a directory containing decoys.
/// This structure includes the operation type, process ID, and the path of the file or directory involved in the operation.
/// </summary>
typedef struct _DECOYMON_SEND_MESSAGE_INFORMATION {
	ULONG operationType;
	ULONG_PTR pid;
	WCHAR path[260];
} DECOYMON_SEND_MESSAGE_INFORMATION, * PDECOYMON_SEND_MESSAGE_INFORMATION;

/// <summary>
/// Defines a structure to represent reply message information for DecoyMon.
/// This structure contains a boolean flag indicating whether the process ID was rejected during the operation.
/// Rejection means that the process is not allowed to perform the operation on the decoy file or directory.
/// </summary>
typedef struct _DECOYMON_REPLY_MESSAGE_INFORMATION {
	BOOLEAN isPIDRejected;
} DECOYMON_REPLY_MESSAGE_INFORMATION, * PDECOYMON_REPLY_MESSAGE_INFORMATION;

/// <summary>
/// Defines a structure for sending messages through the filter port to the service.
/// This structure includes a header for the filter message and a DECOYMON_SEND_MESSAGE_INFORMATION structure that contains the details of the operation.
/// </summary>
typedef struct _DECOYMON_SEND_MESSAGE_STRUCT {
	FILTER_MESSAGE_HEADER header;
	DECOYMON_SEND_MESSAGE_INFORMATION info;
} DECOYMON_SEND_MESSAGE_STRUCT, * PDECOYMON_SEND_MESSAGE_STRUCT;

/// <summary>
/// Defines a structure for a reply message received from the service through the filter port.
/// This structure includes a header for the filter reply message and a DECOYMON_REPLY_MESSAGE_INFORMATION structure that contains the result of the operation.
/// </summary>
typedef struct _DECOYMON_REPLY_MESSAGE_STRUCT {
	FILTER_REPLY_HEADER header;
	DECOYMON_REPLY_MESSAGE_INFORMATION info;
} DECOYMON_REPLY_MESSAGE_STRUCT, *PDECOYMON_REPLY_MESSAGE_STRUCT;


extern FilterState gFilterState;

NTSTATUS DecoyMon_InitMiniFilter(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);

NTSTATUS DecoyMon_Unload(FLT_FILTER_UNLOAD_FLAGS flags);
NTSTATUS DecoyMon_InstanceSetup(PCFLT_RELATED_OBJECTS pFilterObjects, FLT_INSTANCE_SETUP_FLAGS flags, DEVICE_TYPE volumeDeviceType, FLT_FILESYSTEM_TYPE volumeFilesystemType);
NTSTATUS DecoyMon_InstanceQueryTeardown(PCFLT_RELATED_OBJECTS pFilterObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS flags);
VOID DecoyMon_InstanceTeardownStart(PCFLT_RELATED_OBJECTS pFilterObjects, FLT_INSTANCE_TEARDOWN_FLAGS flags);
VOID DecoyMon_InstanceTeardownComplete(PCFLT_RELATED_OBJECTS pFilterObjects, FLT_INSTANCE_TEARDOWN_FLAGS flags);

FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreOpenOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*);
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreCleanupOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*);
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreReadOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*);
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreWriteOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*);
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreMemoryMappingOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*);
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreSetInformationOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*);
FLT_POSTOP_CALLBACK_STATUS DecoyMonIRP_PostDirectoryQueryOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID, FLT_POST_OPERATION_FLAGS flags);
FLT_POSTOP_CALLBACK_STATUS DecoyMonIRP_PostCreateOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID, FLT_POST_OPERATION_FLAGS flags);
FLT_POSTOP_CALLBACK_STATUS DecoyMonIRP_PostSetInformationOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID, FLT_POST_OPERATION_FLAGS flags);

BOOLEAN DecoyMonHelper_ShouldHideFile(PUNICODE_STRING parentDirectory, PUNICODE_STRING fileName);

NTSTATUS DecoyMonPort_ConnectNotify(_In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionPortCookie);
VOID DecoyMonPort_DisconnectNotify(PVOID ConnectionCookie);
NTSTATUS DecoyMonPort_MessageNotify(_In_opt_ PVOID PortCookie, _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength, _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength, _Out_ PULONG ReturnOutputBufferLength);

BOOLEAN IsSystemOrProtectedProcess(PFLT_CALLBACK_DATA pData);