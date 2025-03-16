#pragma once
#include "dx9DrawBuffer.h"
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
				IDirect3DDevice9* _pDevice;
				IDirect3DVertexDeclaration9* _pVertexDeclaration;
				IDirect3DVertexShader9* _pVertexShader;
				IDirect3DPixelShader9* _pPixelShaderPassthrough;
				IDirect3DPixelShader9* _pPixelShaderTexture;
				D3DVIEWPORT9 _viewport;
				IDirect3DStateBlock9* _pStateBlock;
				IDirect3DVertexDeclaration9* _pOriginalVertexDeclaration;

				DrawBuffer _triangleListBuffer;
				DrawBuffer _textureBuffer;
				DrawBuffer _pointListBuffer;

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
				virtual void setHookArguments(void* pArg1 = nullptr, void* pArg2 = nullptr) override;

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

				// Gets a reference to the triangle list buffer of the backend. It is the responsibility of the backend to dispose of the buffer properly.
				// 
				// Return:
				// Pointer to the triangle list buffer.
				virtual AbstractDrawBuffer* getTriangleListBuffer() override;

				// Gets a reference to the point list buffer of the backend. It is the responsibility of the backend to dispose of the buffer properly.
				// 
				// Return:
				// Pointer to the point list buffer.
				virtual AbstractDrawBuffer* getPointListBuffer() override;

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
				bool createShaders();
				void restoreState();
				bool setVertexShaderConstant();

			};

		}

	}
	
}