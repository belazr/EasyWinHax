#pragma once
#include <Windows.h>
#include <gl\GL.h>

namespace hax {

	namespace draw {

		namespace ogl2 {

			constexpr GLenum GL_BLEND_DST_ALPHA = 0x80CAu;
			constexpr GLenum GL_BLEND_SRC_ALPHA = 0x80CBu;
			constexpr GLenum GL_ARRAY_BUFFER = 0x8892u;
			constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893u;
			constexpr GLenum GL_ARRAY_BUFFER_BINDING = 0x8894u;
			constexpr GLenum GL_ELEMENT_ARRAY_BUFFER_BINDING = 0x8895u;
			constexpr GLenum GL_WRITE_ONLY = 0x88B9u;
			constexpr GLenum GL_DYNAMIC_DRAW = 0x88E8u;
			constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30u;
			constexpr GLenum GL_VERTEX_SHADER = 0x8B31u;
			constexpr GLenum GL_CURRENT_PROGRAM = 0x8B8Du;

			typedef char GLchar;

			typedef GLuint (APIENTRY* tGlCreateShader) (GLenum type);
			typedef void (APIENTRY* tGlShaderSource) (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
			typedef void (APIENTRY* tGlCompileShader) (GLuint shader);
			typedef GLuint (APIENTRY* tGlCreateProgram) ();
			typedef void (APIENTRY* tGlAttachShader) (GLuint program, GLuint shader);
			typedef void (APIENTRY* tGlLinkProgram) (GLuint program);
			typedef void (APIENTRY* tGlDetachShader) (GLuint program, GLuint shader);
			typedef void (APIENTRY* tGlDeleteShader) (GLuint shader);
			typedef GLint (APIENTRY* tGlGetUniformLocation) (GLuint program, const GLchar* name);
			typedef GLint (APIENTRY* tGlGetAttribLocation) (GLuint program, const GLchar* name);
			typedef void (APIENTRY* tGlGenBuffers)(GLsizei n, GLuint* buffers);
			typedef void (APIENTRY* tGlBindBuffer)(GLenum target, GLuint buffer);
			typedef void (APIENTRY* tGlBufferData)(GLenum target, size_t size, const GLvoid* data, GLenum usage);
			typedef void* (APIENTRY* tGlMapBuffer)(GLenum target, GLenum access);
			typedef GLboolean (APIENTRY* tGlUnmapBuffer)(GLenum target);
			typedef void (APIENTRY* tGlEnableVertexAttribArray) (GLuint index);
			typedef void (APIENTRY* tGlVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
			typedef void (APIENTRY* tGlDisableVertexAttribArray) (GLuint index);
			typedef void (APIENTRY* tGlDeleteBuffers)(GLsizei n, const GLuint* buffers);
			typedef void (APIENTRY* tGlUseProgram) (GLuint program);
			typedef void (APIENTRY* tGlUniformMatrix4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
			typedef void (APIENTRY* tGlDeleteProgram) (GLuint program);

			typedef struct Functions {
				tGlCreateShader pGlCreateShader;
				tGlShaderSource pGlShaderSource;
				tGlCompileShader pGlCompileShader;
				tGlCreateProgram pGlCreateProgram;
				tGlAttachShader pGlAttachShader;
				tGlLinkProgram pGlLinkProgram;
				tGlDetachShader pGlDetachShader;
				tGlDeleteShader pGlDeleteShader;
				tGlGetUniformLocation pGlGetUniformLocation;
				tGlGetAttribLocation pGlGetAttribLocation;
				tGlGenBuffers pGlGenBuffers;
				tGlBindBuffer pGlBindBuffer;
				tGlBufferData pGlBufferData;
				tGlMapBuffer pGlMapBuffer;
				tGlUnmapBuffer pGlUnmapBuffer;
				tGlEnableVertexAttribArray pGlEnableVertexAttribArray;
				tGlVertexAttribPointer pGlVertexAttribPointer;
				tGlDisableVertexAttribArray pGlDisableVertexAttribArray;
				tGlDeleteBuffers pGlDeleteBuffers;
				tGlUseProgram pGlUseProgram;
				tGlUniformMatrix4fv pGlUniformMatrix4fv;
				tGlDeleteProgram pGlDeleteProgram;
			}Functions;

		}

	}

}