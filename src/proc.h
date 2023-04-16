#pragma once
#include "undocWinTypes.h"

// Functions to retrieve information about of a windows process.
// Some functions are implemented to emulate functions of the Win32 API and delcared as similarly as possible.
// These Win32 API calls could be replaced for example to avoid simple detections.

namespace proc {

	// Alike PROCESSENTRY32 from the TlHelp32 library without the unused or unnecessary fields.
	typedef struct ProcessEntry {
		DWORD processId;
		DWORD threadCount;
		DWORD parentProcessId;
		LONG basePriority;
		char exeFile[MAX_PATH];
	}ProcessEntry;

	typedef struct ThreadEntry {
		DWORD threadId;
		DWORD ownerProcessId;
		THREAD_STATE threadState;
		KWAIT_REASON waitReason;
	}ThreadEntry;

	typedef struct PeHeaders {
		const IMAGE_DOS_HEADER* pDosHeader;
		const IMAGE_NT_HEADERS* pNtHeaders;
		const IMAGE_NT_HEADERS32* pNtHeaders32;
		const IMAGE_NT_HEADERS64* pNtHeaders64;
		const IMAGE_FILE_HEADER* pFileHeader;
		const IMAGE_OPTIONAL_HEADER* pOptHeader;
		const IMAGE_OPTIONAL_HEADER32* pOptHeader32;
		const IMAGE_OPTIONAL_HEADER64* pOptHeader64;
	}PeHeaders;

	// Gets the process id by process name.
	// 
	// Parameters:
	// 
	// [in] processName:
	// Name of the process.
	// 
	// Return:
	// Process id of the process, 0 on failure or no process was found.
	// If there are multiple proccesses with the same name, the id of one of them is returned.
	DWORD getProcessId(const char* processName);

	// Gets a process entry struct by process ID.
	// 
	// Parameters:
	// 
	// [in] processName:
	// Process ID of the process.
	// 
	// [out] pProcessEntry:
	// Address of the process entry structure that receives the information about the process.
	// 
	// Return:
	// True on success, false on failure or if process was not found.
	bool getProcessEntry(DWORD processId, ProcessEntry* pProcessEntry);

	// Gets the ThreadEntry structs of all execution threads started by a process.
	// 
	// Parameters:
	// 
	// [in] processId:
	// Process ID of the owning process of the threads.
	// 
	// [out] pThreadEntries:
	// Address of a buffer for the ThreadEntry structs.
	// 
	// [in]
	// size:
	// Size of the buffer in ThreadEntry structs.
	// 
	// Return:
	// True on success, false on failure or if the buffer was too small.
	bool getProcessThreadEntries(DWORD processId, ThreadEntry* pThreadEntries, size_t size);


	// Retreives a duplicate handle to a process.
	// The original handle will be owned by a process other than caller or target process.
	// Can be used instead of OpenProcess.
	// The caller process should use the handle as if it had opened the handle itself (including closing it).
	// Running as admin increases success rate significantly.
	//
	// Parameters:
	// 
	// [in] desiredAccess:
	// Access rights the returned handle should have.
	// 
	// [in] processId:
	// Process ID of the process  which handle should be duplicated.
	//
	// Return:
	// Handle to the specified process with the specified access rights on success, nullptr on failure.
	HANDLE getDuplicateProcessHandle(DWORD desiredAccess, DWORD processId);

	// Functions to retrieve information about an external process.
	// Compiled as x64 the external functions are designed to work both on x64 targets as well as x86 targets.
	// Compiled as x86 interacting with x64 processes is neihter supported nor feasable.
	namespace ex {

		// Gets the address of a function/procedure exported by a module of an external target process within the virtual address space this process.
		// Works like an external version of GetProcAddress of the Win32 API.
		// Uses only calls to ReadProcessMemory and NtQueryInformationProcess (for forwared functions) of the Win32 API.
		// Supports function forwarding but NOT FOR VIRTUAL DLLS (e.g. api-ms-win-...dll) using the ApiSetSchema.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access rights.
		// 
		// [in] hMod:
		// Handle to the module that exports the function.
		// 
		// [in] funcName:
		// Export name or ordinal of the exported function..
		// 
		// Return:
		// Address of the exported function within the virtual address space of the target process or nullptr on failure or if procedure was not found.
		FARPROC getProcAddress(HANDLE hProc, HMODULE hMod, const char* funcName);

