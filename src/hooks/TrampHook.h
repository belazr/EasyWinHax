#pragma once
#include "IHook.h"
#include <stdint.h>

namespace hax {

	namespace ex {

		// Class to set up a tarmpoline function hook in an external process.
		// It injects shell code into the target process and redirects execution of a function to this code.
		// The shell code has to contain a call of a placeholder address with the same calling convention as the origin function to ensure uninterrupted process execution.
		// This placeholder pointer in the shell code gets replaced with the address of the gateway/trampoline via pattern scanning at runtime.
		// It is neccessary to use a value unique within the shell code (eg. 0XDEADBEEF for x86) as the placeholder.
		// This pattern has to be passed as an argument on object construction.
		// When the origin function gets called by the target process (after enabling the hook) execution first jumps to the injected shell code.
		// At the call of the placeholder execution jumps to the gateway containing the overwritten bytes of the origin function and then jumps back to the origin function.
		// The stolen bytes overwritten by the jump from the origin function may not contain any references to data or code with static addresses.
		// Compiled to x64 the hook works both on x86 and x64 targets. Compiled to x86 it only works on x86 targets.
		// The shell code always has to be compiled for the same architecture as the target process.
		// The hook automatically uninstalls on desctuction of the installing object.
		class TrampHook : public IHook {
		private:
			const HANDLE _hProc;
			void* _origin;
			void* _detour;
			void* _detourOriginCall;
			void* _gateway;
			const size_t _size;
			const size_t _relativeAddressOffset;
			uint32_t _relativeAddress;
			bool _hooked;

		public:
			// Injects shell code into the target process and initializes members.
			//
			// Parameters:
			//
			// [in] hProc:
			// Handle to the process that contains the origin function to be hooked.
			// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_READ and PROCESS_VM_WRITE access rights.
			// 
			// [in] origin:
			// Address of the origin function to be hooked within the virtual address space of the target process. At least the first five bytes will be overwritten.
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
			// 
			// [in] size:
			// Number of bytes that get overwritten by the jump at the beginning of the origin function.
			// The overwritten instructions get executed by the gateway right before executing the origin function.
			// Has to be at least five! Only complete instructions should be overwritten!
			// It is necessary to look at the disassembly of the origin function to determine when the first complete instruction finishes after the first five bytes.
			//			
			// [in] relativeAddressOffset:
			// The offset of a relative Address if there is one in the first <size> bytes of the origin function.
			// It is necessary to look at the disassembly of the origin function to determine if there is a relative address in the first <size> bytes.
			// If there is no relative address in the first <size> bytes of the origin function the default value of SIZE_MAX should be passed. This value will then be ignored.
			TrampHook(HANDLE hProc, BYTE* origin, const BYTE* shell, size_t shellSize, const char* originCallPattern, size_t size, size_t relatvieAddressOffset = SIZE_MAX);

			// Injects shell code into the target process and initializes members. Used to hook a exported function of a module of the target process by module name and export name.
			// Hooks the beginning of the function, not the import address table, import directory or export directory!
			//
			// Parameters:
			// 
			// [in] hProc:
			// Handle to the process that contains the function to be hooked.
			// 
			// [in] modName:
			// Name of the module that exports the function to be hooked.
			// 
			// [in] funcName:
			// Export name of the function to be hooked. The function needs to be exported by a module loaded in the target process. At least the first five bytes will be overwritten.
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
			// 
			// [in] size:
			// Number of bytes that get overwritten by the jump at the beginning of the origin function.
			// The overwritten instructions get executed by the gateway right before executing the origin function.
			// Has to be at least five! Only complete instructions should be overwritten!
			// It is necessary to look at the disassembly of the origin function to determine when the first complete instruction finishes after the first five bytes.
			//			
			// [in] relativeAddressOffset:
			// The offset of a relative Address if there is one in the first <size> bytes of the origin function.
			// It is necessary to look at the disassembly of the origin function to determine if there is a relative address in the first <size> bytes.
			// If there is no relative address in the first <size> bytes of the origin function the default value of SIZE_MAX should be passed. This value will then be ignored.
			TrampHook(
				HANDLE hProc, const char* modName, const char* funcName, const BYTE* shell, size_t shellSize, const char* originCallPattern, size_t size, size_t relativeAddressOffset = SIZE_MAX
			);


			TrampHook(TrampHook&&) = delete;

			TrampHook(const TrampHook&) = delete;

			TrampHook& operator=(TrampHook&&) = delete;

			TrampHook& operator=(const TrampHook&) = delete;

			~TrampHook();

			// Enables the hook. Execution of origin function is redirected after calling this method.
			// 
			// Return:
			// True on success, false on failure
			bool enable();

