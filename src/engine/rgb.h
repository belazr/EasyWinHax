#pragma once

// Predefined colors.

namespace hax {
	
	namespace rgb {

		typedef unsigned long Color;

		// Macro to encode ARGB values in a DWORD
		#define COLOR_ARGB(a,r,g,b) (static_cast<Color>(((((a) & 0xFF) << 24) | (((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF))))
		// Macros to extract the ARGB values from a DWORD
		#define UCHAR_A(color) (static_cast<unsigned char>(color >> 24))
		#define UCHAR_R(color) (static_cast<unsigned char>(color >> 16))
		#define UCHAR_G(color) (static_cast<unsigned char>(color >> 8))
		#define UCHAR_B(color) (static_cast<unsigned char>(color))
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

}