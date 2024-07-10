#pragma once
#include "..\..\Vertex.h"
#include "..\..\IBackend.h"
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

		typedef struct BufferData {
			IDirect3DVertexBuffer9* pVertexBuffer;
			IDirect3DIndexBuffer9* pIndexBuffer;
			Vertex* pLocalVertexBuffer;
			uint32_t* pLocalIndexBuffer;
			uint32_t vertexBufferSize;
			uint32_t indexBufferSize;
			uint32_t curOffset;
		}BufferData;

		class Backend : public IBackend {
		private:
			IDirect3DDevice9* _pDevice;
			IDirect3DVertexDeclaration9* _pOriginalVertexDeclaration;
			IDirect3DVertexDeclaration9* _pVertexDeclaration;

			BufferData _pointListBufferData;
			BufferData _triangleListBufferData;

			D3DVIEWPORT9 _viewport;

			bool _isInit;
			bool _isBegin;

		public:
			Backend();

			~Backend();

			// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			virtual void beginFrame(Engine* pEngine) override;

			// Ends the current frame within a hook. Should be called by an Engine object.
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			virtual void endFrame(const Engine* pEngine) override;

			// Gets the resolution of the current frame. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [out] frameWidth:
			// Pointer that receives the current frame width in pixel.
			//
			// [out] frameHeight:
			// Pointer that receives the current frame height in pixel.
			virtual void getFrameResolution(float* frameWidth, float* frameHeight) override;

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
			bool initialize(IDirect3DDevice9* pDevice);
			bool createBufferData(BufferData* pBufferData, uint32_t vertexCount) const;
			void destroyBufferData(BufferData* pBufferData) const;
			bool mapBufferData(BufferData* pBufferData) const;
			void copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) const;
			bool resizeBufferData(BufferData* pBufferData, uint32_t newVertexCount) const;
			void drawBufferData(BufferData* pBufferData, D3DPRIMITIVETYPE type) const;

		};

	}
	
}


