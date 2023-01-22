#pragma once
#include "..\vecmath.h"
#include <Windows.h>
#include <gl\GL.h>
#include <wingdi.h>

// Helper functions to draw with OpenGL within a wglSwapBuffers hook.

namespace gl {

	// predefined colors
	namespace rgb {

		const GLubyte red[3]	= { 255, 0, 0 };
		const GLubyte green[3]	= { 0, 255, 0 };
		const GLubyte blue[3]	= { 0, 0, 255 };
		const GLubyte black[3]	= { 0, 0, 0 };
		const GLubyte white[3]	= { 255, 255, 255 };
		const GLubyte gray[3]	= { 127, 127, 127 };

	}

	// Draws a filled rectangle with the sides parallel to the screen edges.
	// The current context has to be set to a context returned by gl::getHookContext via wglMakeCurrent before calling this function.
	// Parameters:
	// [in] pos:	Coordinates of the top left corner of the rectangle.
	// [in] width:	Width of the rectangle.
	// [in] height:	Height of the rectangle.
	// [in] color:	Color of the rectangle.
	void drawFilledRect(const Vector2* pos, float width, float height, const GLubyte color[3]);
	
	// Draws a rectangle grid with the sides parallel to the screen edges. The line width is two pixels.
	// The current context has to be set to a context returned by gl::getHookContext via wglMakeCurrent before calling this function.
	// Parameters:
	// [in] pos:	Coordinates of the top left corner of the rectangle.
	// [in] width:	Width of the rectangle.
	// [in] height: Height of the rectangle.
	// [in] color:	Color of the rectangle.
	void drawRectOutline(const Vector2* pos, float width, float height, const GLubyte color[3], float lineWidth);

}