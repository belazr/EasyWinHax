#pragma once
#include "..\..\..\vecmath.h"

// These structs can be used to draw characters via the Font class.
// Their main purpose is to avoid using the deprecated legacy DirectX SDK for text redndering.

namespace hax {

	namespace dx9 {
		// Gets the count of pixels in a pixel array of a character
		#define PIXEL_COUNT(charpixels) (sizeof(charpixels) / sizeof(Vector2))

		// Holds pixel coordinates and size in pixels of a single character.
		// Only monochrome characters are supported.
		typedef struct Fontchar {
			const Vector2* pixel;
			const unsigned int pixelCount;
		}Fontchar;

		// Holds all characters that can be rendered by the Font class and the dimensions of any single character (always monospaced).
		typedef struct Charset {
			const Fontchar exclamationPoint;
			const Fontchar doubleQuotes;
			const Fontchar poundSign;
			const Fontchar dollarSign;
			const Fontchar percentSign;
			const Fontchar ampersand;
			const Fontchar singleQuote;
			const Fontchar openParanthesis;
			const Fontchar closeParanthesis;
			const Fontchar asterisk;
			const Fontchar plus;
			const Fontchar comma;
			const Fontchar dash;
			const Fontchar period;
			const Fontchar slash;
			const Fontchar zero;
			const Fontchar one;
			const Fontchar two;
			const Fontchar three;
			const Fontchar four;
			const Fontchar five;
			const Fontchar six;
			const Fontchar seven;
			const Fontchar eight;
			const Fontchar nine;
			const Fontchar colon;
			const Fontchar semicolon;
			const Fontchar smaller;
			const Fontchar equals;
			const Fontchar greater;
			const Fontchar questionmark;
			const Fontchar atSign;
			const Fontchar upperA;
			const Fontchar upperB;
			const Fontchar upperC;
			const Fontchar upperD;
			const Fontchar upperE;
			const Fontchar upperF;
			const Fontchar upperG;
			const Fontchar upperH;
			const Fontchar upperI;
			const Fontchar upperJ;
			const Fontchar upperK;
			const Fontchar upperL;
			const Fontchar upperM;
			const Fontchar upperN;
			const Fontchar upperO;
			const Fontchar upperP;
			const Fontchar upperQ;
			const Fontchar upperR;
			const Fontchar upperS;
			const Fontchar upperT;
			const Fontchar upperU;
			const Fontchar upperV;
			const Fontchar upperW;
			const Fontchar upperX;
			const Fontchar upperY;
			const Fontchar upperZ;
			const Fontchar openBracket;
			const Fontchar backslash;
			const Fontchar closeBracket;
			const Fontchar circumflex;
			const Fontchar underscore;
			const Fontchar backtick;
			const Fontchar lowerA;
			const Fontchar lowerB;
			const Fontchar lowerC;
			const Fontchar lowerD;
			const Fontchar lowerE;
			const Fontchar lowerF;
			const Fontchar lowerG;
			const Fontchar lowerH;
			const Fontchar lowerI;
			const Fontchar lowerJ;
			const Fontchar lowerK;
			const Fontchar lowerL;
			const Fontchar lowerM;
			const Fontchar lowerN;
			const Fontchar lowerO;
			const Fontchar lowerP;
			const Fontchar lowerQ;
			const Fontchar lowerR;
			const Fontchar lowerS;
			const Fontchar lowerT;
			const Fontchar lowerU;
			const Fontchar lowerV;
			const Fontchar lowerW;
			const Fontchar lowerX;
			const Fontchar lowerY;
			const Fontchar lowerZ;
			const Fontchar openBrace;
			const Fontchar pipe;
			const Fontchar closeBrace;
			const Fontchar tilde;
			const Fontchar degrees;
			const float height;
			const float width;
		}Charset;

		constexpr size_t CHAR_COUNT = (sizeof(Charset) - 2 * sizeof(float)) / sizeof(Fontchar);

		namespace charsets {

			// Charsets for external usage in three different sizes.
			extern const Charset tiny;
			extern const Charset medium;
			extern const Charset large;

		}

	}

}