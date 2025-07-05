#include "pch.h"
#include "minifilter.h"
#include "driver.h"

extern const TraceLoggingHProvider g_hKernelProvider;

/// <summary>
/// Initializes the minifilter driver by setting up registry keys and registering filter callbacks.
/// </summary>
/// <param name="pDriverObject">Pointer to the driver object representing the minifilter driver.</param>
/// <param name="pRegistryPath">Pointer to a Unicode string containing the registry path for the driver.</param>
/// <returns>An NTSTATUS code indicating the success or failure of the initialization process.</returns>
NTSTATUS DecoyMon_InitMiniFilter(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
	HANDLE hRegistryKey = nullptr, hRegistrySubkey = nullptr;
	NTSTATUS initializationStatus = STATUS_SUCCESS;
	do {
		/* * * * * * * * * * * * * */
		/* REGISTRY INITIALIZATION */
		/* * * * * * * * * * * * * */

		// open the given registry key (the one containing properties for the whole driver) 
		OBJECT_ATTRIBUTES keyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(pRegistryPath, OBJ_KERNEL_HANDLE);
		initializationStatus = ZwOpenKey(&hRegistryKey, KEY_WRITE, &keyAttributes);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO OPEN REGISTRY KEY FOR MINIFILTER REGISTER OPERATION (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO OPEN REGISTRY KEY FOR MINIFILTER REGISTER OPERATION", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		// inside said key, create a subkey for the "instances" object
		UNICODE_STRING subKey = RTL_CONSTANT_STRING(L"Instances");
		OBJECT_ATTRIBUTES subkeyAttributes;
		InitializeObjectAttributes(&subkeyAttributes, &subKey, OBJ_KERNEL_HANDLE, hRegistryKey, nullptr);
		initializationStatus = ZwCreateKey(&hRegistrySubkey, KEY_WRITE, &subkeyAttributes, 0, nullptr, 0, nullptr);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO CREATE SUBREGISTRY KEY FOR MINIFILTER REGISTER OPERATION (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO CREATE SUBREGISTRY KEY FOR MINIFILTER REGISTER OPERATION", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		// set the value of the subkey to "DefaultInstance"
		UNICODE_STRING valueName = RTL_CONSTANT_STRING(L"DefaultInstance");
		WCHAR name[] = L"DecoyMonDefaultInstance";
		initializationStatus = ZwSetValueKey(hRegistrySubkey, &valueName, 0, REG_SZ, name, sizeof(name));
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO SET VALUE TO SUBREGISTRY KEY FOR MINIFILTER REGISTER OPERATION (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO SET VALUE TO SUBREGISTRY KEY FOR MINIFILTER REGISTER OPERATION", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		// create an "instance" key under "instances"
		UNICODE_STRING instanceKeyName;
		RtlInitUnicodeString(&instanceKeyName, name);
		HANDLE hInstanceKey;
		InitializeObjectAttributes(&subkeyAttributes, &instanceKeyName, OBJ_KERNEL_HANDLE, hRegistrySubkey, nullptr);
		initializationStatus = ZwCreateKey(&hInstanceKey, KEY_WRITE, &subkeyAttributes, 0, nullptr, 0, nullptr);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO CREATE SUBREGISTRY KEY %ws FOR MINIFILTER REGISTER OPERATION (0x%X)\n", DRIVER_PREFIX, name, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO CREATE SUBREGISTRY INSTANCE KEY FOR MINIFILTER REGISTER OPERATION", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		// inside said instance key, write the filter's altitude.
		// an appropiate value would be "392000 - 394999: FSFilter Security Monitor" or "320000 - 329998: FSFilter Anti-Virus"
		// https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/allocated-altitudes
		WCHAR altitude[] = L"394001";  // old was 427998
		UNICODE_STRING altitudeName = RTL_CONSTANT_STRING(L"Altitude");
		initializationStatus = ZwSetValueKey(hInstanceKey, &altitudeName, 0, REG_SZ, altitude, sizeof(altitude));
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO SET ALTITUDE %ws FOR MINIFILTER REGISTER OPERATION (0x%X)\n", DRIVER_PREFIX, altitude, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO SET ALTITUDE VALUE FOR MINIFILTER REGISTER OPERATION", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		// inside said instance key, write the filter's flags
		UNICODE_STRING flagsName = RTL_CONSTANT_STRING(L"Flags");
		ULONG flags = 0;
		initializationStatus = ZwSetValueKey(hInstanceKey, &flagsName, 0, REG_DWORD, &flags, sizeof(flags));
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO SET FLAGS 0x%X FOR MINIFILTER REGISTER OPERATION (0x%X)\n", DRIVER_PREFIX, flags, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO SET FLAGS FOR MINIFILTER REGISTER OPERATION", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		ZwClose(hInstanceKey);


		/* * * * * * * * * * * * * * * * */
		/* CALLBACK FUNCTION REFERENCING */
		/* * * * * * * * * * * * * * * * */


		FLT_OPERATION_REGISTRATION const callbacks[] = {
			{IRP_MJ_CREATE, 0, DecoyMonIRP_PreOpenOperation, DecoyMonIRP_PostCreateOperation},
			{IRP_MJ_CLEANUP, 0, DecoyMonIRP_PreCleanupOperation, nullptr},
			{IRP_MJ_READ, 0, DecoyMonIRP_PreReadOperation, nullptr},
			{IRP_MJ_WRITE, 0, DecoyMonIRP_PreWriteOperation, nullptr},
			{IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION, 0, DecoyMonIRP_PreMemoryMappingOperation, nullptr},
			{IRP_MJ_SET_INFORMATION, 0, DecoyMonIRP_PreSetInformationOperation, DecoyMonIRP_PostSetInformationOperation},
			{IRP_MJ_DIRECTORY_CONTROL, 0, nullptr, DecoyMonIRP_PostDirectoryQueryOperation},
			{IRP_MJ_OPERATION_END}
		};

		FLT_REGISTRATION const registration = {
			sizeof(FLT_REGISTRATION),
			FLT_REGISTRATION_VERSION,
			0,
			nullptr,
			callbacks,
			DecoyMon_Unload,
			DecoyMon_InstanceSetup,
			DecoyMon_InstanceQueryTeardown,
			DecoyMon_InstanceTeardownStart,
			DecoyMon_InstanceTeardownComplete,
		};

		// register the filter with the system
		initializationStatus = FltRegisterFilter(pDriverObject, &registration, &gFilterState.pFilterObject);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED FltRegisterFilter CALL (inside minifilter.cpp) (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED FltRegisterFilter CALL", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

	} while (false);

	// delete the subkey if it was not successfully initialized
	if (hRegistrySubkey) {
		if (!NT_SUCCESS(initializationStatus))
			ZwDeleteKey(hRegistrySubkey);
		ZwClose(hRegistrySubkey);
	}
	if (hRegistryKey)
		ZwClose(hRegistryKey);

	return initializationStatus;
}


/// <summary>
/// Unloads the DecoyMon minifilter driver and cleans up all associated resources.
/// </summary>
/// <param name="flags">Flags that specify how the filter should be unloaded. Currently unused.</param>
/// <returns>An NTSTATUS code indicating the success or failure of the unload operation. Typically returns STATUS_SUCCESS.</returns>
NTSTATUS DecoyMon_Unload(FLT_FILTER_UNLOAD_FLAGS flags)
{
	// in reverse order, free everything, from the info inside the hashtable, to the filter, to the symlink and the device
	UNREFERENCED_PARAMETER(flags);

	// free "decoy file path" hashtable and ERESOURCE
	gFilterState.fileHashTable->DestroyHashTable();
	ExFreePool(gFilterState.fileHashTable);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FREED DECOY PATH LIST\n", DRIVER_PREFIX);
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"FREED DECOY PATH LIST", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);
	gFilterState.executiveResourceFiles.Delete();

	// free "directory paths containing directories" hashtable and ERESOURCE
	gFilterState.directoryHashTable->DestroyHashTable();
	ExFreePool(gFilterState.directoryHashTable);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FREED DIRECTORY PATH LIST\n", DRIVER_PREFIX);
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"FREED DIRECTORY PATH LIST", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);
	gFilterState.executiveResourceDirectories.Delete();

	// free "excluded directory paths" iterable list and ERESOURCE
	gFilterState.excludedDirectoryList->DestroyIterableList();
	ExFreePool(gFilterState.excludedDirectoryList);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FREED EXCLUDED DIRECTORY LIST\n", DRIVER_PREFIX);
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"FREED EXCLUDED DIRECTORY LIST", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);
	gFilterState.executiveResourceExcluded.Delete();

	// free "armed" ERESOURCE (state of DecoyMon)
	gFilterState.executiveResourceArmedCheck.Delete();

	// close the communication port used to communicate with the service
	FltCloseCommunicationPort(gFilterState.pFilterPort);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: CLOSED COMMUNICATION PORT\n", DRIVER_PREFIX);
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"CLOSED COMMUNICATION PORT", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// unregister the filter from the system
	FltUnregisterFilter(gFilterState.pFilterObject);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: UNREGISTERED FILTER\n", DRIVER_PREFIX);
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"UNREGISTERED FILTER", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// delete the symbolic link created for the minifilter driver
	UNICODE_STRING symbolicLinkName = RTL_CONSTANT_STRING(L"\\??\\DecoyMon");
	IoDeleteSymbolicLink(&symbolicLinkName);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: DELETED SYMBOLIC LINK\n", DRIVER_PREFIX);
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"DELETED SYMBOLIC LINK", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// delete the device object created for the minifilter driver
	IoDeleteDevice(gFilterState.pDriverObject->DeviceObject);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: DELETED DEVICE\n", DRIVER_PREFIX);
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"DELETED DEVICE", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// stop the trace mechanism used for logging
	TraceLoggingUnregister(g_hKernelProvider);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[s] %s: UNLOAD FINISHED SUCCESSFULLY\n", DRIVER_PREFIX);

	return STATUS_SUCCESS;
}

/// <summary>
/// Sets up an instance for a filter driver, ensuring compatibility with NTFS volumes.
/// </summary>
/// <param name="pFilterObjects">A pointer to a structure containing filter-related objects. Currently unused.</param>
/// <param name="flags">Flags that specify setup options for the instance. Currently unused.</param>
/// <param name="volumeDeviceType">The type of device associated with the volume. Currently unused.</param>
/// <param name="volumeFilesystemType">The type of filesystem used by the volume.</param>
/// <returns>Returns STATUS_SUCCESS if the volume's filesystem type is NTFS; otherwise, returns STATUS_FLT_DO_NOT_ATTACH.</returns>
NTSTATUS DecoyMon_InstanceSetup(PCFLT_RELATED_OBJECTS pFilterObjects, FLT_INSTANCE_SETUP_FLAGS flags, DEVICE_TYPE volumeDeviceType, FLT_FILESYSTEM_TYPE volumeFilesystemType)
{
	UNREFERENCED_PARAMETER(pFilterObjects);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(volumeDeviceType);

	// check if the filesystem type is NTFS, as we only support NTFS volumes right now
	return volumeFilesystemType == FLT_FSTYPE_NTFS ? STATUS_SUCCESS : STATUS_FLT_DO_NOT_ATTACH;
}


/// <summary>
/// Handles instance query teardown for a filter driver and always returns a success status.
/// </summary>
/// <param name="pFilterObjects">A pointer to a structure containing filter-related objects. Currently unused.</param>
/// <param name="flags">Flags indicating the context of the instance query teardown. Currently unused.</param>
/// <returns>An NTSTATUS value indicating the result of the operation. Always returns STATUS_SUCCESS.</returns>
NTSTATUS DecoyMon_InstanceQueryTeardown(PCFLT_RELATED_OBJECTS pFilterObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS flags)
{
	UNREFERENCED_PARAMETER(pFilterObjects);
	UNREFERENCED_PARAMETER(flags);
	return STATUS_SUCCESS;
}

/// <summary>
/// Handles the start of instance teardown for a filter driver. Acts as a placeholder.
/// </summary>
/// <param name="pFilterObjects">A pointer to a structure containing filter-related objects. Currently unused.</param>
/// <param name="flags">Flags indicating the teardown operation specifics. Currently unused.</param>
VOID DecoyMon_InstanceTeardownStart(PCFLT_RELATED_OBJECTS pFilterObjects, FLT_INSTANCE_TEARDOWN_FLAGS flags)
{
	UNREFERENCED_PARAMETER(pFilterObjects);
	UNREFERENCED_PARAMETER(flags);
}

/// <summary>
/// Handles the completion of an instance teardown in a file system filter driver. Acts as a placeholder.
/// </summary>
/// <param name="pFilterObjects">A pointer to a structure containing filter-related objects. Currently unused.</param>
/// <param name="flags">Flags indicating the teardown operation specifics. Currently unused.</param>
VOID DecoyMon_InstanceTeardownComplete(PCFLT_RELATED_OBJECTS pFilterObjects, FLT_INSTANCE_TEARDOWN_FLAGS flags)
{
	UNREFERENCED_PARAMETER(pFilterObjects);
	UNREFERENCED_PARAMETER(flags);
}


/// <summary>
/// Handles pre-operation for IRP_MJ_CREATE requests in the DecoyMon minifilter driver. Meant to watch out for copy and delete operations on decoy files.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation</param>
/// <param name="completionContext">Unimplemented.</param>
/// <returns>FLT_PREOP_SUCCESS_NO_CALLBACK only if system is not armed.
/// FLT_PREOP_SUCCESS_WITH_CALLBACK if the operation is allowed to pass through.
/// FLT_PREOP_COMPLETE if the operation is denied, with the IRP being adjusted properly.</returns>
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreOpenOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*)
{
	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */

	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// ignore kernel mode requests
	if (pData->RequestorMode == KernelMode) {
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	// i've disarmed trustless detection, so now anyone can actually open these files. we only need to check for deletion and copy intent
	// https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/km-file-copy - PREACH
	auto& params = pData->Iopb->Parameters.Create;
	BOOLEAN isCopyOperation = IoCheckFileObjectOpenedAsCopySource(pFilterObjects->FileObject);
	BOOLEAN isDeleteOperation = (params.Options & FILE_DELETE_ON_CLOSE) > 0;
	if (!(isCopyOperation || isDeleteOperation))
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;

	UNICODE_STRING operation; 
	if (isCopyOperation)
		operation = RTL_CONSTANT_STRING(L"COPY");
	else
		operation = RTL_CONSTANT_STRING(L"DELETE ON CLOSE");


	/* * * * * * * * * * *  */
	/* TARGET FILE CHECKING */
	/* * * * * * * * * * *  */

	// get the file name information for the target file
	NTSTATUS operationStatus;
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);
	if (!NT_SUCCESS(operationStatus)) {
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	// check if the file is a decoy file or a directory that contains decoy files
	gFilterState.executiveResourceFiles.LockRead();
	gFilterState.executiveResourceDirectories.LockRead();
	auto pDecoyResource = gFilterState.fileHashTable->StringExistsInHashTable(&pFileInfo->Name);
	auto pDirectoryResource = gFilterState.directoryHashTable->StringExistsInHashTable(&pFileInfo->Name);
	gFilterState.executiveResourceDirectories.UnlockRead();
	gFilterState.executiveResourceFiles.UnlockRead();

	// check if the process is a system or protected process, in which case we do not want to interfere with its operations
	if (IsSystemOrProtectedProcess(pData)) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;  // was NO_CALLBACK previously.
	}

	// if the file is not a decoy file or a directory that contains decoy files, we do not want to interfere with its operations
	if (pDecoyResource == nullptr && pDirectoryResource == nullptr) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}


	/* * * * * * * * * * * *  */
	/* PROCESS IDENTIFICATION */  // point in which we know the canary is targeted. fail operations by default
	/* * * * * * * * * * * *  */
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"IRP_MJ_CREATE PRE-CALLBACK TRIGGERED FOR DECOY. PREPARING DELEGATION TO SERVICE", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// get current PID, work up from there.
	PEPROCESS pEProcessFlt = FltGetRequestorProcess(pData);
	HANDLE hProcess; // close this one if successful;
	operationStatus = ObOpenObjectByPointer(pEProcessFlt, OBJ_KERNEL_HANDLE, nullptr, 0x0400, nullptr, KernelMode, &hProcess);
	if (!NT_SUCCESS(operationStatus)) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	PROCESS_BASIC_INFORMATION processInfo;
	ULONG returnedValue = 0;
	operationStatus = ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &processInfo, sizeof(PROCESS_BASIC_INFORMATION), &returnedValue);
	if (!NT_SUCCESS(operationStatus)) {
		FltClose(hProcess);
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FILE %wZ %wZ ATTEMPT BY %llu (PPID: %llu). DELEGATING TO USERMODE FOR VERDICT.\n", DRIVER_PREFIX, &pFileInfo->Name, operation, processInfo.UniqueProcessId, processInfo.InheritedFromUniqueProcessId);
	FltClose(hProcess);


	/* * * * * * * * * * * * */
	/* OPERATION DISPATCHING */
	/* * * * * * * * * * * * */

	// prepare the message to send to the usermode service for verdict, as well as the reply structure
	DECOYMON_SEND_MESSAGE_INFORMATION sendMessage;
	sendMessage.pid = processInfo.UniqueProcessId;
	sendMessage.operationType = (isDeleteOperation) ? OPERATION_TYPE::DECOY_DELETED : OPERATION_TYPE::DECOY_COPIED;
	RtlCopyMemory(sendMessage.path, pFileInfo->Name.Buffer, min(ARRAYSIZE(sendMessage.path) * sizeof(WCHAR), pFileInfo->Name.MaximumLength));

	DECOYMON_REPLY_MESSAGE_INFORMATION replyStruct;
	ULONG replyBuffer = sizeof(DECOYMON_REPLY_MESSAGE_INFORMATION);

	// send the message to the usermode service and wait for a reply. if the reply is not received within 2 seconds, we assume the service is not running or is unresponsive, and we reject the operation by default.
	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000000 * 2; // 2 seconds, to receive the reply. if it times out, it is rejected by default.
	operationStatus = FltSendMessage(gFilterState.pFilterObject, &gFilterState.pClientPort, &sendMessage, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION), &replyStruct, &replyBuffer, &timeout);

	BOOLEAN reject = TRUE;
	if (NT_SUCCESS(operationStatus)) {
		reject = replyStruct.isPIDRejected;
	}

	// explicit reject or timeout => REJECT
	if (reject || operationStatus == STATUS_TIMEOUT) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ %wZ ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, operation, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_CREATE PRE-CALLBACK ACCESS DENIED. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
	// explicit accept => ACCEPT
	else if (reject == FALSE) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ %wZ ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN ALLOWED.\n", DRIVER_PREFIX, &pFileInfo->Name, operation, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_CREATE PRE-CALLBACK ALLOWED TO CONTINUE. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}
	// unknown behaviour => REJECT by default
	else {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ %wZ ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, operation, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_CREATE PRE-CALLBACK ACCESS DENIED. SERVICE TIMEOUT. IMPLICIT DENY", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
}


