#include "DrawBuffer.h"

namespace hax {

	namespace draw {

		DrawBuffer::DrawBuffer() : _pBufferBackend{}, _pLocalVertexBuffer{}, _pLocalIndexBuffer{}, _size{}, _capacity{} {}


		DrawBuffer::~DrawBuffer() {

			if (this->_pBufferBackend) {
				this->_pBufferBackend->unmap();
			}

			return;
		}


		bool DrawBuffer::beginFrame(IBufferBackend* pBufferBackend) {

			if (!pBufferBackend) return false;

			this->_pBufferBackend = pBufferBackend;
			this->_capacity = pBufferBackend->capacity();

			if (!this->_pBufferBackend->map(&this->_pLocalVertexBuffer, &this->_pLocalIndexBuffer)) {
				this->reset();

				return false;
			}

			return true;
		}


		void DrawBuffer::append(const Vertex* data, uint32_t count, TextureId textureId) {
			
			if (!this->_pLocalVertexBuffer) return;
			
			const uint32_t newSize = this->_size + count;

			if (newSize > this->_capacity) {

				if (!this->reserve(newSize * 2u)) return;

			}

			TextureBatch* pTextureBatch = nullptr;

			bool found = false;

			for (size_t i = 0u; i < this->_textures.size(); i++) {
				pTextureBatch = this->_textures + i;

				if (pTextureBatch->id == textureId) {
					found = true;

					break;
				}

			}

			if (!found) {
				const TextureBatch batch{ textureId, {} };
				this->_textures.append(batch);
				pTextureBatch = this->_textures + this->_textures.size() - 1u;
			}

			for (uint32_t i = 0u; i < count; i++) {
				this->_pLocalVertexBuffer[this->_size] = data[i];
				pTextureBatch->indices.append(this->_size);
				this->_size++;
			}

			return;
		}


		void DrawBuffer::endFrame() {
			
			if (!this->_pBufferBackend || !this->_pLocalIndexBuffer) return;
			
			uint32_t offset = 0u;

			for (size_t i = 0u; i < this->_textures.size(); i++) {
				const uint32_t count = static_cast<uint32_t>(this->_textures[i].indices.size());
				memcpy(this->_pLocalIndexBuffer + offset, this->_textures[i].indices.data(), count * sizeof(uint32_t));
				offset += count;
			}

			this->_pBufferBackend->unmap();
			this->_pLocalIndexBuffer = nullptr;
			this->_pLocalVertexBuffer = nullptr;

			if (!this->_pBufferBackend->prepare()) {
				this->reset();
				
				return;
			}

			uint32_t index = 0u;

			for (size_t i = 0u; i < this->_textures.size(); i++) {
				const uint32_t count = static_cast<uint32_t>(this->_textures[i].indices.size());
				this->_pBufferBackend->draw(this->_textures[i].id, index, count);
				this->_textures[i].indices.resize(0u);
				index += count;
			}

			this->reset();
			
			return;
		}


		void DrawBuffer::reset() {
			this->_pBufferBackend = nullptr;
			this->_pLocalVertexBuffer = nullptr;
			this->_pLocalIndexBuffer = nullptr;
			this->_size = 0u;
			this->_capacity = 0u;
		}


		bool DrawBuffer::reserve(uint32_t capacity) {

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

	}
}