#pragma once
#include "..\..\Vertex.h"
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

		typedef struct BufferData {
			ID3D11Buffer* pVertexBuffer;
			Vertex* pLocalVertexBuffer;
			uint32_t vertexBufferSize;
			uint32_t curOffset;
		}BufferData;

		class Draw : public IDraw {
		private:
			ID3D11Device* _pDevice;
			ID3D11DeviceContext* _pContext;
			ID3D11VertexShader* _pVertexShader;
			ID3D11InputLayout* _pVertexLayout;
			ID3D11PixelShader* _pPixelShader;
			ID3D11RenderTargetView* _pRenderTargetView;
			ID3D11Buffer* _pConstantBuffer;
			
			BufferData _pointListBufferData;
			BufferData _triangleListBufferData;
			
			D3D11_VIEWPORT _viewport;
			
			bool _isInit;
			bool _isBegin;

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

			// Draws a filled triangle list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] corners:
			// Screen coordinates of the corners of the triangles in the list.
			// The three corners of the first triangle have to be in clockwise order. From there on the orientation of the triangles has to alternate.
			// 
			// [in] count:
			// Count of the corners of the triangles in the list. Has to be divisble by three.
			// 
			// [in] color:
			// Color of each triangle.
			void drawTriangleList(const Vector2 corners[], uint32_t count, rgb::Color color) override;

			// Draws a point list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] coordinate:
			// Screen coordinates of the points in the list.
			// 
			// [in] count:
			// Count of the points in the list.
			// 
			// [in] color:
			// Color of each point.
			//
			// [in] offest:
			// Offset by which each point is drawn.
			virtual void drawPointList(const Vector2 coordinates[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) override;

		private:
			bool initialize(IDXGISwapChain* pSwapChain);
			bool compileShaders();
			bool createBufferData(BufferData* pBufferData, uint32_t vertexCount) const;
			void destroyBufferData(BufferData* pBufferData) const;
			void getCurrentViewport(D3D11_VIEWPORT* pViewport) const;
			bool createConstantBuffer();
			bool updateConstantBuffer() const;
			bool mapBufferData(BufferData* pBufferData) const;
			void copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) const;
			bool resizeBufferData(BufferData* pBufferData, uint32_t vertexCount) const;
			void drawBufferData(BufferData* pBufferData, D3D11_PRIMITIVE_TOPOLOGY topology) const;
		};

	}

}