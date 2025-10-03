#include "ogl2Backend.h"
#include "ogl2Shaders.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			Backend::Backend() :
				_f{}, _shaderProgramId{ UINT_MAX }, _projectionMatrixIndex{}, _viewport {}, _state{} {}


			Backend::~Backend() {
				this->_bufferBackend.destroy();
				glDeleteTextures(static_cast<GLsizei>(this->_textures.size()), this->_textures.data());

				if (!this->_f.pGlDeleteProgram) return;

				if (this->_shaderProgramId != UINT_MAX) {
					this->_f.pGlDeleteProgram(this->_shaderProgramId);
				}

				return;
			}


			void Backend::setHookParameters(void* pArg1, void* pArg2) {
				UNREFERENCED_PARAMETER(pArg1);
				UNREFERENCED_PARAMETER(pArg2);

				return;
			}


			bool Backend::initialize() {

				if (!this->getProcAddresses()) return false;

				this->createShaderPrograms();
				
				constexpr uint32_t INITIAL_BUFFER_SIZE = 100u;

				this->_bufferBackend.initialize(this->_f, this->_shaderProgramId);

				if (!this->_bufferBackend.capacity()) {

					if (!this->_bufferBackend.create(INITIAL_BUFFER_SIZE)) return false;

				}

				return true;
			}


            TextureId Backend::loadTexture(const Color* data, uint32_t width, uint32_t height) {
				GLint curTextureId = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &curTextureId);

				GLuint textureId = 0u;
				glGenTextures(1, &textureId);
				glBindTexture(GL_TEXTURE_2D, textureId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
				
				glBindTexture(GL_TEXTURE_2D, curTextureId);

				this->_textures.append(textureId);

				return static_cast<TextureId>(textureId);
            }


			bool Backend::beginFrame() {
				this->saveState();

				this->_f.pGlUseProgram(this->_shaderProgramId);
				
				GLint viewport[4]{};
				glGetIntegerv(GL_VIEWPORT, viewport);

				if (this->viewportChanged(viewport)) {
					memcpy(this->_viewport, viewport, sizeof(this->_viewport));
					this->setVertexShaderConstants();
				}

				glDepthFunc(GL_ALWAYS);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				return true;
			}


			void Backend::endFrame() {
				this->restoreState();

				return;
			}


			IBufferBackend* Backend::getBufferBackend() {

				return &this->_bufferBackend;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) const {
				*frameWidth = static_cast<float>(this->_viewport[2]);
				*frameHeight = static_cast<float>(this->_viewport[3]);

				return;
			}


			#define ASSIGN_PROC_ADDRESS(f) this->_f.pGl##f = reinterpret_cast<tGl##f>(wglGetProcAddress("gl"#f))

			bool Backend::getProcAddresses() {
				ASSIGN_PROC_ADDRESS(CreateShader);
				ASSIGN_PROC_ADDRESS(ShaderSource);
				ASSIGN_PROC_ADDRESS(CompileShader);
				ASSIGN_PROC_ADDRESS(CreateProgram);
				ASSIGN_PROC_ADDRESS(AttachShader);
				ASSIGN_PROC_ADDRESS(LinkProgram);
				ASSIGN_PROC_ADDRESS(DetachShader);
				ASSIGN_PROC_ADDRESS(DeleteShader);
				ASSIGN_PROC_ADDRESS(GetUniformLocation);
				ASSIGN_PROC_ADDRESS(GetAttribLocation);
				ASSIGN_PROC_ADDRESS(GenBuffers);
				ASSIGN_PROC_ADDRESS(BindBuffer);
				ASSIGN_PROC_ADDRESS(BufferData);
				ASSIGN_PROC_ADDRESS(MapBuffer);
				ASSIGN_PROC_ADDRESS(UnmapBuffer);
				ASSIGN_PROC_ADDRESS(EnableVertexAttribArray);
				ASSIGN_PROC_ADDRESS(VertexAttribPointer);
				ASSIGN_PROC_ADDRESS(DisableVertexAttribArray);
				ASSIGN_PROC_ADDRESS(DeleteBuffers);
				ASSIGN_PROC_ADDRESS(UseProgram);
				ASSIGN_PROC_ADDRESS(UniformMatrix4fv);
				ASSIGN_PROC_ADDRESS(DeleteProgram);

				for (size_t i = 0u; i < _countof(this->_fPtrs); i++) {

					if (!(this->_fPtrs[i])) return false;

				}

				return true;
			}

			#undef ASSIGN_PROC_ADDRESS


			void Backend::createShaderPrograms() {
				const GLuint vertexShaderId = this->createShader(GL_VERTEX_SHADER, VERTEX_SHADER);
				const GLuint fragmentShaderId = this->createShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
				
				this->_shaderProgramId = this->createShaderProgram(vertexShaderId, fragmentShaderId);

				this->_f.pGlDeleteShader(fragmentShaderId);
				this->_f.pGlDeleteShader(vertexShaderId);

				this->_projectionMatrixIndex = this->_f.pGlGetUniformLocation(this->_shaderProgramId, "projectionMatrix");

				return;
			}


			GLuint Backend::createShader(GLenum type, const GLchar* pShader) const {
				const GLuint shaderId = this->_f.pGlCreateShader(type);
				this->_f.pGlShaderSource(shaderId, 1, &pShader, nullptr);
				this->_f.pGlCompileShader(shaderId);

				return shaderId;
			}


			GLuint Backend::createShaderProgram(GLuint vertexShaderId, GLuint fragmentShaderId) const {
				const GLuint programId = this->_f.pGlCreateProgram();
				this->_f.pGlAttachShader(programId, vertexShaderId);
				this->_f.pGlAttachShader(programId, fragmentShaderId);
				this->_f.pGlLinkProgram(programId);
				this->_f.pGlDetachShader(programId, fragmentShaderId);
				this->_f.pGlDetachShader(programId, vertexShaderId);

				return programId;
			}


			bool Backend::viewportChanged(const GLint* pViewport) const {
				bool topLeftChanged = pViewport[0] != this->_viewport[0] || pViewport[1] != this->_viewport[1];
				bool dimensionChanged = pViewport[2] != this->_viewport[2] || pViewport[3] != this->_viewport[3];
				
				return topLeftChanged || dimensionChanged;
			}


			void Backend::setVertexShaderConstants() const {
				const GLfloat viewLeft = static_cast<GLfloat>(this->_viewport[0]);
				const GLfloat viewRight = static_cast<GLfloat>(this->_viewport[0] + this->_viewport[2]);
				const GLfloat viewTop = static_cast<GLfloat>(this->_viewport[1]);
				const GLfloat viewBottom = static_cast<GLfloat>(this->_viewport[1] + this->_viewport[3]);

				const GLfloat ortho[][4]{
					{ 2.f / (viewRight - viewLeft), 0.f, 0.f, 0.f  },
					{ 0.f, 2.f / (viewTop - viewBottom), 0.f, 0.f },
					{ 0.f, 0.f, .5f, 0.f },
					{ (viewLeft + viewRight) / (viewLeft - viewRight), (viewTop + viewBottom) / (viewBottom - viewTop), .5f, 1.f }
				};

				this->_f.pGlUniformMatrix4fv(this->_projectionMatrixIndex, 1, GL_FALSE, &ortho[0][0]);

				return;
			}


			void Backend::saveState() {
				glPushAttrib(GL_ALL_ATTRIB_BITS);
				glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
				glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&this->_state.shaderProgramId));
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&this->_state.vertexBufferId));
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&this->_state.indexBufferId));
				glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&this->_state.textureId));

				return;
			}


			void Backend::restoreState() const {
				glBindTexture(GL_TEXTURE_2D, this->_state.textureId);
				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_state.indexBufferId);
				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, this->_state.vertexBufferId);
				this->_f.pGlUseProgram(this->_state.shaderProgramId);
				glPopClientAttrib();
				glPopAttrib();

				return;
			}

		}

	}

}