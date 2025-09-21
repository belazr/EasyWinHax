#include "TextureDrawBuffer.h"

namespace hax {

	namespace draw {

		TextureDrawBuffer::TextureDrawBuffer() : AbstractDrawBuffer(), _textures{} {}


		void TextureDrawBuffer::append(const Vertex* data, uint32_t count, TextureId textureId) {
			
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


		void TextureDrawBuffer::endFrame() {
			
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

	}
}