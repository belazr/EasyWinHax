#include "resource.h"
#include "..\..\demo.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 11 application.
// Compile the project and inject it into a process rendering with DirectX 11.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue
static HMODULE hModule;

static hax::Bench bench("200 x hkPresent", 200u);

static hax::draw::dx11::Backend backend;
static hax::draw::Engine engine{ &backend, hax::draw::fonts::inconsolata };

static const hax::draw::Color* pTextureData;
static uint32_t textureWidth;
static uint32_t textureHeight;

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pPresentHook;

static void loadTextureData() {
	const HRSRC hTextureRes = FindResourceA(hModule, MAKEINTRESOURCE(IDR_DEMO_TEXTURE), RT_RCDATA);

	if (!hTextureRes) return;

	const HGLOBAL hLoadedTextureRes = LoadResource(hModule, hTextureRes);

	if (!hLoadedTextureRes) return;

	const uint32_t* pTextureResData = static_cast<const uint32_t*>(LockResource(hLoadedTextureRes));

	if (!pTextureResData) return;

	textureWidth = pTextureResData[0];
	textureHeight = pTextureResData[1];
	pTextureData = reinterpret_cast<const hax::draw::Color*>(pTextureResData + 2);

	return;
}


static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags) {
	
	if (!pTextureData) {
		loadTextureData();
	}

	bench.begin();

	engine.beginFrame(pSwapChain);

	drawDemo(&engine, pTextureData, textureWidth, textureHeight);

	engine.endFrame();

	bench.end();
	bench.printAvg();

	if (GetAsyncKeyState(VK_END) & 1) {
		pPresentHook->disable();
		const hax::draw::dx11::tPresent pPresent = reinterpret_cast<hax::draw::dx11::tPresent>(pPresentHook->getOrigin());
		const HRESULT res = pPresent(pSwapChain, syncInterval, flags);
		ReleaseSemaphore(hHookSemaphore, 1l, nullptr);

		return res;
	}

	return reinterpret_cast<hax::draw::dx11::tPresent>(pPresentHook->getGateway())(pSwapChain, syncInterval, flags);
}


static void cleanup(HANDLE hSemaphore, hax::in::TrampHook* pHook, FILE* file, BOOL freeConsole) {

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


static DWORD WINAPI haxThread() {
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

	void* pD3D11SwapChainVTable[9]{};

	if (!hax::draw::dx11::getD3D11SwapChainVTable(pD3D11SwapChainVTable, sizeof(pD3D11SwapChainVTable))) {
		cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	constexpr unsigned int PRESENT_OFFSET = 8ul;
	BYTE* const pPresent = reinterpret_cast<BYTE*>(pD3D11SwapChainVTable[PRESENT_OFFSET]);

	if (!pPresent) {
		cleanup(hHookSemaphore, pPresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	#ifdef _WIN64

	constexpr size_t HOOK_SIZE = 0x5u;

	#else

	constexpr size_t HOOK_SIZE = 0x8u;

	#endif

	pPresentHook = new hax::in::TrampHook(pPresent, reinterpret_cast<BYTE*>(hkPresent), HOOK_SIZE);

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


BOOL APIENTRY DllMain(HMODULE hMod, DWORD reasonForCall, LPVOID) {

	if (reasonForCall != DLL_PROCESS_ATTACH) {

		return TRUE;
	}

	DisableThreadLibraryCalls(hMod);
	hModule = hMod;
	const HANDLE hThread = CreateThread(nullptr, 0u, reinterpret_cast<LPTHREAD_START_ROUTINE>(haxThread), nullptr, 0ul, nullptr);

	if (!hThread) {

		return TRUE;
	}

	CloseHandle(hThread);

	return TRUE;
}