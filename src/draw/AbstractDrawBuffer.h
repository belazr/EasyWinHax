#pragma once
#include "Vertex.h"
#include <winnt.h>
#include <stdint.h>

// Abstract class the engine class uses as a buffer for vertices and indices to draw primitives.
// The appropriate implementation should be instantiated by a backend and the engine uses a pointer to fill the buffer during a frame.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		class AbstractDrawBuffer {
		public:
			Vertex* pLocalVertexBuffer;
			uint32_t* pLocalIndexBuffer;
			uint32_t vertexBufferSize;
			uint32_t indexBufferSize;
			uint32_t curOffset;
		
			virtual bool create(uint32_t vertexCount) = 0;
			virtual void destroy() = 0;
			virtual bool map() = 0;
			virtual void draw() = 0;
			

			void append(const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) const {
				const uint32_t newVertexCount = this->curOffset + count;

				if (newVertexCount * sizeof(Vertex) > this->vertexBufferSize || newVertexCount * sizeof(uint32_t) > pBufferData->indexBufferSize) {

					if (!this->resize(pBufferData, newVertexCount * 2u)) return;

				}

				if (!this->pLocalVertexBuffer || !this->pLocalIndexBuffer) return;

				for (uint32_t i = 0u; i < count; i++) {
					const uint32_t curIndex = this->curOffset + i;

					const Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, color };
					memcpy(&(this->pLocalVertexBuffer[curIndex]), &curVertex, sizeof(Vertex));

					this->pLocalIndexBuffer[curIndex] = curIndex;
				}

				this->curOffset += count;

				return;
			}


			bool resize(uint32_t newVertexCount) {
				
				if (newVertexCount <= this->curOffset) return true;

				const uint32_t oldOffset = this->curOffset;

				Vertex* const pTmpVertexBuffer = new Vertex[oldOffset];

				if (this->pLocalVertexBuffer) {
					memcpy(pTmpVertexBuffer, this->pLocalVertexBuffer, oldOffset * sizeof(Vertex));
				}
				else {
					memset(pTmpIndexBuffer, 0u, oldOffset * sizeof(Vertex));
				}
				
				int32_t* const pTmpIndexBuffer = new int32_t[oldOffset];

				if (this->pLocalIndexBuffer) {
					memcpy(pTmpIndexBuffer, this->pLocalIndexBuffer, oldOffset * sizeof(uint32_t));
				}
				else {
					memset(pTmpIndexBuffer, 0u, oldOffset * sizeof(uint32_t));
				}
				
				this->destroy();

				if (!this->create(newVertexCount)) {
					delete[] pTmpVertexBuffer;
					delete[] pTmpIndexBuffer;

					return false;
				}

				if (!this->map()) {
					delete[] pTmpVertexBuffer;
					delete[] pTmpIndexBuffer;

					return false;
				}

				memcpy(this->pLocalVertexBuffer, pTmpVertexBuffer, oldOffset * sizeof(Vertex));
				memcpy(this->pLocalIndexBuffer, pTmpIndexBuffer, oldOffset * sizeof(uint32_t));

				this->curOffset = oldOffset;

				delete[] pTmpVertexBuffer;
				delete[] pTmpIndexBuffer;

				return true;

			}

		}

	}

}