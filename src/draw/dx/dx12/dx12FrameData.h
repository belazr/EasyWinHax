#pragma once
#include "dx12BufferBackend.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			struct FrameData {
				ID3D12CommandAllocator* pCommandAllocator;
				BufferBackend triangleListBuffer;
				BufferBackend pointListBuffer;
				BufferBackend textureTriangleListBuffer;
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
				// [in] pTriangleListPipelineStatePassthrough:
				// Pipeline state with primitive topology for triangles and a passthrough pixel shader.
				// 
				// [in] pPointListPipelineStatePassthrough:
				// Pipeline state with primitive topology for triangles and a passthrough pixel shader.
				// 
				// [in] pTriangleListPipelineStateTexture:
				// Pipeline state with primitive topology for triangles and a passthrough pixel shader.
				//
				// Return:
				// True on success, false on failure.
				bool create(
					ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pTriangleListPipelineStatePassthrough,
					ID3D12PipelineState* pPointListPipelineStatePassthrough, ID3D12PipelineState* pTriangleListPipelineStateTexture
				);


				// Destroys all internal resources.
				void destroy();
			};

		}

	}

}