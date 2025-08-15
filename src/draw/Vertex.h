#pragma once
#include "Color.h"
#include "..\vecmath.h"

// Struct to hold the Vertices used for drawing.

namespace hax {

	namespace draw {

		class Vertex {
		private:
			Vector2 _coordinates;
			Color _color;
			Vector2 _uv;

			Vertex() = delete;

		public:
			Vertex(Vector2 coordinates, Color color, Vector2 uv = {}) : _coordinates{ coordinates }, _color{ color }, _uv{ uv } {}
		};

	}

}