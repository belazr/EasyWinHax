#pragma once
#include "..\..\AbstractDrawBuffer.h"
#include <d3d11.h>

namespace hax {

	namespace draw {

		namespace dx11 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				ID3D11Device* _pDevice;
				ID3D11DeviceContext* _pContext;
				D3D11_PRIMITIVE_TOPOLOGY _topology;
				ID3D11Buffer* _pVertexBuffer;
				ID3D11Buffer* _pIndexBuffer;

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
				void initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, D3D11_PRIMITIVE_TOPOLOGY topology);

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