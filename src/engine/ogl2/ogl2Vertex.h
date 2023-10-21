#pragma once
#include "..\rgb.h"
#include "..\..\vecmath.h"

// Struct to hold OpenGL 2 Vertices used for drawing.

namespace hax {

	namespace ogl2 {

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