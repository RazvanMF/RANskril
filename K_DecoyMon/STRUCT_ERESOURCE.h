#pragma once
#include <fltKernel.h>

// Example seen in Windows Kernel Programming by Pavel Yosifovich.

/// <summary>
/// Represents a synchronization primitive for managing shared and exclusive access to a resource.
/// This structure encapsulates an ERESOURCE object, providing methods to initialize, delete, and manage read/write locks.
/// </summary>
struct STRUCT_ERESOURCE {
	NTSTATUS Initialize();
	void Delete();

	void LockWrite();
	void UnlockWrite();

	void LockRead();
	void UnlockRead();
private:
	ERESOURCE lockResource;
};