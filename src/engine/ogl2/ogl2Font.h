#pragma once
#include <Windows.h>

namespace hax {

	namespace ogl2 {

		class Font {
		private:
			float _height;
			float _width;

		public:
			DWORD base;

			Font(HDC hDc, float height);

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