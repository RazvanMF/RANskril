#pragma once
#include <fltKernel.h>

constexpr auto DRIVER_HASHTABLE_TAG = 'Thsh';

/// <summary>
/// Defines a structure representing an element in a hash table.
/// Each element contains a linked list cursor and a pointer to a UNICODE_STRING.
/// </summary>
typedef struct _HASH_TABLE_ELEMENT {
	LIST_ENTRY cursor;
	PUNICODE_STRING pString;
} HASH_TABLE_ELEMENT, *PHASH_TABLE_ELEMENT;


/// <summary>
/// Defines a hash table for managing UNICODE_STRING objects, providing operations for initialization, addition, deletion, lookup, and destruction.
/// The hash table uses DJB2 as a hash function to distribute strings across 256 buckets, each represented by a linked list.
/// </summary>
typedef struct _UNICODE_STRING_HASH_TABLE {
private:
	LIST_ENTRY table[256];

	/// <summary>
	/// Computes a hash value for a given Unicode string using the DJB2 hash algorithm, with a modulus operation to fit the hash into a range of 0-255.
	/// </summary>
	/// <param name="string">A pointer to a UNICODE_STRING structure containing the string to hash.</param>
	/// <returns>A 32-bit unsigned integer, represented as UINT32, containing the computed hash value.</returns>
	UINT32 HashFunction(PUNICODE_STRING string) {
		UINT32 seed = 5381;
		UINT32 length = string->Length / sizeof(WCHAR);
		for (UINT32 i = 0; i < length; i++) {
			seed = (((seed << 5) + seed) + string->Buffer[i]) % 256;  // translates to seed * 33 + string[i], the bitwise squeezes a little more cpu time
		}
		return seed;
	}

public:

	/// <summary>
	/// Initializes the hash table by setting up list heads for each entry.
	/// </summary>
	/// <returns>An NTSTATUS value indicating the success of the operation. Typically returns STATUS_SUCCESS.</returns>
	NTSTATUS InitializeHashTable() {
		for (int i = 0; i < 256; i++)
			InitializeListHead(&table[i]);

		return STATUS_SUCCESS;
	}

	/// <summary>
	/// Checks if a given Unicode string exists in the hash table and returns the corresponding hash table element if found.
	/// </summary>
	/// <param name="pQueriedString">A pointer to the Unicode string to search for in the hash table.</param>
	/// <returns>A pointer to the hash table element containing the queried string if it exists, or `nullptr` if the string is not found.</returns>
	PHASH_TABLE_ELEMENT StringExistsInHashTable(PUNICODE_STRING pQueriedString) {
		UINT32 position = HashFunction(pQueriedString);
		if (IsListEmpty(&table[position]))
			return nullptr;

		PLIST_ENTRY pHead = &table[position];
		PLIST_ENTRY pArrow = pHead->Flink;
		while (pArrow != pHead) {
			PHASH_TABLE_ELEMENT pCurrentTableElement = CONTAINING_RECORD(pArrow, HASH_TABLE_ELEMENT, cursor);  // https://learn.microsoft.com/en-us/windows/win32/api/ntdef/nf-ntdef-containing_record
			if (RtlCompareUnicodeString(pCurrentTableElement->pString, pQueriedString, TRUE) == 0)
				return pCurrentTableElement;

			pArrow = pArrow->Flink;
		}
		return nullptr;
	}

	/// <summary>
	/// Adds a string into the hash table, ensuring no duplicates and handling memory allocation.
	/// </summary>
	/// <param name="pAddedString">A pointer to a UNICODE_STRING structure representing the string to be added to the hash table.</param>
	/// <returns>An NTSTATUS code indicating the result of the operation. Possible values include STATUS_SUCCESS, STATUS_DUPLICATE_OBJECTID, and STATUS_NO_MEMORY.</returns>
	NTSTATUS AddStringIntoHashTable(PUNICODE_STRING pAddedString) {
		UINT32 position = HashFunction(pAddedString);
		if (StringExistsInHashTable(pAddedString) != nullptr)
			return STATUS_DUPLICATE_OBJECTID;

		PUNICODE_STRING pTableEntry = (PUNICODE_STRING)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(UNICODE_STRING), DRIVER_HASHTABLE_TAG);
		if (!pTableEntry)
			return STATUS_NO_MEMORY;


		pTableEntry->Buffer = (WCHAR*)ExAllocatePool2(POOL_FLAG_NON_PAGED, pAddedString->Length, DRIVER_HASHTABLE_TAG);
		if (!pTableEntry->Buffer) {
			ExFreePool(pTableEntry);
			return STATUS_NO_MEMORY;
		}

		pTableEntry->Length = pAddedString->Length;
		pTableEntry->MaximumLength = pAddedString->MaximumLength;
		RtlCopyMemory(pTableEntry->Buffer, pAddedString->Buffer, pAddedString->Length);


		PHASH_TABLE_ELEMENT pHashTableNewElement = (PHASH_TABLE_ELEMENT)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(HASH_TABLE_ELEMENT), DRIVER_HASHTABLE_TAG);
		if (!pHashTableNewElement) {
			ExFreePool(pTableEntry->Buffer);
			ExFreePool(pTableEntry);
			return STATUS_NO_MEMORY;
		}

		pHashTableNewElement->pString = pTableEntry;
		InsertHeadList(&table[position], &pHashTableNewElement->cursor);

		return STATUS_SUCCESS;
	}

	/// <summary>
	/// Deletes a string from the hash table and frees associated memory.
	/// </summary>
	/// <param name="pRemovedString">A pointer to the UNICODE_STRING structure representing the string to be removed from the hash table.</param>
	/// <returns>An NTSTATUS code indicating the result of the operation. Returns STATUS_SUCCESS if the string was successfully removed, or STATUS_NOT_FOUND if the string was not found in the hash table.</returns>
	NTSTATUS DeleteStringFromHashTable(PUNICODE_STRING pRemovedString) {
		UINT32 position = HashFunction(pRemovedString);
		if (IsListEmpty(&table[position]))
			return STATUS_NOT_FOUND;
		
		PHASH_TABLE_ELEMENT pContainingElement = StringExistsInHashTable(pRemovedString);
		if (pContainingElement == nullptr)
			return STATUS_NOT_FOUND;

		RemoveEntryList(&pContainingElement->cursor);
		ExFreePool(pContainingElement->pString->Buffer);
		ExFreePool(pContainingElement->pString);
		ExFreePool(pContainingElement);

		return STATUS_SUCCESS;
	}

	/// <summary>
	/// Destroys the hash table by freeing all allocated memory for its elements.
	/// </summary>
	/// <returns>An NTSTATUS code indicating the success or failure of the operation. Typically returns STATUS_SUCCESS upon successful destruction of the hash table.</returns>
	NTSTATUS DestroyHashTable() {
		for (int i = 0; i < 256; i++) {
			if (IsListEmpty(&table[i]))
				continue;

			while (!IsListEmpty(&table[i])) {
				PLIST_ENTRY pHead = RemoveHeadList(&table[i]);
				PHASH_TABLE_ELEMENT pCurrentElement = CONTAINING_RECORD(pHead, HASH_TABLE_ELEMENT, cursor);
				ExFreePool(pCurrentElement->pString->Buffer);
				ExFreePool(pCurrentElement->pString);
				ExFreePool(pCurrentElement);
			}
			
		}
		return STATUS_SUCCESS;
	}
} UNICODE_STRING_HASH_TABLE, *PUNICODE_STRING_HASH_TABLE;