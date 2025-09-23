#pragma once
#include "..\src\hax.h"

inline void drawDemo(hax::draw::Engine* pEngine, const hax::draw::Color* pTextureData, uint32_t textureWidth, uint32_t textureHeight, bool argb = false) {
	
	// include imgui.h in this file BEFORE hax.h and add the ImGui source files:
	// imconfig.h, imgui.cpp, imgui.h, imgui_demo.cpp, imgui_draw.cpp, imgui_internal.h, imgui_tables.cpp, imgui_widgets.cpp
	// to the example projects to make the examples build rendering the ImGui demo window
	#ifdef IMGUI_VERSION

	UNREFERENCED_PARAMETER(pTextureData);
	UNREFERENCED_PARAMETER(textureWidth);
	UNREFERENCED_PARAMETER(textureHeight);
	UNREFERENCED_PARAMETER(argb);

	static bool init = false;

	if (!init) {
		
		if (!IMGUI_CHECKVERSION()) return;

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
		ImGui::StyleColorsLight();

		init = true;
	}
	
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = pEngine->frameWidth;
	io.DisplaySize.y = pEngine->frameHeight;

	// set mouse cursor position in ImGui overlay
	const HWND hWnd = hax::proc::in::getMainWindowHandle();
	
	if (!hWnd) return;
	
	POINT pos{};

	if (!GetCursorPos(&pos)) return;

	if (!ScreenToClient(hWnd, &pos)) return;

	io.AddMousePosEvent(static_cast<float>(pos.x), static_cast<float>(pos.y));

	// enable clicks in ImGui overlay
	if (GetAsyncKeyState(VK_LBUTTON)) {
		io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
	}
	else {
		io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
	}

	ImGui::NewFrame();
	
	static bool showDemo = true;

	if (showDemo) {
		ImGui::ShowDemoWindow(&showDemo);
	}
	
	ImGui::EndFrame();
	ImGui::Render();

	pEngine->drawImGuiDrawData(ImGui::GetDrawData());

	#else

	const hax::Vector2 middleOfScreen{ pEngine->frameWidth / 2.f, pEngine->frameHeight / 2.f };

	static hax::draw::TextureId textureId = pEngine->loadTexture(pTextureData, textureWidth, textureHeight);

	if (textureId) {
		const float widthTextureRect = pEngine->frameWidth / 4.f;
		const float heightTextureRect = widthTextureRect * static_cast<float>(textureHeight) / static_cast<float>(textureWidth);

		pEngine->drawTexture(textureId, &middleOfScreen, hax::draw::Alignment::CENTER, widthTextureRect, heightTextureRect);
	}

	constexpr char TEXT[] = "EasyWinHax";
	constexpr uint32_t sizeText = 24u;
	const hax::draw::Color colorText = argb ? hax::draw::argb::ORANGE : hax::draw::abgr::ORANGE;
	pEngine->drawString(&middleOfScreen, hax::draw::Alignment::CENTER, TEXT, sizeText, colorText);

	#endif // IMGUI_VERSION

	return;
}