#pragma once
#include "..\..\rgb.h"
#include "..\..\..\vecmath.h"
#include <DirectXMath.h>
#include <d3d11.h>

namespace hax {

	namespace dx11 {

		struct Vertex {
			DirectX::XMFLOAT3 pos;
			D3DCOLORVALUE color;

			Vertex() = delete;
			Vertex(Vector2 vec, hax::rgb::Color col);

		};

	}

}