/// <summary>
/// Handles pre-operation for IRP_MJ_CLEANUP requests in the DecoyMon minifilter driver. Meant to watch out for delete operations on decoy files.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation. Currently unused.</param>
/// <param name="completionContext">Unimplemented.</param>
/// <returns>FLT_PREOP_SUCCESS_NO_CALLBACK if the system is not armed, or if the operation is allowed to pass through.
/// FLT_PREOP_COMPLETE if the operation is denied, with the IRP being adjusted properly.</returns>
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreCleanupOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*)
{
	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */

	UNREFERENCED_PARAMETER(pFilterObjects);

	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// ignore kernel mode requests
	if (pData->RequestorMode == KernelMode) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// specifically, this is an edge case for cross-volume moves.
	if (!pFilterObjects->FileObject->DeletePending)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;


	/* * * * * * * * * * *  */
	/* TARGET FILE CHECKING */
	/* * * * * * * * * * *  */

	// get the file name information for the target file
	NTSTATUS operationStatus;
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);
	if (!NT_SUCCESS(operationStatus)) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// check if the file is a decoy file or a directory that contains decoy files
	gFilterState.executiveResourceFiles.LockRead();
	gFilterState.executiveResourceDirectories.LockRead();
	auto pDecoyResource = gFilterState.fileHashTable->StringExistsInHashTable(&pFileInfo->Name);
	auto pDirectoryResource = gFilterState.directoryHashTable->StringExistsInHashTable(&pFileInfo->Name);
	gFilterState.executiveResourceDirectories.UnlockRead();
	gFilterState.executiveResourceFiles.UnlockRead();

	// check if the process is a system or protected process, in which case we do not want to interfere with its operations
	if (IsSystemOrProtectedProcess(pData)) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// if the file is not a decoy file or a directory that contains decoy files, we do not want to interfere with its operations
	if (pDecoyResource == nullptr && pDirectoryResource == nullptr) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	/* * * * * * * * * * * *  */
	/* PROCESS IDENTIFICATION */  // point in which we know the canary is targeted. fail operations by default
	/* * * * * * * * * * * *  */
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"IRP_MJ_CLEANUP PRE-CALLBACK TRIGGERED FOR DECOY. PREPARING DELEGATION TO SERVICE", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// get current PID, work up from there.
	PEPROCESS pEProcessFlt = FltGetRequestorProcess(pData);
	HANDLE hProcess; // close this one if successful;
	operationStatus = ObOpenObjectByPointer(pEProcessFlt, OBJ_KERNEL_HANDLE, nullptr, 0x0400, nullptr, KernelMode, &hProcess);
	if (!NT_SUCCESS(operationStatus)) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	PROCESS_BASIC_INFORMATION processInfo;
	ULONG returnedValue = 0;
	operationStatus = ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &processInfo, sizeof(PROCESS_BASIC_INFORMATION), &returnedValue);
	if (!NT_SUCCESS(operationStatus)) {
		FltClose(hProcess);
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FILE %wZ CLEANUP ATTEMPT BY %llu (PPID: %llu). DELEGATING TO USERMODE FOR VERDICT.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId, processInfo.InheritedFromUniqueProcessId);
	FltClose(hProcess);


	/* * * * * * * * * * * * */
	/* OPERATION DISPATCHING */
	/* * * * * * * * * * * * */

	// prepare the message to send to the usermode service for verdict, as well as the reply structure
	DECOYMON_SEND_MESSAGE_INFORMATION sendMessage;
	sendMessage.pid = processInfo.UniqueProcessId;
	sendMessage.operationType = OPERATION_TYPE::DECOY_DELETED;
	RtlCopyMemory(sendMessage.path, pFileInfo->Name.Buffer, min(ARRAYSIZE(sendMessage.path) * sizeof(WCHAR), pFileInfo->Name.MaximumLength));

	DECOYMON_REPLY_MESSAGE_INFORMATION replyStruct;
	ULONG replyBuffer = sizeof(DECOYMON_REPLY_MESSAGE_INFORMATION);

	// send the message to the usermode service and wait for a reply. if the reply is not received within 2 seconds, we assume the service is not running or is unresponsive, and we reject the operation by default.
	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000000 * 2; // 2 seconds, to receive the reply. if it times out, it is rejected by default.
	operationStatus = FltSendMessage(gFilterState.pFilterObject, &gFilterState.pClientPort, &sendMessage, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION), &replyStruct, &replyBuffer, &timeout);

	BOOLEAN reject = TRUE;
	if (NT_SUCCESS(operationStatus)) {
		reject = replyStruct.isPIDRejected;
	}

	// explicit reject or timeout => REJECT
	if (reject || operationStatus == STATUS_TIMEOUT) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ CLEANUP ATTEMPT ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_CLEANUP PRE-CALLBACK ACCESS DENIED. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
	// explicit accept => ACCEPT
	else if (reject == FALSE) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ CLEANUP ATTEMPT ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN ALLOWED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_CLEANUP PRE-CALLBACK ALLOWED TO CONTINUE. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	// unknown behaviour => REJECT by default
	else {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ CLEANUP ATTEMPT ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_CLEANUP PRE-CALLBACK ACCESS DENIED. SERVICE TIMEOUT. IMPLICIT DENY.", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
}


