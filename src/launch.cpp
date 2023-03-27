#include "launch.h"
#include "proc.h"
#include "mem.h"
#include <stdint.h>
#include <TlHelp32.h>

// how long should be waited for return value of executed code in ms
#define THREAD_TIMEOUT 5000
#define LODWORD(qword) (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(qword)))

#define LAUNCH_DATA_X64_SPACE 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define LAUNCH_DATA_X86_SPACE 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

namespace launch {

	// structs for data within the shell code
	typedef struct LaunchDataX64 {
		uint64_t pArg;
		uint64_t pFunc;
		uint64_t pRet;
		uint8_t flag;
	}LaunchDataX64;

	typedef struct LaunchDataX86 {
		uint32_t pArg;
		uint32_t pFunc;
		uint32_t pRet;
		uint8_t flag;
	}LaunchDataX86;

	// ASM:
	// push rcx								save pLaunchData
	// mov rax, rcx							load pLaunchData->pArg for pLaunchData->pFunc call
	// mov rcx, QWORD PTR[rax]
	// sub rsp, 0x20						setup shadow space for function call
	// call QWORD PTR[rax + 0x8]			call pFunc
	// add rsp, 0x20						cleanup shadow space from function call
	// pop rcx								restore pLaunchData
	// mov QWORD PTR[rcx + 0x10], rax		write return value to &pLaunchData->pRet
	// xor rax, rax
	// ret
	static BYTE crtShellX64[]{ 0x51, 0x48, 0x8B, 0xC1, 0x48, 0x8B, 0x08, 0x48, 0x83, 0xEC, 0x20, 0xFF, 0x50, 0x08, 0x48, 0x83, 0xC4, 0x20, 0x59, 0x48, 0x89, 0x41, 0x10, 0x48, 0x31, 0xC0, 0xC3, LAUNCH_DATA_X64_SPACE };

	// ASM:
	// push   oldEip						save old eip for return instruction
	// push   ecx							save registers
	// push   eax
	// push   edx
	// pushf
	// mov    ecx, pLaunchData				
	// mov    eax, DWORD PTR [ecx+0x4]		load pLaunchData->pFunc
	// push   ecx							save pLaunchData
	// push   [ecx]							push pLaunchData->pArg for function call
	// call   eax							call pLaunchData->pFunc
	// pop    ecx							restore pLaunchData
	// mov    DWORD PTR [ecx+0x8], eax		write return value to pLaunchData->pRet
	// popf									restore registers
	// pop    edx
	// pop    eax
	// mov    BYTE PTR [ecx+0xc], 0x1		set pLaunchData->flag to one
	// pop    ecx							restore register
	// ret									return to old eip
	static BYTE hijackShellX86[]{ 0x68, 0x00, 0x00, 0x00, 0x00, 0x51, 0x50, 0x52, 0x9C, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x41, 0x04, 0x51, 0xFF, 0x31, 0xFF, 0xD0, 0x59, 0x89, 0x41, 0x08, 0x9D, 0x5A, 0x58, 0xC6, 0x41, 0x0C, 0x01, 0x59, 0xC3, LAUNCH_DATA_X86_SPACE };

	// ASM:
	// push   QWORD PTR[rip + 0x4f]			save old rip (in pLaunchData->pRet) for return instruction
	// push   rax							save registers
	// push   rcx
	// push   rdx
	// push   r8
	// push   r9
	// push   r10
	// push   r11
	// pushf
	// mov    rcx, QWORD PTR[rip + 0x2c]	load pLaunchData->pArg for pLaunchData->pFunc call
	// mov    rax, QWORD PTR[rip + 0x2d]	load pLaunchData->pFunc
	// sub    rsp, 0x20						setup shadow space for function call
	// call   rax
	// add    rsp, 0x20						cleanup shadow space from function call
	// mov    QWORD PTR[rip + 0x24], rax	write return value to pLaunchData->pRet
	// popf									restore registers
	// pop    r11
	// pop    r10
	// pop    r9
	// pop    r8
	// pop    rdx
	// pop    rcx
	// pop    rax
	// mov    BYTE PTR[rip + 0x19], 0x1		set pLaunchData->flag to one
	// ret
	static BYTE hijackShellX64[]{ 0xFF, 0x35, 0x4F, 0x00, 0x00, 0x00, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x9C, 0x48, 0x8B, 0x0D, 0x2C, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x2D, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x89, 0x05, 0x24, 0x00, 0x00, 0x00, 0x9D, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58, 0xC6, 0x05, 0x19, 0x00, 0x00, 0x00, 0x01, 0xC3, LAUNCH_DATA_X64_SPACE };

