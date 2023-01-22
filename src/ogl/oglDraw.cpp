#pragma once
#include "oglDraw.h"

namespace gl {

    void drawFilledRect(const Vector2* pos, float width, float height, const GLubyte color[3]) {
        glColor3ub(color[0], color[1], color[2]);
        glBegin(GL_QUADS);
        glVertex2f(pos->x, pos->y);
        glVertex2f(pos->x + width, pos->y);
        glVertex2f(pos->x + width, pos->y + height);
        glVertex2f(pos->x, pos->y + height);
        glEnd();

        return;
    }


    void drawRectOutline(const Vector2* pos, float width, float height, const GLubyte color[3], float lineWidth) {
        glColor3ub(color[0], color[1], color[2]);
        // beginning of vertecies has to be offset by the line width for a clean overlap at the corners
        glBegin(GL_LINE_STRIP);
        glVertex2f(pos->x - lineWidth, pos->y - lineWidth);
        glVertex2f(pos->x + width + lineWidth, pos->y - lineWidth);
        glVertex2f(pos->x + width + lineWidth, pos->y + height + lineWidth);
        glVertex2f(pos->x - lineWidth, pos->y + height + lineWidth);
        glVertex2f(pos->x - lineWidth, pos->y - lineWidth);
        glEnd();

        return;
    }

}