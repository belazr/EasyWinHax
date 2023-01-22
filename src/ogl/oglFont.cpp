#pragma once
#include "oglFont.h"
#include <stdio.h>

namespace gl {

	// see opengl font and text documentation on msdn
	Font::Font(int height, HDC hDc) {
		this->_base = glGenLists(96);
		this->_height = static_cast<float>(height);
		this->_width = this->_height / 2.f;
		this->_hDc = hDc;

		HFONT hFont = CreateFontA(-height, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Consolas");
		HFONT hOldFont = static_cast<HFONT>(SelectObject(hDc, hFont));
		wglUseFontBitmapsA(hDc, 32, 96, this->_base);
		SelectObject(hDc, hOldFont);
		DeleteObject(hFont);
	}


	void Font::drawString(const Vector2* pos, const GLubyte color[3], const char* format, ...) const {
		glColor3ub(color[0], color[1], color[2]);
		glRasterPos2f(pos->x, pos->y);
		
		// replace format specifiers
		char text[100]{};
		va_list args = nullptr;
		va_start(args, format);
		vsprintf_s(text, 100, format, args);
		va_end(args);

		// draw text
		glPushAttrib(GL_LIST_BIT);
		glListBase(_base - 32);
		glCallLists(static_cast<GLsizei>(strlen(text)), GL_UNSIGNED_BYTE, text);
		glPopAttrib();
	}


	Vector2 Font::center(const Vector2* pos, float width, float height, float textWidth, float textHeight) const {
		const Vector2 textPos{
			pos->x + (width - textWidth) / 2.f,
			pos->y + height / 2.f + textHeight / 3.f
		};

		return textPos;
	}


	float Font::center(float x, float width, float textWidth) const {
		
		return (x + (width - textWidth) / 2.f);
	}


	float Font::getCharWidth() const {
		
		return this->_width;
	}


	float Font::getCharHeight() const {
		
		return this->_height;
	}


	HDC Font::getHdc() const {
		
		return this->_hDc;
	}

}