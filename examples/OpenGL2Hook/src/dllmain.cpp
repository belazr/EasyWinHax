#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within an OpenGL 2 application.
// Compile the project and inject it into a process rendering with Open GL 2.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

static hax::Bench bench("200 x hkWglSwapBuffers", 200u);

static hax::ogl2::Backend backend;
static hax::Engine engine{ &backend };

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pSwapBuffersHook;

BOOL APIENTRY hkWglSwapBuffers(HDC hDc) {
	bench.start();

	engine.beginFrame(hDc);

	const hax::Vector2 middleOfScreen{ engine.fWindowWidth / 2.f, engine.fWindowHeight / 2.f };

	const float widthRect = engine.fWindowWidth / 4.f;
	const float heightRect = engine.fWindowHeight / 4.f;
	const hax::Vector2 topLeftRect{ middleOfScreen.x - widthRect / 2.f, middleOfScreen.y - heightRect / 2.f };

	engine.drawFilledRectangle(&topLeftRect, widthRect, heightRect, hax::rgb::gray);

	constexpr char TEXT[] = "EasyWinHax";
	const float widthText = _countof(TEXT) * hax::font::medium.width;
	const float heightText = hax::font::medium.height;

	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };

	engine.drawString(&hax::font::medium, &bottomLeftText, TEXT, hax::rgb::orange);

	engine.endFrame();

	bench.end();
	bench.printAvg();

	if (GetAsyncKeyState(VK_END) & 1) {
		pSwapBuffersHook->disable();
		const hax::ogl2::twglSwapBuffers pSwapBuffers = reinterpret_cast<hax::ogl2::twglSwapBuffers>(pSwapBuffersHook->getOrigin());
		const BOOL res = pSwapBuffers(hDc);
		ReleaseSemaphore(hHookSemaphore, 1l, nullptr);

		return res;
	}

	return reinterpret_cast<hax::ogl2::twglSwapBuffers>(pSwapBuffersHook->getGateway())(hDc);
}


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
		cleanup(hHookSemaphore, pSwapBuffersHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hHookSemaphore) {
		cleanup(hHookSemaphore, pSwapBuffersHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	pSwapBuffersHook = new hax::in::TrampHook("opengl32.dll", "wglSwapBuffers", reinterpret_cast<BYTE*>(hkWglSwapBuffers), 0x5);

	if (!pSwapBuffersHook) {
		cleanup(hHookSemaphore, pSwapBuffersHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pSwapBuffersHook->enable()) {
		cleanup(hHookSemaphore, pSwapBuffersHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: 0x" << std::hex << reinterpret_cast<uintptr_t>(pSwapBuffersHook->getOrigin()) << std::dec << std::endl;

	WaitForSingleObject(hHookSemaphore, INFINITE);
	
	cleanup(hHookSemaphore, pSwapBuffersHook, file);

	FreeLibraryAndExitThread(hModule, 0ul);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  reasonForCall, LPVOID) {

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