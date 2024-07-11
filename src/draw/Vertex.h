#pragma once
#include "rgb.h"
#include "..\vecmath.h"

// Struct to hold the Vertices used for drawing.

namespace hax {

	namespace draw {

		struct Vertex {
		private:
			Vector2 _coordinates;
			rgb::Color _color;

			Vertex() = delete;

		public:
			Vertex(Vector2 coordinates, rgb::Color color) : _coordinates{ coordinates }, _color{ color } {}
		};

	}

}