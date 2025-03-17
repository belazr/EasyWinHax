#pragma once
#include "..\..\AbstractDrawBuffer.h"
#include <d3d9.h>

namespace hax {

	namespace draw {
		
		namespace dx9 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				IDirect3DDevice9* _pDevice;
				D3DPRIMITIVETYPE _primitiveType;
				IDirect3DPixelShader9* _pPixelShaderPassthrough;
				IDirect3DPixelShader9* _pPixelShaderTexture;
				IDirect3DVertexBuffer9* _pVertexBuffer;
				IDirect3DIndexBuffer9* _pIndexBuffer;

				typedef struct LoadedTexture {
					const Color* data;
					IDirect3DTexture9* texture;
				}LoadedTexture;

				LoadedTexture _textureArray[20];
				uint32_t _textureCount;

			public:
				DrawBuffer();

				~DrawBuffer();

				// Initializes members.
				//
				// Parameters:
				// 
				// [in] pDevice:
				// Device of the backend.
				//
				// [in] primitiveType:
				// Primitive topology the vertices in the buffer should be drawn in.
				//
				// [in] pPixelShaderPassthrough:
				// Pixel shader for drawing without textures.
				// 
				// [in] pPixelShaderTexture:
				// Pixel shader for drawing with textures.
				void initialize(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE primitiveType, IDirect3DPixelShader9* pPixelShaderPassthrough, IDirect3DPixelShader9* pPixelShaderTexture);

				// Creates a new buffer with all internal resources.
				//
				// Parameters:
				// 
				// [in] vertexCount:
				// Size of the buffer in vertices.
				//
				// Return:
				// True on success, false on failure.
				bool create(uint32_t vertexCount) override;

				// Destroys the buffer and all internal resources.
				void destroy() override;

				// Maps the allocated VRAM into the address space of the current process. Needs to be called before the buffer can be filled.
				//
				// Return:
				// True on success, false on failure.
				bool map() override;

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
				// Pointer to the internal texture structure in VRAM that can be passed to append. nullptr on failure.
				void* load(const Color* texture, uint32_t width, uint32_t height) override;

				// Draws the content of the buffer to the screen.
				// Needs to be called between a successful of vk::Backend::beginFrame and a call to vk::Backend::endFrame.
				void draw() override;
			};

		 }

	}

}