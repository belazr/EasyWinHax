#include "oglSetup.h"
#include <gl\GL.h>

namespace gl {
    
    HGLRC getHookContext(HDC hDc, double windowWidth, double windowHeight) {
        // saves current context
        HGLRC gameContext = wglGetCurrentContext();
        HGLRC hackContext = wglCreateContext(hDc);

        wglMakeCurrent(hDc, hackContext);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, windowWidth, windowHeight, 0, 1, -1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glClearColor(0, 0, 0, 1.0);

        wglMakeCurrent(hDc, gameContext);

        return hackContext;
    }

}