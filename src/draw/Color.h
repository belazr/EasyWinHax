#pragma once

// Predefined colors.

namespace hax {

	namespace draw {

		typedef unsigned long Color;

		// Macro to encode alpha, red, green and blue byte values in a DWORD
		constexpr Color COLOR_ABGR(unsigned char a, unsigned char r, unsigned char g, unsigned char b) {

			return static_cast<Color>(((((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((r) & 0xFF)));
		}

		// Macro to encode alpha, red, green and blue byte values in a DWORD
		constexpr Color COLOR_ARGB(unsigned char a, unsigned char r, unsigned char g, unsigned char b) {

			return static_cast<Color>(((((a) & 0xFF) << 24) | (((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF)));
		}

		namespace abgr {

			constexpr Color RED = COLOR_ABGR(255, 255, 0, 0);
			constexpr Color ORANGE = COLOR_ABGR(255, 255, 127, 0);
			constexpr Color YELLOW = COLOR_ABGR(255, 255, 255, 0);
			constexpr Color CHARTREUSE = COLOR_ABGR(255, 127, 255, 0);
			constexpr Color GREEN = COLOR_ABGR(255, 0, 255, 0);
			constexpr Color GUPPIE = COLOR_ABGR(255, 0, 255, 127);
			constexpr Color AQUA = COLOR_ABGR(255, 0, 255, 255);
			constexpr Color AZURE = COLOR_ABGR(255, 0, 127, 255);
			constexpr Color BLUE = COLOR_ABGR(255, 0, 0, 255);
			constexpr Color VIOLET = COLOR_ABGR(255, 127, 0, 255);
			constexpr Color FUCHSIA = COLOR_ABGR(255, 255, 0, 255);
			constexpr Color PINK = COLOR_ABGR(255, 255, 0, 127);

			constexpr Color BLACK = COLOR_ABGR(255, 0, 0, 0);
			constexpr Color WHITE = COLOR_ABGR(255, 255, 255, 255);
			constexpr Color GRAY = COLOR_ABGR(255, 127, 127, 127);

		}

		namespace argb {

			constexpr Color RED = COLOR_ARGB(255, 255, 0, 0);
			constexpr Color ORANGE = COLOR_ARGB(255, 255, 127, 0);
			constexpr Color YELLOW = COLOR_ARGB(255, 255, 255, 0);
			constexpr Color CHARTREUSE = COLOR_ARGB(255, 127, 255, 0);
			constexpr Color GREEN = COLOR_ARGB(255, 0, 255, 0);
			constexpr Color GUPPIE = COLOR_ARGB(255, 0, 255, 127);
			constexpr Color AQUA = COLOR_ARGB(255, 0, 255, 255);
			constexpr Color AZURE = COLOR_ARGB(255, 0, 127, 255);
			constexpr Color BLUE = COLOR_ARGB(255, 0, 0, 255);
			constexpr Color VIOLET = COLOR_ARGB(255, 127, 0, 255);
			constexpr Color FUCHSIA = COLOR_ARGB(255, 255, 0, 255);
			constexpr Color PINK = COLOR_ARGB(255, 255, 0, 127);

			constexpr Color BLACK = COLOR_ARGB(255, 0, 0, 0);
			constexpr Color WHITE = COLOR_ARGB(255, 255, 255, 255);
			constexpr Color GRAY = COLOR_ARGB(255, 127, 127, 127);

		}

	}

}