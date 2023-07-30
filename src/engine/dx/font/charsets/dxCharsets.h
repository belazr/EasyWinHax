#pragma once
#include "..\..\..\..\vecmath.h"

// These structs can be used to draw characters via the Font class.
// Their main purpose is to avoid using the deprecated legacy DirectX SDK for text redndering.

namespace hax {

	namespace dx {
		
		// Holds pixel coordinates and size in pixels of a single character.
		// Only monochrome characters are supported.
		typedef struct Fontchar {
			const Vector2* pixel;
			const unsigned int pixelCount;
		}Fontchar;

		typedef enum CharIndex {
			EXCLAMATION_POINT = 0,
			DOUBLE_QUOTES,
			POUND_SIGN,
			DOLLAR_SIGN,
			PERCENT_SIGN,
			AMPERSAND,
			SINGLE_QUOTE,
			OPEN_PARANTHESIS,
			CLOSE_PARANTHESIS,
			ASTERISK,
			PLUS,
			COMMA,
			DASH,
			PERIOD,
			SLASH,
			ZERO,
			ONE,
			TWO,
			THREE,
			FOUR,
			FIVE,
			SIX,
			SEVEN,
			EIGHT,
			NINE,
			COLON,
			SEMICOLON,
			SMALLER,
			EQUALS,
			GREATER,
			QUESTIONMARK,
			AT_SIGN,
			UPPER_A,
			UPPER_B,
			UPPER_C,
			UPPER_D,
			UPPER_E,
			UPPER_F,
			UPPER_G,
			UPPER_H,
			UPPER_I,
			UPPER_J,
			UPPER_K,
			UPPER_L,
			UPPER_M,
			UPPER_N,
			UPPER_O,
			UPPER_P,
			UPPER_Q,
			UPPER_R,
			UPPER_S,
			UPPER_T,
			UPPER_U,
			UPPER_V,
			UPPER_W,
			UPPER_X,
			UPPER_Y,
			UPPER_Z,
			OPEN_BRACKET,
			BACKSLASH,
			CLOSE_BRACKET,
			CIRCUMFLEX,
			UNDERSCORE,
			BACKTICK,
			LOWER_A,
			LOWER_B,
			LOWER_C,
			LOWER_D,
			LOWER_E,
			LOWER_F,
			LOWER_G,
			LOWER_H,
			LOWER_I,
			LOWER_J,
			LOWER_K,
			LOWER_L,
			LOWER_M,
			LOWER_N,
			LOWER_O,
			LOWER_P,
			LOWER_Q,
			LOWER_R,
			LOWER_S,
			LOWER_T,
			LOWER_U,
			LOWER_V,
			LOWER_W,
			LOWER_X,
			LOWER_Y,
			LOWER_Z,
			OPEN_BRACE,
			PIPE,
			CLOSE_BRACE,
			TILDE,
			DEGREES,
			MAX_CHAR
		}CharIndex;

		// Holds all characters that can be rendered by the Font class and the dimensions of any single character (always monospaced).
		typedef struct Charset {
			const Fontchar chars[CharIndex::MAX_CHAR];
			const float height;
			const float width;
		}Charset;

