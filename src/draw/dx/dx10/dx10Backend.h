#pragma once
#include "dx10DrawBuffer.h"
#include "..\..\IBackend.h"
#include "..\..\Vertex.h"
#include "..\..\..\Vector.h"
#include <d3d10_1.h>

// Class for drawing within a DirectX 10 Present hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		namespace dx10 {

			typedef HRESULT(__stdcall* tPresent)(IDXGISwapChain* pOriginalSwapChain, UINT syncInterval, UINT flags);

			// Gets a copy of the vTable of the DirectX 10 swap chain used by the caller process.
			// 
			// Parameter:
			// 
			// [out] pSwapChainVTable:
			// Contains the devices vTable on success. See the dxgi header for the offset of the Present function (typically 8).
			// 
			// [in] size:
			// Size of the memory allocated at the address pointed to by pDeviceVTable.
			// See the dxgi header for the actual size of the vTable. Has to be at least offset of the function needed + one.
			// 
			// Return:
			// True on success, false on failure.
			bool getD3D10SwapChainVTable(void** pSwapChainVTable, size_t size);

			class Backend : public IBackend {
			private:
				typedef struct TextureData {
					ID3D10Texture2D* pTexture;
					ID3D10ShaderResourceView* pTextureView;
				}TextureData;

				IDXGISwapChain* _pSwapChain;

				ID3D10Device* _pDevice;
				ID3D10VertexShader* _pVertexShader;
				ID3D10InputLayout* _pVertexLayout;
				ID3D10PixelShader* _pPixelShader;
				ID3D10Buffer* _pConstantBuffer;
				D3D10_VIEWPORT _viewport;

				DrawBuffer _triangleListBuffer;
				DrawBuffer _pointListBuffer;

				Vector<TextureData> _textures;

			public:
				Backend();

				~Backend();

				// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [in] pArg1:
				// Pass the IDXGISwapChain*.
				//
				// [in] pArg2:
				// Pass nothing
				virtual void setHookArguments(void* pArg1 = nullptr, void* pArg2 = nullptr) override;

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
				// Pointer to the internal texture structure in VRAM that can be passed to DrawBuffer::append. nullptr on failure.
				virtual void* loadTexture(const Color* texture, uint32_t width, uint32_t height);
				
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
				bool createConstantBuffer();
				bool getCurrentViewport(D3D10_VIEWPORT* pViewport) const;
				bool updateConstantBuffer(D3D10_VIEWPORT viewport) const;
			};

		}

	}

}