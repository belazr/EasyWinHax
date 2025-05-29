#include "SolidDrawBuffer.h"

namespace hax {

	namespace draw {

		SolidDrawBuffer::SolidDrawBuffer() : AbstractDrawBuffer() {}


		void SolidDrawBuffer::append(const Vertex* data, uint32_t count) {

			if (!this->_pLocalVertexBuffer || !this->_pLocalIndexBuffer) return;

			const uint32_t newSize = this->_size + count;

			if (newSize > this->_capacity) {

				if (!this->reserve(newSize * 2u)) return;

			}

			for (uint32_t i = 0u; i < count; i++) {
				this->_pLocalVertexBuffer[this->_size] = data[i];
				this->_pLocalIndexBuffer[this->_size] = this->_size;
				this->_size++;
			}

			return;
		}


		void SolidDrawBuffer::endFrame() {
			
			if (!this->_pBufferBackend) return;

			this->_pBufferBackend->unmap();

			this->_pLocalIndexBuffer = nullptr;
			this->_pLocalVertexBuffer = nullptr;

			if (!this->_pBufferBackend->begin()) {
				this->reset();
				
				return;
			}
		
			this->_pBufferBackend->draw(0ull, 0u, this->_size);
			this->_pBufferBackend->end();
			this->reset();

			return;
		}

	}

}