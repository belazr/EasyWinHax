#pragma once
#include "..\..\IBufferBackend.h"
#include <d3d12.h>

namespace hax {

	namespace draw {

		namespace dx12 {

			class BufferBackend : public IBufferBackend {
			private:
				ID3D12Device* _pDevice;
				ID3D12GraphicsCommandList* _pCommandList;
				ID3D12PipelineState* _pPipelineState;
				ID3D12Resource* _pVertexBufferResource;
				ID3D12Resource* _pIndexBufferResource;
				D3D_PRIMITIVE_TOPOLOGY _topology;

				uint32_t _capacity;

			public:
				BufferBackend();

				~BufferBackend();

				// Initializes members.
				//
				// Parameters:
				//
				// [in] pDevice:
				// Logical device of the backend.
				//
				// [in] pCommandList:
				// Command list of the image the buffer belongs to.
				//
				// [in] pPipelineState:
				// Pipeline state with the appropriate primitive topology.
				//
				// [in] topology:
				// The topology with which the buffer draws its content.
				void initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPipelineState, D3D_PRIMITIVE_TOPOLOGY topology);

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
				virtual void draw(TextureId textureId, uint32_t index, uint32_t count) override;

				// Ends drawing of the content of the buffer. Has to be called after any draw calls.
				//
				// Return:
				// True on success, false on failure.
				virtual void end() override;

			private:
				bool createBuffer(ID3D12Resource** ppBufferResource, uint32_t size) const;
			};

		}

	}

}