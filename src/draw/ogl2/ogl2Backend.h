#pragma once
#include "ogl2BufferBackend.h"
#include "..\IBackend.h"
#include "..\..\Vector.h"

// Class for drawing with OpenGL 2.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		namespace ogl2 {
			typedef BOOL(APIENTRY* twglSwapBuffers)(HDC hDc);

			class Backend : public IBackend {
			private:

				union {
					Functions _f;
					void* _fPtrs[sizeof(Functions) / sizeof(void*)];
				};

				GLuint _shaderProgramPassthroughId;
				GLuint _shaderProgramTextureId;
				GLint _viewport[4];
				GLenum _depthFunc;
				GLboolean _blendEnabled;
				GLenum _srcAlphaBlendFunc;
				GLenum _dstAlphaBlendFunc;

				BufferBackend _triangleListBuffer;
				BufferBackend _textureTriangleListBuffer;

				Vector<GLuint> _textures;

			public:
				Backend();
				~Backend();

				// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [in] pArg1:
				// Pass nothing.
				//
				// [in] pArg2:
				// Pass nothing
				virtual void setHookArguments(void* pArg1 = nullptr, void* pArg2 = nullptr) override;

				// Initializes the backend. Should be called by an Engine object until success.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool initialize() override;
				
				// Loads a texture into VRAM.
				//
				// Parameters:
				// 
				// [in] data:
				// Texture colors in argb format.
				// 
				// [in] width:
				// Width of the texture.
				// 
				// [in] height:
				// Height of the texture.
				//
				// Return:
				// ID of the internal texture structure in VRAM that can be passed to DrawBuffer::append. 0 on failure.
				virtual TextureId loadTexture(const Color* data, uint32_t width, uint32_t height) override;

				// Starts a frame within a hook. Should be called by an Engine object every frame at the begin of the hook.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool beginFrame() override;

				// Ends the current frame within a hook. Should be called by an Engine object every frame at the end of the hook.
				virtual void endFrame() override;

				// Gets a reference to the triangle list buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the triangle list buffer backend.
				virtual IBufferBackend* getTriangleListBufferBackend() override;

				// Gets a reference to the texture triangle list buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the texture triangle list buffer backend.
				virtual IBufferBackend* getTextureTriangleListBufferBackend() override;

				// Gets the resolution of the current frame. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [out] frameWidth:
				// Pointer that receives the current frame width in pixel.
				//
				// [out] frameHeight:
				// Pointer that receives the current frame height in pixel.
				virtual void getFrameResolution(float* frameWidth, float* frameHeight) const override;

			private:
				bool getProcAddresses();
				void createShaders();
			};

		}

	}

}