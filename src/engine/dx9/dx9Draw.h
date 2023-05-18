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
			Font* pFont;
			int iWindowWidth;
			int iWindowHeight;
			float fWindowWidth;
			float fWindowHeight;

			Draw();

			void drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const override;
			void drawString(const Vector2* pos, const char* text, rgb::Color color) const override;

			void beginDraw(IDirect3DDevice9* pOriginalDevice);
			bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size);
			void setWindowSize(int width, int height);
			
		private:
			HRESULT drawFontchar(const Fontchar* pChar, const Vector2* pos, size_t index, rgb::Color color) const;
		};

		BOOL CALLBACK getWindowHandleCallback(HWND hWnd, LPARAM lParam);
	}
	
}


