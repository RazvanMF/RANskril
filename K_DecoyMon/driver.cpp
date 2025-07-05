#include "pch.h"
#include "driver.h"
#include "minifilter.h"
#include "UNICODE_STRING_HASH_TABLE.h"
#include "decoymon_public.h"

UNICODE_STRING gSymbolicLinkName = RTL_CONSTANT_STRING(L"\\??\\DecoyMon");
UNICODE_STRING gPortName = RTL_CONSTANT_STRING(L"\\DecoyMonProcessIDBridge");
PDEVICE_OBJECT gpDeviceObject = nullptr;

FilterState gFilterState;

// provider for kernel logging mechanism
TRACELOGGING_DEFINE_PROVIDER(
	g_hKernelProvider,
	"RANskrilKernelLogger",
	(0xa1c74b78, 0x630e, 0x460a, 0x8d, 0xa6, 0xc6, 0xb4, 0x6b, 0xe1, 0x73, 0xba));


/// <summary>
/// Initializes the driver and sets up necessary resources, including hash tables, synchronization primitives, device creation, symbolic links, and communication ports.
/// </summary>
/// <param name="pDriverObject">Pointer to the driver object representing the current driver instance.</param>
/// <param name="pRegistryPath">Pointer to a Unicode string containing the registry path for the driver's configuration.</param>
/// <returns>An NTSTATUS code indicating the success or failure of the driver initialization process.</returns>
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
	gFilterState.armed = TRUE;
	NTSTATUS initializationStatus = STATUS_SUCCESS;
	bool symlinkCreated = false;

	/* * * * * * * * * * * * */
	/* DRIVER INITIALIZATION */
	/* * * * * * * * * * * * */

	TraceLoggingRegister(g_hKernelProvider);

	do {
		// before we begin filter initialization, we need to handle the hash table that'll hold the decoy file locations; same thing for directories.
		gFilterState.fileHashTable = (PUNICODE_STRING_HASH_TABLE)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(UNICODE_STRING_HASH_TABLE), DRIVER_TAG);
		if (!gFilterState.fileHashTable) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO ALLOCATE MEMORY FOR THE FILE HASH TABLE\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO ALLOCATE MEMORY FOR THE FILE HASH TABLE", "Message"),
				TraceLoggingUInt32(0xC0000017, "Status")
			);
			break;
		}
		gFilterState.fileHashTable->InitializeHashTable();
		initializationStatus = gFilterState.executiveResourceFiles.Initialize();
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO INITIALIZE ERESOURCE FOR FILES\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO INITIALIZE ERESOURCE FOR FILES", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		gFilterState.directoryHashTable = (PUNICODE_STRING_HASH_TABLE)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(UNICODE_STRING_HASH_TABLE), DRIVER_TAG);
		if (!gFilterState.directoryHashTable) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO ALLOCATE MEMORY FOR THE DIRECTORY HASH TABLE\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO ALLOCATE MEMORY FOR THE DIRECTORY HASH TABLE", "Message"),
				TraceLoggingUInt32(0xC0000017, "Status")
			);
			break;
		}
		gFilterState.directoryHashTable->InitializeHashTable();
		initializationStatus = gFilterState.executiveResourceDirectories.Initialize();
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO INITIALIZE ERESOURCE FOR DIRECTORIES\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO INITIALIZE ERESOURCE FOR DIRECTORIES", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		gFilterState.excludedDirectoryList = (PUNICODE_STRING_ITERABLE_LIST)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(UNICODE_STRING_ITERABLE_LIST), DRIVER_TAG);
		if (!gFilterState.excludedDirectoryList) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO ALLOCATE MEMORY FOR THE ITERABLE LIST\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO ALLOCATE MEMORY FOR THE ITERABLE LIST", "Message"),
				TraceLoggingUInt32(0xC0000017, "Status")
			);
			break;
		}
		gFilterState.excludedDirectoryList->InitializeIterableList();
		initializationStatus = gFilterState.executiveResourceExcluded.Initialize();
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO INITIALIZE ERESOURCE FOR EXCLUDED DIRECTORIES\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO INITIALIZE ERESOURCE FOR EXCLUDED DIRECTORIES", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		initializationStatus = gFilterState.executiveResourceArmedCheck.Initialize();
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO INITIALIZE ERESOURCE FOR SYSTEM ARMING\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO INITIALIZE ERESOURCE FOR SYSTEM ARMING", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}


		// since we don't make a .inf file, we will initialize minifilter properties manually
		initializationStatus = DecoyMon_InitMiniFilter(pDriverObject, pRegistryPath);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO INITIALIZE MINI-FILTER PROPERTIES (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO INITIALIZE MINI-FILTER PROPERTIES", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

		// create the device for this driver
		UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\DecoyMon");
		initializationStatus = IoCreateDevice(pDriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &gpDeviceObject);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO CREATE DEVICE (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO CREATE DEVICE", "Message")
			);
			break;
		}

		// create the symbolic link for this driver
		initializationStatus = IoCreateSymbolicLink(&gSymbolicLinkName, &deviceName);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO CREATE SYMBOLIC LINK (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO ALLOCATE MEMORY FOR THE FILE HASH TABLE", "Message")
			);
			break;
		}
		symlinkCreated = true;


		/* * * * * * * * * * * * * * */
		/* FILTER PORT COMMUNICATION */
		/* * * * * * * * * * * * * * */

		// create the security descriptor for the communication port (inside attributes)
		PSECURITY_DESCRIPTOR pSD;
		initializationStatus = FltBuildDefaultSecurityDescriptor(&pSD, FLT_PORT_ALL_ACCESS);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO CREATE SECURITY DESCRIPTOR (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO CREATE SECURITY DESCRIPTOR", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}
		// create the communication port
		OBJECT_ATTRIBUTES portAttributes;
		InitializeObjectAttributes(&portAttributes, &gPortName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, nullptr, pSD);
		gFilterState.pClientPort = nullptr;
		gFilterState.pFilterPort = nullptr;
		initializationStatus = FltCreateCommunicationPort(gFilterState.pFilterObject, &gFilterState.pFilterPort, &portAttributes, nullptr, DecoyMonPort_ConnectNotify, DecoyMonPort_DisconnectNotify, DecoyMonPort_MessageNotify, 1);
		if (!NT_SUCCESS(initializationStatus)) {
			FltFreeSecurityDescriptor(pSD);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[e] %s: FAILED TO ESTABLISH MINIFILTER COMMUNICATION PORT (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED TO ESTABLISH MINIFILTER COMMUNICATION PORT", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}
		FltFreeSecurityDescriptor(pSD);

		// start the driver's filtering functionalities
		initializationStatus = FltStartFiltering(gFilterState.pFilterObject);
		if (!NT_SUCCESS(initializationStatus)) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[e] %s: FAILED FltStartFiltering CALL (0x%X)\n", DRIVER_PREFIX, initializationStatus);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"FAILED FltStartFiltering CALL", "Message"),
				TraceLoggingUInt32(initializationStatus, "Status")
			);
			break;
		}

	} while (false);

	// revert all the changes if something went wrong
	if (!NT_SUCCESS(initializationStatus)) {
		if (gFilterState.pFilterObject) {
			FltUnregisterFilter(gFilterState.pFilterObject);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: REVERTED MINIFILTER REGISTRATION\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"REVERTED MINIFILTER REGISTRATION", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
		}
		if (gFilterState.pFilterPort) {
			FltCloseCommunicationPort(gFilterState.pFilterPort);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: CLOSED COMMUNICATION PORT\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"REVERTED MINIFILTER REGISTRATION", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
		}
		if (symlinkCreated) {
			IoDeleteSymbolicLink(&gSymbolicLinkName);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: DELETED SYMBOLIC LINK\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"REVERTED MINIFILTER REGISTRATION", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
		}
		if (gpDeviceObject) {
			IoDeleteDevice(gpDeviceObject);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: DELETED DEVICE\n", DRIVER_PREFIX);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"REVERTED MINIFILTER REGISTRATION", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
		}
		return initializationStatus;
	}


	/* * * * * * * * * * * */
	/* IOCTL COMMUNICATION */
	/* * * * * * * * * * * */

	gFilterState.pDriverObject = pDriverObject;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DecoyMonCreateClose;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DecoyMonDeviceControl;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[s] INITIALIZATION FINISHED SUCCESSFULLY\n");
	TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
		TraceLoggingWideString(L"DRIVER", "Subcomponent"),
		TraceLoggingWideString(L"INITIALIZATION FINISHED SUCCESSFULLY", "Message"),
		TraceLoggingUInt32(0x0, "Status")
	);

	return initializationStatus;
}


