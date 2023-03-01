#pragma once
#include <Windows.h>

// Functions to interact with the virtual memory of a windows process.
// Most functions are defined to interact with the caller process as well as an external process.


// Defines a class member variable by offset. Has to be used inside a union.
// Example:
// class Entity {
// public:
// 	union {
// 		#pragma warning (push)
// 		#pragma warning (disable: 4201)
// 		DEFINE_MEMBER_N(const int, member, 0x10);
// 		#pragma warning (pop)
// 	};
// };
#define STR_MERGE_IMPL(a, b) a##b
#define STR_MERGE(a, b) STR_MERGE_IMPL(a, b)
#define MAKE_PAD(size) STR_MERGE(_pad, __COUNTER__) [size]
#define DEFINE_MEMBER_N(type, name, offset) struct {unsigned char MAKE_PAD(offset); type name;}

namespace mem {

	// Functions to interact with the virtual memory of an external process.
	// Compiled to x64 the external functions are designed to work both on x64 targets as well as x86 targets.
	// Compiled to x86 interacting with x64 processes is neihter supported nor feasable.
	namespace ex {

		// Installs a trampoline hook in an external process. Has to be placed at the beginning of a function defined in the external process.
		// Typically the detour function is defined in shell code that was injected into the process. This shell code has to call a placeholder address with the same calling convention
		// as the hooked function for uninterrupted process execution.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
		// 
		// [in] origin:
		// Address of the origin function to be hooked within the virtual address space of the target process. At least the first five bytes will be overwritten.
		// 
		// [in] detour:
		// Address of the function that should be executed on a call of the origin function within the virtual address space of the target process.
		// Has to use the same calling convention as the origin function for uninterrupted process execution.
		// 
		// [in] originCall:
		// Address of the call of the origin function call within the detour function. The call should be of the same calling convention as the origin function.
		// Can be null if there is no call to the origin function in the detour.
		// 
		// [in] size:
		// Number of bytes that get overwritten by the jump at the beginning of the origin function.
		// The overwritten instructions get executed by the gateway right before executing the origin function.
		// Has to be at least five! Only complete instructions should be overwritten!
		// It is necessary to look at the disassembly of the origin function to find out when the first complete instruction finishes after the first five bytes.
		// 
		// Return:
		// Pointer to the gateway within the virtual address space of the target process or nullptr on failure (eg because of architecture incompatibility)
		// This address is called by the detour function at the address given by originCall.
		// The stolen bytes of the orgin function are located here.
		BYTE* trampHook(HANDLE hProc, BYTE* origin, const BYTE* detour, BYTE* originCall, size_t size);
		
		#ifdef _WIN64

		// Allocates memory in the virtual address space of an external process that is reachable by a relative jump (op code: E9) from a given address.
		// Reserves and commits the memory pages with PAGE_EXECUTE_READWRITE protection.
		// Only neccessary for x64 target processes because the whole x86 address space can be reached by a relative jump.
		// The Win32 APIs VirtualAllocEx can be used as an alternative for x86.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_OPERATION access rights.
		// 
		// [in] address:
		// The address within the virtual address space of the target process from which the allocated memory should be reachable by a relative jump (op code: E9).
		// 
		// [in] size:
		// Size of the allocated memory region in bytes.
		// 
		// Return:
		// Pointer to the allocated memory region.
		// Nullpointer if no memory is available or the function failed for other reasons.
		BYTE* virtualAllocNear(HANDLE hProc, const BYTE* address, size_t size);
		
		#endif // _WIN64
		
		// Patches a relative jump (op code: E9) into an external process.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_OPERATION, PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
		// 
		// [in] origin:
		// The address that should be patched with the jump instrucion within the virtual address space of the external process. At least five bytes will be overwritten.
		// 
		// [in] detour:
		// The address that should be jumped to within the virtual address space of the target process.
		// Has to be within reach by a relative jump from the origin address (abs(origin - detour) < UNIT32_MAX).
		// 
		// [in] size:
		// Number of bytes that get overwritten by the jump at the origin address.
		// Has to be at least five! Only complete instructions should be overwritten!
		// It is necessary to look at the disassembly at the origin address to find out when the first complete instruction finishes after the first five bytes.
		// 
		// Return:
		// Pointer to the first bytes after the origin address that have not been patched by the jump or nullpointer on failure.
		BYTE* relJmp(HANDLE hProc, BYTE* origin, const BYTE* detour, size_t size);
		
		#ifdef _WIN64
		
