#pragma once
#include "dx9Font.h"
#include "..\IDraw.h"
#include "charsets\dx9Charsets.h"
#include <d3d9.h>

// Class for drawing with DirectX 9.

namespace hax {

	namespace dx9 {

		typedef HRESULT(APIENTRY* tEndScene)(LPDIRECT3DDEVICE9 pDevice);

		class Draw : public IDraw {
		public:
			IDirect3DDevice9* pDevice;

			static bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size);

			Draw();

			void beginDraw(const Engine* pEngine) override;
			void endDraw(const Engine* pEngine) const override;
			void drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const override;
			void drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const override;
			
		private:
			HRESULT drawFontchar(Font* pDx9Font, const Fontchar* pChar, const Vector2* pos, size_t index, rgb::Color color) const;
		};

		static BOOL CALLBACK getWindowHandleCallback(HWND hWnd, LPARAM lParam);
	}
	
}


