#pragma once
#include "mem.h"
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
		static void getNearAddressRange(const BYTE* pBase, AddressRange* pAddrRange);

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

			BYTE* trampHook(HANDLE hProc, BYTE* origin, const BYTE* detour, BYTE* originCall, size_t size) {
				BYTE* gateway = nullptr;
				size_t targetPtrSize = 0;

				BOOL isWow64 = false;
				IsWow64Process(hProc, &isWow64);

				if (isWow64) {
					// allocate enough memory for the relative jump (gateway to origin)
					// VirtualAllocEx can be used for x86 targets since in x86 every address is reachable by a relative jump and the relay is not neccessary
					gateway = static_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, size + sizeof(X86_JUMP), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
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

				if (originCall) {

					// overwrite the origin call placeholder
					if (!WriteProcessMemory(hProc, originCall, &gateway, targetPtrSize, nullptr)) {
						VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

						return nullptr;
					}

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

				// relative jump from the gateway to the origin
				if (!relJmp(hProc, gateway + size, origin + sizeof(X86_JUMP), sizeof(X86_JUMP))) {
					VirtualFreeEx(hProc, gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				if (isWow64) {

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
					BYTE* const relay = gateway + size + sizeof(X86_JUMP);

					// absolute jump from the relay to the detour function
					if (!absJumpX64(hProc, relay, detour, sizeof(X64_JUMP))) {
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

			BYTE* virtualAllocNear(HANDLE hProc, const BYTE* address, size_t size) {
				AddressRange range{};
				getNearAddressRange(address, &range);

				uintptr_t offset = range.pageSize;
				bool exhausted = false;
				BYTE* retAddress = nullptr;

				// try to allocte the memory at the beginning of every memory page in range until successful
				while (!exhausted) {
					const uintptr_t high = range.start + offset;
					const uintptr_t low = (range.start > offset) ? range.start - offset : 0;

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


			BYTE* relJmp(HANDLE hProc, BYTE* origin, const BYTE* detour, size_t size) {
				#ifdef _WIN64

				const uint64_t distance = abs(detour - origin);

				// checks if detour is reachable from origin by a relative jump
				if (distance != (distance & UINT32_MAX)) return nullptr;

				#endif // _WIN64

				if (size < sizeof(X86_JUMP)) return nullptr;

				if (!nop(hProc, origin, size)) return nullptr;

				BYTE jump[sizeof(X86_JUMP)]{};

				if (memcpy_s(jump, sizeof(jump), X86_JUMP, sizeof(X86_JUMP))) return nullptr;
				
				const uint32_t offset = static_cast<uint32_t>(detour - origin - sizeof(jump));

				// copies the jump offset to after the relative jump op code in the stack buffer
				if (memcpy_s(jump + 0x1, sizeof(uint32_t), &offset, sizeof(uint32_t))) return nullptr;

				if (!patch(hProc, origin, jump, sizeof(jump))) return nullptr;

				return origin + size;
			}


			#ifdef _WIN64

			BYTE* absJumpX64(HANDLE hProc, BYTE* origin, const BYTE* detour, size_t size) {
				if (size < sizeof(X64_JUMP)) return nullptr;

				if (!nop(hProc, origin, size)) return nullptr;

				BYTE jump[sizeof(X64_JUMP)]{};

				if (memcpy_s(jump, sizeof(jump), X64_JUMP, sizeof(X64_JUMP))) return nullptr;

				// copies the jump offset to after the jmp QWORD PTR [rip+x] op code in the stack buffer
				if (memcpy_s(jump + 0x6, sizeof(uint64_t), &detour, sizeof(uint64_t))) return nullptr;

				if (!patch(hProc, origin, jump, sizeof(jump))) return nullptr;

				return origin + size;
			}

			#endif


			bool nop(HANDLE hProc, BYTE* dst, size_t size) {
				BYTE* const nops = new BYTE[size]{};
				memset(nops, NOP, size);
				bool result = patch(hProc, dst, nops, size);
				delete[] nops;

				return result;
			}


			bool patch(HANDLE hProc, BYTE* dst, const BYTE src[], size_t size) {
				DWORD protect = 0ul;

				if (!VirtualProtectEx(hProc, dst, size, PAGE_EXECUTE_READWRITE, &protect)) return false;

				if (!WriteProcessMemory(hProc, dst, src, size, nullptr)) return false;

				if (!VirtualProtectEx(hProc, dst, size, protect, &protect)) return false;

				return true;
			}


			BYTE* getMultiLevelPointer(HANDLE hProc, const BYTE* base, const size_t offsets[], size_t size) {
				BYTE* address = const_cast<BYTE*>(base);

				for (size_t i = 0; i < size; i++) {

					if (!ReadProcessMemory(hProc, address, &address, sizeof(BYTE*), 0)) return nullptr;

					address += offsets[i];
				}

				return address;
			}


			BYTE* findSigAddress(HANDLE hProc, const BYTE* base, size_t size, const char* signature) {
				// size of byte string signature of format "DE AD"
				const size_t sigSize = (strlen(signature) + 1) / 3;
				int* const sig = new int[sigSize] {};

				if (!helper::bytestringToInt(signature, sig, sigSize)) {
					delete[] sig;

					return nullptr;
				}

				BYTE* address = nullptr;
				MEMORY_BASIC_INFORMATION mbi{};

				// scan each memory region at a time
				for (size_t i = 0; i < size; i += mbi.RegionSize) {

					// scan only if commited and accessable
					if (!VirtualQueryEx(hProc, &base[i], &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) continue;

					DWORD oldProtect = 0ul;

					if (!VirtualProtectEx(hProc, mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect)) continue;

					// heap buffer to scan
					BYTE* const buffer = new BYTE[mbi.RegionSize];

					if (!ReadProcessMemory(hProc, mbi.BaseAddress, buffer, mbi.RegionSize, nullptr)) {
						delete[] buffer;

						continue;
					}

					if (!VirtualProtectEx(hProc, mbi.BaseAddress, mbi.RegionSize, oldProtect, &oldProtect)) {
						delete[] buffer;

						continue;
					}

					// address of found signature within heap buffer
					const BYTE* const inBufferAddress = helper::findSignature(buffer, mbi.RegionSize, sig, sigSize);

					if (inBufferAddress) {
						address = const_cast<BYTE*>(&base[i]) + (inBufferAddress - buffer);
					}

					delete[] buffer;
				}

				delete[] sig;

				return address;
			}


			bool copyRemoteString(HANDLE hProc, char* dst, const BYTE* src, size_t size) {

				for (size_t i = 0; i < size; i++) {

					if (!ReadProcessMemory(hProc, &src[i], &dst[i], sizeof(char), nullptr)) return false;

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

				LE* flink = reinterpret_cast<LE*>(static_cast<uintptr_t>(listEntry.Flink));

				if (!WriteProcessMemory(hProc, &flink->Blink, &listEntry.Blink, sizeof(listEntry.Blink), nullptr)) return false;

				LE* blink = reinterpret_cast<LE*>(static_cast<uintptr_t>(listEntry.Blink));

				if (!WriteProcessMemory(hProc, &blink->Flink, &listEntry.Flink, sizeof(listEntry.Flink), nullptr)) return false;

				return true;
			}

		}


		namespace in {

			BYTE* trampHook(BYTE* origin, const BYTE* detour, size_t size) {

				// allocate memory for the gateway
				#ifdef _WIN64

				// allocate enough memory for the relative jump (gateway to origin) and the absolute relay jump (relay to detour) near the origin (reachable by relative jump)
				BYTE* const gateway = virtualAllocNear(origin, size + sizeof(X86_JUMP) + sizeof(X64_JUMP));

				#else

				// allocate enough memory for the relative jump (gateway to origin)
				// VirtualAllocEx can be used for x86 targets since in x86 every address is reachable by a relative jump and the relay is not neccessary
				BYTE* gateway = static_cast<BYTE*>(VirtualAlloc(nullptr, size + sizeof(X86_JUMP), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

				#endif

				if (!gateway) return nullptr;

				// write the overwritten bytes of the origin to the gateway
				if (memcpy_s(gateway, size, origin, size)) {
					VirtualFree(gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				// relative jump from the gateway to the origin
				if (!relJmp(gateway + size, origin + sizeof(X86_JUMP), sizeof(X86_JUMP))) {
					VirtualFree(gateway, 0, MEM_RELEASE);

					return nullptr;
				}

				#ifdef _WIN64

				// in x64 targets an absolute jump is needed to reliably jump from the origin to the detour
				// instead of patching the origin with a longer absolute jump, a relay is used that can be reached by a relative jump
				BYTE* const relay = gateway + size + sizeof(X86_JUMP);

				// absolute jump from the relay to the detour function
				if (!absJumpX64(relay, detour, sizeof(X64_JUMP))) {
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

			BYTE* virtualAllocNear(const BYTE* address, size_t size) {
				AddressRange range{};
				getNearAddressRange(address, &range);

				uintptr_t offset = range.pageSize;
				bool exhausted = false;
				BYTE* retAddress = nullptr;

				// try to allocte the memory at the beginning of every memory page in range until successful
				while (!exhausted) {
					const uintptr_t high = range.start + offset;
					const uintptr_t low = (range.start > offset) ? range.start - offset : 0;

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


			BYTE* relJmp(BYTE* origin, const BYTE* detour, size_t size) {
				#ifdef _WIN64

				const uint64_t distance = abs(detour - origin);

				// checks if detour is reachable from origin by a relative jump
				if (distance != (distance & UINT32_MAX)) return nullptr;

				#endif // _WIN64

				if (size < sizeof(X86_JUMP)) return nullptr;

				if (!nop(origin, size)) return nullptr;

				BYTE jump[sizeof(X86_JUMP)]{};

				if (memcpy_s(jump, sizeof(jump), X86_JUMP, sizeof(X86_JUMP))) return nullptr;

				const uint32_t offset = static_cast<uint32_t>(detour - origin - sizeof(jump));

				// copies the jump offset after the relative jump op code in the stack buffer
				if (memcpy_s(jump + 0x1, sizeof(uint32_t), &offset, sizeof(uint32_t))) return nullptr;

				if (!patch(origin, jump, sizeof(jump))) return nullptr;

				return origin + size;
			}


			#ifdef _WIN64

			BYTE* absJumpX64(BYTE* origin, const BYTE* detour, size_t size) {
				if (size < sizeof(X64_JUMP)) return nullptr;

				if (!nop(origin, size)) return nullptr;

				BYTE jump[sizeof(X64_JUMP)]{};

				if (memcpy_s(jump, sizeof(jump), X64_JUMP, sizeof(X64_JUMP))) return nullptr;

				// copies the jump offset to after the jmp QWORD PTR [rip+x] op code in the stack buffer
				if (memcpy_s(jump + 0x6, sizeof(uint64_t), &detour, sizeof(uint64_t))) return nullptr;

				if (!patch(origin, jump, sizeof(jump))) return nullptr;

				return origin + size;
			}

			#endif


			bool patch(BYTE* dst, const BYTE src[], size_t size) {
				DWORD protect = 0ul;

				if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &protect)) return false;

				if (memcpy_s(dst, size, src, size)) return false;

				if (!VirtualProtect(dst, size, protect, &protect)) return false;

				return true;
			}


			bool nop(BYTE* dst, size_t size) {
				BYTE* const nops = new BYTE[size]{};
				memset(nops, NOP, size);
				bool result = patch(dst, nops, size);
				delete[] nops;

				return result;
			}


			BYTE* getMultiLevelPointer(const BYTE* base, const size_t offsets[], size_t size) {
				BYTE* address = const_cast<BYTE*>(base);

				for (size_t i = 0; i < size; i++) {
					address = *reinterpret_cast<BYTE**>(address);
					address += offsets[i];
				}

				return address;
			}


			BYTE* findSigAddress(const BYTE* base, size_t size, const char* signature) {
				// size of byte string signature of format "DE AD"
				const size_t sigSize = (strlen(signature) + 1) / 3;
				int* const sig = new int[sigSize] {};

				if (!helper::bytestringToInt(signature, sig, sigSize)) {
					delete[] sig;

					return nullptr;
				}

				BYTE* address = nullptr;
				MEMORY_BASIC_INFORMATION mbi{};

				// scan each memory region at a time
				for (size_t i = 0; i < size; i += mbi.RegionSize) {

					// scan only if commited and accessable
					if (!VirtualQuery(&base[i], &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) continue;

					address = helper::findSignature(&base[i], mbi.RegionSize, sig, sigSize);

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

				LE* flink = reinterpret_cast<LE*>(static_cast<uintptr_t>(listEntry.Flink));

				if (memcpy_s(&flink->Blink, sizeof(flink->Blink), &listEntry.Blink, sizeof(listEntry.Blink))) return false;

				LE* blink = reinterpret_cast<LE*>(static_cast<uintptr_t>(listEntry.Blink));

				if (memcpy_s(&blink->Flink, sizeof(blink->Flink), &listEntry.Flink, sizeof(listEntry.Flink))) return false;

				return true;
			}

		}


		#ifdef _WIN64

		static void getNearAddressRange(const BYTE* pBase, AddressRange* pAddrRange) {
			SYSTEM_INFO sysInfo{};
			GetSystemInfo(&sysInfo);

			pAddrRange->pageSize = sysInfo.dwPageSize;
			// start at the beginning of the page where pBase is located
			pAddrRange->start = reinterpret_cast<uintptr_t>(pBase) - (reinterpret_cast<uintptr_t>(pBase) % pAddrRange->pageSize);

			const uintptr_t range = INT32_MAX;
			// checks int underflow
			const uintptr_t minAddress = (pAddrRange->start > range) ? pAddrRange->start - range : 0;
			// checks int overflow
			const uintptr_t maxAddress = (UINTPTR_MAX - pAddrRange->start > range) ? pAddrRange->start + range : UINTPTR_MAX;

			pAddrRange->min = max(minAddress, reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress));
			pAddrRange->max = min(maxAddress, reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress));
		}

		#endif // _WIN64		


		namespace helper {

			bool bytestringToInt(const char* charSig, int intSig[], size_t sigSize) {

				// checks for format "DE AD" by character count and asserts the correct size
				if (!((strlen(charSig) + 1) % 3 == 0) || ((strlen(charSig) + 1) / 3 != sigSize)) return false;

				const char* cur = charSig;

				for (size_t i = 0; i < sigSize; i++) {

					if (*cur == '?') {
						// wildcard gets converted to -1
						intSig[i] = -1;
						cur += 3;
					}
					else {
						const int BASE = 0x10;
						intSig[i] = strtoul(cur, const_cast<char**>(&cur), BASE);
						cur++;
					}

				}

				return true;
			}


			BYTE* findSignature(const BYTE* base, size_t size, const int* signature, size_t sigSize) {
				BYTE* address = nullptr;

				// loop over the memory to be searched
				for (size_t i = 0; i < size - sigSize; i++) {
					bool found = true;

					// loop over signature at every position in memory to be searched
					for (size_t j = 0; j < sigSize; j++) {

						// -1 acts as wildcard
						if (base[i + j] != signature[j] && signature[j] != -1) {
							found = false;
							break;
						}

					}

					if (found) {
						address = const_cast<BYTE*>(&base[i]);
						break;
					}

				}

				return address;
			}

		}

	}

}