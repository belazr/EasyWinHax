#pragma once
#include "..\vecmath.h"
#include <Windows.h>

// Interface for drawing with graphics APIs within a hook.
// The appropriate implementation should be instantiated and passed to an engine object.
// All methods are intended to be called by an Engine object and not for direct calls.

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
		#define FLOAT_A(color) (static_cast<float>((color >> 24) & 0xFF) / 255.f)
		#define FLOAT_R(color) (static_cast<float>((color >> 16) & 0xFF) / 255.f)
		#define FLOAT_G(color) (static_cast<float>((color >> 8) & 0xFF) / 255.f)
		#define FLOAT_B(color) (static_cast<float>((color) & 0xFF) / 255.f)

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
		// Initializes drawing within a hook. Should be called by an Engine object.
		//
		// Parameters:
		// 
		// [in] pEngine:
		// Pointer to the Engine object responsible for drawing within the hook.
		virtual void beginDraw(const Engine* pEngine) = 0;

		// Ends drawing within a hook. Should be called by an Engine object.
		// 
		// [in] pEngine:
		// Pointer to the Engine object responsible for drawing within the hook.
		virtual void endDraw(const Engine* pEngine) const = 0;

		// Draws a filled triangle strip. Should be called by an Engine object.
		// 
		// Parameters:
		// 
		// [in] corners:
		// Screen coordinates of the corners of the triangle strip.
		// 
		// [in] count:
		// Count of the corners of the triangle strip.
		// 
		// [in]
		// Color of the triangle strip.
		virtual void drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const = 0;

		// Draws text to the screen. Should be called by an Engine object.
		//
		// Parameters:
		// 
		// [in] pFont:
		// Pointer to an appropriate Font object. ogl2::Font* for OpenGL 2 hooks and dx9::Font* for DirectX 9 hooks.
		//
		// [in] origin:
		// Coordinates of the bottom left corner of the first character of the text.
		// 
		// [in] text:
		// Text to be drawn. See length limitations implementations.
		//
		// [in] color:
		// Color of the text.
		virtual void drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const = 0;
	};

}
