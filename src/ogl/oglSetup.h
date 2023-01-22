#pragma once
#include <windows.h>

// Setup for wglSwapBuffers hook and draw within it with OpenGL.

namespace gl {
	
	typedef BOOL(APIENTRY* twglSwapBuffers) (HDC hDc);

	// Gets a new context to drawn within a wglSwapBuffer hook.
	// Use this context as an argument to a wglMakeCurrent call before calling functions from glDraw.h.
	// Save the old context via wglGetCurrentContext and restore it with wglMakeCurrent after drawing and before calling the original glSwapBuffers from the hook.
	// Parameter:
	// [in] hDc:			Device context of the current wglSwapBuffers call.
	// [in] windowWidth:	Current width of the game window
	// [in] windowHeight:	Current height of the game window
	HGLRC getHookContext(HDC hDc, double windowWidth, double windowHeight);

}
