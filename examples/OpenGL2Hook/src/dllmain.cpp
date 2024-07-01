#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within an OpenGL 2 application.
// Compile the project and inject it into a process rendering with Open GL 2.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

static hax::Bench bench("200 x hkVkQueuePresentKHR", 200u);

static hax::ogl2::Font* pFont;
static hax::ogl2::Draw draw;
static hax::Engine engine{ &draw };

static HANDLE hSemaphore;
static hax::in::TrampHook* pSwapBuffersHook;

BOOL APIENTRY hkwglSwapBuffers(HDC hDc) {

	if (!pFont) {
		pFont = new hax::ogl2::Font(hDc, "Consolas", 25);
	}

	bench.start();

	engine.beginDraw(hDc);

	const hax::Vector2 middleOfScreen{ engine.fWindowWidth / 2.f, engine.fWindowHeight / 2.f };

	const float widthRect = engine.fWindowWidth / 4.f;
	const float heightRect = engine.fWindowHeight / 4.f;
	const hax::Vector2 topLeftRect{ middleOfScreen.x - widthRect / 2.f, middleOfScreen.y - heightRect / 2.f };

	engine.drawFilledRectangle(&topLeftRect, widthRect, heightRect, hax::rgb::gray);

	constexpr char text[] = "EasyWinHax";
	const float widthText = _countof(text) * hax::font::medium.width;
	const float heightText = hax::font::medium.height;

	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };

	engine.drawString(pFont, &bottomLeftText, text, hax::rgb::orange);

	engine.endDraw();

	bench.end();
	bench.printAvg();

	if (GetAsyncKeyState(VK_END) & 1) {
		pSwapBuffersHook->disable();
		const hax::ogl2::twglSwapBuffers pSwapBuffers = reinterpret_cast<hax::ogl2::twglSwapBuffers>(pSwapBuffersHook->getOrigin());
		ReleaseSemaphore(hSemaphore, 1l, nullptr);

		return pSwapBuffers(hDc);
	}

	return reinterpret_cast<hax::ogl2::twglSwapBuffers>(pSwapBuffersHook->getGateway())(hDc);
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

	pSwapBuffersHook = new hax::in::TrampHook("opengl32.dll", "wglSwapBuffers", reinterpret_cast<BYTE*>(hkwglSwapBuffers), 0x5);

	if (!pSwapBuffersHook) {
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pSwapBuffersHook->enable()) {
		delete pSwapBuffersHook;
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: " << std::hex << reinterpret_cast<uintptr_t>(pSwapBuffersHook->getOrigin()) << std::dec << std::endl;

	hSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hSemaphore) {
		delete pSwapBuffersHook;
		fclose(file);
		FreeConsole();

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	WaitForSingleObject(hSemaphore, INFINITE);
	CloseHandle(hSemaphore);
	// just to be save
	Sleep(10ul);

	delete pSwapBuffersHook;
	fclose(file);
	FreeConsole();

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