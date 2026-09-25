/*
 * eapp_gles.h - the OpenGL ES 1.x + EGL 1.1 subset exported by the iPod nano 3G
 * "OpenGLES" module. Every function here is a real export (see tools/modules.py
 * for slot order). Not exported: glColor4ub, glTexEnvi/iv, glDrawTex*OES,
 * glPointSizePointerOES, matrix palette, eglGetProcAddress.
 * Two texture units. glCompressedTexImage2D accepts GL_PALETTE*_OES only.
 * Prefer the fixed-point (x) calls: there is no FPU, floats are soft-float.
 *
 * Quirk (confirmed on hardware): window row 0 is the TOP of the screen, so a y-down
 * 2D projection is glOrthox(0, W<<16, 0, H<<16, -1<<16, 1<<16).
 *
 * Quirk (confirmed on hardware): the renderer is software, and glClear on the 16-bit
 * surface ignores the scissor box whenever glColorMask is all-true, Draw rectangles
 * as quads with glDrawArrays instead of scissored clears.
 * 
 * Written by an LLM
 */
#ifndef EAPP_GLES_H
#define EAPP_GLES_H
#include <stdint.h>

typedef unsigned int  GLenum;
typedef unsigned char GLboolean;
typedef unsigned int  GLbitfield;
typedef signed char   GLbyte;
typedef short         GLshort;
typedef int           GLint;
typedef int           GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int  GLuint;
typedef float         GLfloat;
typedef float         GLclampf;
typedef int           GLfixed;
typedef int           GLclampx;
typedef int           GLintptr;
typedef int           GLsizeiptr;
typedef void          GLvoid;

#define GL_FALSE 0
#define GL_TRUE  1
#define GL_FIXED_ONE 0x10000
#define GL_X(f) ((GLfixed)((f) * 65536))

#define GL_DEPTH_BUFFER_BIT   0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT   0x00004000
#define GL_POINTS         0x0000
#define GL_LINES          0x0001
#define GL_LINE_LOOP      0x0002
#define GL_LINE_STRIP     0x0003
#define GL_TRIANGLES      0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN   0x0006
#define GL_BYTE           0x1400
#define GL_UNSIGNED_BYTE  0x1401
#define GL_SHORT          0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_FLOAT          0x1406
#define GL_FIXED          0x140C
#define GL_TEXTURE_2D     0x0DE1
#define GL_CULL_FACE      0x0B44
#define GL_BLEND          0x0BE2
#define GL_DEPTH_TEST     0x0B71
#define GL_SCISSOR_TEST   0x0C11
#define GL_VERTEX_ARRAY        0x8074
#define GL_NORMAL_ARRAY        0x8075
#define GL_COLOR_ARRAY         0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_MODELVIEW      0x1700
#define GL_PROJECTION     0x1701
#define GL_TEXTURE        0x1702
#define GL_SRC_ALPHA           0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_RGB            0x1907
#define GL_RGBA           0x1908
#define GL_UNSIGNED_SHORT_5_6_5   0x8363
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_NEAREST        0x2600
#define GL_LINEAR         0x2601
#define GL_VENDOR     0x1F00
#define GL_RENDERER   0x1F01
#define GL_VERSION    0x1F02
#define GL_EXTENSIONS 0x1F03