		// Gets the address of a the import address table entry of a function imported by a module of an external target process within the virtual address space this process
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
		// 
		// [in] hImportMod:
		// Handle to the module that imports the function.
		// 
		// [in] exportModName:
		// Name of the module that exports the imported function.
		// 
		// [in] funcName:
		// Export name of the imported procedure. Has to be exported by name.
		// 
		// Return:
		// Address of the IAT entry within the virtual address space of the target process or nullptr on failure or if IAT entry was not found.
		BYTE* getIatEntryAddress(HANDLE hProc, HMODULE hImportMod, const char* exportModName, const char* funcName);

		// Gets the addresses of the PE headers of a module of an external target process within the virtual address space this process
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
		// 
		// [in] hMod:
		// Handle to the module.
		// 
		// [in] pPeHeaders:
		// Address of the pe headers structure that receives the addresses of the pe headers.
		// For architecure specific headers the matching specific addresses are set as well as the general ones with the same value.
		// True statement for an x64 caller and target process: pPeHeaders->pOptHeader == pPeHeaders->pOptHeader64 && pPeHeaders->pOptHeader32 == nullptr
		// True statement for an x64 caller and x86 target process: pPeHeaders->pOptHeader == nullptr && pPeHeaders->pOptHeader64 == nullptr
		// True statement for an x86 caller and target process: pPeHeaders->pOptHeader == pPeHeaders->pOptHeader32 && pPeHeaders->pOptHeader64 == nullptr
		// True statement for an x86 caller and x64 target process: pPeHeaders->pOptHeader == nullptr && pPeHeaders->pOptHeader32 == nullptr
		// 
		// Return:
		// True on success, false on failure.
		bool getPeHeaders(HANDLE hProc, HMODULE hMod, PeHeaders* pPeHeaders);

		// Gets a handle to a module of an external target process.
		// The value of the handle is equivalent to the base address of the module within the virtual address space of the target process.
		// Works like an external version of GetModuleHandle of the Win32 API.
		// Uses only calls to ReadProcessMemory and NtQueryInformationProcess of the Win32 API.
		// 
		// Parameters:
		// 
		// [in] processId:
		// The process id of the target process.
		// 
		// [in] modName:
		// The name of the module. If nullptr returns a handle to the module of the file used to create the calling process (.exe file).
		// 
		// Return:
		// Handle to the module or nullptr on failure or if the module was not found.
		HMODULE getModuleHandle(DWORD processId, const char* modName);

		// Gets a handle to a module of an external target process.
		// The value of the handle is equivalent to the base address of the module within the virtual address space of the target process.
		// Works like an external version of GetModuleHandle of the Win32 API.
		// Uses only calls to ReadProcessMemory and NtQueryInformationProcess of the Win32 API.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access rights.
		// 
		// [in] modName:
		// The name of the module. If nullptr returns a handle to the module of the file used to create the calling process (.exe file).
		// 
		// Return:
		// Handle to the module or nullptr on failure or if the module was not found.
		HMODULE getModuleHandle(HANDLE hProc, const char* modName);

		#ifdef _WIN64

		// Gets the address of the x64 loader data table entry of a module in the x64 loader data table of an external target process within the virtual address space this process.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access rights.
		// 
		// [in] modName:
		// The name of the module. If nullptr returns the address of the x64 loader data table entry of the module of the file used to create the calling process (.exe file).
		//
		// Return:
		// Address of the loader data table entry of the module or nullptr on failure or if the entry was not found.
		LDR_DATA_TABLE_ENTRY64* getLdrDataTableEntry64Address(HANDLE hProc, const char* modName);

		#endif // _WIN64

		// Gets the address of the x86 loader table entry of a module in the x86 loader data table of an external target process within the virtual address space this process.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access rights.
		// 
		// [in] modName:
		// The name of the module. If nullptr returns the address of the x86 loader data table entry of the module of the file used to create the calling process (.exe file).
		// 
		// Return:
		// Address of the loader data table entry of the module or nullptr on failure or if the entry was not found.
		LDR_DATA_TABLE_ENTRY32* getLdrDataTableEntry32Address(HANDLE hProc, const char* modName);
		
		#ifdef _WIN64

		// Gets the address of the x64 process environment block within the virtual address space of an external target process.
		// In x64 Windows every process has an x64 PEB.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION access rights.
		// 
		// Return:
		// Address of the x64 process environment block on success or nullptr on failure.
		PEB64* getPeb64Address(HANDLE hProc);
		
