#include "ogl2BufferBackend.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			BufferBackend::BufferBackend() :
				_f{}, _mode{}, _shaderProgram{}, _projectionMatrix{}, _projectionMatrixIndex{},
				_posIndex{}, _colIndex{}, _uvIndex{}, _vertexBufferId{ UINT_MAX }, _indexBufferId{ UINT_MAX } {}


			BufferBackend::~BufferBackend() {
				this->destroy();

				return;
			}


			void BufferBackend::initialize(Functions f, GLenum mode, GLint* viewport, GLuint shaderProgram) {
				this->_f = f;
				this->_mode = mode;
				this->_shaderProgram = shaderProgram;

				const GLfloat viewLeft = static_cast<GLfloat>(viewport[0]);
				const GLfloat viewRight = static_cast<GLfloat>(viewport[0] + viewport[2]);
				const GLfloat viewTop = static_cast<GLfloat>(viewport[1]);
				const GLfloat viewBottom = static_cast<GLfloat>(viewport[1] + viewport[3]);

				const GLfloat ortho[][4]{
					{ 2.f / (viewRight - viewLeft), 0.f, 0.f, 0.f  },
					{ 0.f, 2.f / (viewTop - viewBottom), 0.f, 0.f },
					{ 0.f, 0.f, .5f, 0.f },
					{ (viewLeft + viewRight) / (viewLeft - viewRight), (viewTop + viewBottom) / (viewBottom - viewTop), .5f, 1.f }
				};

				memcpy(this->_projectionMatrix, ortho, sizeof(this->_projectionMatrix));

				this->_projectionMatrixIndex = this->_f.pGlGetUniformLocation(this->_shaderProgram, "projectionMatrix");
				this->_posIndex = this->_f.pGlGetAttribLocation(this->_shaderProgram, "pos");
				this->_colIndex = this->_f.pGlGetAttribLocation(this->_shaderProgram, "col");
				this->_uvIndex = this->_f.pGlGetAttribLocation(this->_shaderProgram, "uv");

				return;
			}


			bool BufferBackend::create(uint32_t capacity) {
				this->_vertexBufferId = UINT_MAX;
				this->_indexBufferId = UINT_MAX;

				const uint32_t vertexBufferSize = capacity * sizeof(Vertex);

				if (!this->createBuffer(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, vertexBufferSize, &this->_vertexBufferId)) return false;

				const uint32_t indexBufferSize = capacity * sizeof(GLuint);

				if (!this->createBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, indexBufferSize, &this->_indexBufferId)) return false;

				return true;
			}


			void BufferBackend::destroy() {

				if (this->_indexBufferId != UINT_MAX) {
					this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
					this->_f.pGlDeleteBuffers(1, &this->_indexBufferId);
					this->_indexBufferId = UINT_MAX;
				}

				if (this->_vertexBufferId != UINT_MAX) {
					this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
					this->_f.pGlDeleteBuffers(1, &this->_vertexBufferId);
					this->_vertexBufferId = UINT_MAX;
				}

				return;
			}


			bool BufferBackend::map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) {

				GLuint curArrayBufferId = UINT_MAX;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curArrayBufferId));

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);
				*ppLocalVertexBuffer = reinterpret_cast<Vertex*>(this->_f.pGlMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curArrayBufferId);

				if (!(*ppLocalVertexBuffer)) return false;

				GLuint curElementArrayBufferId = UINT_MAX;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curElementArrayBufferId));

				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);
				*ppLocalIndexBuffer = reinterpret_cast<uint32_t*>(this->_f.pGlMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));
				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curElementArrayBufferId);

				if (!(*ppLocalIndexBuffer)) {
					this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
					
					return false;
				}

				return true;
			}


			bool BufferBackend::prepare() {
				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);
				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);
				
				this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
				this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

				this->_f.pGlUseProgram(this->_shaderProgram);
				this->_f.pGlUniformMatrix4fv(this->_projectionMatrixIndex, 1, GL_FALSE, &this->_projectionMatrix[0][0]);

				this->_f.pGlEnableVertexAttribArray(this->_posIndex);
				this->_f.pGlVertexAttribPointer(this->_posIndex, sizeof(Vector2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(0));

				this->_f.pGlEnableVertexAttribArray(this->_colIndex);
				this->_f.pGlVertexAttribPointer(this->_colIndex, sizeof(Color), GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2)));

				this->_f.pGlEnableVertexAttribArray(this->_uvIndex);
				this->_f.pGlVertexAttribPointer(this->_uvIndex, sizeof(Vector2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2) + sizeof(Color)));

				return true;
			}


			void BufferBackend::draw(TextureId textureId, uint32_t index, uint32_t count) {
				
				if (textureId) {
					glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));
				}
					
				glDrawElements(this->_mode, count, GL_UNSIGNED_INT, reinterpret_cast<GLvoid*>(index * sizeof(uint32_t)));

				return;
			}


			bool BufferBackend::createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const {
				this->_f.pGlGenBuffers(1, pId);

				if (*pId == UINT_MAX) return false;

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