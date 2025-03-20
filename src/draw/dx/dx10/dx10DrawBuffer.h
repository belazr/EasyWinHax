#pragma once
#include "..\..\AbstractDrawBuffer.h"
#include <d3d10_1.h>

namespace hax {

	namespace draw {

		namespace dx10 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				ID3D10Device* _pDevice;
				D3D10_PRIMITIVE_TOPOLOGY _topology;
				ID3D10PixelShader* _pPixelShaderPassthrough;
				ID3D10PixelShader* _pPixelShaderTexture;
				ID3D10Buffer* _pVertexBuffer;
				ID3D10Buffer* _pIndexBuffer;

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
				// [in] pContext:
				// Context of the backend.
				// 
				// [in] topology:
				// Primitive topology the vertices in the buffer should be drawn in.
				//
				// [in] pPixelShaderPassthrough:
				// Pixel shader for drawing without textures.
				// 
				// [in] pPixelShaderTexture:
				// Pixel shader for drawing with textures.
				void initialize(ID3D10Device* pDevice, D3D10_PRIMITIVE_TOPOLOGY topology, ID3D10PixelShader* pPixelShaderPassthrough, ID3D10PixelShader* _pPixelShaderTexture);
				
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