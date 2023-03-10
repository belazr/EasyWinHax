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
	// mov rcx, QWORD PTR[rax]				load argument pointer to rcx
	// sub rsp, 0x28						setup stack
	// call QWORD PTR[rax + 0x8]			call function
	// add rsp, 0x28						cleanup stack
	// lea rcx, [rip - 0x28]
	// mov QWORD PTR[rcx], rax				write return value to beginning of shell code
	// xor rax, rax							null rax
	// ret
	static BYTE x64CrtShell[]{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xC1, 0x48, 0x8B, 0x08, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0x50, 0x08, 0x48, 0x83, 0xC4, 0x28, 0x48, 0x8D, 0x0D, 0xD8, 0xFF, 0xFF, 0xFF, 0x48, 0x89, 0x01, 0x48, 0x31, 0xC0, 0xC3 };

	// ASM:
	// push   oldEip						save old eip for return instruction
	// push   eax							save registers
	// push   ecx
	// push   edx
	// pushf
	// mov    ecx, pArg
	// mov    eax, pFunc
	// push   ecx							push pArg for function call
	// call   eax							call pFunc
	// mov    pShellCode, eax				write return value to beginning of shell code
	// popf
	// pop    edx							restore registers
	// pop    ecx
	// pop    eax
	// mov    BYTE PTR ds :&flag, 0x01		set flag to one
	// ret									return to old eip
	static BYTE x86HijackShell[]{ 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x50, 0x51, 0x52, 0x9C, 0xB9, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x51, 0xFF, 0xD0, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x9D, 0x5A, 0x59, 0x58, 0xC6, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0xC3, 0x00 };

	// ASM:
	// push   QWORD PTR [rip - 0x0E]		save old rip for return instruction
	// push   rax							save registers
	// push   rcx
	// push   rdx
	// push   r8
	// push   r9
	// push   r10
	// push   r11
	// pushf
	// movabs rax, pFunc
	// movabs rcx, pArg
	// sub    rsp, 0x20						setup stack for function call
	// call   rax							call pFunc
	// add    rsp, 0x20						cleanup stack from function call
	// lea    rcx, [rip - 0x3F]				write return value to beginning of shell code
	// mov    QWORD PTR[rcx], rax
	// popf
	// pop    r11							restore registers
	// pop    r10
	// pop    r9
	// pop    r8
	// pop    rdx
	// pop    rcx
	// pop    rax
	// mov    BYTE PTR[rip + 0x01], 0x01	set flag to one
	// ret									return to old rip
	static BYTE x64HijackShell[]{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x35, 0xF2, 0xFF, 0xFF, 0xFF, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x9C, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8D, 0x0D, 0xC1, 0xFF, 0xFF, 0xFF, 0x48, 0x89, 0x01, 0x9D, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58, 0xC6, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xC3, 0x00 };


	bool createRemoteThread(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet) {
		BOOL isWow64 = FALSE;
		IsWow64Process(hProc, &isWow64);

		// x64 targets only feasable for x64 compilations
		#ifndef _WIN64

		if (!isWow64) return false;

		#endif // !_WIN64

		if (isWow64) {
			const HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pFunc), pArg, 0, nullptr);

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

			// shell coding for x64 processes is done just to get the full 8 byte return value of x64 threads
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


	static BOOL CALLBACK refreshCallback(HWND hWnd, LPARAM pArg);

	bool hijackThread(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet, bool refreshWnd) {
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

		bool flag = false;

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
			ptrdiff_t flagOffset = sizeof(x86HijackShell) - 0x01;

			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x05) = oldEip;
			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x0E) = LODWORD(pArg);
			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x13) = LODWORD(pFunc);
			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x1B) = LODWORD(pShellCode);
			*reinterpret_cast<uint32_t*>(x86HijackShell + 0x25) = LODWORD(pShellCode + flagOffset);

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

			DWORD procId = GetProcessId(hProc);

			if (refreshWnd) {
				// refresh window to raise thread priority and ensure thread execution
				EnumWindows(refreshCallback, reinterpret_cast<LPARAM>(&procId));
			}

			ULONGLONG start = GetTickCount64();

			do {
				ReadProcessMemory(hProc, pShellCode + flagOffset, &flag, sizeof(flag), nullptr);
				
				if (flag) break;

				Sleep(50);
			} while (GetTickCount64() - start < THREAD_TIMEOUT);

			if (flag) {
				ReadProcessMemory(hProc, pShellCode, pRet, sizeof(uint32_t), nullptr);
			}
			// thread can be suspended
			else if (SuspendThread(hThread) != 0xFFFFFFFF) {
				wow64Context.Eip = oldEip;
				Wow64SetThreadContext(hThread, &wow64Context);
				ResumeThread(hThread);				
			}

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

			uint64_t oldRip = context.Rip;
			// offset of flag in the shell code to signal successfull execution
			ptrdiff_t flagOffset = sizeof(x64HijackShell) - 0x01;

			*reinterpret_cast<uint64_t*>(x64HijackShell) = oldRip;
			*reinterpret_cast<uint64_t*>(x64HijackShell + 0x1C) = reinterpret_cast<const uint64_t>(pFunc);
			*reinterpret_cast<uint64_t*>(x64HijackShell + 0x26) = reinterpret_cast<uint64_t>(pArg);

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

			// thread can not be resumed
			if (ResumeThread(hThread) == 0xFFFFFFFF) {
				context.Rip = oldRip;
				SetThreadContext(hThread, &context);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			DWORD procId = GetProcessId(hProc);

			if (refreshWnd) {
				// refresh window to raise thread priority and ensure thread execution
				EnumWindows(refreshCallback, reinterpret_cast<LPARAM>(&procId));
			}

			ULONGLONG start = GetTickCount64();

			do {
				ReadProcessMemory(hProc, pShellCode + flagOffset, &flag, sizeof(flag), nullptr);

				if (flag) break;

				Sleep(50);
			} while (GetTickCount64() - start < THREAD_TIMEOUT);

			if (flag) {
				ReadProcessMemory(hProc, pShellCode, pRet, sizeof(uint64_t), nullptr);
			}
			// thread can be suspended
			else if (SuspendThread(hThread) != 0xFFFFFFFF) {
				context.Rip = oldRip;
				SetThreadContext(hThread, &context);
				ResumeThread(hThread);
			}

			#endif // _WIN64

		}

		CloseHandle(hThread);
		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

		return flag;
	}


	static BOOL CALLBACK refreshCallback(HWND hWnd, LPARAM pProcId) {
		const DWORD targetProcId = *reinterpret_cast<DWORD*>(pProcId);
		DWORD curProcId = 0;
		GetWindowThreadProcessId(hWnd, &curProcId);

		if (curProcId == targetProcId) {

			if (IsWindowVisible(hWnd)) {
				WINDOWPLACEMENT wndPlacement{};
				wndPlacement.length = sizeof(WINDOWPLACEMENT);
				GetWindowPlacement(hWnd, &wndPlacement);
				UINT oldShowCmd = wndPlacement.showCmd;

				if (wndPlacement.showCmd == SW_MINIMIZE || wndPlacement.showCmd == SW_SHOWMINIMIZED) {
					wndPlacement.showCmd = SW_RESTORE;
				}
				else {
					wndPlacement.showCmd = SW_SHOWMINIMIZED;
				}

				SetWindowPlacement(hWnd, &wndPlacement);
				wndPlacement.showCmd = oldShowCmd;
				SetWindowPlacement(hWnd, &wndPlacement);
			}

		}

		return TRUE;
	}

}