#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 11 application.
// Compile the project and inject it into a process rendering with DirectX 11.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

static hax::Bench bench("200 x hkPresent", 200u);

static hax::draw::dx11::Backend backend;
static hax::draw::Engine engine{ &backend };

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pPresentHook;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags) {
	bench.start();

	engine.beginFrame(pSwapChain);

	const hax::Vector2 middleOfScreen{ engine.frameWidth / 2.f, engine.frameHeight / 2.f };

	const float widthRect = engine.frameWidth / 4.f;
	const float heightRect = engine.frameHeight / 4.f;
	const hax::Vector2 topLeftRect{ middleOfScreen.x - widthRect / 2.f, middleOfScreen.y - heightRect / 2.f };

	engine.drawFilledRectangle(&topLeftRect, widthRect, heightRect, hax::draw::abgr::GRAY);

	constexpr char TEXT[] = "EasyWinHax";
	const float widthText = _countof(TEXT) * hax::draw::font::medium.width;
	const float heightText = hax::draw::font::medium.height;

	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };

	engine.drawString(&hax::draw::font::medium, &bottomLeftText, TEXT, hax::draw::abgr::ORANGE);

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