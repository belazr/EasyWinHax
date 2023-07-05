#include "dx11Vertex.h"

namespace hax {

	namespace dx11 {

		Vertex::Vertex(Vector2 vec, hax::rgb::Color col) : pos(vec.x, vec.y, 1.f), color{ FLOAT_R(col), FLOAT_G(col), FLOAT_B(col), FLOAT_A(col), } {}

	}

}