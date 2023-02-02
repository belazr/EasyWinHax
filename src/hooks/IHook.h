#pragma once
#include <Windows.h>

// Interface for hook classes.

namespace hax {

	class IHook {

		// Enables the hook. Execution of origin function is redirected after calling this method.
		// 
		// Return: True on success, false on failure
		virtual bool enable() = 0;

		// Disables the hook. Execution of origin function is restored after calling this method.
		// 
		// Return:
		// True on success, false on failure.
		virtual bool disable() = 0;

		// Checks if the hook is currently installed.
		// 
		// Return:
		// True if the hook is installed, false if it is not installed.
		virtual bool isHooked() const = 0;

		virtual BYTE* getOrigin() const = 0;
		virtual BYTE* getDetour() const = 0;
	};

}
