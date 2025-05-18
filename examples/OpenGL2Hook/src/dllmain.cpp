#include "resource.h"
#include "..\..\demo.h"
#include <iostream>

// This is a example DLL for hooking and drawing within an OpenGL 2 application.
// Compile the project and inject it into a process rendering with Open GL 2.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue
static HMODULE hModule;

static hax::Bench bench("200 x hkWglSwapBuffers", 200u);

static hax::draw::ogl2::Backend backend;
static hax::draw::Engine engine{ &backend, hax::draw::fonts::inconsolata };

static const hax::draw::Color* pTextureData;
static uint32_t textureWidth;
static uint32_t textureHeight;

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pSwapBuffersHook;

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


static BOOL APIENTRY hkWglSwapBuffers(HDC hDc) {

	if (!pTextureData) {
		loadTextureData();
	}

	bench.start();

	engine.beginFrame(hDc);

	drawDemo(&engine, pTextureData, textureWidth, textureHeight);

	engine.endFrame();

	bench.end();
	bench.printAvg();

	if (GetAsyncKeyState(VK_END) & 1) {
		pSwapBuffersHook->disable();
		const hax::draw::ogl2::twglSwapBuffers pSwapBuffers = reinterpret_cast<hax::draw::ogl2::twglSwapBuffers>(pSwapBuffersHook->getOrigin());
		const BOOL res = pSwapBuffers(hDc);
		ReleaseSemaphore(hHookSemaphore, 1l, nullptr);

		return res;
	}

	return reinterpret_cast<hax::draw::ogl2::twglSwapBuffers>(pSwapBuffersHook->getGateway())(hDc);
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
		cleanup(hHookSemaphore, pSwapBuffersHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hHookSemaphore) {
		cleanup(hHookSemaphore, pSwapBuffersHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	#ifdef _WIN64

	constexpr size_t HOOK_SIZE = 0x7u;

	#else

	constexpr size_t HOOK_SIZE = 0x5u;

	#endif

	pSwapBuffersHook = new hax::in::TrampHook("opengl32.dll", "wglSwapBuffers", reinterpret_cast<BYTE*>(hkWglSwapBuffers), HOOK_SIZE);

	if (!pSwapBuffersHook) {
		cleanup(hHookSemaphore, pSwapBuffersHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pSwapBuffersHook->enable()) {
		cleanup(hHookSemaphore, pSwapBuffersHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: 0x" << std::hex << reinterpret_cast<uintptr_t>(pSwapBuffersHook->getOrigin()) << std::dec << std::endl;

	WaitForSingleObject(hHookSemaphore, INFINITE);
	
	cleanup(hHookSemaphore, pSwapBuffersHook, file, wasConsoleAllocated);

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