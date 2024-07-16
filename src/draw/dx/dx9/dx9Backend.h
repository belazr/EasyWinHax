#pragma once
#include "..\..\IBackend.h"
#include "..\..\Vertex.h"
#include <d3d9.h>

// Class for drawing within a DirectX 9 EndScene hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

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

			class Backend : public IBackend {
			private:
				typedef struct BufferData {
					IDirect3DVertexBuffer9* pVertexBuffer;
					IDirect3DIndexBuffer9* pIndexBuffer;
					Vertex* pLocalVertexBuffer;
					uint32_t* pLocalIndexBuffer;
					uint32_t vertexBufferSize;
					uint32_t indexBufferSize;
					uint32_t curOffset;
				}BufferData;

				IDirect3DDevice9* _pDevice;

				IDirect3DVertexDeclaration9* _pOriginalVertexDeclaration;
				IDirect3DVertexDeclaration9* _pVertexDeclaration;

				BufferData _pointListBufferData;
				BufferData _triangleListBufferData;

				D3DVIEWPORT9 _viewport;

			public:
				Backend();

				~Backend();

				// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [in] pArg1:
				// Pass the LPDIRECT3DDEVICE9.
				//
				// [in] pArg2:
				// Pass nothing
				//
				// [in] pArg3:
				// Pass nothing
				virtual void setHookArguments(void* pArg1 = nullptr, const void* pArg2 = nullptr, void* pArg3 = nullptr) override;

				// Initializes the backend. Should be called by an Engine object until success.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool initialize() override;

				// Starts a frame within a hook. Should be called by an Engine object every frame at the begin of the hook.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool beginFrame() override;

				// Ends the current frame within a hook. Should be called by an Engine object every frame at the end of the hook.
				virtual void endFrame() override;

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

			private:
				bool createBufferData(BufferData* pBufferData, uint32_t vertexCount) const;
				void destroyBufferData(BufferData* pBufferData) const;
				bool mapBufferData(BufferData* pBufferData) const;
				void copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) const;
				bool resizeBufferData(BufferData* pBufferData, uint32_t newVertexCount) const;
				void drawBufferData(BufferData* pBufferData, D3DPRIMITIVETYPE type) const;

			};

		}

	}
	
}