/// <summary>
/// Handles pre-operation for IRP_MJ_READ requests in the DecoyMon minifilter driver. Meant to watch out for read operations on decoy files.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation. Currently unused.</param>
/// <param name="completionContext">Unimplemented.</param>
/// <returns>FLT_PREOP_SUCCESS_NO_CALLBACK if the system is not armed, or if the operation is allowed to pass through.
/// FLT_PREOP_COMPLETE if the operation is denied, with the IRP being adjusted properly.</returns>
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreReadOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*)
{
	UNREFERENCED_PARAMETER(pFilterObjects);


	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */

	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// ignore kernel mode requests
	if (pData->RequestorMode == KernelMode) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	/* * * * * * * * * * *  */
	/* TARGET FILE CHECKING */
	/* * * * * * * * * * *  */

	// get the file name information for the target file
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	NTSTATUS operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);
	if (!NT_SUCCESS(operationStatus))
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// check if the file is a decoy file
	gFilterState.executiveResourceFiles.LockRead();
	auto pDecoyResource = gFilterState.fileHashTable->StringExistsInHashTable(&pFileInfo->Name);
	gFilterState.executiveResourceFiles.UnlockRead();

	// check if the process is a system or protected process, in which case we do not want to interfere with its operations
	if (IsSystemOrProtectedProcess(pData)) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// if the file is not a decoy file or a directory that contains decoy files, we do not want to interfere with its operations
	if (pDecoyResource == nullptr) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	/* * * * * * * * * * * *  */
	/* PROCESS IDENTIFICATION */
	/* * * * * * * * * * * *  */
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"IRP_MJ_READ PRE-CALLBACK TRIGGERED FOR DECOY. PREPARING DELEGATION TO SERVICE", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);
	
	// get current PID, work up from there.
	PEPROCESS pEProcessFlt = FltGetRequestorProcess(pData);
	HANDLE hProcess; // close this one if successful;
	operationStatus = ObOpenObjectByPointer(pEProcessFlt, OBJ_KERNEL_HANDLE, nullptr, 0x0400, nullptr, KernelMode, &hProcess);
	if (!NT_SUCCESS(operationStatus)) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	PROCESS_BASIC_INFORMATION processInfo;
	ULONG returnedValue = 0;
	operationStatus = ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &processInfo, sizeof(PROCESS_BASIC_INFORMATION), &returnedValue);
	if (!NT_SUCCESS(operationStatus)) {
		FltClose(hProcess);
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FILE %wZ READ ATTEMPT BY %llu (PPID: %llu). DELEGATING TO USERMODE FOR VERDICT.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId, processInfo.InheritedFromUniqueProcessId);
	FltClose(hProcess);


	/* * * * * * * * * * * * */
	/* OPERATION DISPATCHING */
	/* * * * * * * * * * * * */

	// prepare the message to send to the usermode service for verdict, as well as the reply structure
	DECOYMON_SEND_MESSAGE_INFORMATION sendMessage;
	sendMessage.pid = processInfo.UniqueProcessId;
	sendMessage.operationType = OPERATION_TYPE::DECOY_READ;
	RtlCopyMemory(sendMessage.path, pFileInfo->Name.Buffer, min(ARRAYSIZE(sendMessage.path) * sizeof(WCHAR), pFileInfo->Name.MaximumLength));

	DECOYMON_REPLY_MESSAGE_INFORMATION replyStruct;
	ULONG replyBuffer = sizeof(DECOYMON_REPLY_MESSAGE_INFORMATION);

	// send the message to the usermode service and wait for a reply. if the reply is not received within 2 seconds, we assume the service is not running or is unresponsive, and we reject the operation by default.
	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000000 * 2; // 2 seconds, to receive the reply. if it times out, it is rejected by default.
	operationStatus = FltSendMessage(gFilterState.pFilterObject, &gFilterState.pClientPort, &sendMessage, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION), &replyStruct, &replyBuffer, &timeout);

	BOOLEAN reject = TRUE;
	if (NT_SUCCESS(operationStatus)) {
		reject = replyStruct.isPIDRejected;
	}

	// explicit reject or timeout => REJECT
	if (reject || operationStatus == STATUS_TIMEOUT) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ READ ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_READ PRE-CALLBACK ACCESS DENIED. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
	// explicit accept => ACCEPT
	else if (reject == FALSE) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ READ ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN ALLOWED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_READ PRE-CALLBACK ALLOWED TO CONTINUE. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	// unknown behaviour => REJECT by default
	else {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ READ ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_CREATE PRE-CALLBACK ACCESS DENIED. SERVICE TIMEOUT. IMPLICIT DENY", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
}


