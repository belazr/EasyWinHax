#pragma once
#include "IHook.h"

namespace hax {

	namespace ex {

		// Class to set up an import address table hook inside a module of an external process.
		// It injects shell code into the target process which gets called instead of the origin function via the import address table.
		// The shell code can contain a call of a placeholder address with the same calling convention as the origin function to ensure uninterrupted process execution.
		// This placeholder pointer in the shell code gets replaced with the address of the origin function via pattern scanning at runtime.
		// It is neccessary to use a value unique within the shell code (eg. 0XDEADBEEF for x86) as the placeholder.
		// This pattern has to be passed as an argument on object construction.
		// When the origin function gets called by the target process (after enabling the hook) execution jumps to the injected shell code via import address table.
		// At the call of the placeholder execution jumps back to the origin function.
		// Compiled to x64 the hook works both on x86 and x64 targets. Compiled to x86 it only works on x86 targets.
		// The shell code always has to be compiled to the same architecture as the target process.
		// The hook automatically uninstalls on desctuction of the installing object.
		class IatHook : public IHook {
		private:
			const HANDLE _hProc;
			BYTE* _origin;
			BYTE* _detour;
			BYTE* _pIatEntry;
			bool _hooked;
			BOOL _isWow64Proc;

		public:
			// Initializes members.
			// 
			// Parameters:
			// 
			// [in] hImportMod:
			// Handle/Base address of the module which IAT should be hooked.
			// 
			// [in] funcName:
			// Export name of the function whos IAT entry should be replaced in the importing module.
			// 
			// [in] exportModName:
			// Name of the module that exports the function whos IAT entry should be replaced in the importing module.
			// 
			// [in] shell:
			// Address of the shell code that should be injected and executed on a call of the origin function.
			// 
			// [in] shellSize:
			// Size of the shell code in bytes.
			// 
			// [in] originCallPattern:
			// Pattern of the origin function call in the shell code. The pattern has to be of the format:
			// "EF BE DA DE". "??" can be used as wildcards. Mind the endianness!
			// Can be nullptr if there is no call to the origin function in the shell code.
			IatHook(
				HANDLE hProc, HMODULE hImportMod, const char* funcName, const char* exportModName, const BYTE* shell, size_t shellSize, const char* originCallPattern
			);


			~IatHook();


			// Enables the hook. Execution of origin function is redirected after calling this method.
			// 
			// Return: True on success, false on failure
			bool enable();


			// Disables the hook. Execution of origin function is restored after calling this method.
			// 
			// Return:
			// True on success, false on failure.
			bool disable();


			// Checks if the hook is currently installed.
			// 
			// Return:
			// True if the hook is installed, false if it is not installed.
			bool isHooked() const;


			BYTE* getDetour() const;
			BYTE* getOrigin() const;
			BYTE* getIatEntryAddress() const;
		};

	}


	namespace in {

		// Class to set up an import address table hook inside a module of the caller process.
		// Typically the detour function is defined in a dll that was injected into the process.
		// The hook overwrites the IAT entry of the importing module.
		// When the origin function gets called by the process (after enabling the hook) execution jumps to the detour function via import address table.
		// The dll has to be compiled to the same architecture (x86 or x64) as the target process.
		// The hook automatically uninstalls on desctuction of the installing object.
		class IatHook : public IHook {
		private:
			const BYTE* _origin;
			const BYTE* const _detour;
			BYTE* _pIatEntry;
			bool _hooked;

		public:
			// Initializes members.
			// 
			// Parameters:
			// 
			// [in] hImportMod:
			// Handle/Base address of the module which IAT should be hooked.
			// 
			// [in] funcName:
			// Export name of the function whos IAT entry should be replaced in the importing module.
			// 
			// [in] exportModName:
			// Name of the module that exports the function whos IAT entry should be replaced in the importing module.
			// 
			// [in] detour:
			// Address of the detour function within the virtual address space of the target process.
			IatHook(HMODULE hImportMod, const char* funcName, const char* exportModName, const BYTE* detour);


			~IatHook();


			// Enables the hook. Execution of origin function is redirected after calling this method.
			// 
			// Return: True on success, false on failure
			bool enable();


			// Disables the hook. Execution of origin function is restored after calling this method.
			// 
			// Return:
			// True on success, false on failure.
			bool disable();


			// Checks if the hook is currently installed.
			// 
			// Return:
			// True if the hook is installed, false if it is not installed.
			bool isHooked() const;


			BYTE* getDetour() const;
			BYTE* getOrigin() const;
			BYTE* getIatEntryAddress() const;
		};

	}

}

