#pragma once
#include "mem.h"
#include "proc.h"
#include <stdint.h>

namespace hax {

	namespace mem {

		#ifdef _WIN64

		typedef struct AddressRange {
			uintptr_t start;
			uintptr_t min;
			uintptr_t max;
			uintptr_t pageSize;
		}AddressRange;

		// gets the range of address that is reachable by a relative jump
		static void getNearAddressRange(const void* pBase, AddressRange* pAddrRange);

		// ASM:
		// jmp QWORD PTR[rip + 0x0000000000000000]
		constexpr BYTE X64_JUMP[]{ 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

		#endif // _WIN64

		// ASM:
		// nop
		constexpr BYTE NOP = 0x90;

		// ASM:
		// jmp 0x00000000
		constexpr BYTE X86_JUMP[]{ 0xE9, 0x00, 0x00, 0x00, 0x00 };

		namespace ex {

			void* trampHook(HANDLE hProc, void* origin, void* detour, size_t originCallOffset, size_t size, size_t relativeAddressOffset) {

				if (relativeAddressOffset != SIZE_MAX && relativeAddressOffset + sizeof(uint32_t) > size)
				{
					return nullptr;
				}

				void* gateway = nullptr;
				size_t targetPtrSize = 0u;

				const proc::ex::ProcessArch arch = proc::ex::getProcessArch(hProc);

				if (arch == proc::ex::PROC_ARCH_UNKNOWN) return nullptr;

				if (arch == proc::ex::PROC_ARCH_X86) {
					// allocate enough memory for the relative jump (gateway to origin)
					// VirtualAllocEx can be used for x86 targets since in x86 every address is reachable by a relative jump and the relay is not neccessary
					gateway = VirtualAllocEx(hProc, nullptr, size + sizeof(X86_JUMP), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
					targetPtrSize = sizeof(uint32_t);
				}
				else {

					// if the target process is x64 and the function compiled to x86 nothing will be done and the function returns a nullptr
					#ifdef _WIN64

					// allocate enough memory for the relative jump (gateway to origin) and the absolute relay jump (relay to detour) near the origin (reachable by relative jump)
					gateway = virtualAllocNear(hProc, origin, size + sizeof(X86_JUMP) + sizeof(X64_JUMP));
					targetPtrSize = sizeof(uint64_t);

					#endif // _WIN64

				}

				if (!gateway) return nullptr;

				void* const pDetourOriginCall = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(detour) + originCallOffset);
				
				// overwrite the origin call placeholder
				if (!WriteProcessMemory(hProc, pDetourOriginCall, &gateway, targetPtrSize, nullptr)) {
					VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				// write the overwritten bytes of the origin to the gateway
				BYTE* stolen = new BYTE[size]{};

				if (!ReadProcessMemory(hProc, origin, stolen, size, nullptr)) {
					VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);
					delete[] stolen;

					return nullptr;
				}

				if (!WriteProcessMemory(hProc, gateway, stolen, size, nullptr)) {
					VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);
					delete[] stolen;

					return nullptr;
				}

				delete[] stolen;

				// correct the relative address
				if (relativeAddressOffset != SIZE_MAX) {

					const void* const pOriginRelativeAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(origin) + relativeAddressOffset);
					int32_t oldRelativeAddress = 0;

					if (!ReadProcessMemory(hProc, pOriginRelativeAddress, &oldRelativeAddress, sizeof(oldRelativeAddress), nullptr)) {
						VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

						return nullptr;
					}

					const ptrdiff_t correctedRelativeAddress = oldRelativeAddress + reinterpret_cast<uintptr_t>(origin) - reinterpret_cast<uintptr_t>(gateway);

					if (correctedRelativeAddress < INT32_MIN || correctedRelativeAddress > INT32_MAX) {
						VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

						return nullptr;
					}

					void* const pGatewayRelativeAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(gateway) + relativeAddressOffset);
					
					if (!WriteProcessMemory(hProc, pGatewayRelativeAddress, &correctedRelativeAddress, sizeof(uint32_t), nullptr)) {
						VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

						return nullptr;
					}

				}

