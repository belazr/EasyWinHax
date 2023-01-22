#pragma once
#include "InternalHook.h"
#include "..\mem.h"
#include "..\proc.h"

namespace hax {
	
	InternalHook::InternalHook(BYTE* origin, const BYTE* detour, size_t size):
		_origin(origin), _detour(detour), _size(size), _gateway(nullptr), _hooked(false) {}

	
	InternalHook::InternalHook(const char* exportName, const char* modName, const BYTE* detour, size_t size):
		_origin(nullptr), _detour(detour), _size(size), _gateway(nullptr), _hooked(false)
	{
		const HMODULE hMod = proc::in::getModuleHandle(modName);
		
		if (hMod) {
			this->_origin = reinterpret_cast<BYTE*>(proc::in::getProcAddress(hMod, exportName));
		}

	}


	InternalHook::~InternalHook() {
		
		if (this->_hooked) {
			this->disable();
		}

	}


	bool InternalHook::enable() {
		
		if (this->_hooked || !this->_origin || !this->_detour) return false;

		this->_gateway = mem::in::trampHook(this->_origin, this->_detour, this->_size);
		
		if (!this->_gateway) return false;

		this->_hooked = true;

		return true;
	}


	bool InternalHook::disable() {
		
		if (!this->_gateway) return false;

		if (this->_origin && this->_hooked) {

			if (mem::in::patch(this->_origin, this->_gateway, this->_size)) {
				this->_hooked = false;

				return VirtualFree(_gateway, 0, MEM_RELEASE);

			}

		}

		return false;
	}


	bool InternalHook::isHooked() const {
		
		return this->_hooked;
	}


	BYTE* InternalHook::getGateway() const {
		
		return this->_gateway;
	}


	BYTE* InternalHook::getOrigin() const {
		
		return this->_origin;
	}

}