		// Patches an absolute jump into an external process. The address of the detour is loaded to the volatile r10 register.
		// Can only be used in an x64 process.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_OPERATION, PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
		// 
		// [in] origin:
		// The address that should be patched with the jump instrucion within the virtual address space of the external process. At least 13 bytes will be overwritten.
		// 
		// [in] detour:
		// The address that should be jumped to within the virtual address space of the target process.
		// 
		// [in] size:
		// Number of bytes that get overwritten by the jump at the origin address.
		// Has to be at least 13! Only complete instructions should be overwritten!
		// It is necessary to look at the disassembly at the origin address to find out when the first complete instruction finishes after the first 13 bytes.
		// 
		// Return:
		// Pointer to the first bytes after the origin address that have not been patched by the jump or nullpointer on failure.
		BYTE* absJumpX64(HANDLE hProc, BYTE* origin, const BYTE* detour, size_t size);
		
		#endif

		// Patches an external process with NOPs (op code 0x90).
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_OPERATION, PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
		// 
		// [in] dst:
		// The address that should be patched with NOPs within the virtual address space of the target process.
		// 
		// [in] size:
		// Amount of bytes that should be overwritten by a NOP instruction.
		// 
		// Return:
		// True on success, false on failure.
		bool nop(HANDLE hProc, BYTE* dst, size_t size);

		// Patches an external process. Overwrites the given amount of bytes in the target process with the bytes in a buffer.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_OPERATION, PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
		// 
		// [in] dst:
		// The address that should be patched within the virtual address space of the target process.
		// 
		// [in] src:
		// The buffer that should be patched into the external process. Has to be allocated in the virtual memory of the caller process.
		// 
		// [in] size:
		// Size of the source buffer. Beware of buffer overflow.
		// 
		// Return:
		// True on success, false on failure.
		bool patch(HANDLE hProc, BYTE* dst, const BYTE src[], size_t size);

		// Gets the address pointed to by a multi level pointer within the virtual address space of an external process.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_READ access rights.
		// 
		// [in] base:
		// The address of the base pointer within the virtual address space of the target process. This is typically a static address.
		// 
		// [in] src:
		// Buffer for the offsets.
		// 
		// [in] size:
		// Size of the offset buffer. Beware of buffer overflow.
		// 
		// Return:
		// The address pointed to by dereferencing the multi level pointer or nullpointer on failure.
		// Example: *(*(*(base + offset[0]) + offset[1]) + offset[2])
		BYTE* getMultiLevelPointer(HANDLE hProc, const BYTE* base, const size_t offsets[], size_t size);

		// Finds the address of a byte signature within the virtual address space of an external process.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION and PROCESS_VM_READ access rights.
		// 
		// [in] base:
		// Address where the search should start.
		// 
		// [in] size:
		// Amount of bytes that should be searched.
		// 
		// [in] signature:
		// The byte signature base hex that should be looked for as null terminated string.
		// Bytes have to be two characters and separeted by spaces. "??" can be used as wildcards.
		// Example: "DE AD ?? EF"
		// 
		// Return:
		// The address where the byte signature was found within the virtual address space of the target process.
		// Nullpointer if the signature was not found or the function failed.
		BYTE* findSigAddress(HANDLE hProc, const BYTE* base, size_t size, const char* signature);

		// Copies a nullterminated string from an external process to a buffer allocated in the virtual memory of the caller process.
		// Copies characters until a null character is copied or the target buffer is full.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_READ access rights.
		// 
		// [out] dst:
		// Target buffer for the string.
		// 
		// [in] src:
		// Address of the string within the virtual address space of the target buffer.
		// 
		// [in] size:
		// Size of the target buffer. Beware of buffer overflows.
		// 
		// Return:
		// True on success, false on failure or if no null char was copied.
		bool copyRemoteString(HANDLE hProc, char* dst, const BYTE* src, size_t size);
		
		// Unlinks an entry of a Win32 API doubly linked list in an external process.
		// Found for example in the loader data of the process environment block of a process (see undocWinTypes.h).
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
		// 
		// [in] listEntry:
		// List entry that should be unlinked.
		// 
		// Return:
		// True on success, false on failure.
		template <typename LE>
		bool unlinkListEntry(HANDLE hProc, LE listEntry);
		
		// Explicit template instantiation for x86 list entries because function is not called in this library.
		template bool unlinkListEntry<LIST_ENTRY32>(HANDLE hProc, LIST_ENTRY32 listEntry);
		
