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
				// [in] capacity:
				// Capacity of the buffer.
				//
				// Return:
				// True on success, false on failure.
				bool create(uint32_t capacity) override;

				// Destroys the buffer and all internal resources.
				void destroy() override;

				// Maps the allocated VRAM into the address space of the current process. Needs to be called before the buffer can be filled.
				//
				// Return:
				// True on success, false on failure.
				bool map() override;

				// Draws the content of the buffer to the screen.
				// Needs to be called between a successful of vk::Backend::beginFrame and a call to vk::Backend::endFrame.
				void draw() override;
			};

		 }

	}

}