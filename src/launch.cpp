#include "launch.h"
#include "proc.h"
#include <stdint.h>
#include <TlHelp32.h>

// how long should be waited for return value of executed code in ms
#define THREAD_TIMEOUT 5000
#define LODWORD(qword) (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(qword)))

namespace launch {

	// ASM:
	// mov rax, rcx							load pArg to rcx
	// mov rcx, QWORD PTR[rax]
	// sub rsp, 0x28						setup shadow space for function call
	// call QWORD PTR[rax + 0x8]			call pFunc
	// add rsp, 0x28						cleanup shadow space from function call
	// lea rcx, [rip - 0x28]
	// mov QWORD PTR[rcx], rax				write return value to beginning of shell code
	// xor rax, rax
	// ret
	static BYTE crtShellX64[]{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xC1, 0x48, 0x8B, 0x08, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0x50, 0x08, 0x48, 0x83, 0xC4, 0x28, 0x48, 0x8D, 0x0D, 0xD8, 0xFF, 0xFF, 0xFF, 0x48, 0x89, 0x01, 0x48, 0x31, 0xC0, 0xC3 };

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
	// mov    BYTE PTR ds:pFlag, 0x01		set *pFlag to one
	// ret									return to old eip
	static BYTE hijackShellX86[]{ 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x50, 0x51, 0x52, 0x9C, 0xB9, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x51, 0xFF, 0xD0, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x9D, 0x5A, 0x59, 0x58, 0xC6, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0xC3, 0x00 };

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
	// sub    rsp, 0x20						setup shadow space for function call
	// call   rax							call pFunc
	// add    rsp, 0x20						cleanup shadow space from function call
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
	static BYTE hijackShellX64[]{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x35, 0xF2, 0xFF, 0xFF, 0xFF, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x9C, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8D, 0x0D, 0xC1, 0xFF, 0xFF, 0xFF, 0x48, 0x89, 0x01, 0x9D, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58, 0xC6, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xC3, 0x00 };

	#ifdef _WIN64

	// ASM:
	// push   rbp							save registers
	// push   rsp
	// push   rbx
	// push   r8							save parameters for CallNextHookEx
	// push   rdx
	// push   rcx
	// lea    rbx, [rip - 0x1E]				load shell code base to rbx
	// jmp    0x0							jump to skip pFunc call after first execution
	// mov    BYTE PTR[rip - 0x0B], 0x1E	fix jump to skip pFunc call
	// movabs rcx, pArg						
	// sub    rsp, 0x28						setup shadow space for function call
	// call   pFunc
	// add    rsp, 0x28						cleanup shadow space from function call
	// mov    QWORD PTR[rbx], rax			write return to beginning of shell code
	// pop    rdx							set up call of CallNextHookEx
	// pop    r8
	// pop    r9
	// sub    rsp, 0x28						setup shadow space for function call
	// call   QWORD PTR [rbx + 0x8]			call CallNextHookEx
	// add    rsp, 0x28						cleanup shadow space from function call
	// pop    rbx
	// pop    rsp
	// pop    rbp
	// mov    BYTE PTR[rip + 0x1], 0x1		set flag to one
	// ret
	static BYTE windowsHookShell[]{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x54, 0x53, 0x41, 0x50, 0x52, 0x51, 0x48, 0x8D, 0x1D, 0xE2, 0xFF, 0xFF, 0xFF, 0xE9, 0x00, 0x00, 0x00, 0x00, 0xC6, 0x05, 0xF5, 0xFF, 0xFF, 0xFF, 0x1E, 0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0x13, 0x48, 0x83, 0xC4, 0x28, 0x48, 0x89, 0x03, 0x5A, 0x41, 0x58, 0x41, 0x59, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0x53, 0x08, 0x48, 0x83, 0xC4, 0x28, 0x5B, 0x5C, 0x5D, 0xC6, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0xC3, 0x00 };

	#else

	// ASM:
	// push   ebp							set up stack frame
	// mov    ebp, esp
	// jmp    0x0							jump to skip pFunc call after first execution
	// push   eax
	// push   ebx
	// mov    ebx, pShellCode
	// mov    BYTE PTR[ebx + 0x08], 0x19	fix jump to skip pFunc call
	// push   pArg
	// call   pFunc
	// mov    DWORD PTR[ebx], eax			write return to beginning of shell code
	// pop    ebx
	// pop    eax
	// push   DWORD PTR[ebp + 0x10]			set up call of CallNextHookEx
	// push   DWORD PTR[ebp + 0xc]
	// push   DWORD PTR[ebp + 0x8]
	// push   0x0
	// call   pCallNextHookEx
	// pop    ebp
	// mov    BYTE PTR ds:pFlag, 0x1		set flag to one
	// ret    0xc
	static BYTE windowsHookShell[]{ 0x00, 0x00, 0x00, 0x00, 0x55, 0x89, 0xE5, 0xEB, 0x00, 0x50, 0x53, 0xBB, 0x00, 0x00, 0x00, 0x00, 0xC6, 0x43, 0x08, 0x19, 0x68, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x89, 0x03, 0x5B, 0x58, 0xFF, 0x75, 0x10, 0xFF, 0x75, 0x0C, 0xFF, 0x75, 0x08, 0x6A, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x5D, 0xC6, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0xC2, 0x0C, 0x00, 0x00 };

