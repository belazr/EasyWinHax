#include "resource.h"
#include "..\..\demo.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a DirectX 9 application.
// Compile the project and inject it into a process rendering with DirectX 9.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue
// It is important to note that DirectX 9 uses ARGB as a color format.
static HMODULE hModule;

static hax::Bench bench("200 x hkEndScene", 200u);

static hax::draw::dx9::Backend backend;
static hax::draw::Engine engine{ &backend };

static const hax::draw::Color* pTextureData;
static uint32_t textureWidth;
static uint32_t textureHeight;

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pEndSceneHook;

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


static void APIENTRY hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
	
	if (!pTextureData) {
		loadTextureData();
	}
	
	bench.start();

	engine.beginFrame(pDevice);

	drawDemo(&engine, pTextureData, textureWidth, textureHeight, true);

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