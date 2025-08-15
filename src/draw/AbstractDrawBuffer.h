#pragma once
#include "IBufferBackend.h"
#include "Vertex.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Abstract base class for draw buffers. It uses a BufferBackend object that is initialized by the Backend object to draw vertices to the screen.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		class AbstractDrawBuffer {
		protected:
			IBufferBackend* _pBufferBackend;
			Vertex* _pLocalVertexBuffer;
			uint32_t* _pLocalIndexBuffer;

			uint32_t _size;
			uint32_t _capacity;

		public:
			AbstractDrawBuffer() : _pBufferBackend{}, _pLocalVertexBuffer{}, _pLocalIndexBuffer{}, _size{}, _capacity{} {}

			
			~AbstractDrawBuffer() {
				
				if (this->_pBufferBackend) {
					this->_pBufferBackend->unmap();
				}

				return;
			}


			// Begins the frame for the buffer. Has to be called before any append calls.
			//
			// Parameters:
			// 
			// [in] pBufferBackend:
			// Backend for the buffer retrieved from the engine backend.
			//
			// Return:
			// True on success, false on failure.
			bool beginFrame(IBufferBackend* pBufferBackend) {

				if (!pBufferBackend) return false;

				this->_pBufferBackend = pBufferBackend;
				this->_capacity = pBufferBackend->capacity();

				if (!this->_pBufferBackend->map(&this->_pLocalVertexBuffer, &this->_pLocalIndexBuffer)) {
					this->reset();

					return false;
				}

				return true;
			}

			// Ends the frame for the buffer and draws the contents. Has to be called after any append calls.
			virtual void endFrame() = 0;

		protected:
			void reset() {
				this->_pBufferBackend = nullptr;
				this->_pLocalVertexBuffer = nullptr;
				this->_pLocalIndexBuffer = nullptr;
				this->_size = 0u;
				this->_capacity = 0u;
			}


			bool reserve(uint32_t capacity) {

				if (!this->_pBufferBackend) return false;

				if (capacity <= this->_capacity) return true;

				Vertex* const pTmpVertexBuffer = reinterpret_cast<Vertex*>(malloc(this->_size * sizeof(Vertex)));

				if (!pTmpVertexBuffer) {
					this->reset();
					
					return false;
				}

				if (this->_pLocalVertexBuffer) {
					memcpy(pTmpVertexBuffer, this->_pLocalVertexBuffer, this->_size * sizeof(Vertex));
				}

				int32_t* const pTmpIndexBuffer = reinterpret_cast<int32_t*>(malloc(this->_size * sizeof(int32_t)));

				if (!pTmpIndexBuffer) {
					free(pTmpVertexBuffer);
					this->reset();

					return false;
				}

				if (this->_pLocalIndexBuffer) {
					memcpy(pTmpIndexBuffer, this->_pLocalIndexBuffer, this->_size * sizeof(uint32_t));
				}

				const uint32_t oldSize = this->_size;

				this->_pBufferBackend->destroy();
				this->_pLocalIndexBuffer = nullptr;
				this->_pLocalVertexBuffer = nullptr;
				this->_capacity = this->_pBufferBackend->capacity();

				if (!this->_pBufferBackend->create(capacity)) {
					free(pTmpVertexBuffer);
					free(pTmpIndexBuffer);
					this->reset();

					return false;
				}

				this->_capacity = this->_pBufferBackend->capacity();

				if (!this->_pBufferBackend->map(&this->_pLocalVertexBuffer, &this->_pLocalIndexBuffer)) {
					free(pTmpVertexBuffer);
					free(pTmpIndexBuffer);
					this->reset();

					return false;
				}

				if (this->_pLocalVertexBuffer && this->_pLocalIndexBuffer) {
					memcpy(this->_pLocalVertexBuffer, pTmpVertexBuffer, oldSize * sizeof(Vertex));
					memcpy(this->_pLocalIndexBuffer, pTmpIndexBuffer, oldSize * sizeof(uint32_t));
				}

				free(pTmpVertexBuffer);
				free(pTmpIndexBuffer);

				return true;
			}

		};

	}

}