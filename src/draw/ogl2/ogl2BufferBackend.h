#pragma once
#include "ogl2Defs.h"
#include "..\IBufferBackend.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			class BufferBackend : public IBufferBackend {
			private:
				Functions _f;
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

				uint32_t _capacity;

			public:
				BufferBackend();

				BufferBackend(BufferBackend&&) = delete;

				BufferBackend(const BufferBackend&) = delete;

				BufferBackend& operator=(BufferBackend&&) = delete;

				BufferBackend& operator=(const BufferBackend&) = delete;

				~BufferBackend();

				// Initializes members.
				//
				// Parameters:
				// 
				// [in] f:
				// Function pointers to the OpenGL 2 functions.
				// 
				// [in] viewport:
				// Current viewport. Has to be an array of four GLint.
				// 
				// [in] shaderProgramId:
				// ID of the shader program for drawing vertices.
				void initialize(Functions f, GLint* viewport, GLuint shaderProgramId);

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

				// Gets the current capacity of the buffer in vertices.
				//
				// Return:
				// The current capacity of the buffer in vertices.
				virtual uint32_t capacity() const override;

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
				virtual void draw(TextureId textureId, uint32_t index, uint32_t count) const override;

				// Ends drawing of the content of the buffer. Has to be called after any draw calls.
				//
				// Return:
				// True on success, false on failure.
				virtual void end() const override;

			private:
				bool createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const;
			};

		}

	}

}