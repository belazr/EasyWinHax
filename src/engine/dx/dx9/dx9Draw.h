#pragma once
#include "..\font\dxFont.h"
#include "..\..\IDraw.h"
#include <d3d9.h>

// Class for drawing within a DirectX 9 EndScene hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace dx9 {

		typedef HRESULT(APIENTRY* tEndScene)(LPDIRECT3DDEVICE9 _pDevice);

		// Gets a copy of the vTable of the DirectX 9 device used by the caller process.
		// 
		// Parameter:
		// 
		// [out] pDeviceVTable:
		// Contains the devices vTable on success. See the d3d9 header for the offset of the EndScene function (typically 42).
		// 
		// [in] size:
		// Size of the memory allocated at the address pointed to by pDeviceVTable.
		// See the d3d9 header for the actual size of the vTable. Has to be at least offset of the function needed + one.
		// 
		// Return:
		// True on success, false on failure.
		bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size);


		class Draw : public IDraw {
		private:
			IDirect3DDevice9* _pDevice;
			bool _isInit;

		public:
			Draw();

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
			// Pointer to a dx9::Font object.
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
			
		private:
			void drawFontchar(dx::Font<Vertex>* pDx9Font, const dx::Fontchar* pChar, const Vector2* pos, size_t index, rgb::Color color) const;
		};

	}
	
}


