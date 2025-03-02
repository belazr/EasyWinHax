#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 10 application.
// Compile the project and inject it into a process rendering with DirectX 10.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pPresentHook;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags) {

	if (GetAsyncKeyState(VK_END) & 1) {
		pPresentHook->disable();
		const hax::draw::dx11::tPresent pPresent = reinterpret_cast<hax::draw::dx11::tPresent>(pPresentHook->getOrigin());
		const HRESULT res = pPresent(pSwapChain, syncInterval, flags);
		ReleaseSemaphore(hHookSemaphore, 1l, nullptr);

		return res;
	}

	return reinterpret_cast<hax::draw::dx11::tPresent>(pPresentHook->getGateway())(pSwapChain, syncInterval, flags);
}


void cleanup(HANDLE hSemaphore, hax::in::TrampHook* pHook, FILE* file, BOOL freeConsole) {

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
		cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hHookSemaphore) {
		cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	void* pD3D10SwapChainVTable[9]{};

	if (!hax::draw::dx10::getD3D10SwapChainVTable(pD3D10SwapChainVTable, sizeof(pD3D10SwapChainVTable))) {
		cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	constexpr unsigned int PRESENT_OFFSET = 8ul;
	BYTE* const pPresent = reinterpret_cast<BYTE*>(pD3D10SwapChainVTable[PRESENT_OFFSET]);

	if (!pPresent) {
		cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	pPresentHook = new hax::in::TrampHook(pPresent, reinterpret_cast<BYTE*>(hkPresent), 0x8u);

	if (!pPresentHook) {
		cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pPresentHook->enable()) {
		cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: 0x" << std::hex << reinterpret_cast<uintptr_t>(pPresentHook->getOrigin()) << std::dec << std::endl;

	WaitForSingleObject(hHookSemaphore, INFINITE);

	cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

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