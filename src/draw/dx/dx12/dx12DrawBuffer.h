#pragma once
#include "..\..\AbstractDrawBuffer.h"
#include <d3d12.h>

namespace hax {

	namespace draw {

		namespace dx12 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				ID3D12Device* _pDevice;
				ID3D12Resource* _pVertexBufferResource;
				ID3D12Resource* _pIndexBufferResource;

			public:
				DrawBuffer();
				~DrawBuffer();

				// Initializes members.
				//
				// Parameters:
				// 
				// [in] hDevice:
				// Logical device of the backend.
				void initialize(ID3D12Device* pDevice);

				// Creates a new buffer with all internal resources.
				//
				// Parameters:
				// 
				// [in] vertexCount:
				// Size of the buffer in vertices.
				//
				// Return:
				// True on success, false on failure.
				bool create(uint32_t vertexCount) override;

			private:
				bool createBuffer(ID3D12Resource** ppBufferResource, uint32_t size) const;

			};

		}

	}

}