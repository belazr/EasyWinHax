#include "ogl2DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			DrawBuffer::DrawBuffer() : AbstractDrawBuffer(),
				_f{}, _mode{}, _shaderProgramPassthrough{}, _shaderProgramTexture{}, _projectionMatrix{},
				_projectionMatrixIndex{}, _posIndex{}, _colIndex{}, _uvIndex{}, _vertexBufferId { UINT_MAX }, _indexBufferId{ UINT_MAX } {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}


			void DrawBuffer::initialize(Functions f, GLenum mode, GLint* viewport, GLuint shaderProgramPassthrough, GLuint shaderProgramTexture) {
				this->_f = f;
				this->_mode = mode;
				this->_shaderProgramPassthrough = shaderProgramPassthrough;
				this->_shaderProgramTexture = shaderProgramTexture;

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

				// indices should always be the same for both shader programs since the use the same vertex shader
				this->_projectionMatrixIndex = this->_f.pGlGetUniformLocation(this->_shaderProgramPassthrough, "projectionMatrix");
				this->_posIndex = this->_f.pGlGetAttribLocation(this->_shaderProgramPassthrough, "pos");
				this->_colIndex = this->_f.pGlGetAttribLocation(this->_shaderProgramPassthrough, "col");
				this->_uvIndex = this->_f.pGlGetAttribLocation(this->_shaderProgramPassthrough, "uv");

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
				GLuint curProgramId = 0u;
				glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&curProgramId));
				
				this->_f.pGlUseProgram(this->_shaderProgramPassthrough);
				this->_f.pGlUniformMatrix4fv(this->_projectionMatrixIndex, 1, GL_FALSE, &this->_projectionMatrix[0][0]);

				GLuint curVertexBufferId = 0u;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curVertexBufferId));

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_vertexBufferId);
				
				this->_f.pGlEnableVertexAttribArray(this->_posIndex);
				this->_f.pGlVertexAttribPointer(this->_posIndex, sizeof(Vector2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(0));

				this->_f.pGlEnableVertexAttribArray(this->_colIndex);
				this->_f.pGlVertexAttribPointer(this->_colIndex, sizeof(Color), GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2)));

				this->_f.pGlEnableVertexAttribArray(this->_uvIndex);
				this->_f.pGlVertexAttribPointer(this->_uvIndex, sizeof(Vector2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2) + sizeof(Color)));

				GLuint curIndexBufferId = 0u;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curIndexBufferId));
				
				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_indexBufferId);

				this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
				this->_pLocalVertexBuffer = nullptr;

				this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
				this->_pLocalIndexBuffer = nullptr;

				glDrawElements(this->_mode, static_cast<GLsizei>(this->_size), GL_UNSIGNED_INT, nullptr);

				this->_f.pGlDisableVertexAttribArray(this->_posIndex);
				this->_f.pGlDisableVertexAttribArray(this->_colIndex);
				this->_f.pGlDisableVertexAttribArray(this->_uvIndex);

				this->_size = 0u;

				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curIndexBufferId);
				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curVertexBufferId);
				this->_f.pGlUseProgram(curProgramId);

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