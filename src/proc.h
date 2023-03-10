#pragma once
#include "undocWinTypes.h"
#include <TlHelp32.h>

// Functions to retrieve information about of a windows process.
// Some functions are implemented to emulate functions of the Win32 API and delcared as similarly as possible.
// These Win32 API calls could be replaced for example to avoid simple detections.
// Most functions are defined to interact with the caller process as well as an external process.
// The x64 compilations of these functions are capable of interacting with both x86 as well as x64 target processes.

namespace proc {

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

	// Functions to retrieve information about an external process.
	// Compiled as x64 the external functions are designed to work both on x64 targets as well as x86 targets.
	// Compiled as x86 interacting with x64 processes is neihter supported nor feasable.
	namespace ex {

		// Gets the process id by process name.
		// 
		// Parameters:
		// 
		// [in] procName:
		// Name of the process.
		// 
		// Return:
		// Process id of the process, 0 on failure or no process was found.
		// If there are multiple proccesses with the same name the id of one of them is returned.
		DWORD getProcId(const char* procName);

		// Gets a process entry of a snapshot of CreateToolhelp32Snapshot by process name.
		// 
		// Parameters:
		// 
		// [in] procName:
		// Name of the process.
		// 
		// [out] pProcEntry:
		// Address of the process entry structure that receives the information about the process.
		// If there are multiple proccesses with the same name the structure of one of them is returned.
		// 
		// Return:
		// True on success, false on failure or if process was not found.
		bool getTlHelpProcEntry(const char* procName, PROCESSENTRY32* pProcEntry);

		// Gets a module entry of a snapshot of CreateToolhelp32Snapshot by module name.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the process which modules should be searched.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION access rights.
		// 
		// [in] modName:
		// Name of the module.
		// 
		// [out] pModEntry:
		// Address of the module entry structure that receives the information about the module.
		// If there are multiple modules with the same name the structure of one of them is returned.
		// 
		// Return:
		// True on success, false on failure or if module was not found.
		bool getTlHelpModEntry(HANDLE hProc, const char* modName, MODULEENTRY32* pModEntry);

		// Gets a thread entry of a snapshot of CreateToolhelp32Snapshot by owning process.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the process which threads should be searched.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION access rights.
		// 
		// [out] pThreadEntry:
		// Address of the thread entry structure that receives the information about the thread.
		// The thread entry will have a different th32ThreadID value than the structure passed to the function.
		// This way threads of a specific process can be enumerated.
		// If there are multiple threads with the same owning process and differing thread ids the structure of one of them is returned.
		// 
		// Return:
		// True on success, false on failure or if no thread was found.
		bool getTlHelpThreadEntry(HANDLE hProc, THREADENTRY32* pThreadEntry);

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
		// [in] procId:
		// The process id of the target process.
		// 
		// [in] modName:
		// The name of the module. If nullptr returns a handle to the module of the file used to create the calling process (.exe file).
		// 
		// Return:
		// Handle to the module or nullptr on failure or if the module was not found.
		HMODULE getModuleHandle(DWORD procId, const char* modName);

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
		// Address of the x64 process environment block or nullptr on failure.
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
		// Address of the x86 process environment block or nullptr on failure or if the process is not x86 (not running in the WOW64 environment).
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