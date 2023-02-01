#include "ExternalHook.h"
#include "..\mem.h"
#include "..\proc.h"

namespace hax {
	
	ExternalHook::ExternalHook(HANDLE hProc, BYTE* origin, const BYTE* shell, size_t shellSize, const char* originCallPattern, size_t size) :
		_hProc(hProc), _origin(origin), _detour(nullptr), _detourOriginCall(nullptr), _size(size), _gateway(nullptr), _hooked(false)
	{
		_detour = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(shell), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		
		if (_detour) {
			
			if (WriteProcessMemory(hProc, _detour, shell, shellSize, nullptr)) {
				// scan for the origin call in the shell code injected in the target
				_detourOriginCall = mem::ex::findSigAddress(hProc, _detour, shellSize, originCallPattern);
			}

		}

	}


	ExternalHook::ExternalHook(
		HANDLE hProc, const char* funcName, const char* modName, const BYTE* shell, size_t shellSize, const char* originCallPattern, size_t size
	) : _hProc(hProc), _origin(nullptr), _detour(nullptr), _detourOriginCall(nullptr), _size(size), _gateway(nullptr), _hooked(false)
	{
		HMODULE hMod = proc::ex::getModuleHandle(hProc, modName);
		
		if (hMod) {
			this->_origin = reinterpret_cast<BYTE*>(proc::ex::getProcAddress(hProc, hMod, funcName));
		}

		_detour = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(shell), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		
		if (_detour) {
			
			if (WriteProcessMemory(hProc, _detour, shell, shellSize, nullptr)) {
				// scan for the origin call in the shell code injected in the target
				_detourOriginCall = mem::ex::findSigAddress(hProc, _detour, shellSize, originCallPattern);
			}

		}

	}


	ExternalHook::~ExternalHook() {
		
		if (this->_hooked) {
			this->disable();
		}

		VirtualFreeEx(_hProc, _detour, 0, MEM_RELEASE);
	}


	bool ExternalHook::enable() {
		
		if (this->_hooked || !this->_origin || !this->_detour || !this->_detourOriginCall) return false;

		// install the trampoline hook
		this->_gateway = mem::ex::trampHook(this->_hProc, this->_origin, this->_detour, this->_detourOriginCall, this->_size);
		
		if (!this->_gateway) return false;

		this->_hooked = true;
		
		return true;
	}


	bool ExternalHook::disable() {
		if (!this->_gateway) return false;

		// overwritten/stolen bytes
		BYTE* const stolen = new BYTE[this->_size]{};

		if (!stolen) return false;
		
		// read the stolen bytes from the gateway
		if (!ReadProcessMemory(this->_hProc, this->_gateway, stolen, this->_size, nullptr)) {
			delete[] stolen;

			return false;
		}

		if (this->_origin && this->_hooked) {
			
			// patch the overwritten bytes back to the origin function
			if (mem::ex::patch(this->_hProc, this->_origin, stolen, this->_size)) {
				this->_hooked = false;
				delete[] stolen;

				return VirtualFreeEx(_hProc, _gateway, 0, MEM_RELEASE);
			}

		}

		delete[] stolen;

		return false;
	}


	bool ExternalHook::isHooked() const {
		
		return this->_hooked;
	}


	BYTE* ExternalHook::getGateway() const {
		
		return this->_gateway;
	}


	BYTE* ExternalHook::getHookedAddress() const {
		
		return this->_origin;
	}


	BYTE* ExternalHook::getDetourAddress() const {
		
		return this->_detour;
	}

}