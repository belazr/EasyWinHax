#include "Engine.h"

namespace hax {

	Engine::Engine(IBackend* pBackend) : _pBackend{ pBackend }, pHookArg1{}, pHookArg2{}, pHookArg3{}, frameWidth {}, frameHeight{} {}


	void Engine::beginFrame(void* pArg1, const void* pArg2, void* pArg3) {
		this->pHookArg1 = pArg1;
		this->pHookArg2 = pArg2;
		this->pHookArg3 = pArg3;

		this->_pBackend->beginFrame(this);
		this->_pBackend->getFrameResolution(&this->frameWidth, &this->frameHeight);

		return;
	}


	void Engine::endFrame() {
		this->_pBackend->endFrame(this);

		return;
	}


	void Engine::drawLine(const Vector2* pos1, const Vector2* pos2, float width, rgb::Color color) const {
		// defaults for horizontal line
		float cosAtan = 0.f;
		float sinAtan = width / 2.f;

		// trigonometry black magic to calculate the corners of the line/rectangle
		if (pos1->y - pos2->y) {
			const float omega = (pos2->x - pos1->x) / (pos1->y - pos2->y);
			const float oSqrt = sqrtf(1.f + omega * omega);

			cosAtan = width / 2.f / oSqrt;
			sinAtan = width / 2.f * omega / oSqrt;

			// has to be drawn from bottom to top
			if (pos1->y < pos2->y) {
				const Vector2* tmp = pos1;
				pos1 = pos2;
				pos2 = tmp;
			}
		}
		// horizontal has to be drawn from left to right
		else if (pos2->x < pos1->x) {
			const Vector2* tmp = pos1;
			pos1 = pos2;
			pos2 = tmp;
		}

		Vector2 corners[6]{};
		corners[0].x = pos1->x - cosAtan;
		corners[0].y = pos1->y - sinAtan;
		corners[1].x = pos2->x - cosAtan;
		corners[1].y = pos2->y - sinAtan;
		corners[2].x = pos1->x + cosAtan;
		corners[2].y = pos1->y + sinAtan;

		corners[3].x = pos2->x + cosAtan;
		corners[3].y = pos2->y + sinAtan;
		corners[4].x = pos1->x + cosAtan;
		corners[4].y = pos1->y + sinAtan;
		corners[5].x = pos2->x - cosAtan;
		corners[5].y = pos2->y - sinAtan;

		this->_pBackend->drawTriangleList(corners, 6, color);
	}


	void Engine::drawPLine(const Vector2* pos1, const Vector2* pos2, float width, rgb::Color color) const {
		// only makes sense if not horizontal
		if (!(pos1->y - pos2->y)) return;

		// trigonometry black magic to calculate the corners of the line/parallelogram
		const float omega = (pos2->x - pos1->x) / (pos1->y - pos2->y);
		const float oSqrt = sqrtf(1.f + omega * omega);
		const float cosAtan = width / 2 / oSqrt;
		const float sinAtan = width / 2 * omega / oSqrt;

		// has to be drawn from bottom to top
		if (pos1->y - pos2->y < 0.f) {
			const Vector2* tmp = pos1;
			pos1 = pos2;
			pos2 = tmp;
		}

		Vector2 corners[6]{};
		corners[0].x = pos1->x - cosAtan - omega * sinAtan;
		corners[0].y = pos1->y;
		corners[1].x = pos2->x - cosAtan - omega * sinAtan;
		corners[1].y = pos2->y;
		corners[2].x = pos1->x + cosAtan + omega * sinAtan;
		corners[2].y = pos1->y;
		
		corners[3].x = pos2->x + cosAtan + omega * sinAtan;
		corners[3].y = pos2->y;
		corners[4].x = pos1->x + cosAtan + omega * sinAtan;
		corners[4].y = pos1->y;
		corners[5].x = pos2->x - cosAtan - omega * sinAtan;
		corners[5].y = pos2->y;

		this->_pBackend->drawTriangleList(corners, 6, color);
	}


