#pragma once
#include "AbstractDrawBuffer.h"

// Class for draw buffers that draw without textures. It calls all vertices in one draw call.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		class SolidDrawBuffer : public AbstractDrawBuffer {
		public:
			SolidDrawBuffer();

			SolidDrawBuffer(SolidDrawBuffer&&) = delete;

			SolidDrawBuffer(const SolidDrawBuffer&) = delete;

			SolidDrawBuffer& operator=(SolidDrawBuffer&&) = delete;

			SolidDrawBuffer& operator=(const SolidDrawBuffer&) = delete;

			// Appends vertices to the buffer. Has to be called between beginFrame and endFrame calls.
			//
			// Parameters:
			// 
			// [in] data:
			// Pointer to an array of vertices to be appended to the buffer.
			//
			// [in] count:
			// Amount of vertices in the data array.
			void append(const Vertex* data, uint32_t count);

			// Appends vertices to the buffer.
			//
			// Parameters:
			// 
			// [in] data:
			// Pointer to an array of screen coordinates of vertices to be appended to the buffer. Has to be called between beginFrame and endFrame calls.
			//
			// [in] count:
			// Amount of vertices in the data array.
			//
			// [in] color:
			// Color that the vertices will be drawn in. Color format: DirectX 9 -> argb, DirectX 10 -> abgr, DirectX 11 -> abgr, DirectX 12 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			//
			// [in] offset:
			// Offset that gets added to all coordinates in the data array before the vertices get appended to the buffer.
			void append(const Vector2* data, uint32_t count, Color color, Vector2 offset = { 0.f, 0.f });

			// Ends the frame for the buffer and draws the contents. Has to be called after any append calls.
			void endFrame() override;
		
		};

	}

}