				void* const pGatewayJump = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(gateway) + size);
				const void* const pOriginJumpDst = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(origin) + sizeof(X86_JUMP));

				// relative jump from the gateway to the origin
				if (!relJmp(hProc, pGatewayJump, pOriginJumpDst, sizeof(X86_JUMP))) {
					VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				if (arch == proc::ex::PROC_ARCH_X86) {

					// relative jump directly from origin to detour (will always be reachable in x86 targets)
					if (!relJmp(hProc, origin, detour, size)) {
						VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

						return nullptr;
					}

				}
				else {

					// only hook x64 process if compiled to x64
					#ifdef _WIN64

					// in x64 targets an absolute jump is needed to reliably jump from the origin to the detour
					// instead of patching the origin with a longer absolute jump, a relay is used that can be reached by a relative jump
					void* const relay = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(gateway) + size + sizeof(X86_JUMP));

					// absolute jump from the relay to the detour function
					if (!absJmpX64(hProc, relay, detour, sizeof(X64_JUMP))) {
						VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

						return nullptr;
					}

					// relative jump from the origin to the relay
					if (!relJmp(hProc, origin, relay, size)) {
						VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

						return nullptr;
					}

					#endif

				}

				return gateway;
			}


			#ifdef _WIN64

			void* virtualAllocNear(HANDLE hProc, const void* address, size_t size) {
				AddressRange range{};
				getNearAddressRange(address, &range);

				uintptr_t offset = range.pageSize;
				bool exhausted = false;
				BYTE* retAddress = nullptr;

				// try to allocte the memory at the beginning of every memory page in range until successful
				while (!exhausted) {
					const uintptr_t high = range.start + offset;
					const uintptr_t low = (range.start > offset) ? range.start - offset : 0u;

					if (high < range.max) {
						retAddress = static_cast<BYTE*>(VirtualAllocEx(hProc, reinterpret_cast<void*>(high), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

						if (retAddress) return retAddress;

					}

					if (low > range.min) {
						retAddress = static_cast<BYTE*>(VirtualAllocEx(hProc, reinterpret_cast<void*>(low), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

						if (retAddress) return retAddress;

					}

					offset += range.pageSize;
					exhausted = high > range.max && low < range.min;
				}

				return nullptr;
			}

			#endif // _WIN64


			bool relJmp(HANDLE hProc, void* origin, const void* detour, size_t size) {
				
				if (size < sizeof(X86_JUMP)) return false;
				
				const intptr_t offset = reinterpret_cast<intptr_t>(detour) - reinterpret_cast<intptr_t>(origin) - static_cast<intptr_t>(sizeof(X86_JUMP));

				#ifdef _WIN64

				// checks if detour is reachable from origin by a relative jump
				if (offset > INT32_MAX || offset < INT32_MIN) return false;

				#endif // _WIN64

				BYTE jump[sizeof(X86_JUMP)]{};

				if (memcpy(jump, X86_JUMP, sizeof(X86_JUMP))) return false;

				// offset is intptr_t (eight bytes on x64) only to validate the distance on x64, relative jump only takes four bytes as offset so sizeof(uint32_t) is fine here
				if (memcpy(jump + 0x1, &offset, sizeof(uint32_t))) return false;

				if (!patch(hProc, origin, jump, sizeof(jump))) return false;

				nop(hProc, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(origin) + sizeof(X86_JUMP)), size - sizeof(X86_JUMP));

				return true;
			}


			#ifdef _WIN64

			bool absJmpX64(HANDLE hProc, void* origin, const void* detour, size_t size) {

				if (size < sizeof(X64_JUMP)) return false;

				BYTE jump[sizeof(X64_JUMP)]{};

				if (memcpy(jump, X64_JUMP, sizeof(X64_JUMP))) return false;

				// copies the jump target to after the jmp QWORD PTR [rip+x] op code in the stack buffer
				if (memcpy(jump + 0x6, &detour, sizeof(uint64_t))) return false;

				if (!patch(hProc, origin, jump, sizeof(jump))) return false;

				nop(hProc, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(origin) + sizeof(X64_JUMP)), size - sizeof(X64_JUMP));

				return true;
			}

			#endif


			bool nop(HANDLE hProc, void* dst, size_t size) {
				BYTE* const nops = new BYTE[size]{};
				memset(nops, NOP, size);
				bool result = patch(hProc, dst, nops, size);
				delete[] nops;

				return result;
			}


			bool patch(HANDLE hProc, void* dst, const void* src, size_t size) {
				DWORD protect = 0ul;

				if (!VirtualProtectEx(hProc, dst, size, PAGE_EXECUTE_READWRITE, &protect)) return false;

				bool success = WriteProcessMemory(hProc, dst, src, size, nullptr);

				VirtualProtectEx(hProc, dst, size, protect, &protect);

				return success;
			}


			void* getVirtualFunction(HANDLE hProc, const void* pInterface, size_t index) {
				const void* const * pVTable = nullptr;

				if (!ReadProcessMemory(hProc, pInterface, &pVTable, sizeof(void**), 0u)) return nullptr;

				const void* const * const pFuncAddress = pVTable + index;

				if (!pFuncAddress) return nullptr;

				void* funcAddress = nullptr;

				if (!ReadProcessMemory(hProc, pFuncAddress, &funcAddress, sizeof(void*), 0u)) return nullptr;

				return funcAddress;
			}


			void* getMultiLevelPointer(HANDLE hProc, const void* base, const size_t* offsets, size_t size) {
				BYTE* address = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(base));

				for (size_t i = 0u; i < size; i++) {

					if (!ReadProcessMemory(hProc, address, &address, sizeof(BYTE*), 0u)) return nullptr;

					address += offsets[i];
				}

				return address;
			}


			void* findSigAddress(HANDLE hProc, const void* base, size_t size, const char* signature) {				
				// size of byte string signature of format "DE AD"
				const size_t sigSize = (strlen(signature) + 1u) / 3u;
				int* const sig = new int[sigSize] {};

				if (!helper::byteStringToInt(signature, sig, sigSize)) {
					delete[] sig;

					return nullptr;
				}

				void* address = nullptr;
				uintptr_t current = reinterpret_cast<uintptr_t>(base);
				const uintptr_t end = current + size;

				// scan each memory region at a time
				while (current < end) {
					MEMORY_BASIC_INFORMATION mbi{};
					// scan only if commited and accessable
					if (!VirtualQueryEx(hProc, reinterpret_cast<void*>(current), &mbi, sizeof(mbi)) || !mbi.RegionSize) break;

					current = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;

					if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS || mbi.Protect & PAGE_GUARD) continue;

					const uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
					const uintptr_t regionEnd = regionStart + mbi.RegionSize;
					const uintptr_t scanStart = max(regionStart, reinterpret_cast<uintptr_t>(base));
					const uintptr_t scanEnd = min(regionEnd, end);

					if (scanEnd <= scanStart || scanEnd - scanStart < sigSize) continue;

					const size_t scanSize = scanEnd - scanStart;

					// heap buffer to scan
					BYTE* const buffer = new BYTE[scanSize];

					if (!ReadProcessMemory(hProc, reinterpret_cast<void*>(scanStart), buffer, scanSize, nullptr)) {
						delete[] buffer;

						continue;
					}

					// address of found signature within heap buffer
					const void* const inBufferAddress = helper::findSignature(buffer, scanSize, sig, sigSize);

					if (inBufferAddress) {
						address = reinterpret_cast<void*>(scanStart + (reinterpret_cast<uintptr_t>(inBufferAddress) - reinterpret_cast<uintptr_t>(buffer)));

						delete[] buffer;

						break;
					}

					delete[] buffer;
				}

				delete[] sig;

				return address;
			}


			bool copyRemoteString(HANDLE hProc, char* dst, const void* src, size_t size) {

				for (size_t i = 0u; i < size; i++) {

					if (!ReadProcessMemory(hProc, reinterpret_cast<const BYTE*>(src) + i, &dst[i], sizeof(char), nullptr)) return false;

					// end of string
					if (dst[i] == '\0') return true;

				}

				// did not contain a null charater so something went wrong
				return false;
			}


			template <typename LE>
			bool unlinkListEntry(HANDLE hProc, LE listEntry) {

				if (!listEntry.Flink || !listEntry.Blink) {
					
					return false;
				}

				LE* pNext = reinterpret_cast<LE*>(static_cast<uintptr_t>(listEntry.Flink));

				if (!WriteProcessMemory(hProc, &pNext->Blink, &listEntry.Blink, sizeof(listEntry.Blink), nullptr)) return false;

				LE* pPrev = reinterpret_cast<LE*>(static_cast<uintptr_t>(listEntry.Blink));

				if (!WriteProcessMemory(hProc, &pPrev->Flink, &listEntry.Flink, sizeof(listEntry.Flink), nullptr)) return false;

				return true;
			}

		}


		namespace in {

			void* trampHook(void* origin, const void* detour, size_t size, size_t relativeAddressOffset) {

				if (relativeAddressOffset != SIZE_MAX && relativeAddressOffset + sizeof(uint32_t) > size)
				{
					return nullptr;
				}

				// allocate memory for the gateway
				#ifdef _WIN64

				// allocate enough memory for the relative jump (gateway to origin) and the absolute relay jump (relay to detour) near the origin (reachable by relative jump)
				void* const gateway = virtualAllocNear(origin, size + sizeof(X86_JUMP) + sizeof(X64_JUMP));

				#else

				// allocate enough memory for the relative jump (gateway to origin)
				// VirtualAllocEx can be used for x86 targets since in x86 every address is reachable by a relative jump and the relay is not neccessary
				void* const gateway = VirtualAlloc(nullptr, size + sizeof(X86_JUMP), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

				#endif

				if (!gateway) return nullptr;

				// write the overwritten bytes of the origin to the gateway
				if (memcpy(gateway, origin, size)) {
					VirtualFree(gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				// correct the relative address
				if (relativeAddressOffset != SIZE_MAX) {

					const int32_t oldRelativeAddress = *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(origin) + relativeAddressOffset);
					const int32_t correctedRelativeAddress = static_cast<int32_t>(oldRelativeAddress + reinterpret_cast<uintptr_t>(origin) - reinterpret_cast<uintptr_t>(gateway));

					if (correctedRelativeAddress < INT32_MIN || correctedRelativeAddress > INT32_MAX) {
						VirtualFree(gateway, 0, MEM_RELEASE);

						return nullptr;
					}

					*reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(gateway) + relativeAddressOffset) = correctedRelativeAddress;

				}

				void* const pGatewayJump = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(gateway) + size);
				const void* const pOriginJumpDst = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(origin) + sizeof(X86_JUMP));

				// relative jump from the gateway to the origin
				if (!relJmp(pGatewayJump, pOriginJumpDst, sizeof(X86_JUMP))) {
					VirtualFree(gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				#ifdef _WIN64

				// in x64 targets an absolute jump is needed to reliably jump from the origin to the detour
				// instead of patching the origin with a longer absolute jump, a relay is used that can be reached by a relative jump
				void* const relay = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(gateway) + size + sizeof(X86_JUMP));

				// absolute jump from the relay to the detour function
				if (!absJmpX64(relay, detour, sizeof(X64_JUMP))) {
					VirtualFree(gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				// relative jump from the origin to the relay
				if (!relJmp(origin, relay, size)) {
					VirtualFree(gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				#else

				// relative jump directly from origin to detour (will always be reachable in x86 targets)
				if (!relJmp(origin, detour, size)) {
					VirtualFree(gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				#endif

				return gateway;
			}


			#ifdef _WIN64

			void* virtualAllocNear(const void* address, size_t size) {
				AddressRange range{};
				getNearAddressRange(address, &range);

				uintptr_t offset = range.pageSize;
				bool exhausted = false;
				BYTE* retAddress = nullptr;

				// try to allocte the memory at the beginning of every memory page in range until successful
				while (!exhausted) {
					const uintptr_t high = range.start + offset;
					const uintptr_t low = (range.start > offset) ? range.start - offset : 0u;

					if (high < range.max) {
						retAddress = static_cast<BYTE*>(VirtualAlloc(reinterpret_cast<void*>(high), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

						if (retAddress) return retAddress;

					}

					if (low > range.min) {
						retAddress = static_cast<BYTE*>(VirtualAlloc(reinterpret_cast<void*>(low), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

						if (retAddress) return retAddress;

					}

					offset += range.pageSize;
					exhausted = high > range.max && low < range.min;
				}

				return nullptr;
			}

			#endif // _WIN64


			bool relJmp(void* origin, const void* detour, size_t size) {

				if (size < sizeof(X86_JUMP)) return false;

				const intptr_t offset = reinterpret_cast<intptr_t>(detour) - reinterpret_cast<intptr_t>(origin) - static_cast<intptr_t>(sizeof(X86_JUMP));

				#ifdef _WIN64

				// checks if detour is reachable from origin by a relative jump
				if (offset > INT32_MAX || offset < INT32_MIN) return false;

				#endif // _WIN64

				BYTE jump[sizeof(X86_JUMP)]{};

				if (memcpy(jump, X86_JUMP, sizeof(X86_JUMP))) return false;

				// offset is intptr_t (eight bytes on x64) only to validate the distance on x64, relative jump only takes four bytes as offset so sizeof(uint32_t) is fine here
				if (memcpy(jump + 0x1, &offset, sizeof(uint32_t))) return false;

				if (!patch(origin, jump, sizeof(jump))) return false;

				nop(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(origin) + sizeof(X86_JUMP)), size - sizeof(X86_JUMP));

				return true;
			}


			#ifdef _WIN64

			bool absJmpX64(void* origin, const void* detour, size_t size) {
				
				if (size < sizeof(X64_JUMP)) return false;

				BYTE jump[sizeof(X64_JUMP)]{};

				if (memcpy(jump, X64_JUMP, sizeof(X64_JUMP))) return false;

				// copies the jump target to after the jmp QWORD PTR [rip+x] op code in the stack buffer
				if (memcpy(jump + 0x6, &detour, sizeof(uint64_t))) return false;

				if (!patch(origin, jump, sizeof(jump))) return false;

				nop(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(origin) + sizeof(X64_JUMP)), size - sizeof(X64_JUMP));

				return true;
			}

			#endif


			bool nop(void* dst, size_t size) {
				BYTE* const nops = new BYTE[size]{};
				memset(nops, NOP, size);
				bool result = patch(dst, nops, size);
				delete[] nops;

				return result;
			}


			bool patch(void* dst, const void* src, size_t size) {
				DWORD protect = 0ul;

				if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &protect)) return false;

				bool success = memcpy(dst, src, size) == 0;

				VirtualProtect(dst, size, protect, &protect);

				return success;
			}


			void* getVirtualFunction(const void* pInterface, size_t index) {
				void* const* const pVTable = *reinterpret_cast<void***>(const_cast<void*>(pInterface));

				void* const* const pFuncAddress = pVTable + index;

				if (!pFuncAddress) return nullptr;

				return *(pFuncAddress);
			}


			void* getMultiLevelPointer(const void* base, const size_t* offsets, size_t size) {
				BYTE* address = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(base));

				for (size_t i = 0u; i < size; i++) {
					address = *reinterpret_cast<BYTE**>(address);
					address += offsets[i];
				}

				return address;
			}


			void* findSigAddress(const void* base, size_t size, const char* signature) {
				// size of byte string signature of format "DE AD"
				const size_t sigSize = (strlen(signature) + 1) / 3;
				int* const sig = new int[sigSize] {};

				if (!helper::byteStringToInt(signature, sig, sigSize)) {
					delete[] sig;

					return nullptr;
				}

				void* address = nullptr;
				uintptr_t current = reinterpret_cast<uintptr_t>(base);
				const uintptr_t end = current + size;

				// scan each memory region at a time
				while (current < end) {
					MEMORY_BASIC_INFORMATION mbi{};
					// scan only if commited and accessable
					if (!VirtualQuery(reinterpret_cast<void*>(current), &mbi, sizeof(mbi)) || !mbi.RegionSize) {
						delete[] sig;

						return nullptr;
					}

					current = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;

					if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS || mbi.Protect & PAGE_GUARD) continue;

					const uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
					const uintptr_t regionEnd = regionStart + mbi.RegionSize;
					const uintptr_t scanStart = max(regionStart, reinterpret_cast<uintptr_t>(base));
					const uintptr_t scanEnd = min(regionEnd, end);

					if (scanEnd <= scanStart || scanEnd - scanStart < sigSize) continue;

					const size_t scanSize = scanEnd - scanStart;

					// address of found signature within heap buffer
					address = helper::findSignature(reinterpret_cast<void*>(scanStart), scanSize, sig, sigSize);

					if (address) break;

				}

				delete[] sig;

				return address;
			}


			template <typename LE>
			bool unlinkListEntry(LE listEntry) {

				if (!listEntry.Flink || !listEntry.Blink) {
					
					return false;
				}

				LE* pNext = reinterpret_cast<LE*>(static_cast<uintptr_t>(listEntry.Flink));
				pNext->Blink = listEntry.Blink;

				LE* pPrev = reinterpret_cast<LE*>(static_cast<uintptr_t>(listEntry.Blink));
				pPrev->Flink = listEntry.Flink;

				return true;
			}

		}


		#ifdef _WIN64

		static void getNearAddressRange(const void* pBase, AddressRange* pAddrRange) {
			SYSTEM_INFO sysInfo{};
			GetSystemInfo(&sysInfo);

			pAddrRange->pageSize = sysInfo.dwPageSize;
			// start at the beginning of the page where pBase is located
			pAddrRange->start = reinterpret_cast<uintptr_t>(pBase) - (reinterpret_cast<uintptr_t>(pBase) % pAddrRange->pageSize);

			const uintptr_t range = INT32_MAX;
			// checks int underflow
			const uintptr_t minAddress = (pAddrRange->start > range) ? pAddrRange->start - range : 0u;
			// checks int overflow
			const uintptr_t maxAddress = (UINTPTR_MAX - pAddrRange->start > range) ? pAddrRange->start + range : UINTPTR_MAX;

			pAddrRange->min = max(minAddress, reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress));
			pAddrRange->max = min(maxAddress, reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress));
		}

		#endif // _WIN64		


		namespace helper {

			bool byteStringToInt(const char* charSig, int* intSig, size_t size) {
				const size_t charSigLen = strlen(charSig);

				// checks for format "DE AD" by character count and asserts the correct size
				if (!((charSigLen + 1) % 3 == 0) || ((charSigLen + 1) / 3 != size)) return false;

				const char* cur = charSig;

				for (size_t i = 0u; i < size; i++) {

					if (*cur == '?') {

						if (*(cur + 1) != '?') return false;

						intSig[i] = -1;
						cur += 3;
					}
					else {
						const char* prev = cur;
						intSig[i] = strtoul(cur, const_cast<char**>(&cur), 16);

						if (cur - prev != 2) return false;

						cur++;
					}

				}

				return true;
			}


			bool intToByteString(const int* intSig, size_t count, char* charSig, size_t size)
			{
				if (count == 0 || size < count * 3) return false;

				for (size_t i = 0u; i < count; i++) {

					if (intSig[i] < - 1 || intSig[i] >  255) {

						return false;
					}
					else if (intSig[i] == -1) {
						charSig[i * 3] = '?';
						charSig[i * 3 + 1] = '?';
						charSig[i * 3 + 2] = ' ';
					}
					else if (intSig[i] < 16) {
						char tmp[2];
						_itoa_s(intSig[i], tmp, 16);
						charSig[i * 3] = '0';
						charSig[i * 3 + 1] = static_cast<char>(toupper(tmp[0]));
						charSig[i * 3 + 2] = ' ';
					}
					else {
						char tmp[3];
						_itoa_s(intSig[i], tmp, 16);
						charSig[i * 3] = static_cast<char>(toupper(tmp[0]));
						charSig[i * 3 + 1] = static_cast<char>(toupper(tmp[1]));
						charSig[i * 3 + 2] = ' ';
					}

					if (i == count - 1)
					{
						charSig[i * 3 + 2] = '\0';
					}

				}
				
				return true;
			}


			void* findSignature(const void* base, size_t size, const int* signature, size_t sigSize) {
				
				if (size < sigSize) return nullptr;

				for (size_t i = 0u; i <= size - sigSize; i++) {
					bool found = true;

					for (size_t j = 0u; j < sigSize; j++) {

						// -1 acts as wildcard
						if (reinterpret_cast<const BYTE*>(base)[i + j] != signature[j] && signature[j] != -1) {
							found = false;
							break;
						}

					}

					if (found) {
						
						return const_cast<BYTE*>(reinterpret_cast<const BYTE*>(base) + i);
					}

				}

				return nullptr;
			}

		}

	}

}