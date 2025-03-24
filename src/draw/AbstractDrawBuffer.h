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

		typedef uint64_t TextureId;

		class AbstractDrawBuffer {
		protected:
			Vertex* _pLocalVertexBuffer;
			uint32_t* _pLocalIndexBuffer;
			TextureId* _pTextureBuffer;

			uint32_t _size;
			uint32_t _capacity;

		public:
			AbstractDrawBuffer() : _pLocalVertexBuffer{}, _pLocalIndexBuffer{}, _pTextureBuffer{}, _size{}, _capacity{} {}

			// Creates a new buffer with all internal resources.
			//
			// Parameters:
			// 
			// [in] capacity:
			// Capacity of the buffer.
			//
			// Return:
			// True on success, false on failure.
			virtual bool create(uint32_t capacity) = 0;

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
			void append(const Vector2* data, uint32_t count, Color color, Vector2 offset = { 0.f, 0.f }, TextureId textureId = 0ull) {
				const uint32_t newSize = this->_size + count;

				if (newSize > this->_capacity) {

					if (!this->reserve(newSize * 2u)) return;

				}

				for (uint32_t i = 0u; i < count; i++) {
					Vector2 uv{};

					if (i % 3) {
						
						if (i % 2) {
							uv.x = 1.f;
							uv.y = 0.f;
						}
						else {
							uv.x = 0.f;
							uv.y = 1.f;
						}

					}
					else {
						
						if ((i / 3) % 2) {
							uv.x = 1.f;
							uv.y = 1.f;
						}
						else {
							uv.x = 0.f;
							uv.y = 0.f;
						}
					}

					this->_pLocalVertexBuffer[this->_size] = { { data[i].x + offset.x, data[i].y + offset.y }, color, uv };
					this->_pLocalIndexBuffer[this->_size] = this->_size;
					this->_pTextureBuffer[this->_size] = textureId;
					this->_size++;
				}

				return;
			}


			// Increases the capacity of the buffer if the desired capacity is greater than the current one.
			//
			// Parameters:
			// 
			// [in] capacity:
			// New capacity of the buffer in vertices after the resize.
			//
			// Return:
			// True on success, false on failure.
			bool reserve(uint32_t capacity) {

				if (capacity <= this->_capacity) return true;

				Vertex* const pTmpVertexBuffer = reinterpret_cast<Vertex*>(calloc(this->_size, sizeof(Vertex)));

				if (!pTmpVertexBuffer) return false;

				if (this->_pLocalVertexBuffer) {
					memcpy(pTmpVertexBuffer, this->_pLocalVertexBuffer, this->_size * sizeof(Vertex));
				}
				else {
					memset(pTmpVertexBuffer, 0, this->_size);
				}

				int32_t* const pTmpIndexBuffer = reinterpret_cast<int32_t*>(calloc(this->_size, sizeof(int32_t)));

				if (!pTmpIndexBuffer) {
					free(pTmpVertexBuffer);

					return false;
				}

				if (this->_pLocalIndexBuffer) {
					memcpy(pTmpIndexBuffer, this->_pLocalIndexBuffer, this->_size * sizeof(uint32_t));
				}
				else {
					memset(pTmpIndexBuffer, 0, this->_size * sizeof(uint32_t));
				}

				int32_t* const pTmpTextureBuffer = reinterpret_cast<int32_t*>(calloc(this->_size, sizeof(TextureId)));

				if (!pTmpTextureBuffer) {
					free(pTmpVertexBuffer);
					free(pTmpIndexBuffer);

					return false;
				}

				if (this->_pTextureBuffer) {
					memcpy(pTmpTextureBuffer, this->_pTextureBuffer, this->_size * sizeof(TextureId));
				}
				else {
					memset(pTmpTextureBuffer, 0, this->_size * sizeof(TextureId));
				}

				const uint32_t oldSize = this->_size;
				this->destroy();

				if (!this->create(capacity)) {
					free(pTmpVertexBuffer);
					free(pTmpIndexBuffer);
					free(pTmpTextureBuffer);

					return false;
				}

				if (!this->map()) {
					free(pTmpVertexBuffer);
					free(pTmpIndexBuffer);
					free(pTmpTextureBuffer);

					return false;
				}

				if (this->_pLocalVertexBuffer && this->_pLocalIndexBuffer && this->_pTextureBuffer) {
					memcpy(this->_pLocalVertexBuffer, pTmpVertexBuffer, oldSize * sizeof(Vertex));
					memcpy(this->_pLocalIndexBuffer, pTmpIndexBuffer, oldSize * sizeof(uint32_t));
					memcpy(this->_pTextureBuffer, pTmpTextureBuffer, oldSize * sizeof(TextureId));

					this->_size = oldSize;
				}
				
				free(pTmpVertexBuffer);
				free(pTmpIndexBuffer);
				free(pTmpTextureBuffer);

				return true;
			}

			protected:
			void reset() {
				this->_pLocalVertexBuffer = nullptr;
				this->_pLocalIndexBuffer = nullptr;
				this->_pTextureBuffer = nullptr;
				this->_size = 0u;
				this->_capacity = 0u;
				
				return;
			}

		};

	}

}