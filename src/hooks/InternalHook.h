#pragma once
#include <Windows.h>

// Class to set up a simple function hook in the caller process.
// It installs a trampoline hook at the specified address of the origin function.
// Typically the detour function is defined in a dll that was injected into the process. This function has to call the gateway with the same calling convention
// as the hooked function for uninterrupted process execution.
// When the origin function gets called by the process (after enabling the hook) execution first jumps to the detour function.
// When the gateway gets called execution jumps to the gateway containing the overwritten bytes of the origin function and then jumps back to the origin function.
// The stolen bytes overwritten by the jump from the origin function may not contain any references to data or code with static addresses.
// The dll has to be compiled to the same architecture (x86 or x64) as the target process.
// The hook automatically uninstalls on desctuction of the installing object.

namespace hax {

	class InternalHook {
	private:
		BYTE* _origin;
		const BYTE* const _detour;
		BYTE* _gateway;
		const size_t _size;
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
		// It is necessary to look at the disassembly of the origin function to find when the first complete instruction finishes after the first five bytes.
		InternalHook(BYTE* origin, const BYTE* detour, size_t size);

		// Initializes members. Used to hook a function of a dll loaded by the target process by module and export name.
		// Hooks the beginning of the function, not the import address table, import directory or export directory!
		// 
		// Parameters:
		// 
		// [in] exportName:
		// Export name of the function to be hooked. The fucntion needs to be exported by a module loaded in the target process. At least the first five bytes will be overwritten.
		// 
		// [in] modName:
		// Name of the module that exports the function to be hooked.
		// 
		// [in] detour:
		// Address of the detour function within the virtual address space of the target process.
		// 
		// [in] size:
		// Number of bytes that get overwritten by the jump at the beginning of the origin function.
		// The overwritten instructions get executed by the gateway right before executing the origin function.
		// Has to be at least five! Only complete instructions should be overwritten!
		// It is necessary to look at the disassembly of the origin function to find out when the first complete instruction finishes after the first five bytes.
		InternalHook(const char* exportName, const char* modName, const BYTE* detour, size_t size);
		
		~InternalHook();

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
		
		BYTE* getGateway() const;
		BYTE* getOrigin() const;
	};

}