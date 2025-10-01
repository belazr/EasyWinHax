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
				ID3D12Resource* _pVertexBufferResource;
				ID3D12Resource* _pIndexBufferResource;

				uint32_t _capacity;

			public:
				BufferBackend();

				BufferBackend(BufferBackend&& bb) noexcept = default;

				BufferBackend(const BufferBackend&) = delete;

				BufferBackend& operator=(BufferBackend&&) = delete;

				BufferBackend& operator=(const BufferBackend&) = delete;

				~BufferBackend();

				// Initializes members.
				//
				// Parameters:
				//
				// [in] pDevice:
				// Logical device of the backend.
				//
				// [in] pCommandList:
				// Command list of the backend.
				void initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

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

				// Gets the current capacity of the buffer in vertices.
				//
				// Return:
				// The current capacity of the buffer in vertices.
				uint32_t capacity() const override;

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
				void unmap() override;

				// Prepares the buffer backend for drawing. Has to be called before any draw calls.
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
				void draw(TextureId textureId, uint32_t index, uint32_t count) const override;

			private:
				bool createBuffer(ID3D12Resource** ppBufferResource, uint32_t size) const;
			};

		}

	}

}