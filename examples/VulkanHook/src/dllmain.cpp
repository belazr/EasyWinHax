#include "..\..\..\src\hax.h"
#include <iostream>

// This is a example DLL for hooking and drawing within a Vulkan application.
// Compile the project and inject it into a process rendering with Vulkan.
// It is specific for NVIDIA vulkan runtimes (drivers 555.99).
// Size parameter of tramp hook might have to be adjusted for different drivers/vulkan runtimes.
// It is not working for AMD GPUs/vulkan runtimes!
// A dll-injector built with EasyWinHax can be found here:
// https://github.com/belazr/JackieBlue

static hax::Bench bench("200 x hkVkQueuePresentKHR", 200u);

static hax::draw::vk::VulkanInitData initData;
static hax::draw::vk::Backend backend;
static hax::draw::Engine engine{ &backend };

static HANDLE hHookSemaphore;
static hax::in::TrampHook* pQueuePresentHook;

VkResult VKAPI_CALL hkVkQueuePresentKHR(VkQueue hQueue, const VkPresentInfoKHR* pPresentInfo) {
	bench.start();

	engine.beginFrame(hQueue, pPresentInfo, initData.hDevice);

	const hax::Vector2 middleOfScreen{ engine.frameWidth / 2.f, engine.frameHeight / 2.f };

	const float widthRect = engine.frameWidth / 4.f;
	const float heightRect = engine.frameHeight / 4.f;
	const hax::Vector2 topLeftRect{ middleOfScreen.x - widthRect / 2.f, middleOfScreen.y - heightRect / 2.f };
	
	engine.drawFilledRectangle(&topLeftRect, widthRect, heightRect, hax::draw::rgb::gray);

	constexpr char TEXT[] = "EasyWinHax";
	const float widthText = _countof(TEXT) * hax::draw::font::medium.width;
	const float heightText = hax::draw::font::medium.height;
	
	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };
	
	engine.drawString(&hax::draw::font::medium, &bottomLeftText, TEXT, hax::draw::rgb::orange);

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
		cleanup(hHookSemaphore, pQueuePresentHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

	if (!hHookSemaphore) {
		cleanup(hHookSemaphore, pQueuePresentHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!hax::draw::vk::getVulkanInitData(&initData)) {
		cleanup(hHookSemaphore, pQueuePresentHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	// size 0x9 only applies for NVIDIA gpus an might be different with futures drivers/vulkan runtimes
	// look at src\hooks\TrampHook.h and assembly at initData.pVkQueuePresentKHR to figure out correct value
	pQueuePresentHook = new hax::in::TrampHook(reinterpret_cast<BYTE*>(initData.pVkQueuePresentKHR), reinterpret_cast<BYTE*>(hkVkQueuePresentKHR), 0x9);

	if (!pQueuePresentHook) {
		cleanup(hHookSemaphore, pQueuePresentHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	if (!pQueuePresentHook->enable()) {
		cleanup(hHookSemaphore, pQueuePresentHook, file);

		FreeLibraryAndExitThread(hModule, 0ul);
	}

	std::cout << "Hooked at: 0x" << std::hex << reinterpret_cast<uintptr_t>(initData.pVkQueuePresentKHR) << std::dec << std::endl;

	WaitForSingleObject(hHookSemaphore, INFINITE);

	cleanup(hHookSemaphore, pQueuePresentHook, file);

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