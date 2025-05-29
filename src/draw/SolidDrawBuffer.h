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

			// Ends the frame for the buffer and draws the contents. Has to be called after any append calls.
			void endFrame() override;
		
		};

	}

}