	void Engine::drawFilledRectangle(const Vector2* pos, float width, float height, rgb::Color color) const {
		Vector2 corners[6]{};
		corners[0].x = pos->x;
		corners[0].y = pos->y;
		corners[1].x = pos->x + width;
		corners[1].y = pos->y;
		corners[2].x = pos->x;
		corners[2].y = pos->y + height;
		
		corners[3].x = pos->x + width;
		corners[3].y = pos->y + height;
		corners[4].x = pos->x;
		corners[4].y = pos->y + height;
		corners[5].x = pos->x + width;
		corners[5].y = pos->y;

		this->_pBackend->drawTriangleList(corners, 6, color);
	}


	void Engine::drawString(const font::Font* pFont, const Vector2* pos, const char* text, rgb::Color color) const {
		
		if (!pFont) return;
		
		const size_t size = strlen(text);

		for (size_t i = 0; i < size; i++) {
			const char c = text[i];

			if (c == ' ') continue;

			const font::CharIndex index = font::charToCharIndex(c);
			const font::Char* pCurChar = &pFont->chars[index];

			if (pCurChar) {
				// current char x coordinate is offset by width of previously drawn chars plus two pixels spacing per char
				const Vector2 curPos{ pos->x + (pFont->width + 2.f) * i, pos->y - pFont->height };
				this->_pBackend->drawPointList(pCurChar->body.coordinates, pCurChar->body.count, color, curPos);
				this->_pBackend->drawPointList(pCurChar->outline.coordinates, pCurChar->outline.count, rgb::black, curPos);
			}

		}

		return;
	}


	void Engine::drawParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, rgb::Color color) const {
		const float height = bot->y - top->y;

		// corners coordinates of the parallelogram
		Vector2 bl{ bot->x - height / (2.f * ratio), bot->y };
		Vector2 br{ bot->x + height / (2.f * ratio), bot->y };
		Vector2 tl{ top->x - height / (2.f * ratio), top->y };
		Vector2 tr{ top->x + height / (2.f * ratio), top->y };

		//top
		this->drawLine(&tl, &tr, width, color);
		//bottom
		this->drawLine(&bl, &br, width, color);

		// columns
		// have to be offset by half of the line width for a clean overlap at the corners
		const float halfWidth = width / 2.f;
		tl.y -= halfWidth;
		tr.y -= halfWidth;
		bl.y += halfWidth;
		br.y += halfWidth;
		this->drawPLine(&bl, &tl, width, color);
		this->drawPLine(&br, &tr, width, color);

		return;
	}


	void Engine::draw2DBox(const Vector2 bot[2], const Vector2 top[2], float width, rgb::Color color) const {
		this->drawLine(&top[0], &top[1], width, color);
		this->drawLine(&bot[0], &bot[1], width, color);

		// columns
		// have to be offset by half of the line width for a clean overlap at the corners
		const float halfWidth = width / 2.f;

		for (int i = 0; i < 2; i++) {
			const Vector2 botCol{ bot[i].x, bot[i].y + halfWidth };
			const Vector2 topCol{ top[i].x, top[i].y - halfWidth };
			this->drawLine(&botCol, &topCol, width, color);
		}

		return;
	}


	void Engine::draw3DBox(const Vector2 bot[4], const Vector2 top[4], float width, rgb::Color color) const {
		// top face
		this->drawLine(&top[0], &top[1], width, color);
		this->drawLine(&top[1], &top[2], width, color);
		this->drawLine(&top[2], &top[3], width, color);
		this->drawLine(&top[3], &top[0], width, color);

		// bottom face
		this->drawLine(&bot[0], &bot[1], width, color);
		this->drawLine(&bot[1], &bot[2], width, color);
		this->drawLine(&bot[2], &bot[3], width, color);
		this->drawLine(&bot[3], &bot[0], width, color);

		// columns
		// have to be offset by half of the line width for a clean overlap at the corners
		const float halfWidth = width / 2.f;

		for (int i = 0; i < 4; i++) {
			const Vector2 botCol{ bot[i].x, bot[i].y + halfWidth };
			const Vector2 topCol{ top[i].x, top[i].y - halfWidth };
			this->drawLine(&botCol, &topCol, width, color);
		}

		return;
	}

}