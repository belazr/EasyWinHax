#pragma once
#include "ogl2Defs.h"
#include "..\AbstractDrawBuffer.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				Functions _f;
				GLenum _mode;
				GLuint _vertexBufferId;
				GLuint _indexBufferId;

			public:
				DrawBuffer();
				~DrawBuffer();

				// Initializes members.
				//
				// Parameters:
				// 
				// [in] f:
				// Function pointers to the OpenGL 2 functions.
				// 
				// [in] mode:
				// Mode the vertices in the buffer should be drawn in.
				void initialize(Functions f, GLenum mode);

				// Creates a new buffer with all internal resources.
				//
				// Parameters:
				// 
				// [in] vertexCount:
				// Size of the buffer in vertices.
				//
				// Return:
				// True on success, false on failure.
				bool create(uint32_t vertexCount) override;

				// Destroys the buffer and all internal resources.
				void destroy() override;

				// Maps the allocated VRAM into the address space of the current process. Needs to be called before the buffer can be filled.
				//
				// Return:
				// True on success, false on failure.
				bool map() override;

				// Draws the content of the buffer to the screen.
				// Needs to be called between a successful of vk::Backend::beginFrame and a call to vk::Backend::endFrame.
				void draw() override;

			private:
				bool createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const;
			};

		}

	}

}