	#ifdef _WIN64

	// ASM:
	// push   rbp							save registers						
	// push   rsp
	// push   rbx
	// push   r8
	// push   rdx
	// push   rcx
	// jmp    0x0							jump to skip pLaunchData->pFunc call after first execution
	// mov    BYTE PTR[rip - 0x08], 0x2A	fix jump to skip pLaunchData->pFunc call
	// mov    rcx, QWORD PTR[rip + 0x39]	load pLaunchData->pArg for pLaunchData->pFunc call
	// sub    rsp, 0x28						setup shadow space for function call
	// call   QWORD PTR[rip + 0x37]			call pLaunchData->pFunc
	// add    rsp, 0x28						cleanup shadow space from function call
	// mov    QWORD PTR[rip + 0x34], rax	write return value to pLaunchData->pRet
	// mov    BYTE PTR[rip + 0x35], 0x1		set pLaunchData->flag to one
	// pop    rdx							restore registers for pCallNextHookEx call
	// pop    r8
	// pop    r9
	// movabs rbx, pCallNextHookEx			
	// sub    rsp, 0x28						setup shadow space for function call
	// call   rbx							call pCallNextHookEx
	// add    rsp, 0x28						cleanup shadow space from function call
	// pop    rbx							restore registers
	// pop    rsp
	// pop    rbp
	// ret
	static BYTE windowsHookShell[]{ 0x55, 0x54, 0x53, 0x41, 0x50, 0x52, 0x51, 0xEB, 0x00, 0xC6, 0x05, 0xF8, 0xFF, 0xFF, 0xFF, 0x2A, 0x48, 0x8B, 0x0D, 0x39, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0x15, 0x37, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0x48, 0x89, 0x05, 0x34, 0x00, 0x00, 0x00, 0xC6, 0x05, 0x35, 0x00, 0x00, 0x00, 0x01, 0x5A, 0x41, 0x58, 0x41, 0x59, 0x48, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x28, 0x5B, 0x5C, 0x5D, 0xC3, LAUNCH_DATA_X64_SPACE };

	#else

	// ASM:
	// push   ebp								setup stack frame
	// mov    ebp, esp
	// jmp    0x0								jump to skip pLaunchData->pFunc call after first execution
	// push   eax								save registers
	// push   ebx
	// mov    ebx, pLaunchData	
	// mov    BYTE PTR[ebx - 0x30], 0x1B		fix jump to skip pLaunchData->pFunc call
	// push   ebx								save pLaunchData
	// push   DWORD PTR[ebx]					load pLaunchData->pArg for pLaunchData->pFunc call
	// call   DWORD PTR[ebx + 0x4]
	// pop    ebx								restore pLaunchData
	// mov    DWORD PTR[ebx + 0x8], eax			write return value to pLaunchData->pRet
	// mov    BYTE PTR[ebx + 0xC], 0x1			set pLaunchData->flag to one
	// pop    ebx								restore registers for pCallNextHookEx call
	// pop    eax
	// push   DWORD PTR[ebp + 0x10]				load arguments for pCallNextHookEx call
	// push   DWORD PTR[ebp + 0xc]
	// push   DWORD PTR[ebp + 0x8]
	// push   0x0
	// call   pCallNextHook
	// pop    ebp
	// ret    0xc
	static BYTE windowsHookShell[]{ 0x55, 0x89, 0xE5, 0xEB, 0x00, 0x50, 0x53, 0xBB, 0x00, 0x00, 0x00, 0x00, 0xC6, 0x43, 0xD0, 0x1B, 0x53, 0xFF, 0x33, 0xFF, 0x53, 0x04, 0x5B, 0x89, 0x43, 0x08, 0xC6, 0x43, 0x0C, 0x01, 0x5B, 0x58, 0xFF, 0x75, 0x10, 0xFF, 0x75, 0x0C, 0xFF, 0x75, 0x08, 0x6A, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x5D, 0xC2, 0x0C, 0x00, LAUNCH_DATA_X86_SPACE };

