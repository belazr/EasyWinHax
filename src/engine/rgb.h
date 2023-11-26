#pragma once

// Predefined colors.

namespace hax {
	
	namespace rgb {

		typedef unsigned long Color;

		// Macro to encode ABGR values in a DWORD
		#define COLOR_ABGR(a,b,g,r) (static_cast<Color>(((((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((r) & 0xFF))))
		
		// Macros to extract the ABGR values from a DWORD
		#define COLOR_UCHAR_A(color) (static_cast<unsigned char>(color >> 24))
		#define COLOR_UCHAR_B(color) (static_cast<unsigned char>(color >> 16))
		#define COLOR_UCHAR_G(color) (static_cast<unsigned char>(color >> 8))
		#define COLOR_UCHAR_R(color) (static_cast<unsigned char>(color))

		// Macro to convert ABGR Color values to ARGB Color values
		#define COLOR_ABGR_TO_ARGB(abgr) (static_cast<rgb::Color>(COLOR_UCHAR_A(color) << 24 | COLOR_UCHAR_R(color) << 16 | COLOR_UCHAR_G(color) << 8 | COLOR_UCHAR_B(color)))

		constexpr Color red = COLOR_ABGR(255, 0, 0, 255);
		constexpr Color orange = COLOR_ABGR(255, 0, 127, 255);
		constexpr Color yellow = COLOR_ABGR(255, 0, 255, 255);
		constexpr Color chartreuse = COLOR_ABGR(255, 0, 255, 127);
		constexpr Color green = COLOR_ABGR(255, 0, 255, 0);
		constexpr Color guppie = COLOR_ABGR(255, 127, 255, 0);
		constexpr Color aqua = COLOR_ABGR(255, 255, 255, 0);
		constexpr Color azure = COLOR_ABGR(255, 255, 127, 0);
		constexpr Color blue = COLOR_ABGR(255, 255, 0, 0);
		constexpr Color violet = COLOR_ABGR(255, 255, 0, 127);
		constexpr Color fuchsia = COLOR_ABGR(255, 255, 0, 255);
		constexpr Color pink = COLOR_ABGR(255, 127, 0, 255);

		constexpr Color black = COLOR_ABGR(255, 0, 0, 0);
		constexpr Color white = COLOR_ABGR(255, 255, 255, 255);
		constexpr Color gray = COLOR_ABGR(255, 127, 127, 127);

	}

}