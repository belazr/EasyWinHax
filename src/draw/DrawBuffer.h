#pragma once
#include "IBufferBackend.h"
#include "Vertex.h"

#include "..\Vector.h"

// Class for draw buffers that draw with textures. It keeps track of the indices per texture and rearranges them for one draw call per texture.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {
	
		class DrawBuffer {
		private:
			typedef struct TextureBatch {
				TextureId id;
				Vector<uint32_t> indices;
			}TextureBatch;

			Vector<TextureBatch> _textures;

			IBufferBackend* _pBufferBackend;
			Vertex* _pLocalVertexBuffer;
			uint32_t* _pLocalIndexBuffer;

			uint32_t _size;
			uint32_t _capacity;

		public:
			DrawBuffer();

			~DrawBuffer();

			DrawBuffer(DrawBuffer&&) = delete;

			DrawBuffer(const DrawBuffer&) = delete;

			DrawBuffer& operator=(DrawBuffer&&) = delete;

			DrawBuffer& operator=(const DrawBuffer&) = delete;

			// Begins the frame for the buffer. Has to be called before any append calls.
			//
			// Parameters:
			// 
			// [in] pBufferBackend:
			// Backend for the buffer retrieved from the engine backend.
			//
			// Return:
			// True on success, false on failure.
			bool beginFrame(IBufferBackend* pBufferBackend);

			// Appends vertices to the buffer.
			//
			// Parameters:
			// 
			// [in] data:
			// Pointer to an array of vertices to be appended to the buffer.
			//
			// [in] count:
			// Amount of vertices in the data array.
			//
			// [in] color:
			// Color that the vertices will be drawn in. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			//
			// [in] textureId:
			// The ID of the texture to draw returned by an IBAckend::loadTexture call
			void append(const Vertex* data, uint32_t count, TextureId textureId);

			// Ends the frame for the buffer and draws the contents. Has to be called after any append calls.
			void endFrame();

		private:
			void reset();
			bool reserve(uint32_t capacity);
		};

	}

}