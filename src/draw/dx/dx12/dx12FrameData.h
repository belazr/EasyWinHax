#pragma once
#include "dx12BufferBackend.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			struct FrameData {
				ID3D12CommandAllocator* pCommandAllocator;
				BufferBackend textureTriangleListBuffer;
				BufferBackend triangleListBuffer;
				HANDLE hEvent;

				FrameData();

				FrameData(FrameData&& fd) noexcept;

				FrameData(const FrameData&) = delete;
				FrameData& operator=(FrameData&&) = delete;
				FrameData& operator=(const FrameData&) = delete;

				~FrameData();

				// Creates internal resources.
				//
				// Parameters:
				//
				// [in] pDevice:
				// Logical device of the backend.
				//
				// [in] pCommandList:
				// Command list of the backend.
				//
				// [in] pPipelineStateTexture:
				// Pipeline state with primitive topology for triangles and a texture pixel shader.
				//
				// [in] pPipelineStatePassthrough:
				// Pipeline state with primitive topology for triangles and a passthrough pixel shader.
				//
				// Return:
				// True on success, false on failure.
				bool create(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPipelineStateTexture, ID3D12PipelineState* pPipelineStatePassthrough);


				// Destroys all internal resources.
				void destroy();
			};

		}

	}

}