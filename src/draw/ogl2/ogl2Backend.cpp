#include "ogl2Backend.h"
#include "..\font\Font.h"
#include <Windows.h>
#include <vector>
namespace hax {

	namespace draw {

		namespace ogl2 {

			Backend::Backend() :
				_f{}, _shaderProgramPassthrough{}, _shaderProgramTexture{}, _projMatrixLocation{},
				_viewport {}, _depthFunc{}, _triangleListBuffer{}, _pointListBuffer{} {}


			Backend::~Backend() {
				this->_pointListBuffer.destroy();
				this->_triangleListBuffer.destroy();
				
				if (this->_f.pGlDeleteProgram) {
					this->_f.pGlDeleteProgram(this->_shaderProgramTexture);
					this->_f.pGlDeleteProgram(this->_shaderProgramPassthrough);
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
				this->_projMatrixLocation = this->_f.pGlGetUniformLocation(this->_shaderProgramPassthrough, "projectionMatrix");

				return true;
			}


            TextureId Backend::loadTexture(const Color* data, uint32_t width, uint32_t height) {
				GLint curTexture{};
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &curTexture);

				GLuint textureId{};
				glGenTextures(1, &textureId);
				glBindTexture(GL_TEXTURE_2D, textureId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
					this->_triangleListBuffer.initialize(this->_f, GL_TRIANGLES);
					this->_triangleListBuffer.destroy();

					constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_SIZE = 99u;

					if (!this->_triangleListBuffer.create(INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return false;

					this->_pointListBuffer.initialize(this->_f, GL_POINTS);
					this->_pointListBuffer.destroy();

					constexpr uint32_t INITIAL_POINT_LIST_BUFFER_SIZE = 1000u;

					if (!this->_pointListBuffer.create(INITIAL_POINT_LIST_BUFFER_SIZE)) return false;

					memcpy(this->_viewport, viewport, sizeof(this->_viewport));
				}

				glGetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<GLint*>(&this->_depthFunc));
				glDepthFunc(GL_ALWAYS);

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

				this->_f.pGlUseProgram(this->_shaderProgramPassthrough);
				this->_f.pGlUniformMatrix4fv(this->_projMatrixLocation, 1, GL_FALSE, &ortho[0][0]);
				
				return true;
			}


			void Backend::endFrame() {
				this->_f.pGlUseProgram(0);
				glDepthFunc(this->_depthFunc);

				return;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) {
				*frameWidth = static_cast<float>(this->_viewport[2]);
				*frameHeight = static_cast<float>(this->_viewport[3]);

				return;
			}

			
			AbstractDrawBuffer* Backend::getTriangleListBuffer() {
				
				return &this->_triangleListBuffer;
			}


			AbstractDrawBuffer* Backend::getPointListBuffer() {

				return &this->_pointListBuffer;
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


			const GLchar* VERTEX_SHADER =
				"#version 120\n"

				"uniform mat4 projectionMatrix;\n"

				"attribute vec2 pos;\n"
				"attribute vec4 col;\n"
				"attribute vec2 uv;\n"

				"varying vec4 colOut;\n"
				"varying vec2 uvOut;\n"

				"void main() {\n"
				"    colOut = col;\n"
				"    uvOut = uv;\n"
				"    gl_Position = projectionMatrix * vec4(pos.xy,0,1);\n"
				"}\n";

			const GLchar* FRAGMENT_SHADER_PASSTHROUGH =
				"#version 120\n"

				"varying vec4 colOut;\n"

				"void main() {\n"
				"    gl_FragColor = colOut;\n"
				"}\n";

			const GLchar* FRAGMENT_SHADER_TEXTURE =
				"#version 120\n"
				
				"uniform sampler2D texSampler;\n"
				
				"varying vec4 colOut;\n"
				"varying vec2 uvOut;\n"
				
				"void main() {\n"
				"    gl_FragColor = colOut * texture2D(texSampler, uvOut);\n"
				"}\n";

			void Backend::createShaders() {
				const GLuint vertexShader = this->_f.pGlCreateShader(GL_VERTEX_SHADER);
				this->_f.pGlShaderSource(vertexShader, 1, &VERTEX_SHADER, nullptr);
				this->_f.pGlCompileShader(vertexShader);

				const GLuint fragmentShaderPassthrough = this->_f.pGlCreateShader(GL_FRAGMENT_SHADER);
				this->_f.pGlShaderSource(fragmentShaderPassthrough, 1, &FRAGMENT_SHADER_PASSTHROUGH, nullptr);
				this->_f.pGlCompileShader(fragmentShaderPassthrough);

				this->_shaderProgramPassthrough = this->_f.pGlCreateProgram();
				this->_f.pGlAttachShader(this->_shaderProgramPassthrough, vertexShader);
				this->_f.pGlAttachShader(this->_shaderProgramPassthrough, fragmentShaderPassthrough);
				this->_f.pGlLinkProgram(this->_shaderProgramPassthrough);

				this->_f.pGlDetachShader(this->_shaderProgramPassthrough, vertexShader);
				this->_f.pGlDetachShader(this->_shaderProgramPassthrough, fragmentShaderPassthrough);

				this->_f.pGlDeleteShader(fragmentShaderPassthrough);

				const GLuint fragmentShaderTexture = this->_f.pGlCreateShader(GL_FRAGMENT_SHADER);
				this->_f.pGlShaderSource(fragmentShaderTexture, 1, &FRAGMENT_SHADER_TEXTURE, nullptr);
				this->_f.pGlCompileShader(fragmentShaderTexture);

				this->_shaderProgramTexture = this->_f.pGlCreateProgram();
				this->_f.pGlAttachShader(this->_shaderProgramTexture, vertexShader);
				this->_f.pGlAttachShader(this->_shaderProgramTexture, fragmentShaderTexture);
				this->_f.pGlLinkProgram(this->_shaderProgramTexture);

				this->_f.pGlDetachShader(this->_shaderProgramTexture, vertexShader);
				this->_f.pGlDetachShader(this->_shaderProgramTexture, fragmentShaderTexture);

				this->_f.pGlDeleteShader(fragmentShaderTexture);
				this->_f.pGlDeleteShader(vertexShader);

				return;
			}

		}

	}

}