/// <summary>
/// Handles pre-operation for IRP_MJ_WRITE requests in the DecoyMon minifilter driver. Meant to watch out for write operations on decoy files.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation. Currently unused.</param>
/// <param name="completionContext">Unimplemented.</param>
/// <returns>FLT_PREOP_SUCCESS_NO_CALLBACK if the system is not armed, or if the operation is allowed to pass through.
/// FLT_PREOP_COMPLETE if the operation is denied, with the IRP being adjusted properly.</returns>
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreWriteOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*)
{
	UNREFERENCED_PARAMETER(pFilterObjects);


	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */

	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// ignore kernel mode requests
	if (pData->RequestorMode == KernelMode) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	/* * * * * * * * * * *  */
	/* TARGET FILE CHECKING */
	/* * * * * * * * * * *  */

	// get the file name information for the target file
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	NTSTATUS operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);
	if (!NT_SUCCESS(operationStatus))
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// check if the file is a decoy file
	gFilterState.executiveResourceFiles.LockRead();
	auto pDecoyResource = gFilterState.fileHashTable->StringExistsInHashTable(&pFileInfo->Name);
	gFilterState.executiveResourceFiles.UnlockRead();

	// check if the process is a system or protected process, in which case we do not want to interfere with its operations
	if (IsSystemOrProtectedProcess(pData)) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// if the file is not a decoy file or a directory that contains decoy files, we do not want to interfere with its operations
	if (pDecoyResource == nullptr) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	/* * * * * * * * * * * *  */
	/* PROCESS IDENTIFICATION */
	/* * * * * * * * * * * *  */
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"IRP_MJ_WRITE PRE-CALLBACK TRIGGERED FOR DECOY. PREPARING DELEGATION TO SERVICE", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// get current PID, work up from there.
	PEPROCESS pEProcessFlt = FltGetRequestorProcess(pData);
	HANDLE hProcess; // close this one if successful;
	operationStatus = ObOpenObjectByPointer(pEProcessFlt, OBJ_KERNEL_HANDLE, nullptr, 0x0400, nullptr, KernelMode, &hProcess);
	if (!NT_SUCCESS(operationStatus)) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	PROCESS_BASIC_INFORMATION processInfo;
	ULONG returnedValue = 0;
	operationStatus = ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &processInfo, sizeof(PROCESS_BASIC_INFORMATION), &returnedValue);
	if (!NT_SUCCESS(operationStatus)) {
		FltClose(hProcess);
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FILE %wZ WRITE ATTEMPT BY %llu (PPID: %llu). DELEGATING TO USERMODE FOR VERDICT.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId, processInfo.InheritedFromUniqueProcessId);
	FltClose(hProcess);


	/* * * * * * * * * * * * */
	/* OPERATION DISPATCHING */
	/* * * * * * * * * * * * */

	// prepare the message to send to the usermode service for verdict, as well as the reply structure
	DECOYMON_SEND_MESSAGE_INFORMATION sendMessage;
	sendMessage.pid = processInfo.UniqueProcessId;
	sendMessage.operationType = OPERATION_TYPE::DECOY_READ;
	RtlCopyMemory(sendMessage.path, pFileInfo->Name.Buffer, min(ARRAYSIZE(sendMessage.path) * sizeof(WCHAR), pFileInfo->Name.MaximumLength));

	DECOYMON_REPLY_MESSAGE_INFORMATION replyStruct;
	ULONG replyBuffer = sizeof(DECOYMON_REPLY_MESSAGE_INFORMATION);

	// send the message to the usermode service and wait for a reply. if the reply is not received within 2 seconds, we assume the service is not running or is unresponsive, and we reject the operation by default.
	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000000 * 2; // 2 seconds, to receive the reply. if it times out, it is rejected by default.
	operationStatus = FltSendMessage(gFilterState.pFilterObject, &gFilterState.pClientPort, &sendMessage, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION), &replyStruct, &replyBuffer, &timeout);

	BOOLEAN reject = TRUE;
	if (NT_SUCCESS(operationStatus)) {
		reject = replyStruct.isPIDRejected;
	}

	// explicit reject or timeout => REJECT
	if (reject || operationStatus == STATUS_TIMEOUT) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING %wZ WRITE ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_WRITE PRE-CALLBACK ACCESS DENIED. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);


		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
	// explicit accept => ACCEPT
	else if (reject == FALSE) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ WRITE ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN ALLOWED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_WRITE PRE-CALLBACK ALLOWED TO CONTINUE. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	// unknown behaviour => REJECT by default
	else {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING %wZ WRITE ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_CREATE PRE-CALLBACK ACCESS DENIED. SERVICE TIMEOUT. IMPLICIT DENY", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
}


/// <summary>
/// Handles pre-operation for IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION requests in the DecoyMon minifilter driver. Meant to watch out for memory mapping operations on decoy files.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation. Currently unused.</param>
/// <param name="completionContext">Unimplemented.</param>
/// <returns>FLT_PREOP_SUCCESS_NO_CALLBACK if the system is not armed, or if the operation is allowed to pass through.
/// FLT_PREOP_COMPLETE if the operation is denied, with the IRP being adjusted properly.</returns>
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreMemoryMappingOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*)
{
	UNREFERENCED_PARAMETER(pFilterObjects);


	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */

	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// ignore kernel mode requests
	if (pData->RequestorMode == KernelMode) {
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	/* * * * * * * * * * *  */
	/* TARGET FILE CHECKING */
	/* * * * * * * * * * *  */

	// get the file name information for the target file
	NTSTATUS operationStatus;
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);
	if (!NT_SUCCESS(operationStatus))
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// check if the file is a decoy file
	gFilterState.executiveResourceFiles.LockRead();
	auto pDecoyResource = gFilterState.fileHashTable->StringExistsInHashTable(&pFileInfo->Name);
	gFilterState.executiveResourceFiles.UnlockRead();

	// check if the process is a system or protected process, in which case we do not want to interfere with its operations
	if (IsSystemOrProtectedProcess(pData)) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// if the file is not a decoy file or a directory that contains decoy files, we do not want to interfere with its operations
	if (pDecoyResource == nullptr) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	/* * * * * * * * * * * *  */
	/* PROCESS IDENTIFICATION */
	/* * * * * * * * * * * *  */
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION PRE-CALLBACK TRIGGERED FOR DECOY. PREPARING DELEGATION TO SERVICE", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// get current PID, work up from there.
	PEPROCESS pEProcessFlt = FltGetRequestorProcess(pData);
	HANDLE hProcess; // close this one if successful;
	operationStatus = ObOpenObjectByPointer(pEProcessFlt, OBJ_KERNEL_HANDLE, nullptr, 0x0400, nullptr, KernelMode, &hProcess);
	if (!NT_SUCCESS(operationStatus)) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	PROCESS_BASIC_INFORMATION processInfo;
	ULONG returnedValue = 0;
	operationStatus = ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &processInfo, sizeof(PROCESS_BASIC_INFORMATION), &returnedValue);
	if (!NT_SUCCESS(operationStatus)) {
		FltClose(hProcess);
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FILE %wZ MEMORY MAP ATTEMPT BY %llu (PPID: %llu). DELEGATING TO USERMODE FOR VERDICT.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId, processInfo.InheritedFromUniqueProcessId);
	FltClose(hProcess);


	/* * * * * * * * * * * * */
	/* OPERATION DISPATCHING */
	/* * * * * * * * * * * * */

	// prepare the message to send to the usermode service for verdict, as well as the reply structure
	DECOYMON_SEND_MESSAGE_INFORMATION sendMessage;
	sendMessage.pid = processInfo.UniqueProcessId;
	sendMessage.operationType = OPERATION_TYPE::DECOY_MEMORYMAPPED;
	RtlCopyMemory(sendMessage.path, pFileInfo->Name.Buffer, min(ARRAYSIZE(sendMessage.path) * sizeof(WCHAR), pFileInfo->Name.MaximumLength));

	DECOYMON_REPLY_MESSAGE_INFORMATION replyStruct;
	ULONG replyBuffer = sizeof(DECOYMON_REPLY_MESSAGE_INFORMATION);

	// send the message to the usermode service and wait for a reply. if the reply is not received within 2 seconds, we assume the service is not running or is unresponsive, and we reject the operation by default.
	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000000 * 2; // 2 seconds, to receive the reply. if it times out, it is rejected by default.
	operationStatus = FltSendMessage(gFilterState.pFilterObject, &gFilterState.pClientPort, &sendMessage, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION), &replyStruct, &replyBuffer, &timeout);

	BOOLEAN reject = TRUE;
	if (NT_SUCCESS(operationStatus)) {
		reject = replyStruct.isPIDRejected;
	}

	// explicit reject or timeout => REJECT
	if (reject || operationStatus == STATUS_TIMEOUT) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ MEMORY MAP ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION PRE-CALLBACK ACCESS DENIED. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
	// explicit accept => ACCEPT
	else if (replyStruct.isPIDRejected == FALSE) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ MEMORY MAP ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN ALLOWED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION PRE-CALLBACK ALLOWED TO CONTINUE. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	// unknown behaviour => REJECT by default
	else {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ MEMORY MAP ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION PRE-CALLBACK ACCESS DENIED. SERVICE TIMEOUT. IMPLICIT DENY", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
}


