#pragma once
#include "..\..\rgb.h"
#include "..\..\..\vecmath.h"
#include <d3d9.h>

namespace hax {
	
	namespace dx9 {

		struct Vertex {
			float x;
			float y;
			float z;
			float rhw;
			D3DCOLOR color;

			Vertex() = delete;
			Vertex(Vector2 vec, hax::rgb::Color col);

		};

	}

}