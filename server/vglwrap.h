// Copyright (C)2019-2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

// When using the GLX back end, these functions simply wrap their GLX or OpenGL
// counterparts, with the exception that the GLX functions are redirected to
// the 3D X server.  When using the EGL back end, these functions emulate the
// same behavior as the GLX functions but without a 3D X server.  These
// functions only handle the GLX features that the faker uses behind the
// scenes, so they only deal with Pbuffers.  The emulation of GLX Windows and
// Pixmaps is handled by the GLX interposer, which calls down to these
// functions.

#ifndef __VGLWRAP_H__
#define __VGLWRAP_H__

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>


typedef struct _VGLFBConfig *VGLFBConfig;


void VGLBindFramebuffer(GLenum target, GLuint framebuffer, bool ext = false);

GLXContext VGLCreateContext(Display *dpy, VGLFBConfig config, GLXContext share,
	Bool direct, const int *glxAttribs);

GLXPbuffer VGLCreatePbuffer(Display *dpy, VGLFBConfig config,
	const int *glxAttribs);

void VGLDeleteFramebuffers(GLsizei n, const GLuint *framebuffers);

void VGLDestroyContext(Display *dpy, GLXContext ctx);

void VGLDestroyPbuffer(Display *dpy, GLXPbuffer pbuf);

void VGLDrawBuffer(GLenum mode);

void VGLDrawBuffers(GLsizei n, const GLenum *bufs);

GLXContext VGLGetCurrentContext(void);

Display *VGLGetCurrentDisplay(void);

GLXDrawable VGLGetCurrentDrawable(void);

GLXDrawable VGLGetCurrentReadDrawable(void);

int VGLGetFBConfigAttrib(Display *dpy, VGLFBConfig config, int attribute,
	int *value);

void VGLGetIntegerv(GLenum pname, GLint *params);

Bool VGLIsDirect(GLXContext ctx);

Bool VGLMakeCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx);

#ifdef GL_VERSION_4_5

void VGLNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf);

void VGLNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n,
	const GLenum *bufs);

void VGLNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode);

#endif

int VGLQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value);

void VGLQueryDrawable(Display *dpy, GLXDrawable draw, int attribute,
	unsigned int *value);

Bool VGLQueryExtension(Display *dpy, int *majorOpcode, int *eventBase,
	int *errorBase);

void VGLReadBuffer(GLenum mode);

void VGLReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
	GLenum format, GLenum type, void *data);

void VGLSwapBuffers(Display *dpy, GLXDrawable drawable);

#endif  // __VGLWRAP_H__
