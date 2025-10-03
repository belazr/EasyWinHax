#pragma once
#include "dx12BufferBackend.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			struct FrameData {
				ID3D12CommandAllocator* pCommandAllocator;
				BufferBackend bufferBackend;
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
				// Return:
				// True on success, false on failure.
				bool create(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);


				// Destroys all internal resources.
				void destroy();
			};

		}

	}

}