		#endif // _WIN64

		// Gets the address of the x86 process environment block within the virtual address space of an external target process.
		// In x64 Windows only x86 processes have an x86 PEB. This is the PEB of within the WOW64 environment.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the target process.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION access rights.
		// 
		// Return:
		// Address of the x86 process environment block on success or nullptr on failure or if the process is not x86 (not running in the WOW64 environment).
		PEB32* getPeb32Address(HANDLE hProc);

	}

	// Functions to retrieve information about the caller process.
	namespace in {

		// Gets the address of a function/procedure exported by a module of the caller process within the virtual address space of the process.
		// Works like GetProcAddress of the Win32 API.
		// Uses no calls to functions of the Win32 API.
		// Supports function forwarding but NOT WITH VIRTUAL DLLS (e.g. api-ms-win-...dll) using the ApiSetSchema.
		// 
		// Parameters:
		// 
		// [in] hMod:
		// Handle to the module that exports the function.
		// 
		// [in] funcName:
		// Export name or ordinal of the exported function..
		// 
		// Return:
		// Address of the exported function within the virtual address space of the caller process or nullptr on failure or if procedure not found.
		FARPROC getProcAddress(HMODULE hMod, const char* funcName);

		// Gets the address of a the import address table entry of a function imported by a module of the caller process within the virtual address space of the process.
		// 
		// Parameters:
		// 
		// [in] hImportMod:
		// Handle to the module that imports the function.
		// 
		// [in] exportModName:
		// Name of the module that exports the imported function.
		// 
		// [in] funcName:
		// Export name of the imported function. Has to be exported by name.
		// 
		// Return:
		// Address of the IAT entry or nullptr on failure or if IAT entry was not found.
		BYTE* getIatEntryAddress(HMODULE hImportMod, const char* exportModName, const char* funcName);
		
		// Gets the addresses of the PE headers of a module of the caller process within the virtual address space of the process.
		// Performs a runtime check for architecture so it can also be used for any pe header in a buffer within the virtual address space of the caller proccess.
		// 
		// Parameters:
		// 
		// [in] hMod:
		// Handle to the module or the base address of a pe header in a local buffer.
		// 
		// [in] pPeHeaders:
		// Address of the pe headers structure that receives the addresses of the pe headers.
		// For architecure specific headers the matching specific addresses are set as well as the general ones with the same value.
		// True statement for an x64 caller and target (buffer): pPeHeaders->pOptHeader == pPeHeaders->pOptHeader64 && pPeHeaders->pOptHeader32 == nullptr
		// True statement for an x64 caller and x86 target (buffer): pPeHeaders->pOptHeader == nullptr && pPeHeaders->pOptHeader64 == nullptr
		// True statement for an x86 caller and target (buffer): pPeHeaders->pOptHeader == pPeHeaders->pOptHeader32 && pPeHeaders->pOptHeader64 == nullptr
		// True statement for an x86 caller and x64 target (buffer): pPeHeaders->pOptHeader == nullptr && pPeHeaders->pOptHeader32 == nullptr
		// 
		// Return:
		// True on success, false on failure.
		bool getPeHeaders(HMODULE hMod, PeHeaders* pPeHeaderPointers);

		// Gets a handle to a module of an external target process.
		// The value of the handle is equivalent to the base address of the module within the virtual address space of the target process.
		// Works like GetModuleHandle of the Win32 API.
		// Uses no calls of the Win32 API.
		// 
		// Parameters:
		// 
		// [in] modName:
		// The name of the module. If nullptr returns a handle to module of the file used to create the calling process (.exe file).
		// 
		// Return:
		// Handle to the module or nullptr if the module was not found.
		HMODULE getModuleHandle(const char* modName);

		// Gets the address of the loader table entry of a module of the caller process within the virtual address space of the process.
		// 
		// Parameters:
		// 
		// [in] modName:
		// The name of the module. If nullptr returns the address of the loader data table entry of the module of the file used to create the calling process (.exe file).
		// 
		// Return:
		// Address of the loader data table entry of the module or nullptr if the entry was not found.
		LDR_DATA_TABLE_ENTRY* getLdrDataTableEntryAddress(const char* modName);

		// Gets the address of the process environment block within the virtual address space of the caller process.
		// 
		// Return:
		// Address of the process environment block. Gets the address of the PEB within the WOW64 evironment for x86 processes.
		PEB* getPebAddress();

	}

}