		#ifdef _WIN64

		// Explicit template instantiation for x64 list entries because function is not called in this library.
		template bool unlinkListEntry<LIST_ENTRY64>(HANDLE hProc, LIST_ENTRY64 listEntry);

		#endif // _WIN64

	}


	// Functions to interact with the virtual memory of the caller process interally.
	namespace in {

		// Installs a trampoline hook in the caller process. Has to be placed at the beginning of a function defined in the caller process.
		// Typically the detour function is defined in a dll that was injected into the process. This function has to call the returned gateway with the same calling convention
		// as the hooked function for uninterrupted process execution.
		// 
		// Parameters:
		// 
		// [in] origin:
		// Address of the origin function to be hooked within the virtual address space of the caller process. At least the first five bytes will be overwritten.
		// 
		// [in] detour:
		// Address of the function that should be executed on a call of the origin function within the virtual address space of the caller process.
		// Has to use the same calling convention as the origin function for uninterrupted process execution.
		// 
		// [in] size:
		// Number of bytes that get overwritten by the jump at the beginning of the origin function.
		// The overwritten instructions get executed by the gateway right before executing the origin function.
		// Has to be at least five! Only complete instructions should be overwritten!
		// It is necessary to look at the disassembly of the origin function to find out when the first complete instruction finishes after the first five bytes.
		// 
		// Return:
		// Pointer to the gateway. This address should be called by the detour function with the same calling convention as the origin function.
		// The stolen bytes of the orgin function are located here.
		BYTE* trampHook(BYTE* origin, const BYTE* detour, size_t size);
		
		#ifdef _WIN64
		
		// Allocates memory in the virtual address space of the caller process that is reachable by a relative jump (op code: E9) from a given address.
		// Reserves and commits the memory pages with PAGE_EXECUTE_READWRITE protection.
		// Only neccessary for x64 target processes because the whole x86 address space can be reached by a relative jump.
		// The Win32 APIs VirtualAllocEx can be used as an alternative for x86.
		// 
		// Parameters:
		// 
		// [in] address:
		// The address within the virtual address space of the caller process from which the allocated memory should be reachable by a relative jump (op code: E9).
		// 
		// [in] size:
		// Size of the allocated memory region in bytes.
		// 
		// Return:
		// Pointer to the allocated memory region
		// Nullpointer if no memory is available or the function failed.
		BYTE* virtualAllocNear(const BYTE* address, size_t size);
		
		#endif // _WIN64
		
		// Patches a relative jump (op code: E9) into the caller process.
		// 
		// Parameters:
		// 
		// [in] origin:
		// The address that should be patched with the jump instrucion within the virtual address space of the caller process. At least five bytes will be overwritten.
		// 
		// [in] detour:
		// The address that should be jumped to within the virtual address space of the caller process.
		// Has to be within reach by a relative jump from the origin address (abs(origin - detour) < UNIT32_MAX).
		// 
		// [in] size:
		// Number of bytes that get overwritten by the jump at the origin address.
		// Has to be at least five! Only complete instructions should be overwritten!
		// It is necessary to look at the disassembly at the origin address to find out when the first complete instruction finishes after the first five bytes.
		// 
		// Return:
		// Pointer to the first bytes after the origin address that have not been patched by the jump or nullpointer on failure.
		BYTE* relJmp(BYTE* origin, const BYTE* detour, size_t size);
		
		#ifdef _WIN64
		
		// Patches an absolute jump into the caller process. The address of the detour is loaded to the volatile r10 register.
		// Can only be used in an x64 process.
		// 
		// Parameters:
		// 
		// [in] origin:
		// The address that should be patched with the jump instrucion within the virtual address space of the caller process. At least 13 bytes will be overwritten.
		// 
		// [in] detour:
		// The address that should be jumped to within the virtual address space of the target process.
		// 
		// [in] size:
		// Number of bytes that get overwritten by the jump at the origin address.
		// Has to be at least 13! Only complete instructions should be overwritten!
		// It is necessary to look at the disassembly at the origin address to find out when the first complete instruction finishes after the first 13 bytes.
		// 
		// Return:
		// Pointer to the first bytes after the origin address that have not been patched by the jump or nullpointer on failure.
		BYTE* absJumpX64(BYTE* origin, const BYTE* detour, size_t size);
		
		#endif
		
