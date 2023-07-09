#pragma once
#include "..\..\rgb.h"
#include "..\..\..\vecmath.h"
#include <d3d9.h>

// Struct to hold DirectX 9 Vertices used for drawing.

namespace hax {
	
	namespace dx9 {

		struct Vertex {
			float x;
			float y;
			float z;
			float rhw;
			D3DCOLOR color;

			Vertex() = delete;

			Vertex(Vector2 vec, hax::rgb::Color col) : x{ vec.x }, y{ vec.y }, z{ 1.f }, rhw{ 1.f }, color{ col } {}

			Vertex(float xPos, float yPos, hax::rgb::Color col) : x{ xPos }, y{ yPos }, z{ 1.f }, rhw{ 1.f }, color{ col } {}

		};

	}

}