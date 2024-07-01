#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 9 application.
// Compile the project and inject it into a process rendering with DirectX 9.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

static hax::Bench bench("200 x hkEndScene", 200u);

static hax::dx9::Draw draw;
static hax::Engine engine{ &draw };

static HANDLE hSemaphore;
static hax::in::TrampHook* pEndScenetHook;

void APIENTRY hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
	bench.start();

	engine.beginDraw(pDevice);

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
		pEndScenetHook->disable();
		const hax::dx9::tEndScene pEndScene = reinterpret_cast<hax::dx9::tEndScene>(pEndScenetHook->getOrigin());
		ReleaseSemaphore(hSemaphore, 1l, nullptr);

		pEndScene(pDevice);

		return;
	}

	reinterpret_cast<hax::dx9::tEndScene>(pEndScenetHook->getGateway())(pDevice);

	return;
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

	void* pD3d9DeviceVTable[119]{};

	if (!hax::dx9::getD3D9DeviceVTable(pD3d9DeviceVTable, sizeof(pD3d9DeviceVTable))) {
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	constexpr unsigned int END_SCENE_OFFSET = 42ul;
	BYTE* const pEndScene = reinterpret_cast<BYTE*>(pD3d9DeviceVTable[END_SCENE_OFFSET]);

	if (!pEndScene) {
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	pEndScenetHook = new hax::in::TrampHook(pEndScene, reinterpret_cast<BYTE*>(hkEndScene), 0x7);

	if (!pEndScenetHook) {
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pEndScenetHook->enable()) {
		delete pEndScenetHook;
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: " << std::hex << reinterpret_cast<uintptr_t>(pEndScenetHook->getOrigin()) << std::dec << std::endl;

	hSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hSemaphore) {
		delete pEndScenetHook;
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	WaitForSingleObject(hSemaphore, INFINITE);
	CloseHandle(hSemaphore);
	// just to be save
	Sleep(10ul);

	delete pEndScenetHook;
	fclose(file);
	FreeConsole();

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