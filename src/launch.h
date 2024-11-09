#pragma once
#include <Windows.h>

// Functions to launch code execution in an external target process.
// x64 compilations of these functions (except setWindowsHook) can launch code in x64 and x86 targets.
// x86 compilations of these functions can only launch code in x86 targets.
// setWindowsHook can only launch code in targets with matching architechure.

namespace hax {

	namespace launch {

		typedef enum Status {
			SUCCESS = 0,
			ERR_CHECK_SHELL_FLAG,
			ERR_CREATE_THREAD,
			ERR_GET_MOD_HANDLE,
			ERR_GET_PAGE_SIZE,
			ERR_GET_PROC_ADDR,
			ERR_GET_PROC_ENTRIES,
			ERR_GET_PROC_ID,
			ERR_GET_THREAD_CONTEXT,
			ERR_GET_THREAD_ENTRIES,
			ERR_MEM_ALLOC,
			ERR_MEM_CPY,
			ERR_NO_SLEEPING_THREAD,
			ERR_OPEN_THREAD,
			ERR_PATCH,
			ERR_QUEUE_APC_WOW64_THREAD,
			ERR_QUEUE_USER_APC,
			ERR_READ_PROC_MEM,
			ERR_RESUME_THREAD,
			ERR_SET_THREAD_CONTEXT,
			ERR_SUSPEND_THREAD,
			ERR_THREAD_TIMEOUT,
			ERR_TRAMP_HOOK,
			ERR_WINDOWS_HOOK,
			ERR_WRITE_PROC_MEM
		}Status;

		typedef void* (WINAPI* tLaunchableFunc)(void* pArg);
		typedef Status (*tLaunchFunc)(HANDLE hProc, tLaunchableFunc pFunc, void* pArg, void* pRet);

		// Launches code execution by creating a thread in the target process via NtCreateThreadEx.
		// Waits for the thread and retrives the return value. Can retrive 8 byte return values for x64 targets (unlike GetExitCodeThread).
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the process in which context the code should be launched.
		// Needs at least PROCESS_CREATE_THREAD, PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
		// 
		// [in] pFunc:
		// Pointer to the code that should be executed by the new thread. Assumes the calling conventions:
		// x86: __stdcall void* pFunc(void* pArg)
		// x64: __fastcall void* pFunc(void* pArg)
		// Additional arguments can be passed in a struct pointed to by pArg.
		// To call API functions with different calling conventions use as a wrapper function.
		// 
		// [in] pArg:
		// Argument of the function called by the thread.
		// 
		// [out] pRet:
		// Pointer for the return value of the function called by the thread.
		// 
		// Return:
		// True on success or false on failure.
		Status createThread(HANDLE hProc, tLaunchableFunc pFunc, void* pArg, void* pRet);

		// Launches code execution by hijacking an existing thread of the target process.
		// Suspends the thread, switches it's context and resumes it executing the desired code.
		// Waits for successfull execution and retrives the return value.
		// Most reliable option if createRemoteThread fails due to counter measures.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the process in which context the code should be launched.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
		// 
		// [in] pFunc:
		// Pointer to the code that should be executed by the hijacked thread. Assumes the calling conventions:
		// x86: __stdcall void* pFunc(void* pArg)
		// x64: __fastcall void* pFunc(void* pArg)
		// Additional arguments can be passed in a struct pointed to by pArg.
		// To call API functions with different calling conventions use as a wrapper function.
		// 
		// [in] pArg:
		// Argument of the function called by the thread.
		// 
		// [out] pRet:
		// Pointer for the return value of the function called by the thread.
		// 
		// Return:
		// True on success or false on failure.
		Status hijackThread(HANDLE hProc, tLaunchableFunc pFunc, void* pArg, void* pRet);

		// Launches code execution by setting a windows hook.
		// Hooks the window procedure to process messages for a window of the target process.
		// After the hook is installed the function foregrounds the window to trigger execution.
		// Waits for successfull execution, retrives the return value and unhooks the window procedure.
		// In any case the function to launch is only executed once.
		// Caller and target architechture have to match.
		// Will fail on console application targets because they do not call window procedures.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the process in which context the code should be launched.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
		// 
		// [in] pFunc:
		// Pointer to the code that should be executed by the hook. Assumes the calling conventions:
		// x86: __stdcall void* pFunc(void* pArg)
		// x64: __fastcall void* pFunc(void* pArg)
		// Additional arguments can be passed in a struct pointed to by pArg.
		// To call API functions with different calling conventions use as a wrapper function.
		// 
		// [in] pArg:
		// Argument of the function called by the hook.
		// 
		// [out] pRet:
		// Pointer for the return value of the function called by the hook.
		// 
		// Return:
		// True on success or false on failure.
		Status setWindowsHook(HANDLE hProc, tLaunchableFunc pFunc, void* pArg, void* pRet);

		// Launches code execution by hooking NtUserBeginPaint from win32u.dll.
		// NtUserBeginPaint gets called when a window is resized or moved.
		// Sets a trampoline hook at the beginning of NtUserBeginPaint and then resizes the window to trigger execution.
		// Waits for successfull execution, retrives the return value and unhooks NtUserBeginPaint.
		// In any case the function to launch is only executed once.
		// Will fail on console application targets because they do not call NtUserBeginPaint.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the process in which context the code should be launched.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
		// 
		// [in] pFunc:
		// Pointer to the code that should be executed by the hook. Assumes the calling conventions:
		// x86: __stdcall void* pFunc(void* pArg)
		// x64: __fastcall void* pFunc(void* pArg)
		// Additional arguments can be passed in a struct pointed to by pArg.
		// To call API functions with different calling conventions use as a wrapper function.
		// 
		// [in] pArg:
		// Argument of the function called by the hook.
		// 
		// [out] pRet:
		// Pointer for the return value of the function called by the hook.
		// 
		// Return:
		// True on success or false on failure.
		Status hookBeginPaint(HANDLE hProc, tLaunchableFunc pFunc, void* pArg, void* pRet);

		// Launches code execution by queuing a user-mode APC.
		// Waits for successfull execution and retrives the return value.
		// APCs only get called when the thread to which they are queued is in an alertable state.
		// Will fail on target processes with no waiting thread in an alertable state.
		// 
		// Parameters:
		// 
		// [in] hProc:
		// Handle to the process in which context the code should be launched.
		// Needs at least PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ access rights.
		// 
		// [in] pFunc:
		// Pointer to the code that should be executed by the APC. Assumes the calling conventions:
		// x86: __stdcall void* pFunc(void* pArg)
		// x64: __fastcall void* pFunc(void* pArg)
		// Additional arguments can be passed in a struct pointed to by pArg.
		// To call API functions with different calling conventions use as a wrapper function.
		// 
		// [in] pArg:
		// Argument of the function called by the APC.
		// 
		// [out] pRet:
		// Pointer for the return value of the function called by the APC.
		// 
		// Return:
		// True on success or false on failure.
		Status queueUserApc(HANDLE hProc, tLaunchableFunc pFunc, void* pArg, void* pRet);

	}

}