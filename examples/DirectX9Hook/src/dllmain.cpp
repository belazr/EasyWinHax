#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 9 application.
// Compile the project and inject it into a process rendering with DirectX 9.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue
// It is important to note that DirectX 9 uses ARGB as a color format.

static hax::Bench bench("200 x hkEndScene", 200u);

static hax::draw::dx9::Backend backend;
static hax::draw::Engine engine{ &backend };

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pEndSceneHook;

void APIENTRY hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
	bench.start();

	engine.beginFrame(pDevice);

	const hax::Vector2 middleOfScreen{ engine.frameWidth / 2.f, engine.frameHeight / 2.f };

	const float widthRect = engine.frameWidth / 4.f;
	const float heightRect = engine.frameHeight / 4.f;
	const hax::Vector2 topLeftRect{ middleOfScreen.x - widthRect / 2.f, middleOfScreen.y - heightRect / 2.f };

	engine.drawFilledRectangle(&topLeftRect, widthRect, heightRect, hax::draw::argb::GRAY);

	constexpr char TEXT[] = "EasyWinHax";
	float widthText = _countof(TEXT) * hax::draw::font::medium.width;
	float heightText = hax::draw::font::medium.height;

	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };

	engine.drawString(&hax::draw::font::medium, &bottomLeftText, TEXT, hax::draw::argb::ORANGE);

	engine.endFrame();

	bench.end();
	bench.printAvg();

	if (GetAsyncKeyState(VK_END) & 1) {
		pEndSceneHook->disable();
		const hax::draw::dx9::tEndScene pEndScene = reinterpret_cast<hax::draw::dx9::tEndScene>(pEndSceneHook->getOrigin());
		pEndScene(pDevice);
		ReleaseSemaphore(hHookSemaphore, 1l, nullptr);

		return;
	}

	reinterpret_cast<hax::draw::dx9::tEndScene>(pEndSceneHook->getGateway())(pDevice);

	return;
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
		cleanup(hHookSemaphore, pEndSceneHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hHookSemaphore) {
		cleanup(hHookSemaphore, pEndSceneHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	void* pD3d9DeviceVTable[43]{};

	if (!hax::draw::dx9::getD3D9DeviceVTable(pD3d9DeviceVTable, sizeof(pD3d9DeviceVTable))) {
		cleanup(hHookSemaphore, pEndSceneHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	constexpr unsigned int END_SCENE_OFFSET = 42ul;
	BYTE* const pEndScene = reinterpret_cast<BYTE*>(pD3d9DeviceVTable[END_SCENE_OFFSET]);

	if (!pEndScene) {
		cleanup(hHookSemaphore, pEndSceneHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	#ifdef _WIN64

	constexpr size_t HOOK_SIZE = 0x5u;

	#else

	constexpr size_t HOOK_SIZE = 0x7u;

	#endif

	pEndSceneHook = new hax::in::TrampHook(pEndScene, reinterpret_cast<BYTE*>(hkEndScene), HOOK_SIZE);

	if (!pEndSceneHook) {
		cleanup(hHookSemaphore, pEndSceneHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pEndSceneHook->enable()) {
		cleanup(hHookSemaphore, pEndSceneHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: 0x" << std::hex << reinterpret_cast<uintptr_t>(pEndSceneHook->getOrigin()) << std::dec << std::endl;

	WaitForSingleObject(hHookSemaphore, INFINITE);

	cleanup(hHookSemaphore, pEndSceneHook, file, wasConsoleAllocated);

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