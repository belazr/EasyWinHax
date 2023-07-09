#pragma once
#include "..\..\rgb.h"
#include "..\..\..\vecmath.h"
#include <DirectXMath.h>
#include <d3d11.h>

// Struct to hold DirectX 11 Vertices used for drawing.

namespace hax {

	namespace dx11 {

		struct Vertex {
			DirectX::XMFLOAT3 pos;
			D3DCOLORVALUE color;

			Vertex() = delete;

			Vertex(Vector2 vec, hax::rgb::Color col) : pos{ vec.x, vec.y, 1.f }, color{ FLOAT_R(col), FLOAT_G(col), FLOAT_B(col), FLOAT_A(col) } {}

			Vertex(float xPos, float yPos, hax::rgb::Color col) : pos{ xPos , yPos, 1.f }, color{ FLOAT_R(col), FLOAT_G(col), FLOAT_B(col), FLOAT_A(col) } {}
		};

	}

}

