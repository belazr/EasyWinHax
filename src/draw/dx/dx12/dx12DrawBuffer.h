#pragma once
#include "..\..\AbstractDrawBuffer.h"
#include <d3d12.h>

namespace hax {

	namespace draw {

		namespace dx12 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				ID3D12Device* _pDevice;
				ID3D12GraphicsCommandList* _pCommandList;
				ID3D12PipelineState* _pPipelineState;
				D3D_PRIMITIVE_TOPOLOGY _topology;
				ID3D12Resource* _pVertexBufferResource;
				ID3D12Resource* _pIndexBufferResource;

			public:
				DrawBuffer();

				~DrawBuffer();

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
				// Pipeline state with the appropriate primitive topology type.
				// 
				// [in] topology:
				// The topology with which the buffer draws its content.
				void initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPipelineState, D3D_PRIMITIVE_TOPOLOGY topology);

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
				// Needs to be called between a successful of dx12::Backend::beginFrame and a call to dx12::Backend::endFrame.
				void draw() override;

			private:
				bool createBuffer(ID3D12Resource** ppBufferResource, uint32_t size) const;

			};

		}

	}

}