/// <summary>
/// Handles pre-operation for IRP_MJ_SET_INFORMATION requests in the DecoyMon minifilter driver. Meant to watch out for rename and delete on decoy files and directories containing decoy files.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation. Currently unused.</param>
/// <param name="completionContext">Unimplemented.</param>
/// <returns>FLT_PREOP_SUCCESS_NO_CALLBACK if the system is not armed or if the requestor is SYSTEM, PP or PPL.
/// FLT_PREOP_SUCCESS_WITH_CALLBACK if the operation is allowed to pass through.
/// FLT_PREOP_COMPLETE if the operation is denied, with the IRP being adjusted properly.</returns>
FLT_PREOP_CALLBACK_STATUS DecoyMonIRP_PreSetInformationOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID*)
{
	UNREFERENCED_PARAMETER(pFilterObjects);


	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */

	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	// ignore kernel mode requests
	if (pData->RequestorMode == KernelMode) {
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	// we only care for delete or rename operations, the others can pass through.
	auto& operationTypeHolder = pData->Iopb->Parameters.SetFileInformation;
	if (!(operationTypeHolder.FileInformationClass == FileDispositionInformation ||
		operationTypeHolder.FileInformationClass == FileDispositionInformationEx ||
		operationTypeHolder.FileInformationClass == FileRenameInformation ||
		operationTypeHolder.FileInformationClass == FileRenameInformationEx))
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;


	/* * * * * * * * * * * * */
	/* TARGET FILE CHECKING  */
	/* * * * * * * * * * * * */

	// get the file name information for the target file
	NTSTATUS operationStatus;
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);
	if (!NT_SUCCESS(operationStatus)) {
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	// check if the file is a decoy file or a directory that contains decoy files
	gFilterState.executiveResourceFiles.LockRead();
	gFilterState.executiveResourceDirectories.LockRead();
	auto pDecoyResource = gFilterState.fileHashTable->StringExistsInHashTable(&pFileInfo->Name);
	auto pDirectoryResource = gFilterState.directoryHashTable->StringExistsInHashTable(&pFileInfo->Name);
	gFilterState.executiveResourceDirectories.UnlockRead();
	gFilterState.executiveResourceFiles.UnlockRead();

	// check if the process is a system or protected process, in which case we do not want to interfere with its operations
	if (IsSystemOrProtectedProcess(pData)) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// if the file is not a decoy file or a directory that contains decoy files, we do not want to interfere with its operations
	if (!(pDecoyResource != nullptr || pDirectoryResource != nullptr)) {
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}


	/* * * * * * * * * * * *  */
	/* PROCESS IDENTIFICATION */
	/* * * * * * * * * * * *  */
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"IRP_MJ_SET_INFORMATION PRE-CALLBACK TRIGGERED FOR DECOY. PREPARING DELEGATION TO SERVICE", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// get current PID, work up from there.
	PEPROCESS pEProcessFlt = FltGetRequestorProcess(pData);
	HANDLE hProcess; // close this one if successful;
	operationStatus = ObOpenObjectByPointer(pEProcessFlt, OBJ_KERNEL_HANDLE, nullptr, 0x0400, nullptr, KernelMode, &hProcess);
	if (!NT_SUCCESS(operationStatus)) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	PROCESS_BASIC_INFORMATION processInfo;
	ULONG returnedValue = 0;
	operationStatus = ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &processInfo, sizeof(PROCESS_BASIC_INFORMATION), &returnedValue);
	if (!NT_SUCCESS(operationStatus)) {
		FltClose(hProcess);
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}
	ZwQueryInformationProcess(hProcess, ProcessBasicInformation, &processInfo, sizeof(PROCESS_BASIC_INFORMATION), &returnedValue);
	FltClose(hProcess);


	/* * * * * * * * * * * * * * * * * * * * * * * */
	/* IRP_MJ_SET_INFORMATION OPERATION BRANCHING  */
	/* * * * * * * * * * * * * * * * * * * * * * * */

	// at this point, we established that the operation is of type "rename" or "delete", and that one of our decoy files is being targeted.
	// next, we can fine-tune this, by checking for specific behaviour for FileDispositionInformation (i.e. DeleteFile being true) or
	// for FileRenameInformation
	if (operationTypeHolder.FileInformationClass == FileRenameInformation || operationTypeHolder.FileInformationClass == FileRenameInformationEx) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FILE %wZ RENAME ATTEMPT BY %llu (PPID: %llu). DELEGATING TO USERMODE FOR VERDICT\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId, processInfo.InheritedFromUniqueProcessId);
	}
	if (operationTypeHolder.FileInformationClass == FileDispositionInformation || operationTypeHolder.FileInformationClass == FileDispositionInformationEx) {
		auto deleteInfo = (FILE_DISPOSITION_INFORMATION*)operationTypeHolder.InfoBuffer;  // normalize this
		if (deleteInfo->DeleteFile & 1) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: FILE %wZ DELETE ATTEMPT BY %llu (PPID: %llu). DELEGATING TO USERMODE FOR VERDICT\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId, processInfo.InheritedFromUniqueProcessId);
		}
	}


	/* * * * * * * * * * * * */
	/* OPERATION DISPATCHING */
	/* * * * * * * * * * * * */

	// prepare the message to send to the usermode service for verdict, as well as the reply structure
	DECOYMON_SEND_MESSAGE_INFORMATION sendMessage;
	sendMessage.pid = processInfo.UniqueProcessId;
	BOOLEAN isDirectory = 0;
	FltIsDirectory(pFilterObjects->FileObject, pFilterObjects->Instance, &isDirectory);
	if (isDirectory)
		sendMessage.operationType = (operationTypeHolder.FileInformationClass == FileRenameInformation || operationTypeHolder.FileInformationClass == FileRenameInformationEx) ? OPERATION_TYPE::DECOY_DIR_RENAMED : OPERATION_TYPE::DECOY_DIR_DELETED;
	else
		sendMessage.operationType = (operationTypeHolder.FileInformationClass == FileRenameInformation || operationTypeHolder.FileInformationClass == FileRenameInformationEx) ? OPERATION_TYPE::DECOY_RENAMED : OPERATION_TYPE::DECOY_DELETED;
	RtlCopyMemory(sendMessage.path, pFileInfo->Name.Buffer, min(ARRAYSIZE(sendMessage.path) * sizeof(WCHAR), pFileInfo->Name.MaximumLength));

	DECOYMON_REPLY_MESSAGE_INFORMATION replyStruct;
	ULONG replyBuffer = sizeof(DECOYMON_REPLY_MESSAGE_INFORMATION);

	// fast track folder creation operation, meant to circumvent instant protection of decoy directories by the service, so that the user can rename or delete directories containing decoys created recently without being blocked.
	FILE_BASIC_INFORMATION fileBasicInfo;
	ULONG bytesReturnedQuery = 0;
	LARGE_INTEGER systemTime;
	if (!NT_SUCCESS(FltQueryInformationFile(pFilterObjects->Instance, pFilterObjects->FileObject, &fileBasicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation, &bytesReturnedQuery))) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}

	KeQuerySystemTime(&systemTime);
	BOOLEAN fastTrackFolderExplorerOP = FALSE;
	if (((systemTime.QuadPart - fileBasicInfo.CreationTime.QuadPart) / 10000000LL <= 10) && (sendMessage.operationType == DECOY_DIR_RENAMED || sendMessage.operationType == DECOY_DIR_DELETED)) {
		sendMessage.pid = 0xFFFFFFFF;
		fastTrackFolderExplorerOP = TRUE;
	}

	// send the message to the usermode service and wait for a reply. if the reply is not received within 2 seconds, we assume the service is not running or is unresponsive, and we reject the operation by default.
	// except for the fast track operation, which is allowed to pass through.
	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000000 * 2; // 2 seconds, to receive the reply. if it times out, it is rejected by default.
	operationStatus = FltSendMessage(gFilterState.pFilterObject, &gFilterState.pClientPort, &sendMessage, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION), &replyStruct, &replyBuffer, &timeout);

	BOOLEAN reject = TRUE;
	if (NT_SUCCESS(operationStatus)) {
		reject = replyStruct.isPIDRejected;
	}

	if (fastTrackFolderExplorerOP) {
		reject = FALSE;
	}

	// explicit reject or timeout => REJECT
	if (reject || operationStatus == STATUS_TIMEOUT) {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ RENAME OR DELETE ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_SET_INFORMATION PRE-CALLBACK ACCESS DENIED. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
	// explicit accept => ACCEPT
	else if (reject == FALSE) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ RENAME OR DELETE ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN ALLOWED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_SET_INFORMATION PRE-CALLBACK ALLOWED TO CONTINUE. VERDICT GIVEN BY SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}
	// unknown behaviour => REJECT by default
	else {
		pData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pData->IoStatus.Information = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REGARDING FILE %wZ RENAME OR DELETE ATTEMPT BY %llu, THE PROCESS'S IRP REQUEST HAS BEEN DENIED.\n", DRIVER_PREFIX, &pFileInfo->Name, processInfo.UniqueProcessId);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
			TraceLoggingWideString(L"IRP_MJ_SET_INFORMATION PRE-CALLBACK ACCESS DENIED. SERVICE TIMEOUT. IMPLICIT DENY", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		FltReleaseFileNameInformation(pFileInfo);
		return FLT_PREOP_COMPLETE;
	}
}


