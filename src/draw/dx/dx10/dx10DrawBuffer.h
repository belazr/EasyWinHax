#pragma once
#include "..\..\AbstractDrawBuffer.h"
#include <d3d10_1.h>

namespace hax {

	namespace draw {

		namespace dx10 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				ID3D10Device* _pDevice;
				D3D10_PRIMITIVE_TOPOLOGY _topology;

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
				void initialize(ID3D10Device* pDevice, D3D10_PRIMITIVE_TOPOLOGY topology);
			};

		}

	}

}