	#endif

	// ASM:
	// jmp    0x2							jump to skip pLaunchData->pFunc call after first execution
	// mov    BYTE PTR[rip - 0x8], 0x2e		fix jump to skip pLaunchData->pFunc call
	// push   rcx							save registers
	// push   rdx
	// mov    rcx, QWORD PTR[rip + 0x2a]	load pLaunchData->pArg for pLaunchData->pFunc call
	// sub    rsp, 0x28						setup shadow space for function call
	// call   QWORD PTR[rip + 0x28]			call pLaunchData->pFunc
	// add    rsp, 0x28						cleanup shadow space of function call
	// mov    QWORD PTR[rip + 0x25], rax	write return value to pLaunchData->pRet
	// mov    BYTE PTR[rip + 0x26], 0x1		set pLaunchData->flag to one
	// pop    rdx							restore registers
	// pop    rcx
	// movabs rax, pGateway					jump to gateway to execute NtUserBeginPaint
	// jmp    rax
	BYTE hookBeginPaintShellX64[]{ 0xEB, 0x00, 0xC6, 0x05, 0xF8, 0xFF, 0xFF, 0xFF, 0x2E, 0x51, 0x52, 0x48, 0x8B, 0x0D, 0x2A, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0x15, 0x28, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0x48, 0x89, 0x05, 0x25, 0x00, 0x00, 0x00, 0xC6, 0x05, 0x26, 0x00, 0x00, 0x00, 0x01, 0x5A, 0x59, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0, LAUNCH_DATA_X64_SPACE };

	// ASM:
	// jmp    0x2							jump to skip pLaunchData->pFunc call after first execution
	// push   ebx							save register
	// mov    ebx, pLaunchData						
	// mov    BYTE PTR[ebx - 0x1f], 0x17	fix jump to skip pLaunchData->pFunc call
	// push   DWORD PTR[ebx]				load pLaunchData->pArg for pLaunchData->pFunc call
	// call   DWORD PTR[ebx + 0x4]			call pLaunchData->pFunc
	// mov    DWORD PTR[ebx + 0x8], eax		write return value to pLaunchData->pRet
	// mov    BYTE PTR[ebx + 0xc], 0x1		set pLaunchData->flag to one
	// pop    ebx							restore register
	// mov    eax, pGateway					jump to gateway to execute NtUserBeginPaint
	// jmp    eax
	BYTE hookBeginPaintShellX86[]{ 0xEB, 0x00, 0x53, 0xBB, 0x00, 0x00, 0x00, 0x00, 0xC6, 0x43, 0xE1, 0x17, 0xFF, 0x33, 0xFF, 0x53, 0x04, 0x89, 0x43, 0x08, 0xC6, 0x43, 0x0C, 0x01, 0x5B, 0xB8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0, LAUNCH_DATA_X86_SPACE };



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
			// GetExitCodeThread only gets a DWORD value
			BYTE* const pShellCode = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(crtShellX64), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

			if (!pShellCode) return false;

			LaunchDataX64* const pLaunchData = reinterpret_cast<LaunchDataX64*>(crtShellX64 + sizeof(crtShellX64) - sizeof(LaunchDataX64));
			pLaunchData->pArg = reinterpret_cast<uint64_t>(pArg);
			pLaunchData->pFunc = reinterpret_cast<uint64_t>(pFunc);