	#endif


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
			*reinterpret_cast<void**>(crtShellX64) = pArg;
			*reinterpret_cast<const void**>(crtShellX64 + 0x8) = pFunc;

			BYTE* const pShellCode = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

			if (!pShellCode) return false;

			if (!WriteProcessMemory(hProc, pShellCode, crtShellX64, sizeof(crtShellX64), nullptr)) {
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


	static bool checkShellCodeFlag(HANDLE hProc, const BYTE* pFlag);

	bool hijackThread(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet) {
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

		if (SuspendThread(hThread) == 0xFFFFFFFF) {
			CloseHandle(hThread);

			return false;
		}

		// size of larger shell code is used to be safe, allocates a whole page anyway
		BYTE* const pShellCode = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(hijackShellX64), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

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

			const uint32_t oldEip = wow64Context.Eip;
			// set new instruction pointer to beginning of shell code function
			wow64Context.Eip = LODWORD(pShellCode + 0x04);
			// flag in the shell code to signal successfull execution
			const BYTE* const pFlag = pShellCode + sizeof(hijackShellX86) - 0x01;

			*reinterpret_cast<uint32_t*>(hijackShellX86 + 0x05) = oldEip;
			*reinterpret_cast<uint32_t*>(hijackShellX86 + 0x0E) = LODWORD(pArg);
			*reinterpret_cast<uint32_t*>(hijackShellX86 + 0x13) = LODWORD(pFunc);
			*reinterpret_cast<uint32_t*>(hijackShellX86 + 0x1B) = LODWORD(pShellCode);
			*reinterpret_cast<uint32_t*>(hijackShellX86 + 0x25) = LODWORD(pFlag);

			if (!WriteProcessMemory(hProc, pShellCode, hijackShellX86, sizeof(hijackShellX86), nullptr)) {
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

			// post thread message to ensure thread execution
			PostThreadMessageA(threadEntry.th32ThreadID, 0, 0, 0);

			if (ResumeThread(hThread) == 0xFFFFFFFF) {
				wow64Context.Eip = oldEip;
				Wow64SetThreadContext(hThread, &wow64Context);
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!checkShellCodeFlag(hProc, pFlag)) {

				if (SuspendThread(hThread) != 0xFFFFFFFF) {
					wow64Context.Eip = oldEip;
					Wow64SetThreadContext(hThread, &wow64Context);
					ResumeThread(hThread);
				}

				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!ReadProcessMemory(hProc, pShellCode, pRet, sizeof(uint32_t), nullptr)) {
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
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

			const uint64_t oldRip = context.Rip;
			// set new instruction pointer to beginning of shell code function
			context.Rip = reinterpret_cast<uint64_t>(pShellCode + 0x08);
			// flag in the shell code to signal successfull execution
			const BYTE* const pFlag = pShellCode + sizeof(hijackShellX64) - 0x01;

			*reinterpret_cast<uint64_t*>(hijackShellX64 + 0x00) = oldRip;
			*reinterpret_cast<uint64_t*>(hijackShellX64 + 0x1C) = reinterpret_cast<const uint64_t>(pFunc);
			*reinterpret_cast<uint64_t*>(hijackShellX64 + 0x26) = reinterpret_cast<uint64_t>(pArg);

			if (!WriteProcessMemory(hProc, pShellCode, hijackShellX64, sizeof(hijackShellX64), nullptr)) {
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

			// post thread message to ensure thread execution
			PostThreadMessageA(threadEntry.th32ThreadID, 0, 0, 0);

			if (ResumeThread(hThread) == 0xFFFFFFFF) {
				context.Rip = oldRip;
				SetThreadContext(hThread, &context);
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!checkShellCodeFlag(hProc, pFlag)) {

				if (SuspendThread(hThread) != 0xFFFFFFFF) {
					context.Rip = oldRip;
					SetThreadContext(hThread, &context);
					ResumeThread(hThread);
				}

				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!ReadProcessMemory(hProc, pShellCode, pRet, sizeof(uint64_t), nullptr)) {
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			#endif // _WIN64

		}

		CloseHandle(hThread);
		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

		return true;
	}


	typedef struct HookData {
		DWORD procId;
		HMODULE hModule;
		HOOKPROC pHookFunc;
		HHOOK hHook;
		HWND hWnd;
	}HookData;

	static BOOL CALLBACK setHookCallback(HWND hWnd, LPARAM pArg);

	bool setWindowsHook(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet) {
		BOOL isWow64 = FALSE;
		IsWow64Process(hProc, &isWow64);

		// installing hook only possible from process with matching architechture
		#ifdef _WIN64

		if (isWow64) return false;

		#else

		if (!isWow64) return false;

		#endif // _WIN64

		const HMODULE hUser32 = proc::ex::getModuleHandle(hProc, "User32.dll");

		if (!hUser32) return false;

		const FARPROC pCallNextHookEx = proc::ex::getProcAddress(hProc, hUser32, "CallNextHookEx");

		if (!pCallNextHookEx) return false;

		BYTE* const pShellCode = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(windowsHookShell), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

		if (!pShellCode) return false;

		// flag in the shell code to signal successfull execution
		const BYTE* const pFlag = pShellCode + sizeof(windowsHookShell) - 0x01;

		HookData hookData{};
		hookData.procId = GetProcessId(hProc);
		// arbitrary but has to be loaded in both caller and target process
		hookData.hModule = proc::in::getModuleHandle("Kernel32.dll");

		#ifdef _WIN64

		hookData.pHookFunc = reinterpret_cast<HOOKPROC>(pShellCode + 0x10);

		*reinterpret_cast<uint64_t*>(windowsHookShell) = reinterpret_cast<uint64_t>(pFunc);
		*reinterpret_cast<uint64_t*>(windowsHookShell + 0x08) = reinterpret_cast<uint64_t>(pCallNextHookEx);
		*reinterpret_cast<uint64_t*>(windowsHookShell + 0x2C) = reinterpret_cast<uint64_t>(pArg);

		#else

		hookData.pHookFunc = reinterpret_cast<HOOKPROC>(pShellCode + 0x04);

		*reinterpret_cast<uint32_t*>(windowsHookShell + 0x0C) = reinterpret_cast<uint32_t>(pShellCode);
		*reinterpret_cast<uint32_t*>(windowsHookShell + 0x15) = reinterpret_cast<uint32_t>(pArg);
		*reinterpret_cast<uint32_t*>(windowsHookShell + 0x1A) = reinterpret_cast<uint32_t>(pFunc) - (reinterpret_cast<uint32_t>(pShellCode) + 0x1E);
		*reinterpret_cast<uint32_t*>(windowsHookShell + 0x2E) = reinterpret_cast<uint32_t>(pCallNextHookEx) - (reinterpret_cast<uint32_t>(pShellCode) + 0x32);
		*reinterpret_cast<uint32_t*>(windowsHookShell + 0x35) = reinterpret_cast<uint32_t>(pFlag);

		#endif // _WIN64

		if (!WriteProcessMemory(hProc, pShellCode, windowsHookShell, sizeof(windowsHookShell), nullptr)) {
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		if (!EnumWindows(setHookCallback, reinterpret_cast<LPARAM>(&hookData))) {
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		if (!hookData.hHook || !hookData.hWnd) {
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		// foreground window to activate the hook
		const HWND hFgWnd = GetForegroundWindow();
		SetForegroundWindow(hookData.hWnd);
		SetForegroundWindow(hFgWnd);

		if (!checkShellCodeFlag(hProc, pFlag)) {
			UnhookWindowsHookEx(hookData.hHook);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		UnhookWindowsHookEx(hookData.hHook);

		if (!ReadProcessMemory(hProc, pShellCode, pRet, sizeof(uintptr_t), nullptr)) {
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

		return true;
	}


	static bool checkShellCodeFlag(HANDLE hProc, const BYTE* pFlag) {
		bool flag = false;
		ULONGLONG start = GetTickCount64();

		do {
			ReadProcessMemory(hProc, pFlag, &flag, sizeof(flag), nullptr);

			if (flag) break;

			Sleep(50);
		} while (GetTickCount64() - start < THREAD_TIMEOUT);

		return flag;
	}


	// set hook in first possible window thread
	static BOOL CALLBACK setHookCallback(HWND hWnd, LPARAM pArg) {
		HookData* const pHookCallbackData = reinterpret_cast<HookData*>(pArg);
		DWORD curProcId = 0;
		const DWORD curThreadId = GetWindowThreadProcessId(hWnd, &curProcId);

		if (!curThreadId || !curProcId) return TRUE;

		if (!pHookCallbackData->hHook && curProcId == pHookCallbackData->procId) {
			char className[MAX_PATH]{};

			// window procedures hooks only possible for non-console windows
			if (IsWindowVisible(hWnd) && GetClassNameA(hWnd, className, MAX_PATH) && strcmp(className, "ConsoleWindowClass")) {
				// pretend the shell code is loaded by the module
				HHOOK hHook = SetWindowsHookExA(WH_CALLWNDPROC, pHookCallbackData->pHookFunc, pHookCallbackData->hModule, curThreadId);

				if (hHook) {
					pHookCallbackData->hHook = hHook;
					pHookCallbackData->hWnd = hWnd;
				}

			}

		}

		return TRUE;
	}

}