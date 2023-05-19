#pragma once
#include "..\vecmath.h"
#include <Windows.h>

// Interface for drawing with graphics APIs within a hook.

namespace hax{

	// Predefined colors.
	namespace rgb {

		typedef DWORD Color;

		// Macro to encode ARGB values in a DWORD
		#define COLOR_ARGB(a,r,g,b) (static_cast<Color>(((((a) & 0xFF) << 24) | (((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF))))
		// Macros to extract the ARGB values from a DWORD
		#define UCHAR_A(color) (static_cast<UCHAR>(color >> 24))
		#define UCHAR_R(color) (static_cast<UCHAR>(color >> 16))
		#define UCHAR_G(color) (static_cast<UCHAR>(color >> 8))
		#define UCHAR_B(color) (static_cast<UCHAR>(color))

		constexpr Color red = COLOR_ARGB(255, 255, 0, 0);
		constexpr Color orange = COLOR_ARGB(255, 255, 127, 0);
		constexpr Color yellow = COLOR_ARGB(255, 255, 255, 0);
		constexpr Color chartreuse = COLOR_ARGB(255, 127, 255, 0);
		constexpr Color green = COLOR_ARGB(255, 0, 255, 0);
		constexpr Color guppie = COLOR_ARGB(255, 0, 255, 127);
		constexpr Color aqua = COLOR_ARGB(255, 0, 255, 255);
		constexpr Color azure = COLOR_ARGB(255, 0, 127, 255);
		constexpr Color blue = COLOR_ARGB(255, 0, 0, 255);
		constexpr Color violet = COLOR_ARGB(255, 127, 0, 255);
		constexpr Color fuchsia = COLOR_ARGB(255, 255, 0, 255);
		constexpr Color pink = COLOR_ARGB(255, 255, 0, 127);

		constexpr Color black = COLOR_ARGB(255, 0, 0, 0);
		constexpr Color white = COLOR_ARGB(255, 255, 255, 255);
		constexpr Color gray = COLOR_ARGB(255, 127, 127, 127);

	}

	class Engine;

	class IDraw {
	public:
		virtual void beginDraw(const Engine* pEngine) = 0;
		virtual void endDraw(const Engine* pEngine) const = 0;
		virtual void drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const = 0;
		virtual void drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const = 0;
	};

}
