#include "launch.h"
#include "proc.h"
#include <stdint.h>
#include <TlHelp32.h>

// how long should be waited for return value of executed code in ms
#define THREAD_TIMEOUT 5000
#define LODWORD(qword) (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(qword)))

namespace launch {
	
	// ASM:
	// mov rax, rcx
	// mov rcx, QWORD PTR[rax]					load argument pointer to rcx
	// sub rsp, 0x28							setup stack
	// call QWORD PTR[rax + 0x8]				call function
	// add rsp, 0x28							cleanup stack
	// lea rcx, [rip + 0xffffffffffffffd8]			
	// mov QWORD PTR[rcx], rax					write return value to beginning of shell code
	// xor rax, rax								null rax
	// ret
	static BYTE x64CrtShell[]{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xC1, 0x48, 0x8B, 0x08, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0x50, 0x08, 0x48, 0x83, 0xC4, 0x28, 0x48, 0x8D, 0x0D, 0xD8, 0xFF, 0xFF, 0xFF, 0x48, 0x89, 0x01, 0x48, 0x31, 0xC0, 0xC3 };

	// ASM:
	// sub    esp,0x4							setup stack for return value
	// mov    DWORD PTR[esp], oldEip			save old eip for return instruction
	// push   eax								save registers
	// push   ecx
	// push   edx
	// pushf
	// mov    ecx, pArg
	// mov    eax, pFunc
	// push   ecx								push pArg for function call
	// call   eax								call pFunc
	// mov    x64CrtShell, eax					write return value to beginning of shell code
	// popf
	// pop    edx								restore registers
	// pop    ecx
	// pop    eax
	// mov    BYTE PTR ds : &flag, 0x0			set flag to zero
	// ret										return to old eip
	static BYTE x86HijackShell[]{ 0x00, 0x00, 0x00, 0x00, 0x83, 0xEC, 0x04,	0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00, 0x50, 0x51, 0x52, 0x9C, 0xB9, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x51,	0xFF, 0xD0, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x9D, 0x5A, 0x59, 0x58, 0xC6, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3 };

	// ASM:
	// sub    rsp, 0x8									setup stack for return value
	// mov    DWORD PTR[rsp], oldRip.LowPart			save old rip for return instruction
	// mov    DWORD PTR[rsp + 0x4], oldRip.HighPart
	// push   rax										save registers
	// push   rcx
	// push   rdx
	// push   r8
	// push   r9
	// push   r10
	// push   r11
	// pushf
	// movabs rax, pFunc
	// movabs rcx, pArg
	// sub    rsp, 0x20									setup stack for function call
	// call   rax										call pFunc
	// add    rsp, 0x20									cleanup stack from function call
	// lea    rcx, [rip + 0xffffffffffffffb4]			write return value to beginning of shell code
	// mov    QWORD PTR[rcx], rax
	// popf
	// pop    r11										restore registers
	// pop    r10
	// pop    r9
	// pop    r8
	// pop    rdx
	// pop    rcx
	// pop    rax
	// mov    BYTE PTR[rip + 0xffffffffffffffa9], 0x0	set flag to zero
	// ret												return to old rip
	static BYTE x64HijackShell[]{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x08, 0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,	0xC7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x9C, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8D, 0x0D, 0xB4, 0xFF, 0xFF, 0xFF, 0x48, 0x89, 0x01, 0x9D, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58,0xC6, 0x05, 0xA9, 0xFF, 0xFF, 0xFF, 0x00, 0xC3 };


