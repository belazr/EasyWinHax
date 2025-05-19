#include "resource.h"
#include "..\..\demo.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a Vulkan application.
// Compile the project and inject it into a process rendering with Vulkan.
// It is specific for NVIDIA Vulkan runtimes (drivers 555.99) and AMD Vulkan runtimes (drivers 24.7.1).
// Size parameter of tramp hook might have to be adjusted for different drivers/vulkan runtimes.
// The macros AMD or NVIDIA need to be defined depending on the GPU in the system the target application is running on.
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue
// It is important to note that Vulkan uses either ARGB or ABGR as the color format.
// This is dependent on the target application and the colors have to be passed in the correct format to the draw calls.

#define AMD
// #define NVIDIA

static HMODULE hModule;

static hax::Bench bench("200 x hkVkQueuePresentKHR", 200u);

static hax::draw::vk::InitData initData;
static hax::draw::vk::Backend backend;
static hax::draw::Engine engine{ &backend, hax::draw::fonts::inconsolata };

static const hax::draw::Color* pTextureData;
static uint32_t textureWidth;
static uint32_t textureHeight;

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pQueuePresentHook;

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


static VkResult VKAPI_CALL hkVkQueuePresentKHR(VkQueue hQueue, VkPresentInfoKHR* pPresentInfo) {

	if (!pTextureData) {
		loadTextureData();
	}

	bench.start();

	engine.beginFrame(pPresentInfo, initData.hDevice);

	drawDemo(&engine, pTextureData, textureWidth, textureHeight);
	
	engine.endFrame();

	bench.end();
	bench.printAvg();

	if (GetAsyncKeyState(VK_END) & 1) {
		pQueuePresentHook->disable();
		const PFN_vkQueuePresentKHR pQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(pQueuePresentHook->getOrigin());
		VkResult res = pQueuePresentKHR(hQueue, pPresentInfo);
		ReleaseSemaphore(hHookSemaphore, 1l, nullptr);
		
		return res;
	}

	return reinterpret_cast<PFN_vkQueuePresentKHR>(pQueuePresentHook->getGateway())(hQueue, pPresentInfo);
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
		cleanup(hHookSemaphore, pQueuePresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hHookSemaphore) {
		cleanup(hHookSemaphore, pQueuePresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!hax::draw::vk::getInitData(&initData)) {
		cleanup(hHookSemaphore, pQueuePresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	// size and relative address offset are runtime specific
	// look at src\hooks\TrampHook.h and assembly at initData.pVkQueuePresentKHR to figure out correct value
	#if defined(AMD)
	
	constexpr size_t HOOK_SIZE = 0x9u;
	constexpr size_t RELATIVE_ADDRESS_OFFSET = 0x5u;
	
	#elif defined(NVIDIA)

	constexpr size_t HOOK_SIZE = 0x9u;
	constexpr size_t RELATIVE_ADDRESS_OFFSET = SIZE_MAX;
	
	#else

	#error "Define AMD or NVIDIA"
	
	#endif

	pQueuePresentHook = new hax::in::TrampHook(reinterpret_cast<BYTE*>(initData.pVkQueuePresentKHR), reinterpret_cast<BYTE*>(hkVkQueuePresentKHR), HOOK_SIZE, RELATIVE_ADDRESS_OFFSET);

	if (!pQueuePresentHook) {
		cleanup(hHookSemaphore, pQueuePresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pQueuePresentHook->enable()) {
		cleanup(hHookSemaphore, pQueuePresentHook, file, wasConsoleAllocated);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: 0x" << std::hex << reinterpret_cast<uintptr_t>(initData.pVkQueuePresentKHR) << std::dec << std::endl;

	WaitForSingleObject(hHookSemaphore, INFINITE);

	cleanup(hHookSemaphore, pQueuePresentHook, file, wasConsoleAllocated);

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