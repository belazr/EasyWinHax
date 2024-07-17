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
		public:
			Vertex* pLocalVertexBuffer{};
			uint32_t* pLocalIndexBuffer{};
			uint32_t vertexBufferSize{};
			uint32_t indexBufferSize{};
			uint32_t curOffset{};

			virtual bool create(uint32_t vertexCount) = 0;
			virtual void destroy() = 0;
			virtual bool map() = 0;
			virtual void draw() = 0;


			void append(const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) {
				const uint32_t newVertexCount = this->curOffset + count;

				if (newVertexCount * sizeof(Vertex) > this->vertexBufferSize || newVertexCount * sizeof(uint32_t) > this->indexBufferSize) {

					if (!this->resize(newVertexCount * 2u)) return;

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

				Vertex* const pTmpVertexBuffer = reinterpret_cast<Vertex*>(calloc(oldOffset, sizeof(Vertex)));

				if (!pTmpVertexBuffer) return false;

				if (this->pLocalVertexBuffer) {
					memcpy(pTmpVertexBuffer, this->pLocalVertexBuffer, oldOffset * sizeof(Vertex));
				}
				else {
					memset(pTmpVertexBuffer, 0, oldOffset * sizeof(Vertex));
				}

				int32_t* const pTmpIndexBuffer = reinterpret_cast<int32_t*>(calloc(oldOffset, sizeof(int32_t)));

				if (!pTmpIndexBuffer) {
					free(pTmpVertexBuffer);

					return false;
				}

				if (this->pLocalIndexBuffer) {
					memcpy(pTmpIndexBuffer, this->pLocalIndexBuffer, oldOffset * sizeof(uint32_t));
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

				if (this->pLocalVertexBuffer && this->pLocalIndexBuffer) {
					memcpy(this->pLocalVertexBuffer, pTmpVertexBuffer, oldOffset * sizeof(Vertex));
					memcpy(this->pLocalIndexBuffer, pTmpIndexBuffer, oldOffset * sizeof(uint32_t));

					this->curOffset = oldOffset;
				}
				
				free(pTmpVertexBuffer);
				free(pTmpIndexBuffer);

				return true;
			}

		};

	}

}