/* ---- GL (slots 0-138) ---- */
void glActiveTexture(GLenum texture);
void glAlphaFunc(GLenum func, GLclampf ref);
void glAlphaFuncx(GLenum func, GLclampx ref);
void glBindBuffer(GLenum target, GLuint buffer);
void glBindTexture(GLenum target, GLuint texture);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
void glClear(GLbitfield mask);
void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void glClearColorx(GLclampx r, GLclampx g, GLclampx b, GLclampx a);
void glClearDepthf(GLclampf depth);
void glClearDepthx(GLclampx depth);
void glClearStencil(GLint s);
void glClientActiveTexture(GLenum texture);
void glClipPlanex(GLenum plane, const GLfixed *equation);
void glClipPlanef(GLenum plane, const GLfloat *equation);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glColor4x(GLfixed r, GLfixed g, GLfixed b, GLfixed a);
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei w, GLsizei h, GLint border, GLsizei imageSize, const GLvoid *data);
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLsizei w, GLsizei h, GLenum format, GLsizei imageSize, const GLvoid *data);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei w, GLsizei h, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h);
void glCullFace(GLenum mode);
void glDeleteBuffers(GLsizei n, const GLuint *buffers);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glDepthRangef(GLclampf zNear, GLclampf zFar);
void glDepthRangex(GLclampx zNear, GLclampx zFar);
void glDisable(GLenum cap);
void glDisableClientState(GLenum array);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void glEnable(GLenum cap);
void glEnableClientState(GLenum array);
void glFinish(void);   /* no-op in this OS; work is flushed by eglSwapBuffers */
void glFlush(void);    /* no-op in this OS */
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogx(GLenum pname, GLfixed param);
void glFogxv(GLenum pname, const GLfixed *params);
void glFrontFace(GLenum mode);
void glFrustumf(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
void glFrustumx(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f);
void glGetBooleanv(GLenum pname, GLboolean *params);
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params);
void glGetClipPlanef(GLenum pname, GLfloat *eqn);
void glGetClipPlanex(GLenum pname, GLfixed *eqn);
void glGenBuffers(GLsizei n, GLuint *buffers);
void glGenTextures(GLsizei n, GLuint *textures);
GLenum glGetError(void);
void glGetFixedv(GLenum pname, GLfixed *params);
void glGetFloatv(GLenum pname, GLfloat *params);
void glGetIntegerv(GLenum pname, GLint *params);
void glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void glGetLightxv(GLenum light, GLenum pname, GLfixed *params);
void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void glGetMaterialxv(GLenum face, GLenum pname, GLfixed *params);
void glGetPointerv(GLenum pname, GLvoid **params);
const GLubyte *glGetString(GLenum name);
void glGetTexEnviv(GLenum env, GLenum pname, GLint *params);
void glGetTexEnvfv(GLenum env, GLenum pname, GLfloat *params);
void glGetTexEnvxv(GLenum env, GLenum pname, GLfixed *params);
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params); /* always GL_INVALID_ENUM */
void glGetTexParameterxv(GLenum target, GLenum pname, GLfixed *params); /* always GL_INVALID_ENUM */
void glHint(GLenum target, GLenum mode);
GLboolean glIsBuffer(GLuint buffer);
GLboolean glIsEnabled(GLenum cap);
GLboolean glIsTexture(GLuint texture);
void glLightModelf(GLenum pname, GLfloat param);
void glLightModelfv(GLenum pname, const GLfloat *params);
void glLightModelx(GLenum pname, GLfixed param);
void glLightModelxv(GLenum pname, const GLfixed *params);
void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLightx(GLenum light, GLenum pname, GLfixed param);
void glLightxv(GLenum light, GLenum pname, const GLfixed *params);
void glLineWidth(GLfloat width);
void glLineWidthx(GLfixed width);
void glLoadIdentity(void);
void glLoadMatrixf(const GLfloat *m);
void glLoadMatrixx(const GLfixed *m);
void glLogicOp(GLenum opcode);
void glMaterialf(GLenum face, GLenum pname, GLfloat param);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void glMaterialx(GLenum face, GLenum pname, GLfixed param);
void glMaterialxv(GLenum face, GLenum pname, const GLfixed *params);
void glMatrixMode(GLenum mode);
void glMultMatrixf(const GLfloat *m);
void glMultMatrixx(const GLfixed *m);
void glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void glMultiTexCoord4x(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void glNormal3x(GLfixed nx, GLfixed ny, GLfixed nz);
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
void glOrthox(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f);
void glPixelStorei(GLenum pname, GLint param);
void glPointParameterf(GLenum pname, GLfloat param);
void glPointParameterfv(GLenum pname, const GLfloat *params);
void glPointParameterx(GLenum pname, GLfixed param);
void glPointParameterxv(GLenum pname, const GLfixed *params);
void glPointSize(GLfloat size);
void glPointSizex(GLfixed size);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glPolygonOffsetx(GLfixed factor, GLfixed units);
void glPopMatrix(void);
void glPushMatrix(void);
void glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, GLvoid *pixels);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glRotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
void glSampleCoverage(GLclampf value, GLboolean invert);
void glSampleCoveragex(GLclampx value, GLboolean invert);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glScalex(GLfixed x, GLfixed y, GLfixed z);
void glScissor(GLint x, GLint y, GLsizei w, GLsizei h);
void glShadeModel(GLenum mode);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilMask(GLuint mask);
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
void glTexEnvx(GLenum target, GLenum pname, GLfixed param);
void glTexEnvxv(GLenum target, GLenum pname, const GLfixed *params);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);   /* i and x slots behave identically */
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void glTexParameterx(GLenum target, GLenum pname, GLfixed param);
void glTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLsizei w, GLsizei h, GLenum format, GLenum type, const GLvoid *pixels);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glTranslatex(GLfixed x, GLfixed y, GLfixed z);
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
GLbitfield glQueryMatrixxOES(GLfixed mantissa[16], GLint exponent[16]);

