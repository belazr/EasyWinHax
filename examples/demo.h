#pragma once
#include "demoTexture.h"

inline void drawDemo(hax::draw::Engine* pEngine, bool argb = false) {
	static hax::draw::TextureId textureId = pEngine->loadTexture(DEMO_TEXTURE, DEMO_TEXTURE_WIDTH, DEMO_TEXTURE_HEIGHT);
	
	const hax::Vector2 middleOfScreen{ pEngine->frameWidth / 2.f, pEngine->frameHeight / 2.f };

	if (textureId) {
		const float widthTexture = pEngine->frameWidth / 2.f;
		const float heightTexture = widthTexture * static_cast<float>(DEMO_TEXTURE_HEIGHT) / static_cast<float>(DEMO_TEXTURE_WIDTH);
		const hax::Vector2 topLeftTexture{ middleOfScreen.x - widthTexture / 2.f, middleOfScreen.y - heightTexture / 2.f };

		pEngine->drawTexture(textureId, &topLeftTexture, widthTexture, heightTexture);
	}

	constexpr char TEXT[] = "EasyWinHax";
	const float widthText = _countof(TEXT) * hax::draw::font::medium.width;
	const float heightText = hax::draw::font::medium.height;
	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };
	const hax::draw::Color colorText = argb ? hax::draw::argb::ORANGE : hax::draw::abgr::ORANGE;

	pEngine->drawString(&hax::draw::font::medium, &bottomLeftText, TEXT, colorText);

	return;
}