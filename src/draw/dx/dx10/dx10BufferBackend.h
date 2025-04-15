#pragma once
#include "..\..\IBufferBackend.h"
#include <d3d10.h>

namespace hax {

	namespace draw {

		namespace dx10 {

			class BufferBackend : public IBufferBackend {
			private:
				ID3D10Device* _pDevice;
				ID3D10PixelShader* _pPixelShader;
				ID3D10Buffer* _pVertexBuffer;
				ID3D10Buffer* _pIndexBuffer;
				D3D10_PRIMITIVE_TOPOLOGY _topology;

			public:
				BufferBackend();

				~BufferBackend();

				// Initializes members.
				//
				// Parameters:
				// 
				// [in] pDevice:
				// Device of the backend.
				//
				// [in] pPixelShader:
				// Pixel shader for drawing the vertices.
				// 
				// [in] topology:
				// Primitive topology the vertices in the buffer should be drawn in.
				void initialize(ID3D10Device* pDevice, ID3D10PixelShader* pPixelShader, D3D10_PRIMITIVE_TOPOLOGY topology);

				// Creates internal resources.
				//
				// Parameters:
				// 
				// [in] capacity:
				// Capacity of verticies the buffer backend can hold.
				//
				// Return:
				// True on success, false on failure.
				bool create(uint32_t capacity) override;

				// Destroys all internal resources.
				void destroy() override;

				// Maps the allocated VRAM into the address space of the current process.
				// 
				// Parameters:
				// 
				// [out] ppLocalVertexBuffer:
				// The mapped vertex buffer.
				//
				// 
				// [out] ppLocalIndexBuffer:
				// The mapped index buffer.
				//
				// Return:
				// True on success, false on failure.
				bool map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) override;

				// Prepares the buffer backend for drawing a batch.
				//
				// Return:
				// True on success, false on failure.
				bool prepare() override;

				// Draws a batch.
				// 
				// Parameters:
				// 
				// [in] textureId:
				// ID of the texture that should be drawn return by Backend::loadTexture.
				// If this is 0ull, no texture will be drawn.
				//
				// [in] index:
				// Index into the index buffer where the batch begins.
				// 
				// [in] count:
				// Vertex count in the batch.
				void draw(TextureId textureId, uint32_t index, uint32_t count) override;
			};

		}

	}

}