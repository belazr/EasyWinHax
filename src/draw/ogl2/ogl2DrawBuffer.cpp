#include "ogl2DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			DrawBuffer::DrawBuffer(Functions f, GLenum mode) : _f{ f }, _vertexBufferId{ UINT_MAX }, _indexBufferId{ UINT_MAX }, _mode{ mode } {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();
			}


			bool DrawBuffer::create(uint32_t vertexCount) {
				memset(this, 0, sizeof(DrawBuffer));
				this->_vertexBufferId = UINT_MAX;
				this->_indexBufferId = UINT_MAX;

				const uint32_t newVertexBufferSize = vertexCount * sizeof(Vertex);

				if (!this->createBuffer(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, newVertexBufferSize, &this->_vertexBufferId)) return false;

				this->vertexBufferSize = newVertexBufferSize;

				const uint32_t newIndexBufferSize = vertexCount * sizeof(GLuint);

				if (!this->createBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, newIndexBufferSize, &this->_indexBufferId)) return false;

				this->indexBufferSize = indexBufferSize;

				return true;
			}


			void DrawBuffer::destroy() {

				if (this->_indexBufferId != UINT_MAX) {
					GLuint curBufferId{};
					glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curBufferId));

					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);
					this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curBufferId);

					this->_f.pGlDeleteBuffers(1, &this->_indexBufferId);
					this->_indexBufferId = UINT_MAX;
				}

				if (this->_vertexBufferId != UINT_MAX) {
					GLuint curArrayBufferId{};
					glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curArrayBufferId));

					this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);
					this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
					this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curArrayBufferId);

					this->_f.pGlDeleteBuffers(1, &this->_vertexBufferId);
					this->_vertexBufferId = UINT_MAX;
				}


				this->vertexBufferSize = 0ul;
				this->pLocalVertexBuffer = nullptr;
				this->indexBufferSize = 0ul;
				this->pLocalIndexBuffer = nullptr;
				this->curOffset = 0u;

				return;
			}


			bool DrawBuffer::map() {

				if (!this->pLocalVertexBuffer) {
					GLuint curBufferId{};
					glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curBufferId));

					this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);

					this->pLocalVertexBuffer = reinterpret_cast<Vertex*>(this->_f.pGlMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

					this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curBufferId);

					if (!this->pLocalVertexBuffer) return false;

				}

				if (!this->pLocalIndexBuffer) {
					GLuint curBufferId{};
					glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curBufferId));

					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);

					this->pLocalIndexBuffer = reinterpret_cast<uint32_t*>(this->_f.pGlMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curBufferId);

					if (!this->pLocalIndexBuffer) return false;

				}

				return true;
			}


			void DrawBuffer::draw() {
				GLuint curVertexBufferId{};
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curVertexBufferId));

				GLuint curIndexBufferId{};
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curIndexBufferId));

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);
				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_COLOR_ARRAY);

				// third argument describes the offset in the Vertext struct, not the actuall address of the buffer
				glVertexPointer(2, GL_FLOAT, sizeof(Vertex), nullptr);
				glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2)));

				this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
				this->pLocalVertexBuffer = nullptr;

				this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
				this->pLocalIndexBuffer = nullptr;

				glDrawElements(this->_mode, static_cast<GLsizei>(this->curOffset), GL_UNSIGNED_INT, nullptr);

				glDisableClientState(GL_COLOR_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);

				this->curOffset = 0u;

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