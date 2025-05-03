#pragma once

// Headers for functionallity not related to graphics apis
#include "Bench.h"
#include "FileLoader.h"
#include "launch.h"
#include "mem.h"
#include "proc.h"
#include "undocWinTypes.h"
#include "vecmath.h"
#include "Vector.h"

// Headers for hooks
#include "hooks\TrampHook.h"
#include "hooks\IatHook.h"

// Headers for engine
#include "draw\Color.h"
#include "draw\Engine.h"
#include "draw\Vertex.h"

#include "draw\font\Font.h"

// Headers for DirectX 9
#pragma comment(lib, "d3d9.lib")

#include "draw\dx\dx9\dx9Backend.h"

// Headers for DirectX 10
#pragma comment(lib, "d3d10.lib")

#include "draw\dx\dx10\dx10Backend.h"

// Headers for DirectX 11
#pragma comment(lib, "d3d11.lib")

#include "draw\dx\dx11\dx11Backend.h"

// Headers for DirectX 12
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "draw\dx\dx12\dx12Backend.h"

// Headers for OpenGL 2
#pragma comment( lib, "OpenGL32.lib" )

#include "draw\ogl2\ogl2Backend.h"

// Headers for Vulkan
#include "draw\vulkan\vkBackend.h"