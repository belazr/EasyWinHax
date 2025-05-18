#pragma once
#include "..\src\hax.h"

inline void drawDemo(hax::draw::Engine* pEngine, const hax::draw::Color* pTextureData, uint32_t textureWidth, uint32_t textureHeight, bool argb = false) {
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
	pEngine->drawString(&middleOfScreen, hax::draw::Alignment::CENTER_LEFT, TEXT, sizeText, colorText);

	return;
}