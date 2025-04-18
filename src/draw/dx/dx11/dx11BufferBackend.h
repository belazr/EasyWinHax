#pragma once
#include "..\..\IBufferBackend.h"
#include <d3d11.h>

namespace hax {

	namespace draw {

		namespace dx11 {

			class BufferBackend : public IBufferBackend {
			private:
				ID3D11Device* _pDevice;
				ID3D11DeviceContext* _pContext;
				ID3D11PixelShader* _pPixelShader;
				ID3D11Buffer* _pVertexBuffer;
				ID3D11Buffer* _pIndexBuffer;
				D3D11_PRIMITIVE_TOPOLOGY _topology;
				D3D11_PRIMITIVE_TOPOLOGY _curTopology;

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
				// [in] pContext:
				// Context of the backend.
				//
				// [in] pPixelShader:
				// Pixel shader for drawing the vertices.
				// 
				// [in] topology:
				// Primitive topology the vertices in the buffer should be drawn in.
				void initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader, D3D11_PRIMITIVE_TOPOLOGY topology);

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

				// Unmaps the allocated VRAM from the address space of the current process.
				virtual void unmap() override;

				// Begins drawing of the content of the buffer. Has to be called before any draw calls.
				//
				// Return:
				// True on success, false on failure.
				virtual bool begin() override;

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
				virtual void draw(TextureId textureId, uint32_t index, uint32_t count) const override;

				// Ends drawing of the content of the buffer. Has to be called after any draw calls.
				//
				// Return:
				// True on success, false on failure.
				virtual void end() const override;
			};

		}

	}

}