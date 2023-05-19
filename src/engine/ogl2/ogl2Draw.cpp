#include "ogl2Draw.h"
#include "ogl2Font.h"
#include "..\Engine.h"
#include <Windows.h>
#include <gl\GL.h>

namespace hax {

	namespace ogl2 {

		Draw::Draw() : _hGameContext(nullptr), _hHookContext(nullptr) {}


		void Draw::drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const {
			glColor3ub(UCHAR_R(color), UCHAR_G(color), UCHAR_B(color));
			glBegin(GL_TRIANGLE_STRIP);

			for (UINT i = 0; i < count; i++) {
				glVertex2f(corners[i].x, corners[i].y);
			}

			glEnd();

			return;
        }


		void Draw::drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const {
			Font* pOgl2Font = reinterpret_cast<Font*>(pFont);
			GLsizei strLen = static_cast<GLsizei>(strlen(text));
			
			if (!pOgl2Font || strLen > 100) return;

			glColor3ub(UCHAR_R(color), UCHAR_G(color), UCHAR_B(color));
			glRasterPos2f(pos->x, pos->y);
			glPushAttrib(GL_LIST_BIT);
			glListBase(pOgl2Font->base - 32);
			glCallLists(strLen, GL_UNSIGNED_BYTE, text);
			glPopAttrib();
            return;
		}


		void Draw::beginDraw(const Engine* pEngine) {
			const HDC hDc = reinterpret_cast<HDC>(pEngine->pHookArg);

			if (!this->_hGameContext || !this->_hHookContext) {
				this->_hGameContext = wglGetCurrentContext();
				this->_hHookContext = wglCreateContext(hDc);
			}

			wglMakeCurrent(hDc, this->_hHookContext);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, static_cast<double>(pEngine->fWindowWidth), static_cast<double>(pEngine->fWindowHeight), 0, 1, -1);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glClearColor(0, 0, 0, 1.0);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) const {

			if (!this->_hGameContext) return;

			wglMakeCurrent(reinterpret_cast<HDC>(pEngine->pHookArg), this->_hGameContext);

			return;
		}

	}

}