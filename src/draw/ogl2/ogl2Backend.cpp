#include "ogl2Backend.h"
#include "ogl2Shaders.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			Backend::Backend() :
				_f{}, _shaderProgramTextureId{}, _shaderProgramPassthroughId{}, _viewport{}, _depthFunc{},
				_blendEnabled{}, _srcAlphaBlendFunc{}, _dstAlphaBlendFunc{}, _textureTriangleListBuffer{}, _triangleListBuffer{} {}


			Backend::~Backend() {
				this->_triangleListBuffer.destroy();
				this->_textureTriangleListBuffer.destroy();
				
				if (this->_f.pGlDeleteProgram) {
					this->_f.pGlDeleteProgram(this->_shaderProgramPassthroughId);
					this->_f.pGlDeleteProgram(this->_shaderProgramTextureId);
				}

				glDeleteTextures(static_cast<GLsizei>(this->_textures.size()), this->_textures.data());
			}


			void Backend::setHookArguments(void* pArg1, void* pArg2) {
				UNREFERENCED_PARAMETER(pArg1);
				UNREFERENCED_PARAMETER(pArg2);

				return;
			}


			bool Backend::initialize() {

				if (!this->getProcAddresses()) return false;

				this->createShaders();

				return true;
			}


            TextureId Backend::loadTexture(const Color* data, uint32_t width, uint32_t height) {
				GLint curTexture = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &curTexture);

				GLuint textureId = 0u;
				glGenTextures(1, &textureId);
				glBindTexture(GL_TEXTURE_2D, textureId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
				
				glBindTexture(GL_TEXTURE_2D, curTexture);

				this->_textures.append(textureId);

				return static_cast<TextureId>(textureId);
            }


			bool Backend::beginFrame() {
				GLint viewport[4]{};
				glGetIntegerv(GL_VIEWPORT, viewport);

				if (viewport[2] != this->_viewport[2] || viewport[3] != this->_viewport[3]) {
					constexpr uint32_t INITIAL_BUFFER_SIZE = 100u;

					this->_textureTriangleListBuffer.initialize(this->_f, viewport, this->_shaderProgramTextureId);

					if (!this->_textureTriangleListBuffer.capacity()) {

						if (!this->_textureTriangleListBuffer.create(INITIAL_BUFFER_SIZE)) return false;

					}

					this->_triangleListBuffer.initialize(this->_f, viewport, this->_shaderProgramPassthroughId);

					if (!this->_triangleListBuffer.capacity()) {

						if (!this->_triangleListBuffer.create(INITIAL_BUFFER_SIZE)) return false;

					}

					memcpy(this->_viewport, viewport, sizeof(this->_viewport));
				}

				glGetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<GLint*>(&this->_depthFunc));
				glDepthFunc(GL_ALWAYS);

				this->_blendEnabled = glIsEnabled(GL_BLEND);

				if (!this->_blendEnabled) {
					glEnable(GL_BLEND);
				}

				glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint*>(&this->_srcAlphaBlendFunc));
				glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint*>(&this->_dstAlphaBlendFunc));
				
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				return true;
			}


			void Backend::endFrame() {
				glBlendFunc(this->_srcAlphaBlendFunc, this->_dstAlphaBlendFunc);

				if (!this->_blendEnabled) {
					glDisable(GL_BLEND);
				}

				glDepthFunc(this->_depthFunc);

				return;
			}


			IBufferBackend* Backend::getTextureTriangleListBufferBackend() {

				return &this->_textureTriangleListBuffer;
			}


			IBufferBackend* Backend::getTriangleListBufferBackend()  {

				return &this->_triangleListBuffer;
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


			void Backend::createShaders() {
				const GLuint vertexShader = this->_f.pGlCreateShader(GL_VERTEX_SHADER);
				this->_f.pGlShaderSource(vertexShader, 1, &VERTEX_SHADER, nullptr);
				this->_f.pGlCompileShader(vertexShader);

				const GLuint fragmentShaderTexture = this->_f.pGlCreateShader(GL_FRAGMENT_SHADER);
				this->_f.pGlShaderSource(fragmentShaderTexture, 1, &FRAGMENT_SHADER_TEXTURE, nullptr);
				this->_f.pGlCompileShader(fragmentShaderTexture);

				this->_shaderProgramTextureId = this->_f.pGlCreateProgram();
				this->_f.pGlAttachShader(this->_shaderProgramTextureId, vertexShader);
				this->_f.pGlAttachShader(this->_shaderProgramTextureId, fragmentShaderTexture);
				this->_f.pGlLinkProgram(this->_shaderProgramTextureId);

				this->_f.pGlDetachShader(this->_shaderProgramTextureId, vertexShader);
				this->_f.pGlDetachShader(this->_shaderProgramTextureId, fragmentShaderTexture);

				this->_f.pGlDeleteShader(fragmentShaderTexture);

				const GLuint fragmentShaderPassthrough = this->_f.pGlCreateShader(GL_FRAGMENT_SHADER);
				this->_f.pGlShaderSource(fragmentShaderPassthrough, 1, &FRAGMENT_SHADER_PASSTHROUGH, nullptr);
				this->_f.pGlCompileShader(fragmentShaderPassthrough);

				this->_shaderProgramPassthroughId = this->_f.pGlCreateProgram();
				this->_f.pGlAttachShader(this->_shaderProgramPassthroughId, vertexShader);
				this->_f.pGlAttachShader(this->_shaderProgramPassthroughId, fragmentShaderPassthrough);
				this->_f.pGlLinkProgram(this->_shaderProgramPassthroughId);

				this->_f.pGlDetachShader(this->_shaderProgramPassthroughId, vertexShader);
				this->_f.pGlDetachShader(this->_shaderProgramPassthroughId, fragmentShaderPassthrough);

				this->_f.pGlDeleteShader(fragmentShaderPassthrough);
				this->_f.pGlDeleteShader(vertexShader);

				return;
			}

		}

	}

}