#pragma once
#include <fltKernel.h>

constexpr auto DRIVER_LIST_TAG = 'tslP';

/// <summary>
/// Defines a structure representing an element in an iterable list.
/// Each element contains a cursor for list traversal, represented as a LIST_ENTRY and a pointer to a UNICODE_STRING.
/// </summary>
typedef struct _ITERABLE_LIST_ELEMENT {
	LIST_ENTRY cursor;
	PUNICODE_STRING pString;
} ITERABLE_LIST_ELEMENT, * PITERABLE_LIST_ELEMENT;


/// <summary>
/// Defines a structure for managing a list of Unicode strings with iterable functionality.
/// The list is implemented using a linked list structure, allowing for efficient insertion and traversal.
/// The list supports operations such as adding strings, checking for existence, and iterating through elements.
/// </summary>
typedef struct _UNICODE_STRING_ITERABLE_LIST {
private:
	LIST_ENTRY listHead;
public:

	/// <summary>
	/// Initializes the iterable list by setting up its head and returns a success status.
	/// </summary>
	/// <returns>An NTSTATUS value indicating the success of the initialization. Typically returns STATUS_SUCCESS.</returns>
	NTSTATUS InitializeIterableList() {
		InitializeListHead(&listHead);
		return STATUS_SUCCESS;
	}

	/// <summary>
	/// Checks if a specified Unicode string exists in the linked list and returns the corresponding list element if found.
	/// </summary>
	/// <param name="pQueriedString">A pointer to the Unicode string to search for in the list.</param>
	/// <returns>A pointer to the list element containing the queried string if it exists, or `nullptr` if the string is not found.</returns>
	PITERABLE_LIST_ELEMENT ElementExistsInList(PUNICODE_STRING pQueriedString) {
		PLIST_ENTRY pHead = &listHead;
		PLIST_ENTRY pArrow = pHead->Flink;
		while (pArrow != pHead) {
			PITERABLE_LIST_ELEMENT pCurrentListElement = CONTAINING_RECORD(pArrow, ITERABLE_LIST_ELEMENT, cursor);
			if (RtlCompareUnicodeString(pCurrentListElement->pString, pQueriedString, TRUE) == 0)
				return pCurrentListElement;

			pArrow = pArrow->Flink;
		}
		return nullptr;
	}

	/// <summary>
	/// Adds a Unicode string to the list, ensuring no duplicates, and allocates necessary memory for storage.
	/// </summary>
	/// <param name="pAddedString">A pointer to the Unicode string to be added to the list.</param>
	/// <returns>An NTSTATUS code indicating the result of the operation. Possible values include STATUS_SUCCESS, STATUS_DUPLICATE_OBJECTID, and STATUS_NO_MEMORY.</returns>
	NTSTATUS AddStringIntoList(PUNICODE_STRING pAddedString) {
		if (ElementExistsInList(pAddedString) != nullptr)
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


		PITERABLE_LIST_ELEMENT pIterableListNewElement = (PITERABLE_LIST_ELEMENT)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(ITERABLE_LIST_ELEMENT), DRIVER_HASHTABLE_TAG);
		if (!pIterableListNewElement) {
			ExFreePool(pTableEntry->Buffer);
			ExFreePool(pTableEntry);
			return STATUS_NO_MEMORY;
		}

		pIterableListNewElement->pString = pTableEntry;
		InsertHeadList(&listHead, &pIterableListNewElement->cursor);

		return STATUS_SUCCESS;
	}

	/// <summary>
	/// Retrieves the next element in the iterable list.
	/// </summary>
	/// <param name="pCurrent">A pointer to the current element in the list. If null, the function returns the first element.</param>
	/// <returns>A pointer to the next element in the list, or null if the list is empty or the end of the list is reached.</returns>
	PITERABLE_LIST_ELEMENT GetNextElement(PITERABLE_LIST_ELEMENT pCurrent) {
		if (IsListEmpty(&listHead))
			return nullptr;

		if (pCurrent == nullptr) {
			return CONTAINING_RECORD(listHead.Flink, ITERABLE_LIST_ELEMENT, cursor);
		}

		if (pCurrent->cursor.Flink == &listHead)
			return nullptr;

		return CONTAINING_RECORD(pCurrent->cursor.Flink, ITERABLE_LIST_ELEMENT, cursor);
	}

	/// <summary>
	/// Determines whether there is a next element in the iterable list.
	/// </summary>
	/// <param name="pCurrent">A pointer to the current list element being checked.</param>
	/// <returns>TRUE if there is a next element in the list; otherwise, FALSE.</returns>
	BOOLEAN HasNextElement(PITERABLE_LIST_ELEMENT pCurrent) {
		if (pCurrent == nullptr || IsListEmpty(&listHead))
			return FALSE;
		return (pCurrent->cursor.Flink != &listHead);
	}

	/// <summary>
	/// Retrieves the first element from the linked list if the list is not empty.
	/// </summary>
	/// <returns>A pointer to the first element in the list, or nullptr if the list is empty.</returns>
	PITERABLE_LIST_ELEMENT GetFirstElement() {
		if (IsListEmpty(&listHead))
			return nullptr;
		return CONTAINING_RECORD(listHead.Flink, ITERABLE_LIST_ELEMENT, cursor);
	}

	/// <summary>
	/// Destroys the iterable list by freeing its elements and associated memory.
	/// </summary>
	/// <returns>An NTSTATUS value indicating the success of the operation. Returns STATUS_SUCCESS if the list is successfully destroyed.</returns>
	NTSTATUS DestroyIterableList() {
		if (IsListEmpty(&listHead))
			return STATUS_SUCCESS;

		while (!IsListEmpty(&listHead)) {
			PLIST_ENTRY pHead = RemoveHeadList(&listHead);
			PITERABLE_LIST_ELEMENT pCurrentElement = CONTAINING_RECORD(pHead, ITERABLE_LIST_ELEMENT, cursor);
			ExFreePool(pCurrentElement->pString->Buffer);
			ExFreePool(pCurrentElement->pString);
			ExFreePool(pCurrentElement);
		}
		return STATUS_SUCCESS;
	}

} UNICODE_STRING_ITERABLE_LIST, * PUNICODE_STRING_ITERABLE_LIST;