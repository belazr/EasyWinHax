#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 10 application.
// Compile the project and inject it into a process rendering with DirectX 10.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

void cleanup(FILE* file, BOOL freeConsole) {

	if (file) {
		fclose(file);
	}

	if (freeConsole) {
		FreeConsole();
	}

	// just to be save
	Sleep(500ul);
}


DWORD WINAPI haxThread(HMODULE hModule) {
	const BOOL wasConsoleAllocated = AllocConsole();

	FILE* file = nullptr;

	if (freopen_s(&file, "CONOUT$", "w", stdout) || !file) {
		cleanup(file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	void* pD3D10SwapChainVTable[9]{};

	if (!hax::draw::dx10::getD3D10SwapChainVTable(pD3D10SwapChainVTable, sizeof(pD3D10SwapChainVTable))) {
		cleanup(file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

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