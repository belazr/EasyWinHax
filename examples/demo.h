#pragma once
#include "..\src\hax.h"

inline void drawDemo(hax::draw::Engine* pEngine, bool argb = false) {
	const hax::Vector2 middleOfScreen{ pEngine->frameWidth / 2.f, pEngine->frameHeight / 2.f };

	const float widthRect = pEngine->frameWidth / 4.f;
	const float heightRect = pEngine->frameHeight / 4.f;
	const hax::Vector2 topLeftRect{ middleOfScreen.x - widthRect / 2.f, middleOfScreen.y - heightRect / 2.f };
	const hax::draw::Color rectColor = argb ? hax::draw::argb::GRAY : hax::draw::abgr::GRAY;

	pEngine->drawFilledRectangle(&topLeftRect, widthRect, heightRect, rectColor);

	constexpr char TEXT[] = "EasyWinHax";
	const float widthText = _countof(TEXT) * hax::draw::font::medium.width;
	const float heightText = hax::draw::font::medium.height;

	const hax::Vector2 bottomLeftText{ middleOfScreen.x - widthText / 2.f, middleOfScreen.y + heightText / 2.f };
	const hax::draw::Color textColor = argb ? hax::draw::argb::ORANGE : hax::draw::abgr::ORANGE;

	pEngine->drawString(&hax::draw::font::medium, &bottomLeftText, TEXT, textColor);

	return;
}