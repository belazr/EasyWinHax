#pragma once
#include "..\IDraw.h"
#include <d3d11.h>

namespace hax {

	namespace dx11 {

		bool getD3D11SwapChainVTable(void** pSwapChainVTable, size_t size);

		class Draw : IDraw {
		private:
			IDXGISwapChain* _pSwapChain;
			ID3D11Device* _pDevice;
			ID3D11DeviceContext* _pContext;
			ID3D11RenderTargetView* _pRenderTargetView;
			ID3D11VertexShader* _pVertexShader;
			ID3D11InputLayout* _pVertexLayout;
			ID3D11PixelShader* _pPixelShader;
			ID3D11Buffer* _pConstantBuffer;

			D3D11_VIEWPORT _viewport;

		public:
			Draw();

			// Initializes drawing within a hook. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void beginDraw(const Engine* pEngine) override;

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
			ID3D11RenderTargetView* createRenderTargetView();
			bool compileShaders();
			bool getViewport();
			bool setupOrtho();

		};

	}

}


