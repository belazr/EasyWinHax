#pragma once
#include "..\..\AbstractDrawBuffer.h"
#include <d3d9.h>

namespace hax {

	namespace draw {
		
		namespace dx9 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				IDirect3DDevice9* _pDevice;
				IDirect3DVertexBuffer9* _pVertexBuffer;
				IDirect3DIndexBuffer9* _pIndexBuffer;
				D3DPRIMITIVETYPE _primitiveType;

			public:
				DrawBuffer(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE primitiveType);
				~DrawBuffer();

				bool create(uint32_t vertexCount) override;
				void destroy() override;
				bool map() override;
				void draw() override;
			};

		 }

	}

}