#pragma once
#include "..\vecmath.h"
#include <d3d9.h>

// Helper functions to draw shapes within an DirectX 9 hook.

namespace dx9 {

	// predefined colors
	namespace rgb {

		const D3DCOLOR red			= D3DCOLOR_ARGB(255, 255, 0, 0);
		const D3DCOLOR orange		= D3DCOLOR_ARGB(255, 255, 127, 0);
		const D3DCOLOR yellow		= D3DCOLOR_ARGB(255, 255, 255, 0);
		const D3DCOLOR chartreuse	= D3DCOLOR_ARGB(255, 127, 255, 0);
		const D3DCOLOR green		= D3DCOLOR_ARGB(255, 0, 255, 0);
		const D3DCOLOR guppie		= D3DCOLOR_ARGB(255, 0, 255, 127);
		const D3DCOLOR aqua			= D3DCOLOR_ARGB(255, 0, 255, 255);
		const D3DCOLOR azure		= D3DCOLOR_ARGB(255, 0, 127, 255);
		const D3DCOLOR blue			= D3DCOLOR_ARGB(255, 0, 0, 255);
		const D3DCOLOR violet		= D3DCOLOR_ARGB(255, 127, 0, 255);
		const D3DCOLOR fuchsia		= D3DCOLOR_ARGB(255, 255, 0, 255);
		const D3DCOLOR pink			= D3DCOLOR_ARGB(255, 255, 0, 127);
		
		const D3DCOLOR black		= D3DCOLOR_ARGB(255, 0, 0, 0);
		const D3DCOLOR white		= D3DCOLOR_ARGB(255, 255, 255, 255);
		const D3DCOLOR gray			= D3DCOLOR_ARGB(255, 127, 127, 127);

	}

	typedef struct Vertex {
		float x, y, z, rhw;
		D3DCOLOR color;
	}Vertex;

	// Device for drawing.
	// Has to be assigned with a pointer to the device passed as an argument to the original EndScene in the begining of an EndScene hook.
	extern IDirect3DDevice9* pDevice;

	// Draws a parallelogram grid with the bottom and top sides horizontally alligned.
	//
	// Parameters:
	//
	// [in] bot:
	// Screen coordinates of the middle of the bottom side.
	// 
	// [in] top:
	// Screen coordinates of the middle of the top side.
	// 
	// [in] ratio:
	// Ratio of bottom/top side length to height of the parallelogram
	// 
	// [in] width:
	// Line width in pixels.
	// 
	// [in] color:
	// Line color.
	void drawPrimitiveParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, D3DCOLOR color);
	
	// Draws a 2D box grid.
	//
	// Parameters:
	//
	// [in] bot:
	// Screen coordinates of the bottom left and right corner of the box.
	// 
	// [in] top:
	// Screen coordinates of the top left and right corner of the box.
	// 
	// [in] width:
	// Line width in pixels.
	// 
	// [in] color:
	// Line color
	void drawPrimitive2DBox(const Vector2 bot[2], const Vector2 top[2], float width, D3DCOLOR color);

	// Draws a 3D box grid.
	//
	// Parameters:
	//
	// [in] bot:
	// Screen coordinates of the four bottom corners of the box.
	// 
	// [in] top:
	// Screen coordinates of the four corners of the box.
	// 
	// [in] width:
	// Line width in pixels.
	// 
	// [in] color:
	// Line color.
	void drawPrimitive3DBox(const Vector2 bot[4], const Vector2 top[4], float width, D3DCOLOR color);

	// Draws a filled parallelogram with the bottom and top sides horizontally alligned. Intended to be used for line drawing (width << lenght) but can be used otherwise.
	//
	// Parameters:
	//
	// [in] pos1:
	// Screen coordinates of the beginning of the line.
	// 
	// [in] pos2:
	// Screen coordinates of the end of the line.
	// 
	// [in] width:
	// Width in pixels.
	// 
	// [in] color:
	// Color of the line.
	// 
	// Return:
	// HRESULT of the device method call or E_FAIL if pos1 and pos2 have the same y coordinates.
	HRESULT drawPrimitivePLine(const Vector2* pos1, const Vector2* pos2, float width, D3DCOLOR color);

	// Draws a filled rectangle. Intended to be used for line drawing (width << lenght) but can be used otherwise.
	//
	// Parameters:
	//
	// [in] pos1:
	// Screen coordinates of the beginning of the line.
	// 
	// [in] pos2:
	// Screen coordinates of the end of the line.
	// 
	// [in] width:
	// Width in pixels.
	// 
	// [in] color:
	// Color of the line.
	// 
	// Return:
	// HRESULT of the device method call.
	HRESULT drawPrimitiveLine(const Vector2* pos1, const Vector2* pos2, float width, D3DCOLOR color);

	// Draws a set of points.
	//
	// Parameters:
	//
	// [in] origin:
	// Origin of the set of points to be drawn.
	// 
	// [in] points:
	// Coordinates of the points as offsets to origin.
	// 
	// [in] count:
	// Number of points to be drawn.
	// 
	// [in] color:
	// Color of the points.
	// 
	// Return:
	// HRESULT of the device method call.
	HRESULT drawPrimitivePoints(const Vector2* origin, const Vector2 points[], UINT count, D3DCOLOR color);

	// Draws a filled rectangle with the sides parallel to the screen edges.
	//
	// Parameters:
	//
	// [in] pos:
	// Coordinates of the top left corner of the rectangle.
	// 
	// [in] width:
	// Width of the rectangle.
	// 
	// [in] height:
	// Height of the rectangle.
	// 
	// [in] color:
	// Color of the rectangle.
	// 
	// Return:
	// HRESULT of the device method call.
	HRESULT drawFilledRect(const Vector2* pos, float width, float height, D3DCOLOR color);

}