/// <summary>
/// Handles post-operation for IRP_MJ_DIRECTORY_CONTROL requests in the DecoyMon minifilter driver. Meant to hide decoy files from directory queries made by explorer.exe.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation. Currently unused.</param>
/// <param name="flags">Flags that specify how the post-operation callback is to be performed. Currently unused.</param>
/// <returns>FLT_POSTOP_FINISHED_PROCESSING always.</returns>
FLT_POSTOP_CALLBACK_STATUS DecoyMonIRP_PostDirectoryQueryOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID, FLT_POST_OPERATION_FLAGS flags)
{
	UNREFERENCED_PARAMETER(pFilterObjects);
	UNREFERENCED_PARAMETER(flags);


	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */


	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_POSTOP_FINISHED_PROCESSING;

	// ignore kernel mode requests, or requests that AREN'T EXPLICITELY requesting a query, or requests from detaching filters
	if (pData->RequestorMode == KernelMode || pData->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY || (flags & FLTFL_POST_OPERATION_DRAINING)) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// no reason to go through unsuccessful IRPs or IRPs that contain no files for us to hide.
	NTSTATUS irpStatus = pData->IoStatus.Status;
	if (!NT_SUCCESS(irpStatus) || irpStatus == STATUS_NO_MORE_FILES || irpStatus == STATUS_BUFFER_OVERFLOW) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// these 2 file classes are used by explorer, at least, to showcase files.
	auto& params = pData->Iopb->Parameters.DirectoryControl.QueryDirectory;
	if (params.FileInformationClass != FileBothDirectoryInformation && params.FileInformationClass != FileIdBothDirectoryInformation) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}


	/* * * * * * * * * * */
	/* PROCESS FILTERING */
	/* * * * * * * * * * */


	// use FltGetRequestorProcess to get a pointer to a EPROCESS struct (undocumented, can't work with this, need a handle)
	PEPROCESS pEProcessFlt = FltGetRequestorProcess(pData);

	HANDLE hProcess = NULL;  // if this succeeds, close this.
	NTSTATUS operationStatus = STATUS_UNSUCCESSFUL;

	// open a handle to the process requesting the operation
	operationStatus = ObOpenObjectByPointer(pEProcessFlt, OBJ_KERNEL_HANDLE, nullptr, 0x0400, nullptr, KernelMode, &hProcess);
	if (!NT_SUCCESS(operationStatus)) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// get the process's path length, before allocating the resources for saving the path;
	ULONG returnedProcessImageNameLength = 0;
	operationStatus = ZwQueryInformationProcess(hProcess, ProcessImageFileName, nullptr, 0, &returnedProcessImageNameLength);
	if (!(operationStatus == STATUS_INFO_LENGTH_MISMATCH && returnedProcessImageNameLength > 0 && returnedProcessImageNameLength < PAGE_SIZE)) {
		FltClose(hProcess);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// get the process image name via ZwQueryInformationProcess, inside the defined UNICODE_STRING
	PUNICODE_STRING processImageName = (PUNICODE_STRING)ExAllocatePool2(POOL_FLAG_NON_PAGED, returnedProcessImageNameLength, DRIVER_TAG);  // normalize this
	ULONG expectedProcessImageNameLength = returnedProcessImageNameLength;
	if (processImageName == nullptr) {
		FltClose(hProcess);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	operationStatus = ZwQueryInformationProcess(hProcess, ProcessImageFileName, processImageName, expectedProcessImageNameLength, &returnedProcessImageNameLength);

	// exclude any process that ISN'T explorer.exe from this operation (to be tweaked to be modular in the future)
	UNICODE_STRING explorerImage = RTL_CONSTANT_STRING(L"*\\explorer.exe");
	BOOLEAN searchStatus = FsRtlIsNameInUnUpcasedExpression(&explorerImage, processImageName, TRUE, NULL);
	if (!searchStatus) {
		ExFreePool(processImageName);
		FltClose(hProcess);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}


	/* * * * * * * * * * * * * */
	/* DECOY HIDING MECHANISM  */  // might be the hardest part in this whole minifilter
	/* * * * * * * * * * * * * */

	// get the full path to the queried directory, including volume.
	// this is necessary, as the data inside the folder is relative to it, and we need to build a path to the decoy file to check against the hash table.
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);
	if (!NT_SUCCESS(operationStatus)) {
		ExFreePool(processImageName);
		FltClose(hProcess);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// exclude calls to anything other than FileBothDirectoryInformation or FileIdBothDirectoryInformation, as these two are what explorer.exe uses.
	// (still need to check what happens with other explorers)
	PVOID pBase = (params.MdlAddress) ? MmGetSystemAddressForMdlSafe(params.MdlAddress, NormalPagePriority) : params.DirectoryBuffer;
	if (pBase == nullptr) {
		FltReleaseFileNameInformation(pFileInfo);
		ExFreePool(processImageName);
		FltClose(hProcess);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}


	/* * * * * * * * * * * * * * * * * *  */
	/* FILE_BOTH_DIR_INFORMATION HANDLING */
	/* * * * * * * * * * * * * * * * * *  */


	// we will iterate through the entries in the directory, and remove any entry that is a decoy file from explorer.exe's directory listing.
	BOOLEAN entryRemoved = FALSE;
	if (params.FileInformationClass == FileBothDirectoryInformation) {
		PFILE_BOTH_DIR_INFORMATION previousEntry = NULL;  // normalize this
		PFILE_BOTH_DIR_INFORMATION currentEntry = (PFILE_BOTH_DIR_INFORMATION)pBase;  // normalize this

		do {
			UNICODE_STRING fileName;
			fileName.Length = (USHORT)currentEntry->FileNameLength;
			fileName.MaximumLength = fileName.Length;
			fileName.Buffer = (PWCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, fileName.Length + sizeof(WCHAR), DRIVER_TAG);

			if (!fileName.Buffer) break;

			RtlCopyMemory(fileName.Buffer, currentEntry->FileName, fileName.Length);
			BOOLEAN isDecoy = DecoyMonHelper_ShouldHideFile(&pFileInfo->Name, &fileName);
			ExFreePool(fileName.Buffer);

			if (isDecoy) {
				entryRemoved = TRUE;

				if (currentEntry->NextEntryOffset == 0) {
					if (previousEntry) {
						previousEntry->NextEntryOffset = 0;
						pData->IoStatus.Information = (ULONG)((PUCHAR)previousEntry - (PUCHAR)pBase) +
							ALIGN_UP_BY(FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName) + previousEntry->FileNameLength, sizeof(ULONG_PTR));
					}
					else {
						pData->IoStatus.Information = 0;
					}
					break;
				}
				else {
					if (previousEntry) {
						previousEntry->NextEntryOffset += currentEntry->NextEntryOffset;
						pData->IoStatus.Information -= currentEntry->NextEntryOffset;
					}
				}
			}

			// Update previous only if current wasn't removed
			if (!isDecoy)
				previousEntry = currentEntry;

			if (currentEntry->NextEntryOffset == 0)
				break;
			currentEntry = (PFILE_BOTH_DIR_INFORMATION)((PUCHAR)currentEntry + currentEntry->NextEntryOffset);

		} while (currentEntry);
	}


	/* * * * * * * * * * * * * * * * * * * * */
	/* FILE_ID_BOTH_DIR_INFORMATION HANDLING */
	/* * * * * * * * * * * * * * * * * * * * */


	// we will iterate through the entries in the directory, and remove any entry that is a decoy file from explorer.exe's directory listing.
	else if (params.FileInformationClass == FileIdBothDirectoryInformation) {
		PFILE_ID_BOTH_DIR_INFORMATION previousEntry = NULL;  // normalize this
		PFILE_ID_BOTH_DIR_INFORMATION currentEntry = (PFILE_ID_BOTH_DIR_INFORMATION)pBase;  // normalize this

		do {
			UNICODE_STRING fileName;
			fileName.Length = (USHORT)currentEntry->FileNameLength;
			fileName.MaximumLength = fileName.Length;
			fileName.Buffer = (PWCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, fileName.Length + sizeof(WCHAR), DRIVER_TAG);

			if (!fileName.Buffer) break;

			RtlCopyMemory(fileName.Buffer, currentEntry->FileName, fileName.Length);

			BOOLEAN isDecoy = DecoyMonHelper_ShouldHideFile(&pFileInfo->Name, &fileName);
			ExFreePool(fileName.Buffer);

			if (isDecoy) {
				entryRemoved = TRUE;

				if (currentEntry->NextEntryOffset == 0) {
					if (previousEntry) {
						previousEntry->NextEntryOffset = 0;
						pData->IoStatus.Information = (ULONG)((PUCHAR)previousEntry - (PUCHAR)pBase) +
							ALIGN_UP_BY(FIELD_OFFSET(FILE_ID_BOTH_DIR_INFORMATION, FileName) + previousEntry->FileNameLength, sizeof(ULONG_PTR));
					}
					else {
						pData->IoStatus.Information = 0;
					}
					break;
				}
				else {
					if (previousEntry) {
						previousEntry->NextEntryOffset += currentEntry->NextEntryOffset;
						pData->IoStatus.Information -= currentEntry->NextEntryOffset;
					}
				}
			}

			if (!isDecoy)
				previousEntry = currentEntry;

			if (currentEntry->NextEntryOffset == 0)
				break;
			currentEntry = (PFILE_ID_BOTH_DIR_INFORMATION)((PUCHAR)currentEntry + currentEntry->NextEntryOffset);

		} while (currentEntry);
	}

	if (entryRemoved)
		FltSetCallbackDataDirty(pData);


	/* * * * * * * * * *  */
	/* CLEANUP AND ACCEPT */
	/* * * * * * * * * *  */


	FltReleaseFileNameInformation(pFileInfo);
	ExFreePool(processImageName);
	FltClose(hProcess);
	return FLT_POSTOP_FINISHED_PROCESSING;

}


