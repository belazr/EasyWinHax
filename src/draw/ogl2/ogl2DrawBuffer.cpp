#include "ogl2DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			DrawBuffer::DrawBuffer() : AbstractDrawBuffer(), _f{}, _vertexBufferId{ UINT_MAX }, _indexBufferId{ UINT_MAX }, _mode{} {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}


			void DrawBuffer::initialize(Functions f, GLenum mode) {
				this->_f = f;
				this->_mode = mode;

				return;
			}


			bool DrawBuffer::create(uint32_t capacity) {
				this->reset();

				this->_vertexBufferId = UINT_MAX;
				this->_indexBufferId = UINT_MAX;

				const uint32_t vertexBufferSize = capacity * sizeof(Vertex);

				if (!this->createBuffer(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, vertexBufferSize, &this->_vertexBufferId)) return false;

				const uint32_t indexBufferSize = capacity * sizeof(GLuint);

				if (!this->createBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, indexBufferSize, &this->_indexBufferId)) return false;

				this->_pTextureBuffer = reinterpret_cast<TextureId*>(malloc(capacity * sizeof(TextureId)));

				if (!this->_pTextureBuffer) return false;

				this->_capacity = capacity;

				return true;
			}


			void DrawBuffer::destroy() {

				if (this->_indexBufferId != UINT_MAX) {
					this->_f.pGlDeleteBuffers(1, &this->_indexBufferId);
					this->_indexBufferId = UINT_MAX;
				}

				if (this->_vertexBufferId != UINT_MAX) {
					this->_f.pGlDeleteBuffers(1, &this->_vertexBufferId);
					this->_vertexBufferId = UINT_MAX;
				}

				if (this->_pTextureBuffer) {
					free(this->_pTextureBuffer);
					this->_pTextureBuffer = nullptr;
				}

				this->reset();

				return;
			}


			bool DrawBuffer::map() {

				if (!this->_pLocalVertexBuffer) {
					GLuint curBufferId = 0u;
					glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curBufferId));

					this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);

					this->_pLocalVertexBuffer = reinterpret_cast<Vertex*>(this->_f.pGlMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

					this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curBufferId);

					if (!this->_pLocalVertexBuffer) return false;

				}

				if (!this->_pLocalIndexBuffer) {
					GLuint curBufferId = 0u;
					glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curBufferId));

					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);

					this->_pLocalIndexBuffer = reinterpret_cast<uint32_t*>(this->_f.pGlMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curBufferId);

					if (!this->_pLocalIndexBuffer) return false;

				}

				return true;
			}


			void DrawBuffer::draw() {
				GLuint curVertexBufferId = 0u;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curVertexBufferId));

				GLuint curIndexBufferId = 0u;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curIndexBufferId));

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);

				// these should always be the same, no need to get them dynamically
				constexpr GLuint POS_INDEX = 2u;
				constexpr GLuint COL_INDEX = 0u;
				constexpr GLuint UV_INDEX = 1u;
				
				this->_f.pGlEnableVertexAttribArray(POS_INDEX);
				this->_f.pGlVertexAttribPointer(POS_INDEX, sizeof(Vector2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(0));

				this->_f.pGlEnableVertexAttribArray(COL_INDEX);
				this->_f.pGlVertexAttribPointer(COL_INDEX, sizeof(Color), GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2)));

				this->_f.pGlEnableVertexAttribArray(UV_INDEX);
				this->_f.pGlVertexAttribPointer(UV_INDEX, sizeof(Vector2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2) + sizeof(Color)));

				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);

				this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
				this->_pLocalVertexBuffer = nullptr;

				this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
				this->_pLocalIndexBuffer = nullptr;

				glDrawElements(this->_mode, static_cast<GLsizei>(this->_size), GL_UNSIGNED_INT, nullptr);

				this->_f.pGlDisableVertexAttribArray(POS_INDEX);
				this->_f.pGlDisableVertexAttribArray(COL_INDEX);
				this->_f.pGlDisableVertexAttribArray(UV_INDEX);

				this->_size = 0u;

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curVertexBufferId);
				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curIndexBufferId);

				return;
			}


			bool DrawBuffer::createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const {
				this->_f.pGlGenBuffers(1, pId);

				if (*pId == UINT_MAX) return false;

				GLuint curBufferId = 0u;
				glGetIntegerv(binding, reinterpret_cast<GLint*>(&curBufferId));

				this->_f.pGlBindBuffer(target, *pId);
				this->_f.pGlBufferData(target, size, nullptr, GL_DYNAMIC_DRAW);

				this->_f.pGlBindBuffer(target, curBufferId);

				return true;
			}

		}

	}

}