			if (!WriteProcessMemory(hProc, pShellCode, crtShellX64, sizeof(crtShellX64), nullptr)) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			LaunchDataX64* const pLaunchDataEx = reinterpret_cast<LaunchDataX64*>(pShellCode + sizeof(crtShellX64) - sizeof(LaunchDataX64));
			const HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellCode), pLaunchDataEx, 0, nullptr);

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

			ReadProcessMemory(hProc, &pLaunchDataEx->pRet, pRet, sizeof(uint64_t), nullptr);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);
			CloseHandle(hThread);

			#endif

		}

		return true;
	}


	static bool checkShellCodeFlag(HANDLE hProc, const void* pFlag);

	bool hijackThread(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet) {
		BOOL isWow64 = FALSE;
		IsWow64Process(hProc, &isWow64);

		// x64 targets only feasable for x64 compilations
		#ifndef _WIN64

		if (!isWow64) return false;

		#endif // !_WIN64

		// get the first thread entry
		proc::ex::ThreadEntry threadEntry{};
		proc::ex::getProcessThreadEntries(GetProcessId(hProc), &threadEntry, sizeof(threadEntry));

		const HANDLE hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, threadEntry.threadId);

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

			const ptrdiff_t launchDataOffset = sizeof(hijackShellX86) - sizeof(LaunchDataX86);
			
			LaunchDataX86* const pLaunchData = reinterpret_cast<LaunchDataX86*>(hijackShellX86 + launchDataOffset);
			pLaunchData->pArg = LODWORD(pArg);
			pLaunchData->pFunc = LODWORD(pFunc);

			const uint32_t oldEip = wow64Context.Eip;
			*reinterpret_cast<uint32_t*>(hijackShellX86 + 0x01) = oldEip;
			
			const LaunchDataX86* const pLaunchDataEx = reinterpret_cast<LaunchDataX86*>(pShellCode + launchDataOffset);
			*reinterpret_cast<uint32_t*>(hijackShellX86 + 0x0A) = LODWORD(pLaunchDataEx);

			if (!WriteProcessMemory(hProc, pShellCode, hijackShellX86, sizeof(hijackShellX86), nullptr)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			wow64Context.Eip = LODWORD(pShellCode);

			if (!Wow64SetThreadContext(hThread, &wow64Context)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			// post thread message to ensure thread execution
			PostThreadMessageA(threadEntry.threadId, 0, 0, 0);

			if (ResumeThread(hThread) == 0xFFFFFFFF) {
				wow64Context.Eip = oldEip;
				Wow64SetThreadContext(hThread, &wow64Context);
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!checkShellCodeFlag(hProc, &pLaunchDataEx->flag)) {

				if (SuspendThread(hThread) != 0xFFFFFFFF) {
					wow64Context.Eip = oldEip;
					Wow64SetThreadContext(hThread, &wow64Context);
					ResumeThread(hThread);
				}

				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!ReadProcessMemory(hProc, &pLaunchDataEx->pRet, pRet, sizeof(uint32_t), nullptr)) {
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

			const ptrdiff_t launchDataOffset = sizeof(hijackShellX64) - sizeof(LaunchDataX64);

			LaunchDataX64* const pLaunchData = reinterpret_cast<LaunchDataX64*>(hijackShellX64 + launchDataOffset);
			pLaunchData->pArg = reinterpret_cast<uint64_t>(pArg);
			pLaunchData->pFunc = reinterpret_cast<uint64_t>(pFunc);

			const uint64_t oldRip = context.Rip;
			// save old rip at pRet because we need it before it is overwritten
			pLaunchData->pRet = oldRip;

			if (!WriteProcessMemory(hProc, pShellCode, hijackShellX64, sizeof(hijackShellX64), nullptr)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			context.Rip = reinterpret_cast<uint64_t>(pShellCode);

			if (!SetThreadContext(hThread, &context)) {
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			// post thread message to ensure thread execution
			PostThreadMessageA(threadEntry.threadId, 0, 0, 0);

			if (ResumeThread(hThread) == 0xFFFFFFFF) {
				context.Rip = oldRip;
				SetThreadContext(hThread, &context);
				ResumeThread(hThread);
				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			const LaunchDataX64* const pLaunchDataEx = reinterpret_cast<LaunchDataX64*>(pShellCode + launchDataOffset);

			if (!checkShellCodeFlag(hProc, &pLaunchDataEx->flag)) {

				if (SuspendThread(hThread) != 0xFFFFFFFF) {
					context.Rip = oldRip;
					SetThreadContext(hThread, &context);
					ResumeThread(hThread);
				}

				CloseHandle(hThread);
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!ReadProcessMemory(hProc, &pLaunchDataEx->pRet, pRet, sizeof(uint64_t), nullptr)) {
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

	static BOOL CALLBACK setHookCallback(HWND hWnd, LPARAM lParam);

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

		HookData hookData{};
		hookData.procId = GetProcessId(hProc);
		// arbitrary but has to be loaded in both caller and target process
		hookData.hModule = proc::in::getModuleHandle("Kernel32.dll");

		#ifdef _WIN64

		const ptrdiff_t launchDataOffset = sizeof(windowsHookShell) - sizeof(LaunchDataX64);

		LaunchDataX64* const pLaunchData = reinterpret_cast<LaunchDataX64*>(windowsHookShell + launchDataOffset);
		pLaunchData->pArg = reinterpret_cast<uint64_t>(pArg);
		pLaunchData->pFunc = reinterpret_cast<uint64_t>(pFunc);
		
		*reinterpret_cast<uint64_t*>(windowsHookShell + 0x3A) = reinterpret_cast<uint64_t>(pCallNextHookEx);

		const LaunchDataX64* const pLaunchDataEx = reinterpret_cast<LaunchDataX64*>(pShellCode + launchDataOffset);

		hookData.pHookFunc = reinterpret_cast<HOOKPROC>(pShellCode);

		#else

		const ptrdiff_t launchDataOffset = sizeof(windowsHookShell) - sizeof(LaunchDataX86);

		LaunchDataX86* const pLaunchData = reinterpret_cast<LaunchDataX86*>(windowsHookShell + launchDataOffset);
		pLaunchData->pArg = reinterpret_cast<uint32_t>(pArg);
		pLaunchData->pFunc = reinterpret_cast<uint32_t>(pFunc);

		const LaunchDataX86* const pLaunchDataEx = reinterpret_cast<LaunchDataX86*>(pShellCode + launchDataOffset);
		*reinterpret_cast<uint32_t*>(windowsHookShell + 0x08) = reinterpret_cast<uint32_t>(pLaunchDataEx);
		*reinterpret_cast<uint32_t*>(windowsHookShell + 0x2C) = reinterpret_cast<uint32_t>(pCallNextHookEx) - (reinterpret_cast<uint32_t>(pShellCode) + 0x30);

		hookData.pHookFunc = reinterpret_cast<HOOKPROC>(pShellCode);

		#endif // _WIN64

		if (!WriteProcessMemory(hProc, pShellCode, windowsHookShell, sizeof(windowsHookShell), nullptr)) {
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		EnumWindows(setHookCallback, reinterpret_cast<LPARAM>(&hookData));

		if (!hookData.hHook || !hookData.hWnd) {
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		// foreground window to activate the hook
		const HWND hFgWnd = GetForegroundWindow();
		SetForegroundWindow(hookData.hWnd);
		SetForegroundWindow(hFgWnd);

		if (!checkShellCodeFlag(hProc, &pLaunchDataEx->flag)) {
			UnhookWindowsHookEx(hookData.hHook);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		UnhookWindowsHookEx(hookData.hHook);

		if (!ReadProcessMemory(hProc, &pLaunchDataEx->pRet, pRet, sizeof(uintptr_t), nullptr)) {
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

			return false;
		}

		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

		return true;
	}


	static BOOL CALLBACK resizeCallback(HWND hWnd, LPARAM lParam);

	bool hookBeginPaint(HANDLE hProc, tLaunchFunc pFunc, void* pArg, void** pRet) {
		BOOL isWow64 = FALSE;
		IsWow64Process(hProc, &isWow64);

		// x64 targets only feasable for x64 compilations
		#ifndef _WIN64

		if (!isWow64) return false;

		#endif // !_WIN64

		const HMODULE hNtdll = proc::ex::getModuleHandle(hProc, "win32u.dll");

		if (!hNtdll) return false;

		BYTE* const pNtUserBeginPaint = reinterpret_cast<BYTE*>(proc::ex::getProcAddress(hProc, hNtdll, "NtUserBeginPaint"));

		if (!pNtUserBeginPaint) return false;

		// size of larger shell code is used to be safe, allocates a whole page anyway
		BYTE* const pShellCode = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, sizeof(hookBeginPaintShellX64), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

		if (!pShellCode) return false;

		if (isWow64) {
			const ptrdiff_t launchDataOffset = sizeof(hookBeginPaintShellX86) - sizeof(LaunchDataX86);

			LaunchDataX86* const pLaunchData = reinterpret_cast<LaunchDataX86*>(hookBeginPaintShellX86 + launchDataOffset);
			pLaunchData->pArg = LODWORD(pArg);
			pLaunchData->pFunc = LODWORD(pFunc);

			const LaunchDataX86* const pLaunchDataEx = reinterpret_cast<LaunchDataX86*>(pShellCode + launchDataOffset);
			*reinterpret_cast<uint32_t*>(hookBeginPaintShellX86 + 0x04) = LODWORD(pLaunchDataEx);

			if (!WriteProcessMemory(hProc, pShellCode, hookBeginPaintShellX86, sizeof(hookBeginPaintShellX86), nullptr)) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			const size_t lenStolen = 10;
			BYTE* const pGateway = mem::ex::trampHook(hProc, pNtUserBeginPaint, pShellCode, pShellCode + 0x1A, lenStolen);

			if (!pGateway) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			const DWORD procId = GetProcessId(hProc);
			EnumWindows(resizeCallback, reinterpret_cast<LPARAM>(&procId));

			const bool success = checkShellCodeFlag(hProc, &pLaunchDataEx->flag);
			
			BYTE* const stolen = new BYTE[lenStolen]{};

			// read the stolen bytes from the gateway
			if (!ReadProcessMemory(hProc, pGateway, stolen, lenStolen, nullptr)) {
				delete[] stolen;
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			// patch the stolen bytes back
			if (!mem::ex::patch(hProc, pNtUserBeginPaint, stolen, lenStolen)) {
				delete[] stolen;
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			delete[] stolen;
			VirtualFreeEx(hProc, pGateway, 0, MEM_RELEASE);

			if (!success) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!ReadProcessMemory(hProc, &pLaunchDataEx->pRet, pRet, sizeof(uint32_t), nullptr)) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

		}
		else {

			#ifdef _WIN64

			const ptrdiff_t launchDataOffset = sizeof(hookBeginPaintShellX64) - sizeof(LaunchDataX64);

			LaunchDataX64* const pLaunchData = reinterpret_cast<LaunchDataX64*>(hookBeginPaintShellX64 + launchDataOffset);
			pLaunchData->pArg = reinterpret_cast<uint64_t>(pArg);
			pLaunchData->pFunc = reinterpret_cast<uint64_t>(pFunc);

			if (!WriteProcessMemory(hProc, pShellCode, hookBeginPaintShellX64, sizeof(hookBeginPaintShellX64), nullptr)) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			const size_t lenStolen = 8;
			BYTE* const pGateway = mem::ex::trampHook(hProc, pNtUserBeginPaint, pShellCode, pShellCode + 0x32, lenStolen);

			if (!pGateway) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			const DWORD processId = GetProcessId(hProc);
			EnumWindows(resizeCallback, reinterpret_cast<LPARAM>(&processId));

			const LaunchDataX64* const pLaunchDataEx = reinterpret_cast<LaunchDataX64*>(pShellCode + launchDataOffset);
			const bool success = checkShellCodeFlag(hProc, &pLaunchDataEx->flag);

			BYTE* const pStolen = new BYTE[lenStolen]{};

			// read the stolen bytes from the gateway
			if (!ReadProcessMemory(hProc, pGateway, pStolen, lenStolen, nullptr)) {
				delete[] pStolen;
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			// patch the stolen bytes back
			if (!mem::ex::patch(hProc, pNtUserBeginPaint, pStolen, lenStolen)) {
				delete[] pStolen;
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			delete[] pStolen;
			VirtualFreeEx(hProc, pGateway, 0, MEM_RELEASE);

			if (!success) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			if (!ReadProcessMemory(hProc, &pLaunchDataEx->pRet, pRet, sizeof(uint64_t), nullptr)) {
				VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

				return false;
			}

			#endif // _WIN64

		}

		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

		return true;
	}


	// check via flag if shell code has been executed
	static bool checkShellCodeFlag(HANDLE hProc, const void* pFlag) {
		bool flag = false;
		ULONGLONG start = GetTickCount64();

		do {
			ReadProcessMemory(hProc, pFlag, &flag, sizeof(flag), nullptr);

			if (flag) break;

			Sleep(50);
		} while (GetTickCount64() - start < THREAD_TIMEOUT);

		return flag;
	}


	// set WNDPROC hook in the thread of a window of the target process
	static BOOL CALLBACK setHookCallback(HWND hWnd, LPARAM lParam) {
		HookData* const pHookCallbackData = reinterpret_cast<HookData*>(lParam);

		if (pHookCallbackData->hHook) return TRUE;

		DWORD curProcId = 0;
		const DWORD curThreadId = GetWindowThreadProcessId(hWnd, &curProcId);

		if (!curThreadId || !curProcId) return TRUE;

		if (curProcId != pHookCallbackData->procId)  return TRUE;

		if (!IsWindowVisible(hWnd)) return TRUE;

		char className[MAX_PATH]{};

		if (!GetClassNameA(hWnd, className, MAX_PATH)) return TRUE;

		// window procedure hooks only possible for non-console windows
		if (!strcmp(className, "ConsoleWindowClass")) return TRUE;

		// pretend the shell code is loaded by the module
		HHOOK hHook = SetWindowsHookExA(WH_CALLWNDPROC, pHookCallbackData->pHookFunc, pHookCallbackData->hModule, curThreadId);

		if (!hHook)  return TRUE;

		pHookCallbackData->hHook = hHook;
		pHookCallbackData->hWnd = hWnd;

		return FALSE;
	}


	// resize a window of a process
	static BOOL CALLBACK resizeCallback(HWND hWnd, LPARAM lParam) {
		DWORD curProcId = 0;
		GetWindowThreadProcessId(hWnd, &curProcId);

		if (!curProcId) return TRUE;

		const DWORD procId = *reinterpret_cast<DWORD*>(lParam);

		if (curProcId != procId) return TRUE;

		if (!IsWindowVisible(hWnd)) return TRUE;

		char className[MAX_PATH]{};

		if (!GetClassNameA(hWnd, className, MAX_PATH)) return TRUE;

		// NtUserBeginPaint only called for non-console windows
		if (!strcmp(className, "ConsoleWindowClass")) return TRUE;

		WINDOWPLACEMENT wndPlacement{};
		wndPlacement.length = sizeof(WINDOWPLACEMENT);

		if (!GetWindowPlacement(hWnd, &wndPlacement)) return TRUE;

		UINT oldShowCmd = wndPlacement.showCmd;

		if (oldShowCmd == SW_MINIMIZE || oldShowCmd == SW_SHOWMINIMIZED || oldShowCmd == SW_SHOWMINNOACTIVE) {
			wndPlacement.showCmd = SW_RESTORE;
		}
		else {
			wndPlacement.showCmd = SW_SHOWMINIMIZED;
		}

		if (!SetWindowPlacement(hWnd, &wndPlacement)) return TRUE;

		wndPlacement.showCmd = oldShowCmd;

		if (!SetWindowPlacement(hWnd, &wndPlacement)) return TRUE;

		return FALSE;
	}

}