			// Disables the hook. Execution of origin function is restored after calling this method.
			// 
			// Return:
			// True on success, false on failure. It is possible that the unhooking of the origin function succeeds but the deallocation of the gateway fails.
			// In this case the function will also return false as well as a call to isHooked().
			bool disable();

			// Checks if the hook is currently installed.
			// 
			// Return:
			// True if the hook is installed, false if it is not installed.
			bool isHooked() const;

			void* getOrigin() const;
			void* getDetour() const;
			void* getGateway() const;
		};

	}


	namespace in {

		// Class to set up a trampoline function hook inside the caller process.
		// Typically the detour function is defined in a dll that was injected into the process. This function has to call the gateway with the same calling convention
		// as the hooked function for uninterrupted process execution.
		// When the origin function gets called by the process (after enabling the hook) execution first jumps to the detour function.
		// When the gateway gets called execution jumps to the gateway containing the overwritten bytes of the origin function and then jumps back to the origin function.
		// The stolen bytes overwritten by the jump from the origin function may not contain any references to data or code with static addresses.
		// The injected dll has to be compiled to the same architecture (x86 or x64) as the target process.
		// The hook automatically uninstalls on desctuction of the installing object.
		class TrampHook : public IHook {
		private:
			void* _origin;
			const void* const _detour;
			void* _gateway;
			const size_t _size;
			const size_t _relativeAddressOffset;
			uint32_t _relativeAddress;
			bool _hooked;

		public:
			// Initializes members.
			// 
			// Parameters:
			// 
			// [in] origin:
			// Address of the origin function to be hooked within the virtual address space of the target process. At least the first five bytes will be overwritten.
			// 
			// [in] detour:
			// Address of the detour function within the virtual address space of the target process.
			// 
			// [in] size:
			// Number of bytes that get overwritten by the jump at the beginning of the origin function.
			// The overwritten instructions get executed by the gateway right before executing the origin function.
			// Has to be at least five! Only complete instructions should be overwritten!
			// It is necessary to look at the disassembly of the origin function to determine when the first complete instruction finishes after the first five bytes.
			//
			// [in] relativeAddressOffset:
			// The offset of a relative Address if there is one in the first <size> bytes of the origin function.
			// It is necessary to look at the disassembly of the origin function to determine if there is a relative address in the first <size> bytes.
			// If there is no relative address in the first <size> bytes of the origin function the default value of SIZE_MAX should be passed. This value will then be ignored.
			TrampHook(BYTE* origin, const BYTE* detour, size_t size, size_t relativeAddressOffset = SIZE_MAX);

			// Initializes members. Used to hook a exported function of a module of the target process by module name and export name.
			// Hooks the beginning of the function, not the import address table, import directory or export directory!
			// 
			// Parameters:
			// 
			// [in] modName:
			// Name of the module that exports the function to be hooked.
			// 
			// [in] funcName:
			// Export name of the function to be hooked. The function needs to be exported by a module loaded in the target process. At least the first five bytes will be overwritten.
			// 
			// [in] detour:
			// Address of the detour function within the virtual address space of the target process.
			// 
			// [in] size:
			// Number of bytes that get overwritten by the jump at the beginning of the origin function.
			// The overwritten instructions get executed by the gateway right before executing the origin function.
			// Has to be at least five! Only complete instructions should be overwritten!
			// It is necessary to look at the disassembly of the origin function to determine when the first complete instruction finishes after the first five bytes.
			//
			// [in] relativeAddressOffset:
			// The offset of a relative Address if there is one in the first <size> bytes of the origin function.
			// It is necessary to look at the disassembly of the origin function to determine if there is a relative address in the first <size> bytes.
			// If there is no relative address in the first <size> bytes of the origin function the default value of SIZE_MAX should be passed. This value will then be ignored.
			TrampHook(const char* modName, const char* funcName, const BYTE* detour, size_t size, size_t relativeAddressOffset = SIZE_MAX);

			~TrampHook();

			// Enables the hook. Execution of origin function is redirected after calling this method.
			// 
			// Return: True on success, false on failure
			bool enable();

			// Disables the hook. Execution of origin function is restored after calling this method.
			// 
			// Return:
			// True on success, false on failure. It is possible that the unhooking of the origin function succeeds but the deallocation of the gateway fails.
			// In this case the function will also return false as well as a call to isHooked.
			bool disable();

			// Checks if the hook is currently installed.
			// 
			// Return:
			// True if the hook is installed, false if it is not installed.
			bool isHooked() const;

			void* getOrigin() const;
			void* getDetour() const;
			void* getGateway() const;
		};

	}

}