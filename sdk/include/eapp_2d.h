/*
 * eapp_2d.h - minimal 2D drawing helpers, in screen pixels with (0,0) at the top-left.
 *
 * Call eapp_gl2d_begin() once per frame, then eapp_gl2d_rect() as needed. Needs the
 * OpenGLES import. Finish the frame with eglSwapBuffers().
 */
#ifndef EAPP_2D_H
#define EAPP_2D_H
#include "eapp.h"
#include "eapp_gles.h"

// Set up GL for y-down pixel coordinates. On this OS GL window row 0 is the TOP of the screen, so the ortho call uses bottom=0, top=H.
static inline void eapp_gl2d_begin(void) {
    glViewport(0, 0, EAPP_SCREEN_W, EAPP_SCREEN_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthox(0, EAPP_SCREEN_W << 16, 0, EAPP_SCREEN_H << 16, -GL_FIXED_ONE, GL_FIXED_ONE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnableClientState(GL_VERTEX_ARRAY);
}

// Filled rectangle, colour 0xRRGGBB. Drawn as a quad: glClear ignores the scissor box, so scissored clears cannot draw rectangles.
static inline void eapp_gl2d_rect(int x, int y, int w, int h, uint32_t rgb) {
    GLshort v[8] = { (GLshort)x, (GLshort)y,  (GLshort)(x + w), (GLshort)y,
                     (GLshort)x, (GLshort)(y + h),  (GLshort)(x + w), (GLshort)(y + h) };
    glColor4x(((rgb >> 16) & 0xFF) * 257, ((rgb >> 8) & 0xFF) * 257, (rgb & 0xFF) * 257, GL_FIXED_ONE);
    glVertexPointer(2, GL_SHORT, 0, v);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#endif
