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

#ifndef __BACKEND_H__
#define __BACKEND_H__

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>


typedef struct _VGLFBConfig *VGLFBConfig;


namespace backend
{
	void bindFramebuffer(GLenum target, GLuint framebuffer, bool ext = false);

	GLXContext createContext(Display *dpy, VGLFBConfig config, GLXContext share,
		Bool direct, const int *glxAttribs);

	GLXPbuffer createPbuffer(Display *dpy, VGLFBConfig config,
		const int *glxAttribs);

	void deleteFramebuffers(GLsizei n, const GLuint *framebuffers);

	void destroyContext(Display *dpy, GLXContext ctx);

	void destroyPbuffer(Display *dpy, GLXPbuffer pbuf);

	void drawBuffer(GLenum mode);

	void drawBuffers(GLsizei n, const GLenum *bufs);

	GLXContext getCurrentContext(void);

	Display *getCurrentDisplay(void);

	GLXDrawable getCurrentDrawable(void);

	GLXDrawable getCurrentReadDrawable(void);

	int getFBConfigAttrib(Display *dpy, VGLFBConfig config, int attribute,
		int *value);

	void getFramebufferAttachmentParameteriv(GLenum target, GLenum attachment,
		GLenum pname, GLint *params);

	void getFramebufferParameteriv(GLenum target, GLenum pname, GLint *params);

	void getIntegerv(GLenum pname, GLint *params);

	void getNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname,
		GLint *param);

	Bool isDirect(GLXContext ctx);

	Bool makeCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
		GLXContext ctx);

	void namedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf,
		bool ext = false);

	void namedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n,
		const GLenum *bufs, bool ext = false);

	void namedFramebufferReadBuffer(GLuint framebuffer, GLenum mode,
		bool ext = false);

	int queryContext(Display *dpy, GLXContext ctx, int attribute, int *value);

	void queryDrawable(Display *dpy, GLXDrawable draw, int attribute,
		unsigned int *value);

	Bool queryExtension(Display *dpy, int *majorOpcode, int *eventBase,
		int *errorBase);

	void readBuffer(GLenum mode);

	void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
		GLenum format, GLenum type, void *data);

	void swapBuffers(Display *dpy, GLXDrawable drawable);
}

#endif  // __BACKEND_H__
