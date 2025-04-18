#pragma once
#include "ogl2Defs.h"
#include "..\IBufferBackend.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			class BufferBackend : public IBufferBackend {
			private:
				Functions _f;
				GLenum _mode;
				GLuint _shaderProgramId;
				GLfloat _projectionMatrix[4][4];
				GLuint _projectionMatrixIndex;
				GLuint _posIndex;
				GLuint _colIndex;
				GLuint _uvIndex;
				GLuint _vertexBufferId;
				GLuint _indexBufferId;
				GLuint _curShaderProgramId;
				GLuint _curVertexBufferId;
				GLuint _curIndexBufferId;

			public:
				BufferBackend();

				~BufferBackend();

				// Initializes members.
				//
				// Parameters:
				// 
				// [in] f:
				// Function pointers to the OpenGL 2 functions.
				// 
				// [in] mode:
				// Mode the vertices in the buffer should be drawn in.
				// 
				// [in] viewport:
				// Current viewport. Has to be an array of four GLint.
				// 
				// [in] shaderProgramId:
				// ID of the shader program for drawing vertices.
				void initialize(Functions f, GLenum mode, GLint* viewport, GLuint shaderProgramId);

				// Creates internal resources.
				//
				// Parameters:
				//
				// [in] capacity:
				// Capacity of verticies the buffer backend can hold.
				//
				// Return:
				// True on success, false on failure.
				bool create(uint32_t capacity) override;

				// Destroys all internal resources.
				void destroy() override;

				// Maps the allocated VRAM into the address space of the current process.
				// 
				// Parameters:
				// 
				// [out] ppLocalVertexBuffer:
				// The mapped vertex buffer.
				//
				// 
				// [out] ppLocalIndexBuffer:
				// The mapped index buffer.
				//
				// Return:
				// True on success, false on failure.
				bool map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) override;

				// Unmaps the allocated VRAM from the address space of the current process.
				virtual void unmap() override;

				// Begins drawing of the content of the buffer. Has to be called before any draw calls.
				//
				// Return:
				// True on success, false on failure.
				virtual bool begin() override;

				// Draws a batch.
				// 
				// Parameters:
				// 
				// [in] textureId:
				// ID of the texture that should be drawn return by Backend::loadTexture.
				// If this is 0ull, no texture will be drawn.
				//
				// [in] index:
				// Index into the index buffer where the batch begins.
				// 
				// [in] count:
				// Vertex count in the batch.
				virtual void draw(TextureId textureId, uint32_t index, uint32_t count) override;

				// Ends drawing of the content of the buffer. Has to be called after any draw calls.
				//
				// Return:
				// True on success, false on failure.
				virtual void end() override;

			private:
				bool createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const;
			};

		}

	}

}