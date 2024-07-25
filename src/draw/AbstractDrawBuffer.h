#pragma once
#include "Vertex.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Abstract class the engine class uses as a buffer for vertices and indices to draw primitives.
// The appropriate implementation should be instantiated by a backend and the engine uses a pointer to fill the buffer during a frame.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		class AbstractDrawBuffer {
		protected:
			Vertex* _pLocalVertexBuffer{};
			uint32_t* _pLocalIndexBuffer{};
			uint32_t _vertexBufferSize{};
			uint32_t _indexBufferSize{};
			uint32_t _curOffset{};

		public:
			// Creates a new buffer with all internal resources.
			//
			// Parameters:
			// 
			// [in] vertexCount:
			// Size of the buffer in vertices.
			//
			// Return:
			// True on success, false on failure.
			virtual bool create(uint32_t vertexCount) = 0;

			// Destroys the buffer and all internal resources.
			virtual void destroy() = 0;

			// Maps the allocated VRAM into the address space of the current process. Needs to be called before the buffer can be filled.
			//
			// Return:
			// True on success, false on failure.
			virtual bool map() = 0;

			// Draws the content of the buffer to the screen.
			// Needs to be called between a successful of IBackend::beginFrame and a call to IBackend::endFrame.
			virtual void draw() = 0;


			// Appends vertices to the buffer.
			//
			// Parameters:
			// 
			// [in] data:
			// Pointer to an array of screen coordinates of vertices to be appended to the buffer.
			//
			// [in] count:
			// Amount of vertices in the data array.
			//
			// [in] color:
			// Color that the vertices will be drawn in. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			//
			// [in] offset:
			// Offset that gets added to all coordinates in the data array before the vertices get appended to the buffer.
			void append(const Vector2 data[], uint32_t count, Color color, Vector2 offset = { 0.f, 0.f }) {
				const uint32_t newVertexCount = this->_curOffset + count;

				if (newVertexCount * sizeof(Vertex) > this->_vertexBufferSize || newVertexCount * sizeof(uint32_t) > this->_indexBufferSize) {

					if (!this->resize(newVertexCount * 2u)) return;

				}

				for (uint32_t i = 0u; i < count; i++) {
					const Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, color };
					memcpy(&(this->_pLocalVertexBuffer[this->_curOffset]), &curVertex, sizeof(Vertex));

					this->_pLocalIndexBuffer[this->_curOffset] = this->_curOffset;
					this->_curOffset++;
				}

				return;
			}


			// Resizes the buffer.
			//
			// Parameters:
			// 
			// [in] newVertexCount:
			// New size of the buffer in vertices after the resize.
			//
			// Return:
			// True on success, false on failure.
			bool resize(uint32_t newVertexCount) {

				if (newVertexCount <= this->_curOffset) return true;

				const uint32_t oldOffset = this->_curOffset;

				Vertex* const pTmpVertexBuffer = reinterpret_cast<Vertex*>(calloc(oldOffset, sizeof(Vertex)));

				if (!pTmpVertexBuffer) return false;

				if (this->_pLocalVertexBuffer) {
					memcpy(pTmpVertexBuffer, this->_pLocalVertexBuffer, oldOffset * sizeof(Vertex));
				}
				else {
					memset(pTmpVertexBuffer, 0, oldOffset * sizeof(Vertex));
				}

				int32_t* const pTmpIndexBuffer = reinterpret_cast<int32_t*>(calloc(oldOffset, sizeof(int32_t)));

				if (!pTmpIndexBuffer) {
					free(pTmpVertexBuffer);

					return false;
				}

				if (this->_pLocalIndexBuffer) {
					memcpy(pTmpIndexBuffer, this->_pLocalIndexBuffer, oldOffset * sizeof(uint32_t));
				}
				else {
					memset(pTmpIndexBuffer, 0, oldOffset * sizeof(uint32_t));
				}

				this->destroy();

				if (!this->create(newVertexCount)) {
					free(pTmpVertexBuffer);
					free(pTmpIndexBuffer);

					return false;
				}

				if (!this->map()) {
					free(pTmpVertexBuffer);
					free(pTmpIndexBuffer);

					return false;
				}

				if (this->_pLocalVertexBuffer && this->_pLocalIndexBuffer) {
					memcpy(this->_pLocalVertexBuffer, pTmpVertexBuffer, oldOffset * sizeof(Vertex));
					memcpy(this->_pLocalIndexBuffer, pTmpIndexBuffer, oldOffset * sizeof(uint32_t));

					this->_curOffset = oldOffset;
				}
				
				free(pTmpVertexBuffer);
				free(pTmpIndexBuffer);

				return true;
			}

			protected:
			void reset() {
				this->_pLocalVertexBuffer = nullptr;
				this->_pLocalIndexBuffer = nullptr;
				this->_vertexBufferSize = 0u;
				this->_indexBufferSize = 0u;
				this->_curOffset = 0u;
				
				return;
			}

		};

	}

}