#pragma once
#include "dx9BufferBackend.h"
#include "..\..\IBackend.h"
#include "..\..\..\Vector.h"

// Class for drawing within a DirectX 9 Present hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		namespace dx9 {

			typedef HRESULT(APIENTRY* tPresent)(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

			// Gets a copy of the vTable of the DirectX 9 device used by the caller process.
			// 
			// Parameter:
			// 
			// [out] pDeviceVTable:
			// Contains the devices vTable on success. See the d3d9 header for the offset of the Present function (typically 17).
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
				typedef struct State {
					IDirect3DStateBlock9* pStateBlock;
					IDirect3DVertexDeclaration9* pVertexDeclaration;
					IDirect3DVertexShader9* pVertexShader;
					IDirect3DPixelShader9* pPixelShader;
					IDirect3DVertexBuffer9* pVertexBuffer;
					UINT offset;
					UINT stride;
					IDirect3DIndexBuffer9* pIndexBuffer;
					IDirect3DBaseTexture9* pBaseTexture;
				}State;

				IDirect3DDevice9* _pDevice;
				IDirect3DVertexDeclaration9* _pVertexDeclaration;
				IDirect3DVertexShader9* _pVertexShader;
				IDirect3DPixelShader9* _pPixelShaderPassthrough;
				IDirect3DPixelShader9* _pPixelShaderTexture;
				D3DVIEWPORT9 _viewport;

				State _state;

				BufferBackend _textureBufferBackend;
				BufferBackend _solidBufferBackend;

				Vector<IDirect3DTexture9*> _textures;

			public:
				Backend();

				Backend(Backend&&) = delete;

				Backend(const Backend&) = delete;

				Backend& operator=(Backend&&) = delete;

				Backend& operator=(const Backend&) = delete;

				~Backend();

				// Sets the parameters of the current call of the hooked function.
				//
				// Parameters:
				// 
				// [in] pArg1:
				// Pass the LPDIRECT3DDEVICE9.
				//
				// [in] pArg2:
				// Pass nothing
				virtual void setHookParameters(void* pArg1 = nullptr, void* pArg2 = nullptr) override;

				// Initializes the backend. Should be called by an Engine object until success.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool initialize() override;

				// Loads a texture into VRAM.
				//
				// Parameters:
				// 
				// [in] data:
				// Texture colors in argb format.
				// 
				// [in] width:
				// Width of the texture.
				// 
				// [in] height:
				// Height of the texture.
				//
				// Return:
				// ID of the internal texture structure in VRAM that can be passed to DrawBuffer::append. 0 on failure.
				TextureId loadTexture(const Color* data, uint32_t width, uint32_t height) override;

				// Starts a frame within a hook. Should be called by an Engine object every frame at the begin of the hook.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool beginFrame() override;

				// Ends the current frame within a hook. Should be called by an Engine object every frame at the end of the hook.
				virtual void endFrame() override;

				// Gets a reference to the texture buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the texture buffer backend.
				virtual IBufferBackend* getTextureBufferBackend()  override;

				// Gets a reference to the solid buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the solid buffer backend.
				virtual IBufferBackend* getSolidBufferBackend()  override;

				// Gets the resolution of the current frame. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [out] frameWidth:
				// Pointer that receives the current frame width in pixel.
				//
				// [out] frameHeight:
				// Pointer that receives the current frame height in pixel.
				virtual void getFrameResolution(float* frameWidth, float* frameHeight) const override;

			private:
				bool createVertexDeclaration();
				bool createShaders();
				bool saveState();
				void restoreState();
				void releaseState();
				bool setVertexShaderConstant();

			};

		}

	}
	
}