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

// Headers for DirectX
#include "engine\dx\font\dxFont.h"
#include "engine\dx\font\charsets\dxCharsets.h"

// Headers for DirectX 9
#include "engine\dx\dx9\dx9Draw.h"
#include "engine\dx\dx9\dx9Vertex.h"

// Headers for DirectX 11
#include "engine\dx\dx11\dx11Draw.h"
#include "engine\dx\dx11\dx11Vertex.h"

// Headers for OpenGL 2
#include "engine\ogl2\ogl2Draw.h"
#include "engine\ogl2\ogl2Font.h"