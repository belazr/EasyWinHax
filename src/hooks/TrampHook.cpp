#include "TrampHook.h"
#include "..\mem.h"
#include "..\proc.h"

namespace hax {

	namespace ex {

		TrampHook::TrampHook(HANDLE hProc, BYTE* origin, const BYTE* shell, size_t shellSize, const char* originCallPattern, size_t size) :
			_hProc(hProc), _origin(origin), _size(size), _detour{}, _detourOriginCall{}, _gateway{}, _hooked{}
		{
			this->_detour = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(shell), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

			if (this->_detour && shell) {

				if (WriteProcessMemory(hProc, this->_detour, shell, shellSize, nullptr)) {
					// scan for the origin call in the shell code injected into the target
					_detourOriginCall = mem::ex::findSigAddress(hProc, this->_detour, shellSize, originCallPattern);
				}

			}

		}


		TrampHook::TrampHook(
			HANDLE hProc, const char* modName, const char* funcName, const BYTE* shell, size_t shellSize, const char* originCallPattern, size_t size
		) : _hProc(hProc), _size(size), _origin{}, _detour{}, _detourOriginCall{}, _gateway{}, _hooked{}
		{
			HMODULE hMod = proc::ex::getModuleHandle(hProc, modName);

			if (hMod) {
				this->_origin = reinterpret_cast<BYTE*>(proc::ex::getProcAddress(hProc, hMod, funcName));
			}

			this->_detour = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, shellSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

			if (this->_detour && shell) {

				if (WriteProcessMemory(this->_hProc, this->_detour, shell, shellSize, nullptr)) {
					// scan for the origin call in the shell code injected into the target
					this->_detourOriginCall = mem::ex::findSigAddress(hProc, this->_detour, shellSize, originCallPattern);
				}

			}

		}


		TrampHook::~TrampHook() {
			this->disable();

			if (this->_detour) {
				VirtualFreeEx(this->_hProc, this->_detour, 0, MEM_RELEASE);
			}
			
		}


		bool TrampHook::enable() {

			if (this->_hooked || !this->_origin || !this->_detour || !this->_detourOriginCall) return false;

			// install the trampoline hook
			this->_gateway = mem::ex::trampHook(this->_hProc, this->_origin, this->_detour, this->_detourOriginCall, this->_size);

			if (!this->_gateway) return false;

			this->_hooked = true;

			return true;
		}


		bool TrampHook::disable() {
			if (!this->_hooked || !this->_origin || !this->_gateway) return false;

			// overwritten/stolen bytes
			BYTE* const stolen = new BYTE[this->_size]{};

			// read the stolen bytes from the gateway
			if (!ReadProcessMemory(this->_hProc, this->_gateway, stolen, this->_size, nullptr)) {
				delete[] stolen;

				return false;
			}

			// patch the stolen bytes back
			if (!mem::ex::patch(this->_hProc, this->_origin, stolen, this->_size)) {
				delete[] stolen;

				return false;
			}

			this->_hooked = false;
			delete[] stolen;

			return VirtualFreeEx(this->_hProc, this->_gateway, 0, MEM_RELEASE);
		}


		bool TrampHook::isHooked() const {

			return this->_hooked;
		}


		BYTE* TrampHook::getOrigin() const {

			return this->_origin;
		}


		BYTE* TrampHook::getDetour() const {

			return this->_detour;
		}


		BYTE* TrampHook::getGateway() const {

			return this->_gateway;
		}

	}


	namespace in {

		TrampHook::TrampHook(BYTE* origin, const BYTE* detour, size_t size, size_t relativeAddressOffset) :
			_origin(origin), _detour(detour), _size(size), _gateway(nullptr), _hooked(false), _relativeAddressOffset(relativeAddressOffset), _relativeAddress(0) {
			const int32_t* const pRelativeAddress = reinterpret_cast<int32_t*>(this->_origin + this->_relativeAddressOffset);

			if (this->_relativeAddressOffset != SIZE_MAX && pRelativeAddress) {
				this->_relativeAddress = *pRelativeAddress;
			}
		
		}


		TrampHook::TrampHook(const char* modName, const char* funcName, const BYTE* detour, size_t size, size_t relativeAddressOffset) :
			_origin(nullptr), _detour(detour), _size(size), _gateway(nullptr), _hooked(false), _relativeAddressOffset(relativeAddressOffset), _relativeAddress(0)
		{
			const int32_t* const pRelativeAddress = reinterpret_cast<int32_t*>(this->_origin + this->_relativeAddressOffset);
			
			if (this->_relativeAddressOffset != SIZE_MAX && pRelativeAddress) {
				this->_relativeAddress = *pRelativeAddress;
			}
			
			const HMODULE hMod = proc::in::getModuleHandle(modName);

			if (hMod) {
				this->_origin = reinterpret_cast<BYTE*>(proc::in::getProcAddress(hMod, funcName));
			}

		}


		TrampHook::~TrampHook() {
			this->disable();
		}


		bool TrampHook::enable() {

			if (this->_hooked || !this->_origin || !this->_detour) return false;

			this->_gateway = mem::in::trampHook(this->_origin, this->_detour, this->_size, this->_relativeAddressOffset);

			if (!this->_gateway) return false;

			this->_hooked = true;

			return true;
		}


		bool TrampHook::disable() {

			if (!this->_hooked || !this->_origin || !this->_gateway) return false;

			// patch the stolen bytes back
			if (!mem::in::patch(this->_origin, this->_gateway, this->_size)) return false;

			// patch the saved relative address back
			if (this->_relativeAddress) {

				if (!mem::in::patch(this->_origin + this->_relativeAddressOffset, reinterpret_cast<BYTE*>(&this->_relativeAddress), sizeof(this->_relativeAddress))) return false;

			}
					
			this->_hooked = false;

			return VirtualFree(this->_gateway, 0, MEM_RELEASE);
		}


		bool TrampHook::isHooked() const {

			return this->_hooked;
		}



		BYTE* TrampHook::getOrigin() const {

			return this->_origin;
		}


		BYTE* TrampHook::getDetour() const {

			return const_cast<BYTE*>(this->_detour);
		}


		BYTE* TrampHook::getGateway() const {

			return this->_gateway;
		}

	}
	
}