#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 11 application.
// Compile the project and inject it into a process rendering with DirectX 11.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

static hax::Bench bench("200 x hkPresent", 200u);

static hax::dx11::Draw draw;
static hax::Engine engine{ &draw };

static HANDLE hSemaphore;
static hax::in::TrampHook* pPresentHook;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags) {
	bench.start();

	engine.beginDraw(pSwapChain);

	const hax::Vector2 middleOfScreen{ engine.fWindowWidth / 2.f, engine.fWindowHeight / 2.f };

	const float widthRect = engine.fWindowWidth / 4.f;
	const float heightRect = engine.fWindowHeight / 4.f;
	const hax::Vector2 topLeftRect{ middleOfScreen.x - widthRect / 2.f, middleOfScreen.y - heightRect / 2.f };

	engine.drawFilledRectangle(&topLeftRect, widthRect, heightRect, hax::rgb::gray);

	constexpr char TEXT[] = "EasyWinHax";
	float widthText = _countof(TEXT) * hax::font::medium.width;
	float heightText = hax::font::medium.height;

	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };

	engine.drawString(&hax::font::medium, &bottomLeftText, TEXT, hax::rgb::orange);

	engine.endDraw();

	bench.end();
	bench.printAvg();

	if (GetAsyncKeyState(VK_END) & 1) {
		pPresentHook->disable();
		const hax::dx11::tPresent pPresent = reinterpret_cast<hax::dx11::tPresent>(pPresentHook->getOrigin());
		const HRESULT res = pPresent(pSwapChain, syncInterval, flags);
		ReleaseSemaphore(hSemaphore, 1l, nullptr);

		return res;
	}

	return reinterpret_cast<hax::dx11::tPresent>(pPresentHook->getGateway())(pSwapChain, syncInterval, flags);
}


DWORD WINAPI haxThread(HMODULE hModule) {

	if (!AllocConsole()) {

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	FILE* file{};

	if (freopen_s(&file, "CONOUT$", "w", stdout) || !file) {
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	void* pD3D11SwapChainVTable[9]{};

	if (!hax::dx11::getD3D11SwapChainVTable(pD3D11SwapChainVTable, sizeof(pD3D11SwapChainVTable))) {
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	constexpr unsigned int PRESENT_OFFSET = 8ul;
	BYTE* const pEndScene = reinterpret_cast<BYTE*>(pD3D11SwapChainVTable[PRESENT_OFFSET]);

	if (!pEndScene) {
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	pPresentHook = new hax::in::TrampHook(pEndScene, reinterpret_cast<BYTE*>(hkPresent), 0x8);

	if (!pPresentHook) {
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pPresentHook->enable()) {
		delete pPresentHook;
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: " << std::hex << reinterpret_cast<uintptr_t>(pPresentHook->getOrigin()) << std::dec << std::endl;

	hSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hSemaphore) {
		delete pPresentHook;
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	WaitForSingleObject(hSemaphore, INFINITE);
	CloseHandle(hSemaphore);
	
	// just to be save
	Sleep(10ul);

	delete pPresentHook;
	fclose(file);
	FreeConsole();

	// just to be save
	Sleep(100ul);

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