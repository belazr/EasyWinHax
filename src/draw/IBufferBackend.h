#include "Vertex.h"
#include <stdint.h>

// Interface the DrawBuffer and TextureDrawBuffer classes use for drawing the buffer content.
// The appropriate implementation is instantiated by the Backend and passed to a DrawBuffer or TextureDrawBuffer object by the Engine object.
// All methods are intended to be called by a DrawBuffer or TextureDrawBuffer object and not for direct calls.

namespace hax {

	namespace draw {

		typedef uint64_t TextureId;

		class IBufferBackend {
		public:
			// Creates internal resources.
			//
			// Parameters:
			// 
			// [in] capacity:
			// Capacity of verticies the buffer backend can hold.
			//
			// Return:
			// True on success, false on failure.
			virtual bool create(uint32_t capacity) = 0;

			// Destroys all internal resources.
			virtual void destroy() = 0;

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
			virtual bool map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) = 0;

			// Unmaps the allocated VRAM from the address space of the current process.
			virtual void unmap() = 0;

			// Begins drawing of the content of the buffer. Has to be called before any draw calls.
			//
			// Return:
			// True on success, false on failure.
			virtual bool begin() = 0;

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
			virtual void draw(TextureId textureId, uint32_t index, uint32_t count) const = 0;

			// Ends drawing of the content of the buffer. Has to be called after any draw calls.
			//
			// Return:
			// True on success, false on failure.
			virtual void end() const = 0;
		};

	}

}
