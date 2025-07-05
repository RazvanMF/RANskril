#include "pch.h"
#include "STRUCT_ERESOURCE.h"

/// <summary>
/// Initializes the ERESOURCE structure for synchronization purposes.
/// </summary>
/// <returns>An NTSTATUS value indicating the success or failure of the initialization.</returns>
NTSTATUS STRUCT_ERESOURCE::Initialize() {
	return ExInitializeResourceLite(&lockResource);
}

/// <summary>
/// Deletes the resource associated with the ERESOURCE.
/// </summary>
void STRUCT_ERESOURCE::Delete() {
	ExDeleteResourceLite(&lockResource);
}

/// <summary>
/// Acquires a write lock on the executive resource, ensuring the current thread is in a critical region that forbids asynchronous procedure calls (APCs).
/// </summary>
void STRUCT_ERESOURCE::LockWrite() {
	ExEnterCriticalRegionAndAcquireResourceExclusive(&lockResource);
}

/// <summary>
/// Releases the write lock on the resource and exits the critical region.
/// </summary>
void STRUCT_ERESOURCE::UnlockWrite() {
	ExReleaseResourceAndLeaveCriticalRegion(&lockResource);
}

/// <summary>
/// Acquires a shared (read) lock on the resource, ensuring thread-safe access and write protection.
/// </summary>
void STRUCT_ERESOURCE::LockRead() {
	ExEnterCriticalRegionAndAcquireResourceShared(&lockResource);
}

/// <summary>
/// Releases the read lock on the resource by invoking the write unlock method.
/// </summary>
void STRUCT_ERESOURCE::UnlockRead() {
	UnlockWrite();
}