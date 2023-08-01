#pragma once
#include "..\..\rgb.h"
#include "..\..\..\vecmath.h"

// Struct to hold DirectX 9 Vertices used for drawing.

namespace hax {
	
	namespace dx9 {

		struct Vertex {
		private:
			Vector3 _coordinates;
			float _rhw;
			rgb::Color _color;

			Vertex() = delete;

		public:
			Vertex(Vector2 coordinates, hax::rgb::Color color) : _coordinates{ coordinates.x, coordinates.y, 1.f }, _color{ color }, _rhw{ 1.f } {}
		};

	}

}