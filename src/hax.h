#pragma once

// Headers for functionallity not related to graphics apis
#include "Bench.h"
#include "FileLoader.h"
#include "vecmath.h"
#include "hooks\TrampHook.h"
#include "hooks\IatHook.h"
#include "mem.h"
#include "proc.h"
#include "launch.h"

// Headers for engine
#include "engine\Engine.h"
#include "engine\IDraw.h"
#include "engine\Vertex.h"
#include "engine\rgb.h"
#include "engine\font\Font.h"

// Headers for DirectX 9
#pragma comment(lib, "d3d9.lib")

#include "engine\dx\dx9\dx9Draw.h"

// Headers for DirectX 11
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "engine\dx\dx11\dx11Draw.h"

// Headers for OpenGL 2
#pragma comment( lib, "OpenGL32.lib" )

#include "engine\ogl2\ogl2Draw.h"
#include "engine\ogl2\ogl2Font.h"

// Headers for Vulkan
#include "engine\vulkan\vkDraw.h"