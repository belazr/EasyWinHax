#pragma once
#include <Windows.h>
#include <gl\GL.h>

namespace hax {

	namespace draw {

		namespace ogl2 {
			constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
			constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
			constexpr GLenum GL_ARRAY_BUFFER_BINDING = 0x8894;
			constexpr GLenum GL_ELEMENT_ARRAY_BUFFER_BINDING = 0x8895;
			constexpr GLenum GL_DYNAMIC_DRAW = 0x88E8;
			constexpr GLenum GL_WRITE_ONLY = 0x88B9;

			typedef void(APIENTRY* tGlGenBuffers)(GLsizei n, GLuint* buffers);
			typedef void(APIENTRY* tGlBindBuffer)(GLenum target, GLuint buffer);
			typedef void(APIENTRY* tGlBufferData)(GLenum target, size_t size, const GLvoid* data, GLenum usage);
			typedef void* (APIENTRY* tGlMapBuffer)(GLenum target, GLenum access);
			typedef GLboolean(APIENTRY* tGlUnmapBuffer)(GLenum target);
			typedef void(APIENTRY* tGlDeleteBuffers)(GLsizei n, const GLuint* buffers);

			typedef struct Functions {
				tGlGenBuffers pGlGenBuffers;
				tGlBindBuffer pGlBindBuffer;
				tGlBufferData pGlBufferData;
				tGlMapBuffer pGlMapBuffer;
				tGlUnmapBuffer pGlUnmapBuffer;
				tGlDeleteBuffers pGlDeleteBuffers;
			}Functions;

		}

	}

}