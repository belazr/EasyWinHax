#include "ogl2Font.h"
#include <gl\GL.h>

namespace hax {

	namespace ogl2 {
		Font::Font(HDC hDc, const char* faceName, float height) {
			this->displayLists = glGenLists(100);
			this->_height = height;
			this->_width = this->_height / 2.f;

			HFONT hFont = CreateFontA(-static_cast<int>(height), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_DONTCARE | DEFAULT_PITCH, faceName);
			HFONT hOldFont = static_cast<HFONT>(SelectObject(hDc, hFont));
			wglUseFontBitmapsA(hDc, 32, 100, this->displayLists);
			SelectObject(hDc, hOldFont);
			DeleteObject(hFont);
		}

		float Font::getCharWidth() const {
			
			return this->_width;
		}


		float Font::getCharHeight() const {

			return this->_height;
		}

	}

}