		constexpr CharIndex charToCharIndex(char c) {
			CharIndex index = CharIndex::MAX_CHAR;
			
			switch (c) {
			case '!':
				index = CharIndex::EXCLAMATION_POINT;
				break;
			case '"':
				index = CharIndex::DOUBLE_QUOTES;
				break;
			case '#':
				index = CharIndex::POUND_SIGN;
				break;
			case '$':
				index = CharIndex::DOLLAR_SIGN;
				break;
			case '%':
				index = CharIndex::PERCENT_SIGN;
				break;
			case '&':
				index = CharIndex::AMPERSAND;
				break;
			case '\'':
				index = CharIndex::SINGLE_QUOTE;
				break;
			case '(':
				index = CharIndex::OPEN_PARANTHESIS;
				break;
			case ')':
				index = CharIndex::CLOSE_PARANTHESIS;
				break;
			case '*':
				index = CharIndex::ASTERISK;
				break;
			case '+':
				index = CharIndex::PLUS;
				break;
			case ',':
				index = CharIndex::COMMA;
				break;
			case '-':
				index = CharIndex::DASH;
				break;
			case '.':
				index = CharIndex::PERIOD;
				break;
			case '/':
				index = CharIndex::SLASH;
				break;
			case '0':
				index = CharIndex::ZERO;
				break;
			case '1':
				index = CharIndex::ONE;
				break;
			case '2':
				index = CharIndex::TWO;
				break;
			case '3':
				index = CharIndex::THREE;
				break;
			case '4':
				index = CharIndex::FOUR;
				break;
			case '5':
				index = CharIndex::FIVE;
				break;
			case '6':
				index = CharIndex::SIX;
				break;
			case '7':
				index = CharIndex::SEVEN;
				break;
			case '8':
				index = CharIndex::EIGHT;
				break;
			case '9':
				index = CharIndex::NINE;
				break;
			case ':':
				index = CharIndex::COLON;
				break;
			case ';':
				index = CharIndex::SEMICOLON;
				break;
			case '<':
				index = CharIndex::SMALLER;
				break;
			case '=':
				index = CharIndex::EQUALS;
				break;
			case '>':
				index = CharIndex::GREATER;
				break;
			case '?':
				index = CharIndex::QUESTIONMARK;
				break;
			case '@':
				index = CharIndex::AT_SIGN;
				break;
			case 'A':
				index = CharIndex::UPPER_A;
				break;
			case 'B':
				index = CharIndex::UPPER_B;
				break;
			case 'C':
				index = CharIndex::UPPER_C;
				break;
			case 'D':
				index = CharIndex::UPPER_D;
				break;
			case 'E':
				index = CharIndex::UPPER_E;
				break;
			case 'F':
				index = CharIndex::UPPER_F;
				break;
			case 'G':
				index = CharIndex::UPPER_G;
				break;
			case 'H':
				index = CharIndex::UPPER_H;
				break;
			case 'I':
				index = CharIndex::UPPER_I;
				break;
			case 'J':
				index = CharIndex::UPPER_J;
				break;
			case 'K':
				index = CharIndex::UPPER_K;
				break;
			case 'L':
				index = CharIndex::UPPER_L;
				break;
			case 'M':
				index = CharIndex::UPPER_M;
				break;
			case 'N':
				index = CharIndex::UPPER_N;
				break;
			case 'O':
				index = CharIndex::UPPER_O;
				break;
			case 'P':
				index = CharIndex::UPPER_P;
				break;
			case 'Q':
				index = CharIndex::UPPER_Q;
				break;
			case 'R':
				index = CharIndex::UPPER_R;
				break;
			case 'S':
				index = CharIndex::UPPER_S;
				break;
			case 'T':
				index = CharIndex::UPPER_T;
				break;
			case 'U':
				index = CharIndex::UPPER_U;
				break;
			case 'V':
				index = CharIndex::UPPER_V;
				break;
			case 'W':
				index = CharIndex::UPPER_W;
				break;
			case 'X':
				index = CharIndex::UPPER_X;
				break;
			case 'Y':
				index = CharIndex::UPPER_Y;
				break;
			case 'Z':
				index = CharIndex::UPPER_Z;
				break;
			case '[':
				index = CharIndex::OPEN_BRACKET;
				break;
			case '\\':
				index = CharIndex::BACKSLASH;
				break;
			case ']':
				index = CharIndex::CLOSE_BRACKET;
				break;
			case '^':
				index = CharIndex::CIRCUMFLEX;
				break;
			case '_':
				index = CharIndex::UNDERSCORE;
				break;
			case '`':
				index = CharIndex::BACKTICK;
				break;
			case 'a':
				index = CharIndex::LOWER_A;
				break;
			case 'b':
				index = CharIndex::LOWER_B;
				break;
			case 'c':
				index = CharIndex::LOWER_C;
				break;
			case 'd':
				index = CharIndex::LOWER_D;
				break;
			case 'e':
				index = CharIndex::LOWER_E;
				break;
			case 'f':
				index = CharIndex::LOWER_F;
				break;
			case 'g':
				index = CharIndex::LOWER_G;
				break;
			case 'h':
				index = CharIndex::LOWER_H;
				break;
			case 'i':
				index = CharIndex::LOWER_I;
				break;
			case 'j':
				index = CharIndex::LOWER_J;
				break;
			case 'k':
				index = CharIndex::LOWER_K;
				break;
			case 'l':
				index = CharIndex::LOWER_L;
				break;
			case 'm':
				index = CharIndex::LOWER_M;
				break;
			case 'n':
				index = CharIndex::LOWER_N;
				break;
			case 'o':
				index = CharIndex::LOWER_O;
				break;
			case 'p':
				index = CharIndex::LOWER_P;
				break;
			case 'q':
				index = CharIndex::LOWER_Q;
				break;
			case 'r':
				index = CharIndex::LOWER_R;
				break;
			case 's':
				index = CharIndex::LOWER_S;
				break;
			case 't':
				index = CharIndex::LOWER_T;
				break;
			case 'u':
				index = CharIndex::LOWER_U;
				break;
			case 'v':
				index = CharIndex::LOWER_V;
				break;
			case 'w':
				index = CharIndex::LOWER_W;
				break;
			case 'x':
				index = CharIndex::LOWER_X;
				break;
			case 'y':
				index = CharIndex::LOWER_Y;
				break;
			case 'z':
				index = CharIndex::LOWER_Z;
				break;
			case '{':
				index = CharIndex::OPEN_BRACE;
				break;
			case '|':
				index = CharIndex::PIPE;
				break;
			case '}':
				index = CharIndex::CLOSE_BRACE;
				break;
			case '~':
				index = CharIndex::TILDE;
				break;
			case '°':
				index = CharIndex::DEGREES;
				break;
			}

			return index;
		}

		namespace charsets {

			// Charsets for external usage in three different sizes.
			extern const Charset tiny;
			extern const Charset medium;
			extern const Charset large;

		}

	}

}