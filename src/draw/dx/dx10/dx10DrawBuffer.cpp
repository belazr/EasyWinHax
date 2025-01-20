#include "dx10DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx10 {

			DrawBuffer::DrawBuffer() : AbstractDrawBuffer(), _pDevice{}, _topology{} {}


			void DrawBuffer::initialize(ID3D10Device* pDevice, D3D10_PRIMITIVE_TOPOLOGY topology) {
				this->_pDevice = pDevice;
				this->_topology = topology;

				return;
			}

		}

	}

}