/// <summary>
/// Handles post-operation for IRP_MJ_CREATE requests in the DecoyMon minifilter driver. Meant to add decoy files into new folders created by the user, if the folder is not excluded from decoy operations.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation.</param>
/// <param name="flags">Flags that specify how the post-operation callback is to be performed.</param>
/// <returns>FLT_POSTOP_FINISHED_PROCESSING always.</returns>
FLT_POSTOP_CALLBACK_STATUS DecoyMonIRP_PostCreateOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID, FLT_POST_OPERATION_FLAGS flags)
{
	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */


	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_POSTOP_FINISHED_PROCESSING;

	// ignore kernel mode requests, draining operation, non-successful IRPs, and non-directory operations.
	BOOLEAN isDirectory = 0;
	FltIsDirectory(pFilterObjects->FileObject, pFilterObjects->Instance, &isDirectory);  // use ONLY in POST-CREATE, you need APC_LEVEL or lower for this.
	if (flags & FLTFL_POST_OPERATION_DRAINING) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	if (pData->RequestorMode == KernelMode || pData->IoStatus.Status != STATUS_SUCCESS || !isDirectory || pData->IoStatus.Information != FILE_CREATED) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}


	/* * * * * * * * * * * * */
	/* TARGET FILE CHECKING  */
	/* * * * * * * * * * * * */


	// get the file name information for the target file
	NTSTATUS operationStatus;
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);  // use ONLY in POST-CREATE, you need APC_LEVEL or lower for this.
	if (!NT_SUCCESS(operationStatus)) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// check if the directory is excluded from decoy operations
	gFilterState.executiveResourceExcluded.LockRead();
	PITERABLE_LIST_ELEMENT excludedDirectoryCursor = gFilterState.excludedDirectoryList->GetFirstElement();
	if (excludedDirectoryCursor == nullptr) {
		gFilterState.executiveResourceExcluded.UnlockRead();
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	while (gFilterState.excludedDirectoryList->HasNextElement(excludedDirectoryCursor)) {
		excludedDirectoryCursor = gFilterState.excludedDirectoryList->GetNextElement(excludedDirectoryCursor);
		if (excludedDirectoryCursor == nullptr)
			break;

		if (FsRtlIsNameInUnUpcasedExpression(excludedDirectoryCursor->pString, &pFileInfo->Name, TRUE, nullptr)) {
			gFilterState.executiveResourceExcluded.UnlockRead();
			FltReleaseFileNameInformation(pFileInfo);
			return FLT_POSTOP_FINISHED_PROCESSING;
		}
	}
	gFilterState.executiveResourceExcluded.UnlockRead();


	/* * * * * * * * * * * * */
	/* OPERATION DISPATCHING */
	/* * * * * * * * * * * * */
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"IRP_MJ_CREATE POST-CALLBACK ALLOWED TO CONTINUE, TO CREATE FOLDER. IMPLICIT ALLOW", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// prepare the message to send to the usermode service for folder arming, as well as the reply structure
	DECOYMON_SEND_MESSAGE_INFORMATION sendMessage;
	sendMessage.pid = 0xFFFFFFFF;
	sendMessage.operationType = OPERATION_TYPE::DECOY_DIR_ADD;
	RtlCopyMemory(sendMessage.path, pFileInfo->Name.Buffer, min(ARRAYSIZE(sendMessage.path) * sizeof(WCHAR), pFileInfo->Name.MaximumLength));

	// send the message to the usermode service.
	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000 * 100; // 100 msecs to assure message success
	operationStatus = FltSendMessage(gFilterState.pFilterObject, &gFilterState.pClientPort, &sendMessage, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION), nullptr, nullptr, &timeout);

	return FLT_POSTOP_FINISHED_PROCESSING;
}


/// <summary>
/// Handles post-operation for IRP_MJ_SET_INFORMATION requests in the DecoyMon minifilter driver. Meant to add decoy files into renamed folders, if the folder is not excluded from decoy operations.
/// </summary>
/// <param name="pData">A pointer to a structure representing an I/O operation.</param>
/// <param name="pFilterObjects">A pointer to a structure containing opaque pointers for the objects associated with an operation.</param>
/// <param name="flags">Flags that specify how the post-operation callback is to be performed.</param>
/// <returns>FLT_POSTOP_FINISHED_PROCESSING always.</returns>
FLT_POSTOP_CALLBACK_STATUS DecoyMonIRP_PostSetInformationOperation(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFilterObjects, PVOID, FLT_POST_OPERATION_FLAGS flags)
{
	/* * * * * * * * * * */
	/* REQUEST EXCLUSION */
	/* * * * * * * * * * */

	// if system is off, pass through
	BOOLEAN armed;
	gFilterState.executiveResourceArmedCheck.LockRead();
	armed = gFilterState.armed;
	gFilterState.executiveResourceArmedCheck.UnlockRead();
	if (armed == FALSE)
		return FLT_POSTOP_FINISHED_PROCESSING;

	// ignore kernel mode requests, draining operation, non-successful IRPs, and non-directory operations.
	BOOLEAN isDirectory = 0;
	FltIsDirectory(pFilterObjects->FileObject, pFilterObjects->Instance, &isDirectory);  // use ONLY in POST-CREATE, you need APC_LEVEL or lower for this.
	if (flags & FLTFL_POST_OPERATION_DRAINING) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// we only care for rename operations, the others can pass through.
	auto& operationTypeHolder = pData->Iopb->Parameters.SetFileInformation;
	if (!(operationTypeHolder.FileInformationClass == FileRenameInformation ||
		operationTypeHolder.FileInformationClass == FileRenameInformationEx))
		return FLT_POSTOP_FINISHED_PROCESSING;

	// ignore requests that are from the kernel, not successful, or that are not directory operations.
	if (pData->RequestorMode == KernelMode || pData->IoStatus.Status != STATUS_SUCCESS || !isDirectory) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}


	/* * * * * * * * * * * * */
	/* TARGET FILE CHECKING  */
	/* * * * * * * * * * * * */


	// get the file name information for the target file
	NTSTATUS operationStatus;
	PFLT_FILE_NAME_INFORMATION pFileInfo;
	operationStatus = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pFileInfo);  // use ONLY in POST-CREATE, you need APC_LEVEL or lower for this.
	if (!NT_SUCCESS(operationStatus)) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// check if the directory is excluded from decoy operations
	gFilterState.executiveResourceExcluded.LockRead();
	PITERABLE_LIST_ELEMENT excludedDirectoryCursor = gFilterState.excludedDirectoryList->GetFirstElement();
	if (excludedDirectoryCursor == nullptr) {
		gFilterState.executiveResourceExcluded.UnlockRead();
		FltReleaseFileNameInformation(pFileInfo);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	while (gFilterState.excludedDirectoryList->HasNextElement(excludedDirectoryCursor)) {
		excludedDirectoryCursor = gFilterState.excludedDirectoryList->GetNextElement(excludedDirectoryCursor);
		if (excludedDirectoryCursor == nullptr)
			break;

		if (FsRtlIsNameInUnUpcasedExpression(excludedDirectoryCursor->pString, &pFileInfo->Name, TRUE, nullptr)) {
			gFilterState.executiveResourceExcluded.UnlockRead();
			FltReleaseFileNameInformation(pFileInfo);
			return FLT_POSTOP_FINISHED_PROCESSING;
		}
	}
	gFilterState.executiveResourceExcluded.UnlockRead();


	/* * * * * * * * * * * * */
	/* OPERATION DISPATCHING */
	/* * * * * * * * * * * * */
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"MINIFILTER", "Subcomponent"),
		TraceLoggingWideString(L"IRP_MJ_SET_INFORMATION POST-CALLBACK ALLOWED TO CONTINUE, TO RENAME FOLDER. IMPLICIT ALLOW", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	// prepare the message to send to the usermode service for folder arming, as well as the reply structure
	DECOYMON_SEND_MESSAGE_INFORMATION sendMessage;
	sendMessage.pid = 0xFFFFFFFF;
	sendMessage.operationType = OPERATION_TYPE::DECOY_DIR_ADD;
	RtlCopyMemory(sendMessage.path, pFileInfo->Name.Buffer, min(ARRAYSIZE(sendMessage.path) * sizeof(WCHAR), pFileInfo->Name.MaximumLength));

	// send the message to the usermode service.
	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000 * 100; // 100 msecs to assure message success
	operationStatus = FltSendMessage(gFilterState.pFilterObject, &gFilterState.pClientPort, &sendMessage, sizeof(DECOYMON_SEND_MESSAGE_INFORMATION), nullptr, nullptr, &timeout);

	return FLT_POSTOP_FINISHED_PROCESSING;
}

