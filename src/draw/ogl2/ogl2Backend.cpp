#include "ogl2Backend.h"
#include "..\font\Font.h"
#include <Windows.h>
#include <vector>
namespace hax {

	namespace draw {

		namespace ogl2 {

			Backend::Backend() : _f{}, _viewport{}, _depthFunc{}, _triangleListBuffer {}, _pointListBuffer{} {}


			Backend::~Backend() {
				this->_pointListBuffer.destroy();
				this->_triangleListBuffer.destroy();
				glDeleteTextures(static_cast<GLsizei>(this->_textures.size()), this->_textures.data());
			}


			void Backend::setHookArguments(void* pArg1, void* pArg2) {
				UNREFERENCED_PARAMETER(pArg1);
				UNREFERENCED_PARAMETER(pArg2);

				return;
			}


			bool Backend::initialize() {

				if (!this->getProcAddresses()) return false;

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

				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glOrtho(0, static_cast<double>(this->_viewport[2]), static_cast<double>(this->_viewport[3]), 0, -1.f, 1.f);

				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();

				return true;
			}


			void Backend::endFrame() {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();

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
				ASSIGN_PROC_ADDRESS(GenBuffers);
				ASSIGN_PROC_ADDRESS(BindBuffer);
				ASSIGN_PROC_ADDRESS(BufferData);
				ASSIGN_PROC_ADDRESS(MapBuffer);
				ASSIGN_PROC_ADDRESS(UnmapBuffer);
				ASSIGN_PROC_ADDRESS(DeleteBuffers);

				for (size_t i = 0u; i < _countof(this->_fPtrs); i++) {

					if (!(this->_fPtrs[i])) return false;

				}

				return true;
			}

			#undef ASSIGN_PROC_ADDRESS
		}

	}

}