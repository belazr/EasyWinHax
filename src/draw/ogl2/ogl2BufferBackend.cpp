#include "ogl2BufferBackend.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			BufferBackend::BufferBackend() :
				_f{}, _posIndex{}, _colIndex{}, _uvIndex{}, _vertexBufferId{ UINT_MAX }, _indexBufferId{ UINT_MAX }, _capacity{} {}


			BufferBackend::~BufferBackend() {

				if (!this->_f.pGlBindBuffer || !this->_f.pGlUnmapBuffer || !this->_f.pGlDeleteBuffers) return;
				
				this->destroy();

				return;
			}


			void BufferBackend::initialize(Functions f, GLuint shaderProgramId) {
				this->_f = f;

				this->_posIndex = this->_f.pGlGetAttribLocation(shaderProgramId, "pos");
				this->_colIndex = this->_f.pGlGetAttribLocation(shaderProgramId, "col");
				this->_uvIndex = this->_f.pGlGetAttribLocation(shaderProgramId, "uv");

				return;
			}


			bool BufferBackend::create(uint32_t capacity) {
				
				if (this->_capacity) return false;

				const uint32_t vertexBufferSize = capacity * sizeof(Vertex);

				if (!this->createBuffer(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, vertexBufferSize, &this->_vertexBufferId)) {
					this->destroy();

					return false;
				}

				const uint32_t indexBufferSize = capacity * sizeof(GLuint);

				if (!this->createBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, indexBufferSize, &this->_indexBufferId)) {
					this->destroy();
					
					return false;
				}

				this->_capacity = capacity;

				return true;
			}


			void BufferBackend::destroy() {

				if (this->_indexBufferId != UINT_MAX) {
					GLuint curIndexBufferId = UINT_MAX;
					glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curIndexBufferId));

					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);

					this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
					this->_f.pGlDeleteBuffers(1, &this->_indexBufferId);
					this->_indexBufferId = UINT_MAX;
					
					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curIndexBufferId);
				}

				if (this->_vertexBufferId != UINT_MAX) {
					GLuint curVertexBufferId = UINT_MAX;
					glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curVertexBufferId));

					this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);

					this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
					this->_f.pGlDeleteBuffers(1, &this->_vertexBufferId);
					this->_vertexBufferId = UINT_MAX;
					
					this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curVertexBufferId);
				}

				this->_capacity = 0u;

				return;
			}


			uint32_t BufferBackend::capacity() const {

				return this->_capacity;
			}


			bool BufferBackend::map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) {
				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);
				*ppLocalVertexBuffer = reinterpret_cast<Vertex*>(this->_f.pGlMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

				if (!(*ppLocalVertexBuffer)) {
					this->unmap();
				
					return false;
				}

				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);
				*ppLocalIndexBuffer = reinterpret_cast<uint32_t*>(this->_f.pGlMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

				if (!(*ppLocalIndexBuffer)) {
					this->unmap();

					return false;
				}

				return true;
			}


			void BufferBackend::unmap() {
				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);
				this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);
				this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);

				return;
			}


			bool BufferBackend::prepare() {
				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);

				this->_f.pGlEnableVertexAttribArray(this->_posIndex);
				this->_f.pGlVertexAttribPointer(this->_posIndex, sizeof(Vector2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(0));

				this->_f.pGlEnableVertexAttribArray(this->_colIndex);
				this->_f.pGlVertexAttribPointer(this->_colIndex, sizeof(Color), GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2)));

				this->_f.pGlEnableVertexAttribArray(this->_uvIndex);
				this->_f.pGlVertexAttribPointer(this->_uvIndex, sizeof(Vector2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2) + sizeof(Color)));

				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);

				return true;
			}


			void BufferBackend::draw(TextureId textureId, uint32_t index, uint32_t count) const {
				
				if (textureId) {
					glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));
				}
					
				glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, reinterpret_cast<GLvoid*>(index * sizeof(uint32_t)));

				return;
			}


			bool BufferBackend::createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const {
				this->_f.pGlGenBuffers(1, pId);
				
				GLuint curBufferId = UINT_MAX;
				glGetIntegerv(binding, reinterpret_cast<GLint*>(&curBufferId));

				this->_f.pGlBindBuffer(target, *pId);
				
				this->_f.pGlBufferData(target, size, nullptr, GL_DYNAMIC_DRAW);
				
				this->_f.pGlBindBuffer(target, curBufferId);

				return true;
			}

		}

	}

}