	bool createRemoteThread(HANDLE hProc, LPTHREAD_START_ROUTINE pFunc, void* pArg, void** pRet) {
		BOOL isWow64 = FALSE;
		IsWow64Process(hProc, &isWow64);

		// x64 targets only feasable for x64 compilations
		#ifndef _WIN64

		if (!isWow64) return false;

		#endif // !_WIN64

		if (isWow64) {
			const HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, pFunc, pArg, 0, nullptr);

			if (!hThread) return false;

			if (WaitForSingleObject(hThread, THREAD_TIMEOUT)) {

				#pragma warning (push)
				#pragma warning (disable: 6258)
				TerminateThread(hThread, 0);
				#pragma warning (pop)

				CloseHandle(hThread);

				return false;
			}

			GetExitCodeThread(hThread, reinterpret_cast<DWORD*>(pRet));
			CloseHandle(hThread);
		}
		else {

			#ifdef _WIN64

			// shell coding for x64 processes is just done to get the full 8 byte return value of x64 threads
			// GetExitCodeThread can only get a DWORD value
			*reinterpret_cast<void**>(x64CrtShell) = pArg;
			*reinterpret_cast<const void**>(x64CrtShell + 0x8) = pFunc;

			BYTE* const pShellCode = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

			if (!pShellCode) return false;

			if (!WriteProcessMemory(hProc, pShellCode, x64CrtShell, sizeof(x64CrtShell), nullptr)) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			// function in shell code starts at offset 0x10
			const HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellCode + 0x10), pShellCode, 0, nullptr);

			if (!hThread) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (WaitForSingleObject(hThread, THREAD_TIMEOUT)) {

				#pragma warning (push)
				#pragma warning (disable: 6258)
				TerminateThread(hThread, 0);
				#pragma warning (pop)

				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			// retrive the full 8 byte return value of the thread from the shell code
			ReadProcessMemory(hProc, pShellCode, pRet, sizeof(uint64_t), nullptr);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);
			CloseHandle(hThread);

			#endif

		}

		return true;
	}


	bool hijackThread(HANDLE hProc, LPTHREAD_START_ROUTINE pFunc, void* pArg, void** pRet) {
		BOOL isWow64 = FALSE;
		IsWow64Process(hProc, &isWow64);

		// x64 targets only feasable for x64 compilations
		#ifndef _WIN64

		if (!isWow64) return false;

		#endif // !_WIN64

		THREADENTRY32 threadEntry{};

		if (!proc::ex::getTlHelpThreadEntry(hProc, &threadEntry)) return false;

		const HANDLE hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);

		if (!hThread) return false;

		// thread can not be suspended
		if (SuspendThread(hThread) == 0xFFFFFFFF) {
			CloseHandle(hThread);

			return false;
		}

		// size of bigger shell code is used to be safe, allocates a whole page anyway
		BYTE* const pShellCode = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(x64HijackShell), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

		if (!pShellCode) {
			ResumeThread(hThread);
			CloseHandle(hThread);

			return false;
		}

		if (isWow64) {
			WOW64_CONTEXT wow64Context{};
			wow64Context.ContextFlags = CONTEXT_CONTROL;

			if (!Wow64GetThreadContext(hThread, &wow64Context)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			DWORD oldEip = wow64Context.Eip;
			// offset of flag in the shell code to signal successfull execution
			ptrdiff_t flagOffset = 0x06;

			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x0A) = oldEip;
			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x13) = LODWORD(pArg);
			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x18) = LODWORD(pFunc);
			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x20) = LODWORD(pShellCode);
			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x2A) = LODWORD(pShellCode + flagOffset);

			// set new instruction pointer to beginning of shell code function
			wow64Context.Eip = LODWORD(pShellCode + 0x04);

			if (!WriteProcessMemory(hProc, pShellCode, x86HijackShell, sizeof(x86HijackShell), nullptr)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!Wow64SetThreadContext(hThread, &wow64Context)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			// thread can not be resumed
			if (ResumeThread(hThread) == 0xFFFFFFFF) {
				wow64Context.Eip = oldEip;
				Wow64SetThreadContext(hThread, &wow64Context);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			ULONGLONG start = GetTickCount64();
			bool flag = true;

			do {
				ReadProcessMemory(hProc, pShellCode + flagOffset, &flag, sizeof(flag), nullptr);

				if (GetTickCount64() - start > THREAD_TIMEOUT) {

					if (SuspendThread(hThread) != 0xFFFFFFFF) {
						wow64Context.Eip = oldEip;
						Wow64SetThreadContext(hThread, &wow64Context);
						ResumeThread(hThread);
					}

					VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

					return false;
				}

				Sleep(50);
			} while (flag);

			CloseHandle(hThread);
			ReadProcessMemory(hProc, pShellCode, pRet, sizeof(uint32_t), nullptr);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);
		}
		else {

			#ifdef _WIN64

			CONTEXT context{};
			context.ContextFlags = CONTEXT_CONTROL;

			if (!GetThreadContext(hThread, &context)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			LARGE_INTEGER oldRip{};
			oldRip.QuadPart = context.Rip;
			// offset of flag in the shell code to signal successfull execution
			ptrdiff_t flagOffset = 0x0B;

			*reinterpret_cast<uint32_t*>(x64HijackShell + 0x0F) = oldRip.LowPart;
			*reinterpret_cast<uint32_t*>(x64HijackShell + 0x17) = oldRip.HighPart;
			*reinterpret_cast<uint64_t*>(x64HijackShell + 0x29) = reinterpret_cast<const uint64_t>(pFunc);
			*reinterpret_cast<uint64_t*>(x64HijackShell + 0x33) = reinterpret_cast<uint64_t>(pArg);

			// set new instruction pointer to beginning of shell code function
			context.Rip = reinterpret_cast<uint64_t>(pShellCode + 0x08);

			if (!WriteProcessMemory(hProc, pShellCode, x64HijackShell, sizeof(x64HijackShell), nullptr)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!SetThreadContext(hThread, &context)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (ResumeThread(hThread) == 0xFFFFFFFF) {
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			ULONGLONG start = GetTickCount64();
			bool flag = true;

			do {
				ReadProcessMemory(hProc, pShellCode + flagOffset, &flag, sizeof(flag), nullptr);

				if (GetTickCount64() - start > THREAD_TIMEOUT) {

					if (SuspendThread(hThread) != 0xFFFFFFFF) {
						context.Rip = oldRip.QuadPart;
						SetThreadContext(hThread, &context);
						ResumeThread(hThread);
					}

					VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

					return false;
				}

				Sleep(50);
			} while (flag);

			CloseHandle(hThread);
			ReadProcessMemory(hProc, pShellCode, pRet, sizeof(uint64_t), nullptr);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			#endif // _WIN64

		}

		return true;
	}

}