/* ---- EGL 1.1 (slots 139-166) ---- */
typedef int   EGLint;
typedef unsigned int EGLBoolean;
typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLSurface;
typedef void *EGLContext;
#define EGL_DRAW 0x3059
#define EGL_READ 0x305A

EGLint     eglGetError(void);
EGLDisplay eglGetDisplay(void *native);          /* returns its argument */
EGLBoolean eglInitialize(EGLDisplay d, EGLint *major, EGLint *minor);  /* 1.0 */
EGLBoolean eglTerminate(EGLDisplay d);
const char *eglQueryString(EGLDisplay d, EGLint name);
EGLBoolean eglGetConfigs(EGLDisplay d, EGLConfig *configs, EGLint size, EGLint *num);
EGLBoolean eglChooseConfig(EGLDisplay d, const EGLint *attrs, EGLConfig *configs, EGLint size, EGLint *num);
EGLBoolean eglGetConfigAttrib(EGLDisplay d, EGLConfig c, EGLint attr, EGLint *value);
EGLSurface eglCreateWindowSurface(EGLDisplay d, EGLConfig c, void *win, const EGLint *attrs);
EGLSurface eglCreatePixmapSurface(EGLDisplay d, EGLConfig c, void *pix, const EGLint *attrs);
EGLSurface eglCreatePbufferSurface(EGLDisplay d, EGLConfig c, const EGLint *attrs);
EGLBoolean eglDestroySurface(EGLDisplay d, EGLSurface s);
EGLBoolean eglQuerySurface(EGLDisplay d, EGLSurface s, EGLint attr, EGLint *value);
EGLBoolean eglSurfaceAttrib(EGLDisplay d, EGLSurface s, EGLint attr, EGLint value); /* stub: false */
EGLBoolean eglBindTexImage(EGLDisplay d, EGLSurface s, EGLint buffer);             /* stub: false */
EGLBoolean eglReleaseTexImage(EGLDisplay d, EGLSurface s, EGLint buffer);          /* stub: false */
EGLBoolean eglSwapInterval(EGLDisplay d, EGLint interval);                         /* stub: false */
EGLContext eglCreateContext(EGLDisplay d, EGLConfig c, EGLContext share, const EGLint *attrs);
EGLBoolean eglDestroyContext(EGLDisplay d, EGLContext ctx);
EGLBoolean eglMakeCurrent(EGLDisplay d, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLContext eglGetCurrentContext(void);
EGLSurface eglGetCurrentSurface(EGLint readdraw);
EGLDisplay eglGetCurrentDisplay(void);           /* returns 0 */
EGLBoolean eglQueryContext(EGLDisplay d, EGLContext ctx, EGLint attr, EGLint *value);
EGLBoolean eglWaitGL(void);
EGLBoolean eglWaitNative(EGLint engine);
EGLBoolean eglSwapBuffers(EGLDisplay d, EGLSurface s);  /* presents s's back buffer */
EGLBoolean eglCopyBuffers(EGLDisplay d, EGLSurface s, void *target);

#endif
