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
				ID3D11Buffer* _pVertexBuffer;
				ID3D11Buffer* _pIndexBuffer;
				D3D11_PRIMITIVE_TOPOLOGY _topology;

			public:
				DrawBuffer();
				~DrawBuffer();

				void initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, D3D11_PRIMITIVE_TOPOLOGY topology);

				bool create(uint32_t vertexCount) override;
				void destroy() override;
				bool map() override;
				void draw() override;
			};

		}

	}

}