		// Patches the caller process with NOPs (op code 0x90).
		// 
		// Parameters:
		// 
		// [in] dst:
		// The address that should be patched with NOPs within the virtual address space of the caller process.
		// 
		// [in] size:
		// Amount of bytes that should be overwritten by a NOP instruction.
		// 
		// Return:
		// True on success, false on failure.
		bool nop(BYTE* dst, size_t size);

		// Patches the caller process. Overwrites the given amount of bytes in the caller process with the bytes in a buffer.
		// 
		// Parameters:
		// 
		// [in] dst:
		// The address that should be patched within the virtual address space of the caller process.
		// 
		// [in] src:
		// The buffer that should be patched into the target process. Has to be allocated in the virtual memory of the caller process.
		// 
		// [in] size:
		// Size of the source buffer. Beware of buffer overflow.
		// 
		// Return:
		// True on success, false on failure.
		bool patch(BYTE* dst, const BYTE src[], size_t size);
		
		// Gets the address pointed to by a multi level pointer within the virtual address space of the caller process.
		// 
		// Parameters:
		// 
		// [in] base:
		// The address of the base pointer within the virtual address space of the caller process. This is typically a static address.
		// 
		// [in] src:
		// Buffer for the offsets.
		// 
		// [in] size:
		// Size of the offset buffer. Beware of buffer overflow.
		// 
		// Return:
		// The address pointed to by dereferencing the multi level pointer or nullpointer on failure.
		// Example: *(*(*(base + offset[0]) + offset[1]) + offset[2])
		BYTE* getMultiLevelPointer(const BYTE* base, const size_t offsets[], size_t size);

		// Finds the address of a byte signature within the virtual address space of the caller process.
		// 
		// Parameters:
		// 
		// [in] base:
		// Address where the search should start.
		// 
		// [in] size:
		// Amount of bytes that should be searched.
		// 
		// [in] signature:
		// The byte signature base hex that should be looked for as null terminated string.
		// Bytes have to be two characters and separeted by spaces. "??" can be used as wildcards.
		// Example: "DE AD ?? EF"
		// 
		// Return:
		// The address where the byte signature was found within the virtual address space of the caller process.
		// Nullpointer if the signature was not found or the function failed.
		BYTE* findSigAddress(const BYTE* base, size_t size, const char* signature);
		
		// Unlinks an entry of a Win32 API doubly linked list in the target process.
		// Found for example in the loader data of the process environment block of a process (see undocWinDefs.h).
		// 
		// Parameters:
		// 
		// [in] listEntry:
		// List entry that should be unlinked.
		// 
		// Return:
		// True on success, false on failure.
		template <typename LE>
		bool unlinkListEntry(LE listEntry);

		#ifdef _WIN64
		
		// Explicit template instantiation for x64 list entries because function is not called in this library.
		template bool unlinkListEntry<LIST_ENTRY64>(LIST_ENTRY64 listEntry);

		#else

		// Explicit template instantiation for x86 list entries because function is not called in this library.
		template bool unlinkListEntry<LIST_ENTRY32>(LIST_ENTRY32 listEntry);
		
		#endif // !_WIN64

	}


	// Helper function that are called both by the internal and external functions.
	namespace helper {

		// Converts a byte signature provided by characters in a string to a siganture of integers as used by findSignature.
		// 
		// Parameters:
		// 
		// [in] charSig:
		// The byte signature base hex that should be looked for as null terminated string.
		// Bytes have to be two characters and separeted by spaces. "??" can be used as wildcards and will be converted to -1.
		// Example: "DE AD ?? EF"
		// 
		// [out] intSig:
		// Buffer for the integer signature. Enough memory has to be allocted to hold all the bytes from the charSig.
		// 
		// [in] size:
		// Size of the buffer for the integer signature.
		// 
		// Return:
		// True on success, false if the buffer has an invalid size.
		bool bytestringToInt(const char* charSig, int intSig[], size_t size);

		// Finds the address of a byte signature within a single memory region of the caller process.
		// Do not use across multiple memory regions. Use findSignatureAddress instead.
		// 
		// Parameters:
		// 
		// [in] base:
		// Address where the search should start.
		// 
		// [in] size:
		// Amount of bytes that should be searched. Should not cross memory region boundries.
		// 
		// [in] signature:
		// The byte signature that should be looked for as an array of integers.
		// Values have to be between -1 and 255. -1 is treated as a wildcard.
		// 
		// Return:
		// The address where the byte signature was found within the memory region of the caller process.
		// Nullpointer if the signature was not found.
		BYTE* findSignature(const BYTE* base, size_t size, const int* signature, size_t sigSize);

	}

}