#pragma once
#include <Windows.h>

// Class for fonts for text drawing within a OpenGL 2 hook.

namespace hax {

	namespace ogl2 {

		class Font {
		private:
			float _height;
			float _width;

		public:
			// Display lists for character drawing.
			DWORD displayLists;

			// Initializes members.
			// 
			// Parameters:
			// 
			// [in] hDc:
			// Device context returned by the gl::getHookContext function.
			// 
			// [in] faceName:
			// A pointer to a null-terminated string that specifies the typeface name of the font.
			// The length of this string must not exceed 32 characters, including the terminating null character.
			// 
			// [in] height:
			// Height of the characters drawn in pixels.
			Font(HDC hDc, const char* faceName, float height);

			// Gets the width of a character in pixels.
			// 
			// Return:
			// The width of a character in pixels.
			float getCharWidth() const;

			// Gets the height of a character in pixels.
			// 
			// Return:
			// The height of a character in pixels.
			float getCharHeight() const;
		};

	}

}