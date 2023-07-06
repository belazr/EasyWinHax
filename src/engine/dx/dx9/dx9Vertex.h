#pragma once
#include "..\..\rgb.h"
#include "..\..\..\vecmath.h"
#include <d3d9.h>

// Struct to hold DirectX 9 Vertices used for drawing with constructors to convert from general types for convenience.

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
			Vertex(float xPos, float yPos, hax::rgb::Color col);

		};

	}

}