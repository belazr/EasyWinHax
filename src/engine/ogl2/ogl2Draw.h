#pragma once
#include "..\IDraw.h"

namespace hax {

	namespace ogl2 {

		typedef BOOL(APIENTRY* twglSwapBuffers)(HDC hDc);

		class Draw : public IDraw {
		private:
			HGLRC _hGameContext;
			HGLRC _hHookContext;

		public:
			Draw();

			void beginDraw(const Engine* pEngine) override;
			void endDraw(const Engine* pEngine) const override;
			void drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const override;
			void drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const override;
		};

	}

}

