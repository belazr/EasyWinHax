#include "dx9Vertex.h"

namespace hax {

	namespace dx9 {
	
		Vertex::Vertex(Vector2 vec, hax::rgb::Color col) : x(vec.x), y(vec.y), z(1.f), rhw(1.f), color(col) {}

	}

}