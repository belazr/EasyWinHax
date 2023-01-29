#pragma once
#include "..\vecmath.h"
#include <Windows.h>
#include <gl\GL.h>

// Class to draw text with OpenGL within a glSwapBuffers hook.

namespace gl {
	
	class Font {
	private:
		DWORD _base; 
		HDC _hDc;
		float _height;
		float _width;

	public:
		// Initializes members.
		// 
		// Parameters:
		// 
		// [in] height: Height of the characters drawn in pixels.
		// [in] hDc:	Device context returned by the gl::getHookContext function.
		Font(int height, HDC hDc);

		// Draws a string to the screen inside a wglSwapBuffers call. The maximum lenght of the string is 100 characters (including null character).
		// The current context has to be set to a context returned by gl::getHookContext via wglMakeCurrent before calling this function.
		// 
		// Parameters:
		// 
		// [in] pos:
		// Coordinates of the bottom left corner of the first character of the text.
		// 
		// [in] color:
		// Color of the text.
		// 
		// [in] format:
		// Text to be drawn. Can contain printf style format specifiers.
		// 
		// [in] ...:
		// Additional arguments that replace the format specifiers in format (see printf documentation).
		void drawString(const Vector2* pos, const GLubyte color[3], const char* format, ...) const;
		
		// Centers text within a rectangle. Center of text is on center of rectangle.
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
		// [in] textWidth:
		// Width of the text.
		// 
		// [in] textHeight:
		// Height of the text.
		// 
		// Return:
		// Coordinates of the bottom left corner of the first character of the centered text.
		Vector2 center(const Vector2* pos, float width, float height, float textWidth, float textHeight) const;

		// Centers text on a horizontal line. Center of text is on center of line.
		// 
		// Parameters:
		// 
		// [in] x:
		// X coordinate of the left end of the line.
		// 
		// [in] width:
		// Width of the line.
		// 
		// [in] textWidth:
		// Width of the text.
		// 
		// Return:
		// Coordinates of the bottom left corner of the first character of the centered text.
		float center(float x, float width, float textWidth) const;

		// Gets the height of a character in pixels.
		// 
		// Return:
		// The height of a character in pixels.
		float getCharWidth() const;

		// Gets the width of a character in pixels.
		// 
		// Return:
		// The width of a character in pixels.
		float getCharHeight() const;

		HDC getHdc() const;
	};

}