#pragma once
#include "Color.h"
#include "..\vecmath.h"

// Struct to hold the Vertices used for drawing.

namespace hax {

	namespace draw {

		struct Vertex {
		private:
			Vector2 _coordinates;
			Color _color;

			Vertex() = delete;

		public:
			Vertex(Vector2 coordinates, Color color) : _coordinates{ coordinates }, _color{ color } {}
		};

	}

}