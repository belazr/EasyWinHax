#pragma once
#include "..\..\rgb.h"
#include "..\..\..\vecmath.h"
#include <DirectXMath.h>
#include <d3d11.h>

// Struct to hold DirectX 9 Vertices used for drawing with constructors to convert from general types for convenience.

namespace hax {

	namespace dx11 {

		struct Vertex {
			DirectX::XMFLOAT3 pos;
			D3DCOLORVALUE color;

			Vertex() = delete;
			Vertex(Vector2 vec, hax::rgb::Color col);
			Vertex(float x, float y, hax::rgb::Color col);

		};

	}

}