/// <summary>
/// Completes an I/O request and sets its status and information.
/// </summary>
/// <param name="pIRP">A pointer to the IRP (I/O Request Packet) to complete.</param>
/// <param name="operationStatus">The status code to set for the I/O operation.</param>
/// <param name="information">Additional information about the I/O operation, typically the number of bytes processed.</param>
/// <returns>The status code of the completed operation.</returns>
NTSTATUS CompleteRequest(_In_ PIRP pIRP, _In_ NTSTATUS operationStatus, _In_ ULONG_PTR information) {
	pIRP->IoStatus.Status = operationStatus;
	pIRP->IoStatus.Information = information;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);

	return operationStatus;
}


/// <summary>
/// Handles the creation and closure of a device object in a driver.
/// </summary>
/// <param name="pDeviceObject">A pointer to the device object associated with the IRP. Currently unused.</param>
/// <param name="pIRP">A pointer to the I/O request packet (IRP) being processed.</param>
/// <returns>An NTSTATUS code indicating the result of the operation.</returns>
NTSTATUS DecoyMonCreateClose(_In_ PDEVICE_OBJECT pDeviceObject, _In_ PIRP pIRP)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteRequest(pIRP);
}


/// <summary>
/// Handles device control requests for managing decoy files and directories, and state.
/// </summary>
/// <param name="pDeviceObject">Pointer to the device object associated with the request. Currently unused.</param>
/// <param name="pIRP">Pointer to the I/O request packet (IRP) containing the control code and associated data.</param>
/// <returns>An NTSTATUS code indicating the result of the operation. Possible values include STATUS_SUCCESS, STATUS_INVALID_PARAMETER, STATUS_INVALID_DEVICE_REQUEST, and STATUS_UNSUCCESSFUL.</returns>
NTSTATUS DecoyMonDeviceControl(_In_ PDEVICE_OBJECT pDeviceObject, _In_ PIRP pIRP)
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	PIO_STACK_LOCATION pIRPStackLocation = IoGetCurrentIrpStackLocation(pIRP);
	auto& deviceIoControl = pIRPStackLocation->Parameters.DeviceIoControl;
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG_PTR len = 0;

	auto pBuffer = (WCHAR*)pIRP->AssociatedIrp.SystemBuffer;
	UNICODE_STRING path;
	switch (deviceIoControl.IoControlCode) {
		// Handle IOCTL for adding a decoy file path.
	case IOCTL_ADD_DECOY_FILE:
		if (pBuffer == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlInitUnicodeString(&path, pBuffer);

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: RECEIVED A DECOY FILE PATH FROM THE SERVICE. (%wZ)", DRIVER_PREFIX, path);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"RECEIVED A DECOY FILE PATH FROM THE SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		gFilterState.executiveResourceFiles.LockWrite();
		status = gFilterState.fileHashTable->AddStringIntoHashTable(&path);
		if (!NT_SUCCESS(status)) {
			gFilterState.executiveResourceFiles.UnlockWrite();
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[e] %s: DECOY FILE PATH ALREADY EXISTS. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"DECOY FILE PATH ALREADY EXISTS", "Message"),
				TraceLoggingUInt32(status, "Status")
			);
			break;
		}

		if (gFilterState.fileHashTable->StringExistsInHashTable(&path) != nullptr) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: SUCCESSFULLY ADDED DECOY FILE PATH. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"SUCCESSFULLY ADDED DECOY FILE PATH", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
			len = 1;
		}

		gFilterState.executiveResourceFiles.UnlockWrite();

		status = (len == 1) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		break;
	
		// Handle IOCTL for adding a directory containing decoy files.
	case IOCTL_ADD_DECOY_DIRECTORY:
		if (pBuffer == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlInitUnicodeString(&path, pBuffer);

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: RECEIVED A DECOY-CONTAINING DIRECTORY FROM THE SERVICE. (%wZ)", DRIVER_PREFIX, path);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"RECEIVED A DECOY-CONTAINING DIRECTORY FROM THE SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		gFilterState.executiveResourceDirectories.LockWrite();
		status = gFilterState.directoryHashTable->AddStringIntoHashTable(&path);
		if (!NT_SUCCESS(status)) {
			gFilterState.executiveResourceDirectories.UnlockWrite();
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[e] %s: DECOY-CONTAINING DIRECTORY ALREADY EXISTS. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"DECOY-CONTAINING DIRECTORY ALREADY EXISTS", "Message"),
				TraceLoggingUInt32(status, "Status")
			);
			break;
		}

		if (gFilterState.directoryHashTable->StringExistsInHashTable(&path) != nullptr) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: SUCCESSFULLY ADDED DECOY-CONTAINING DIRECTORY. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"SUCCESSFULLY ADDED DECOY-CONTAINING DIRECTORY", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
			len = 1;
		}

		gFilterState.executiveResourceDirectories.UnlockWrite();

		status = (len == 1) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		break;

		// Handle IOCTL for removing a decoy file path.
	case IOCTL_REMOVE_DECOY_FILE:
		if (pBuffer == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlInitUnicodeString(&path, pBuffer);

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: RECEIVED A DECOY FILE PATH FROM THE SERVICE. (%wZ)", DRIVER_PREFIX, path);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"RECEIVED A DECOY FILE PATH FROM THE SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		gFilterState.executiveResourceFiles.LockWrite();
		status = gFilterState.fileHashTable->DeleteStringFromHashTable(&path);
		if (!NT_SUCCESS(status)) {
			gFilterState.executiveResourceFiles.UnlockWrite();
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[e] %s: DECOY FILE PATH DOES NOT EXIST. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"DECOY FILE PATH DOES NOT EXIST", "Message"),
				TraceLoggingUInt32(status, "Status")
			);
			break;
		}

		if (gFilterState.fileHashTable->StringExistsInHashTable(&path) == nullptr) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: SUCCESSFULLY DELETED DECOY FILE PATH. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"SUCCESSFULLY DELETED DECOY FILE PATH", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
			len = 1;
		}

		gFilterState.executiveResourceFiles.UnlockWrite();

		status = (len == 1) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		break;

		// Handle IOCTL for removing a directory containing decoy files.
	case IOCTL_REMOVE_DECOY_DIRECTORY:
		if (pBuffer == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlInitUnicodeString(&path, pBuffer);

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: RECEIVED A DECOY-CONTAINING DIRECTORY FROM THE SERVICE. (%wZ)", DRIVER_PREFIX, path);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"RECEIVED A DECOY-CONTAINING DIRECTORY FROM THE SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		gFilterState.executiveResourceDirectories.LockWrite();
		status = gFilterState.directoryHashTable->DeleteStringFromHashTable(&path);
		if (!NT_SUCCESS(status)) {
			gFilterState.executiveResourceDirectories.UnlockWrite();
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[e] %s: DECOY-CONTAINING DIRECTORY ALREADY EXISTS. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"DECOY-CONTAINING DIRECTORY ALREADY EXISTS", "Message"),
				TraceLoggingUInt32(status, "Status")
			);
			break;
		}

		if (gFilterState.directoryHashTable->StringExistsInHashTable(&path) == nullptr) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: SUCCESSFULLY DELETED DECOY-CONTAINING DIRECTORY. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"SUCCESSFULLY DELETED DECOY-CONTAINING DIRECTORY", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
			len = 1;
		}

		gFilterState.executiveResourceDirectories.UnlockWrite();

		status = (len == 1) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		break;

		// Handle IOCTL for adding an excluded directory.
	case IOCTL_ADD_EXCLUDED_DIRECTORY:
		if (pBuffer == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlInitUnicodeString(&path, pBuffer);

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: RECEIVED AN EXCLUDED DIRECTORY FROM THE SERVICE. (%wZ)", DRIVER_PREFIX, path);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"RECEIVED AN EXCLUDED DIRECTORY FROM THE SERVICE", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		gFilterState.executiveResourceExcluded.LockWrite();
		status = gFilterState.excludedDirectoryList->AddStringIntoList(&path);
		if (!NT_SUCCESS(status)) {
			gFilterState.executiveResourceExcluded.UnlockWrite();
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[e] %s: EXCLUDED DIRECTORY ALREADY EXISTS. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"EXCLUDED DIRECTORY ALREADY EXISTS", "Message"),
				TraceLoggingUInt32(status, "Status")
			);
			break;
		}

		if (gFilterState.excludedDirectoryList->ElementExistsInList(&path) != nullptr) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: SUCCESSFULLY ADDED EXCLUDED DIRECTORY. (%wZ)", DRIVER_PREFIX, path);
			TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
				TraceLoggingWideString(L"DRIVER", "Subcomponent"),
				TraceLoggingWideString(L"SUCCESSFULLY ADDED EXCLUDED DIRECTORY", "Message"),
				TraceLoggingUInt32(0x0, "Status")
			);
			len = 1;
		}

		gFilterState.executiveResourceExcluded.UnlockWrite();

		status = (len == 1) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		break;

		// Handle IOCTL for turning off the decoy monitor.
	case IOCTL_RANSKRILOVERRIDE_TURNOFF:
		gFilterState.executiveResourceArmedCheck.LockWrite();
		gFilterState.armed = FALSE;
		gFilterState.executiveResourceArmedCheck.UnlockWrite();
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: TURNED OFF DECOYMON. IRPS WILL PASS THROUGH NOW.", DRIVER_PREFIX);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"TURNED OFF DECOYMON. IRPS WILL PASS THROUGH NOW.", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		status = STATUS_SUCCESS;
		break;

		// Handle IOCTL for turning on the decoy monitor.
	case IOCTL_RANSKRILOVERRIDE_TURNON:
		gFilterState.executiveResourceArmedCheck.LockWrite();
		gFilterState.armed = TRUE;
		gFilterState.executiveResourceArmedCheck.UnlockWrite();
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: TURNED ON DECOYMON. IRPS WILL BE CHECKED NOW.", DRIVER_PREFIX);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"TURNED ON DECOYMON. IRPS WILL BE CHECKED NOW.", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		status = STATUS_SUCCESS;
		break;

		// Handle IOCTL for emptying the decoy file path hashtable.
	case IOCTL_EMPTY_DECOY_FILE_MEMORY:
		gFilterState.executiveResourceFiles.LockWrite();
		gFilterState.fileHashTable->DestroyHashTable();
		ExFreePool(gFilterState.fileHashTable);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: EMPTIED DECOY PATH LIST\n", DRIVER_PREFIX);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"EMPTIED DECOY PATH LIST", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);

		gFilterState.fileHashTable = (PUNICODE_STRING_HASH_TABLE)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(UNICODE_STRING_HASH_TABLE), DRIVER_TAG);
		gFilterState.fileHashTable->InitializeHashTable();
		gFilterState.executiveResourceFiles.UnlockWrite();
		status = STATUS_SUCCESS;
		break;

		// Handle IOCTL for emptying the directory containing decoy file hashtable.
	case IOCTL_EMPTY_DECOY_DIRECTORY_MEMORY:
		gFilterState.executiveResourceDirectories.LockWrite();
		gFilterState.directoryHashTable->DestroyHashTable();
		ExFreePool(gFilterState.directoryHashTable);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: EMPTIED DIRECTORY PATH LIST\n", DRIVER_PREFIX);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"EMPTIED DIRECTORY PATH LIST", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		gFilterState.directoryHashTable = (PUNICODE_STRING_HASH_TABLE)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(UNICODE_STRING_HASH_TABLE), DRIVER_TAG);
		gFilterState.directoryHashTable->InitializeHashTable();
		gFilterState.executiveResourceDirectories.UnlockWrite();
		status = STATUS_SUCCESS;
		break;

		// Handle IOCTL for emptying the excluded directory list.
	case IOCTL_EMPTY_EXCLUDED_DIRECTORY_MEMORY:
		gFilterState.executiveResourceExcluded.LockWrite();
		gFilterState.excludedDirectoryList->DestroyIterableList();
		ExFreePool(gFilterState.excludedDirectoryList);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i] %s: EMPTIED EXCLUDED DIRECTORY LIST\n", DRIVER_PREFIX);
		TraceLoggingWrite(g_hKernelProvider, "DecoyMon", TraceLoggingLevel(WINEVENT_LEVEL_INFO),
			TraceLoggingWideString(L"DRIVER", "Subcomponent"),
			TraceLoggingWideString(L"EMPTIED EXCLUDED DIRECTORY LIST", "Message"),
			TraceLoggingUInt32(0x0, "Status")
		);
		gFilterState.excludedDirectoryList = (PUNICODE_STRING_ITERABLE_LIST)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(UNICODE_STRING_ITERABLE_LIST), DRIVER_TAG);
		gFilterState.excludedDirectoryList->InitializeIterableList();
		gFilterState.executiveResourceExcluded.UnlockWrite();
		status = STATUS_SUCCESS;
		break;
	}

	return CompleteRequest(pIRP, status, len);
}

