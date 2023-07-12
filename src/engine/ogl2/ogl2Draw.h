#pragma once
#include "..\IDraw.h"
#include <gl\GL.h>

// Class for drawing with OpenGL 2.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace ogl2 {

		typedef BOOL(APIENTRY* twglSwapBuffers)(HDC hDc);

		class Draw : public IDraw {
		private:
			HGLRC _hGameContext;
			HGLRC _hHookContext;
			GLint _width;
			GLint _height;
			bool _isInit;

		public:
			Draw();

			~Draw();

			// Initializes drawing within a hook. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void beginDraw(Engine* pEngine) override;

			// Ends drawing within a hook. Should be called by an Engine object.
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void endDraw(const Engine* pEngine) const override;

			// Draws a filled triangle strip. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] corners:
			// Screen coordinates of the corners of the triangle strip.
			// 
			// [in] count:
			// Count of the corners of the triangle strip.
			// 
			// [in]
			// Color of the triangle strip.
			void drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const override;

			// Draws text to the screen. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pFont:
			// Pointer to an ogl2::Font object.
			//
			// [in] origin:
			// Coordinates of the bottom left corner of the first character of the text.
			// 
			// [in] text:
			// Text to be drawn. See length limitations implementations.
			//
			// [in] color:
			// Color of the text.
			void drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const override;
		};

	}

}

