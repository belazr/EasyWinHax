#pragma once
#include "IDraw.h"

// Class for drawing advanced shapes independent of graphics API.

namespace hax {

	class Engine {
	private:
		const IDraw* _pDraw;

	public:
		Engine(const IDraw* pDraw);

		void drawLine(const Vector2* pos1, const Vector2* pos2, float width, rgb::Color color) const;
		void drawPLine(const Vector2* pos1, const Vector2* pos2, float width, rgb::Color color) const;
		void drawFilledRectangle(const Vector2* pos, float width, float height, rgb::Color color) const;
		void drawString(const Vector2* origin, const char* text, rgb::Color color) const;
		void drawParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, rgb::Color color) const;
		void draw2DBox(const Vector2 bot[2], const Vector2 top[2], float width, rgb::Color color) const;
		void draw3DBox(const Vector2 bot[4], const Vector2 top[4], float width, rgb::Color color) const;
	};

}