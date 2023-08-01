#pragma once
#include "..\font\dxFonts.h"
#include "..\..\IDraw.h"
#include <d3d11.h>

// Class for drawing within a DirectX 11 Present hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace dx11 {

		typedef HRESULT(__stdcall* tPresent)(IDXGISwapChain* pOriginalSwapChain, UINT syncInterval, UINT flags);

		// Gets a copy of the vTable of the DirectX 11 swap chain used by the caller process.
		// 
		// Parameter:
		// 
		// [out] pSwapChainVTable:
		// Contains the devices vTable on success. See the d3d11 header for the offset of the Present function (typically 8).
		// 
		// [in] size:
		// Size of the memory allocated at the address pointed to by pDeviceVTable.
		// See the d3d11 header for the actual size of the vTable. Has to be at least offset of the function needed + one.
		// 
		// Return:
		// True on success, false on failure.
		bool getD3D11SwapChainVTable(void** pSwapChainVTable, size_t size);

		class Draw : public IDraw {
		private:
			ID3D11Device* _pDevice;
			ID3D11DeviceContext* _pContext;
			ID3D11VertexShader* _pVertexShader;
			ID3D11InputLayout* _pVertexLayout;
			ID3D11PixelShader* _pPixelShader;
			ID3D11Buffer* _pConstantBuffer;
			ID3D11Buffer* _pVertexBuffer;
			
			UINT _vertexBufferSize;

			D3D11_VIEWPORT _viewport;
			D3D11_PRIMITIVE_TOPOLOGY _originalTopology;
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
			void endDraw(const Engine* pEngine) override;

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
			void drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) override;

			// Draws text to the screen. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pFont:
			// Pointer to a dx::Font object.
			//
			// [in] origin:
			// Coordinates of the bottom left corner of the first character of the text.
			// 
			// [in] text:
			// Text to be drawn. See length limitations implementations.
			//
			// [in] color:
			// Color of the text.
			void drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) override;

		private:
			bool compileShaders();
			bool createConstantBuffer();
			void drawFontchar(const dx::Fontchar* pChar, const Vector2* pos, rgb::Color color);
			bool setupVertexBuffer(const Vector2 data[], UINT count, rgb::Color color, Vector2 offset = { 0.f, 0.f });
		};

	}

}