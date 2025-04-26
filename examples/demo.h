#pragma once
#include "..\src\hax.h"

inline void drawDemo(hax::draw::Engine* pEngine, const hax::draw::Color* pTextureData, uint32_t textureWidth, uint32_t textureHeight, bool argb = false) {
	const hax::Vector2 middleOfScreen{ pEngine->frameWidth / 2.f, pEngine->frameHeight / 2.f };

	static hax::draw::TextureId textureId = pEngine->loadTexture(pTextureData, textureWidth, textureHeight);

	if (textureId) {
		const float widthTextureRect = pEngine->frameWidth / 4.f;
		const float heightTextureRect = widthTextureRect * static_cast<float>(textureHeight) / static_cast<float>(textureWidth);
		const hax::Vector2 topLeftTexture{ middleOfScreen.x - widthTextureRect / 2.f, middleOfScreen.y - heightTextureRect / 2.f };

		pEngine->drawTexture(textureId, &topLeftTexture, widthTextureRect, heightTextureRect);
	}

	constexpr char TEXT[] = "EasyWinHax";
	const float widthText = _countof(TEXT) * hax::draw::font::medium.width;
	const float heightText = hax::draw::font::medium.height;
	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };
	const hax::draw::Color colorText = argb ? hax::draw::argb::ORANGE : hax::draw::abgr::ORANGE;

	pEngine->drawString(&hax::draw::font::medium, &bottomLeftText, TEXT, colorText);

	return;
}