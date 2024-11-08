#include "IatHook.h"
#include "..\mem.h"
#include "..\proc.h"
#include <stdint.h>

namespace hax {

	namespace ex {

		IatHook::IatHook(HANDLE hProc, HMODULE hImportMod, const char* exportModName, const char* funcName, const BYTE* shell, size_t shellSize, const char* originCallPattern) :
			_hProc{ hProc }, _origin{}, _detour{}, _pIatEntry{}, _hooked{}, _isWow64Proc{}
		{
			IsWow64Process(this->_hProc, &this->_isWow64Proc);
			this->_pIatEntry = proc::ex::getIatEntryAddress(this->_hProc, hImportMod, exportModName, funcName);

			if (this->_pIatEntry) {

				if (this->_isWow64Proc) {
					// saves original IAT entry
					ReadProcessMemory(this->_hProc, this->_pIatEntry, &this->_origin, sizeof(uint32_t), nullptr);
				}
				else {

					#ifdef _WIN64

					// saves original IAT entry
					ReadProcessMemory(this->_hProc, this->_pIatEntry, &this->_origin, sizeof(uint64_t), nullptr);
					
					#endif // _WIN64

				}

			}

			if (originCallPattern) {
				// scan for the origin call in the shell code
				void* const shellOriginCall = mem::in::findSigAddress(shell, shellSize, originCallPattern);

				if (shellOriginCall) {
					
					if (this->_isWow64Proc) {
						memcpy_s(shellOriginCall, sizeof(uint32_t), &this->_origin, sizeof(uint32_t));
					}
					else {

						#ifdef _WIN64

						memcpy_s(shellOriginCall, sizeof(uint64_t), &this->_origin, sizeof(uint64_t));

						#endif // _WIN64

					}
				
				}

			}

			this->_detour = static_cast<BYTE*>(VirtualAllocEx(this->_hProc, nullptr, shellSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

			if (this->_detour && shell) {
				WriteProcessMemory(hProc, this->_detour, shell, shellSize, nullptr);
			}

		}


		IatHook::~IatHook() {
			this->disable();

			if (this->_detour) {
				VirtualFreeEx(this->_hProc, this->_detour, 0, MEM_RELEASE);
			}

		}


		bool IatHook::enable() {

			if (this->_hooked || !this->_pIatEntry || !this->_detour) return false;

			if (this->_isWow64Proc) {
				
				// overwrites the IAT entry in the process
				if (!mem::ex::patch(this->_hProc, this->_pIatEntry, reinterpret_cast<const BYTE*>(&this->_detour), sizeof(uint32_t))) return false;

			}
			else {

				// x64 target is only supported for x64 compilation
				#ifdef _WIN64

				// overwrites the IAT entry in the process
				if (!mem::ex::patch(this->_hProc, this->_pIatEntry, reinterpret_cast<const BYTE*>(&this->_detour), sizeof(uint64_t))) return false;
				
				#else
				
				return false;
				
				#endif // _WIN64

			}
			
			this->_hooked = true;

			return true;
		}


		bool IatHook::disable() {

			if (!this->_hooked || !this->_pIatEntry || !this->_origin) return false;

			if (this->_isWow64Proc) {
				
				// restores the IAT entry in the process
				if (!mem::ex::patch(this->_hProc, this->_pIatEntry, reinterpret_cast<const BYTE*>(&this->_origin), sizeof(uint32_t))) return false;

			}
			else {

				// x64 traget is only supported for x64 compilation of this class
				#ifdef _WIN64

				// restores the IAT entry in the process
				if (!mem::ex::patch(this->_hProc, this->_pIatEntry, reinterpret_cast<const BYTE*>(&this->_origin), sizeof(uint64_t))) return false;
				
				#else

				return false;

				#endif // _WIN64

			}

			this->_hooked = false;

			return true;
		}


		bool IatHook::isHooked() const {

			return this->_hooked;
		}


		void* IatHook::getOrigin() const {

			return const_cast<void*>(this->_origin);
		}


		void* IatHook::getDetour() const {

			return this->_detour;
		}


		void* IatHook::getIatEntryAddress() const {

			return this->_pIatEntry;
		}

	}


	namespace in {

		IatHook::IatHook(HMODULE hImportMod, const char* exportModName, const char* funcName, const BYTE* detour) :
			_detour{ detour }, _origin{}, _pIatEntry{}, _hooked{}
		{
			this->_pIatEntry = proc::in::getIatEntryAddress(hImportMod, exportModName, funcName);

			if (this->_pIatEntry) {
				// saves original IAT entry
				this->_origin = *reinterpret_cast<BYTE**>(this->_pIatEntry);
			}

		}


		IatHook::~IatHook() {
			this->disable();
		}


		bool IatHook::enable() {

			if (this->_hooked || !this->_pIatEntry || !this->_detour) return false;

			// overwrites the IAT entry in the process
			if (!mem::in::patch(this->_pIatEntry, reinterpret_cast<const BYTE*>(&this->_detour), sizeof(BYTE*))) return false;

			this->_hooked = true;

			return true;
		}


		bool IatHook::disable() {

			if (!this->_hooked || !this->_pIatEntry || !this->_origin) return false;

			// restores the IAT entry in the process
			if (!mem::in::patch(this->_pIatEntry, reinterpret_cast<const BYTE*>(&this->_origin), sizeof(BYTE*))) return false;

			this->_hooked = false;

			return true;
		}


		bool IatHook::isHooked() const {

			return this->_hooked;
		}


		void* IatHook::getOrigin() const {

			return const_cast<void*>(this->_origin);
		}


		void* IatHook::getDetour() const {

			return const_cast<void*>(this->_detour);
		}


		void* IatHook::getIatEntryAddress() const {

			return this->_pIatEntry;
		}

	}

}