/// <summary>
/// Determines whether a file should be hidden based on its path and a decoy file list.
/// </summary>
/// <param name="parentDirectory">A pointer to a UNICODE_STRING representing the parent directory of the file.</param>
/// <param name="fileName">A pointer to a UNICODE_STRING representing the name of the file.</param>
/// <returns>A BOOLEAN value indicating whether the file should be hidden (TRUE) or not (FALSE).</returns>
BOOLEAN DecoyMonHelper_ShouldHideFile(PUNICODE_STRING parentDirectory, PUNICODE_STRING fileName)
{
	/* * * * * * * * * * * * * *  */
	/* BUILDING FULL PATH TO FILE */
	/* * * * * * * * * * * * * *  */

	UNICODE_STRING backslash = RTL_CONSTANT_STRING(L"\\");
	UNICODE_STRING expressionBackslash = RTL_CONSTANT_STRING(L"*\\");
	BOOLEAN trailingBackslash = FsRtlIsNameInUnUpcasedExpression(&expressionBackslash, parentDirectory, TRUE, NULL);
	UINT32 offset = 0;

	UNICODE_STRING fullPath;
	fullPath.Length = parentDirectory->Length + fileName->Length;
	if (!trailingBackslash)
		fullPath.Length += backslash.Length;
	fullPath.MaximumLength = parentDirectory->MaximumLength + fileName->MaximumLength + sizeof(WCHAR);
	if (!trailingBackslash)
		fullPath.MaximumLength += backslash.MaximumLength;

	fullPath.Buffer = (PWCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, fullPath.MaximumLength, DRIVER_TAG);

	RtlCopyMemory(fullPath.Buffer, parentDirectory->Buffer, parentDirectory->Length);
	offset += (parentDirectory->Length / sizeof(WCHAR));
	if (!trailingBackslash) {
		RtlCopyMemory(&fullPath.Buffer[offset], backslash.Buffer, backslash.Length);
		offset += (backslash.Length / sizeof(WCHAR));
	}
	RtlCopyMemory(&fullPath.Buffer[offset], fileName->Buffer, fileName->Length);
	fullPath.Buffer[fullPath.Length / sizeof(WCHAR)] = L'\0';


	/* * * * * * * * * * * * * * * * *  */
	/* CHECKING PATH AGAINST DECOY LIST */
	/* * * * * * * * * * * * * * * * *  */

	gFilterState.executiveResourceFiles.LockRead();
	BOOLEAN foundDecoy = FALSE;
	if (gFilterState.fileHashTable->StringExistsInHashTable(&fullPath) != nullptr) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: DECOY FILE %wZ QUERIED INSIDE FILESYSTEM FOR FLAGGED EXECUTABLE. HIDING.\n", DRIVER_PREFIX, fullPath);
		foundDecoy = TRUE;
	}
	ExFreePool(fullPath.Buffer);
	gFilterState.executiveResourceFiles.UnlockRead();

	return foundDecoy;
}


/// <summary>
/// Handles client connection setup and notifications for a filter communication port.
/// </summary>
/// <param name="ClientPort">The handle to the client port that is connecting.</param>
/// <param name="ServerPortCookie">Optional context information associated with the server port. Currently unused.</param>
/// <param name="ConnectionContext">Optional context information provided by the client during the connection. Currently unused.</param>
/// <param name="SizeOfContext">The size, in bytes, of the ConnectionContext. Currently unused.</param>
/// <param name="ConnectionPortCookie">A pointer to receive optional context information for the connection port. Currently unused.</param>
/// <returns>An NTSTATUS value indicating the success or failure of the connection notification handling. Typically returns STATUS_SUCCESS.</returns>
NTSTATUS DecoyMonPort_ConnectNotify(_In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID* ConnectionPortCookie)
{
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionPortCookie);

	gFilterState.pClientPort = ClientPort;
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: A CLIENT HAS SUCCESSFULLY CONNECTED TO THE SERVER.\n", DRIVER_PREFIX);

	return STATUS_SUCCESS;
}


/// <summary>
/// Handles the disconnection of a client from the server and resets the client port pointer to nullptr.
/// </summary>
/// <param name="ConnectionCookie">A pointer to the connection-specific data provided during the connection setup. Currently unused.</param>
VOID DecoyMonPort_DisconnectNotify(PVOID ConnectionCookie)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);
	FltCloseClientPort(gFilterState.pFilterObject, &gFilterState.pClientPort);

	gFilterState.pClientPort = nullptr;
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: A CLIENT HAS DISCONNECTED FROM THE SERVER.\n", DRIVER_PREFIX);
}


/// <summary>
/// Handles a message receive notification from a client.
/// </summary>
/// <param name="PortCookie">Optional context information associated with the port. Currently unused.</param>
/// <param name="InputBuffer">Optional pointer to the input buffer containing data sent by the client. Currently unused.</param>
/// <param name="InputBufferLength">The size, in bytes, of the input buffer. Currently unused.</param>
/// <param name="OutputBuffer">Optional pointer to the output buffer where the response data will be written. Currently unused.</param>
/// <param name="OutputBufferLength">The size, in bytes, of the output buffer. Currently unused.</param>
/// <param name="ReturnOutputBufferLength">Pointer to a variable that receives the size, in bytes, of the data written to the output buffer. Currently unused.</param>
/// <returns>An NTSTATUS code indicating the success or failure of the operation. Typically returns STATUS_SUCCESS.</returns>
NTSTATUS DecoyMonPort_MessageNotify(_In_opt_ PVOID PortCookie, _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength, _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength, _Out_ PULONG ReturnOutputBufferLength)
{
	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: A MESSAGE FROM A CLIENT HAS BEEN RECEIVED.\n", DRIVER_PREFIX);

	return STATUS_SUCCESS;
}


/// <summary>
/// Determines if the process associated with the given callback data is a system or protected process.
/// </summary>
/// <param name="pData">Pointer to the callback data structure (PFLT_CALLBACK_DATA) representing the operation being processed.</param>
/// <returns>TRUE if the process is a system or protected process; otherwise, FALSE.</returns>
BOOLEAN IsSystemOrProtectedProcess(PFLT_CALLBACK_DATA pData)
{
	//get the process from the callback data
	PEPROCESS process = FltGetRequestorProcess(pData);
	if (!process) 
		return FALSE;

	// check if the process is a protected process or protected process light.
	PS_PROTECTION protection = PsGetProcessProtection(process);
	if (protection.Flags.Type == PsProtectedTypeProtected || protection.Flags.Type == PsProtectedTypeLight)
		return TRUE;

	// check if the process is a system process by comparing its token with the system SID.
	// get the primary token of the process
	HANDLE processTokenHandle = NULL;
	PACCESS_TOKEN token = PsReferencePrimaryToken(process);
	if (!token)
		return FALSE;


	if (!NT_SUCCESS(ObOpenObjectByPointer(token, 0, NULL, TOKEN_QUERY, *SeTokenObjectType, KernelMode, &processTokenHandle)))
	{
		PsDereferencePrimaryToken(token);
		return FALSE;
	}

	PsDereferencePrimaryToken(token);

	// query the token for the user SID's length
	ULONG returnLength = 0;
	ZwQueryInformationToken(processTokenHandle, TokenUser, NULL, 0, &returnLength);

	if (returnLength == 0) {
		ZwClose(processTokenHandle);
		return FALSE;
	}

	PTOKEN_USER tokenUser = (PTOKEN_USER)ExAllocatePool2(POOL_FLAG_NON_PAGED, returnLength, DRIVER_TAG);
	if (!tokenUser) {
		ZwClose(processTokenHandle);
		return FALSE;
	}

	// query the token for the user SID
	NTSTATUS status = ZwQueryInformationToken(processTokenHandle, TokenUser, tokenUser, returnLength, &returnLength);

	ZwClose(processTokenHandle);

	if (!NT_SUCCESS(status)) {
		ExFreePool(tokenUser);
		return FALSE;
	}

	// build the system SID to compare against the token's user SID
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	UCHAR systemSidBuffer[SECURITY_MAX_SID_SIZE];
	PSID systemSid = (PSID)&systemSidBuffer;

	BOOLEAN isSystem = FALSE;

	// initialize the system SID with the NT authority and the SYSTEM RID
	if (NT_SUCCESS(RtlInitializeSid(systemSid, &ntAuthority, 1))) {
		// set the first sub-authority to SYSTEM RID
		*RtlSubAuthoritySid(systemSid, 0) = SECURITY_LOCAL_SYSTEM_RID;

		if (RtlValidSid(systemSid) && RtlEqualSid(tokenUser->User.Sid, systemSid)) {
			isSystem = TRUE;
		}
	}

	ExFreePool(tokenUser);
	return isSystem;
}
