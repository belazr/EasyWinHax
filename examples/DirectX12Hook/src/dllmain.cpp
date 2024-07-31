#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 12 application.
// Compile the project and inject it into a process rendering with DirectX 12.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pPresentHook;


void cleanup(HANDLE hSemaphore, hax::in::TrampHook* pHook, FILE* file) {

	if (pHook) {
		delete pHook;
	}

	if (hSemaphore) {
		CloseHandle(hSemaphore);
		// just to be save
		Sleep(10ul);
	}

	if (file) {
		fclose(file);
	}

	FreeConsole();
	// just to be save
	Sleep(500ul);
}


DWORD WINAPI haxThread(HMODULE hModule) {

	if (!AllocConsole()) {
		// just to be save
		Sleep(500ul);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	FILE* file{};

	if (freopen_s(&file, "CONOUT$", "w", stdout) || !file) {
		cleanup(hHookSemaphore, pPresentHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hHookSemaphore) {
		cleanup(hHookSemaphore, pPresentHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	void* pDXGISwapChain3VTable[9]{};

	if (!hax::draw::dx12::getDXGISwapChain3VTable(pDXGISwapChain3VTable, sizeof(pDXGISwapChain3VTable))) {
		cleanup(hHookSemaphore, pPresentHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	cleanup(hHookSemaphore, pPresentHook, file);

	FreeLibraryAndExitThread(hModule, 0ul);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reasonForCall, LPVOID) {

	if (reasonForCall != DLL_PROCESS_ATTACH) {

		return TRUE;
	}

	DisableThreadLibraryCalls(hModule);
	const HANDLE hThread = CreateThread(nullptr, 0u, reinterpret_cast<LPTHREAD_START_ROUTINE>(haxThread), hModule, 0ul, nullptr);

	if (!hThread) {

		return TRUE;
	}

	CloseHandle(hThread);

	return TRUE;
}