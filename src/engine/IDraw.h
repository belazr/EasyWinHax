#pragma once
#include "rgb.h"
#include "font\Font.h"
#include "..\vecmath.h"
#include <Windows.h>

// Interface for drawing with graphics APIs within a hook.
// The appropriate implementation should be instantiated and passed to an engine object.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax{

	class Engine;

	class IDraw {
	public:
		// Initializes drawing within a hook. Should be called by an Engine object.
		//
		// Parameters:
		// 
		// [in] pEngine:
		// Pointer to the Engine object responsible for drawing within the hook.
		virtual void beginDraw(Engine* pEngine) = 0;

		// Ends drawing within a hook. Should be called by an Engine object.
		// 
		// [in] pEngine:
		// Pointer to the Engine object responsible for drawing within the hook.
		virtual void endDraw(const Engine* pEngine) = 0;

		// Draws a filled triangle list. Should be called by an Engine object.
		// 
		// Parameters:
		// 
		// [in] corners:
		// Screen coordinates of the corners of the triangles in the list.
		// The three corners of the first triangle have to be in clockwise order. For there on the orientation of the triangles has to alternate.
		// 
		// [in] count:
		// Count of the corners of the triangles in the list. Has to be divisble by three.
		// 
		// [in]
		// Color of the triangle list.
		virtual void drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) = 0;

		// Draws text to the screen. Should be called by an Engine object.
		//
		// Parameters:
		// 
		// [in] pFont:
		// Pointer to an appropriate Font object. dx::Font<dx1:Vertex>* for DirectX 11 hooks, dx::Font<dx9:Vertex>* for DirectX 9 hooks and ogl2::Font* for OpenGL 2 hooks.
		//
		// [in] origin:
		// Coordinates of the bottom left corner of the first character of the text.
		// 
		// [in] text:
		// Text to be drawn. See length limitations implementations.
		//
		// [in] color:
		// Color of the text.
		virtual void drawString(const font::Font* pFont, const Vector2* pos, const char* text, rgb::Color color) = 0;
	};

}
