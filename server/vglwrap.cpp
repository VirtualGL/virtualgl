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

#include "vglwrap.h"
#ifdef EGLBACKEND
#include "EGLContextHash.h"
#include "VGLPbufferHash.h"
#include "EGLError.h"
#include "BufferState.h"
#endif
#include "PixmapHash.h"
#include "glxvisual.h"
#include "threadlocal.h"
#include <X11/Xmd.h>
#include <GL/glxproto.h>

#ifndef X_GLXCreateContextAttribsARB
#define X_GLXCreateContextAttribsARB  34
#endif

#ifdef EGLBACKEND

#define CATCH_EGL(minorCode) \
	catch(vglfaker::EGLError &e) \
	{ \
		int glxError = e.getGLXError(); \
		bool isX11Error = e.isX11Error(); \
		if(glxError >= 0) \
		{ \
			if(fconfig.verbose) \
				vglout.print("[VGL] ERROR: in %s--\n[VGL]    %s\n", GET_METHOD(e), \
					e.what()); \
			vglfaker::sendGLXError(dpy, minorCode, glxError, isX11Error); \
		} \
		else throw; \
	} \


VGL_THREAD_LOCAL(CurrentDrawable, GLXDrawable, None)
VGL_THREAD_LOCAL(CurrentReadDrawable, GLXDrawable, None)


static vglfaker::VGLPbuffer *getCurrentVGLPbuffer(EGLint readdraw)
{
	vglfaker::VGLPbuffer *pb = vpbhash.find(readdraw == EGL_READ ?
		getCurrentReadDrawable() : getCurrentDrawable());
	if(pb)
	{
		GLint fbo = -1;
		_glGetIntegerv(readdraw == EGL_READ ?
			GL_READ_FRAMEBUFFER_BINDING : GL_DRAW_FRAMEBUFFER_BINDING, &fbo);
		if(fbo == (GLint)pb->getFBO())
			return pb;
	}
	return NULL;
}

#endif


void VGLBindFramebuffer(GLenum target, GLuint framebuffer)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(framebuffer == 0)
		{
			if(target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
			{
				vglfaker::VGLPbuffer *pb = vpbhash.find(getCurrentDrawable());
				if(pb)
				{
					framebuffer = pb->getFBO();
					ectxhash.setDrawFBO(_eglGetCurrentContext(), 0);
				}
			}
			if(target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
			{
				vglfaker::VGLPbuffer *pb = vpbhash.find(getCurrentReadDrawable());
				if(pb)
				{
					framebuffer = pb->getFBO();
					ectxhash.setReadFBO(_eglGetCurrentContext(), 0);
				}
			}
		}
		else
		{
			if(target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
				ectxhash.setDrawFBO(_eglGetCurrentContext(), framebuffer);
			if(target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
				ectxhash.setReadFBO(_eglGetCurrentContext(), framebuffer);
		}
	}
	#endif
	_glBindFramebuffer(target, framebuffer);
}


void VGLDeleteFramebuffers(GLsizei n, const GLuint *framebuffers)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(n > 0 && framebuffers)
		{
			GLint drawFBO = -1, readFBO = -1;
			_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
			_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
			for(GLsizei i = 0; i < n; i++)
			{
				if((GLint)framebuffers[i] == drawFBO)
					VGLBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				if((GLint)framebuffers[i] == readFBO)
					VGLBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			}
		}
	}
	#endif
	_glDeleteFramebuffers(n, framebuffers);
}


GLXContext VGLCreateContext(Display *dpy, VGLFBConfig config, GLXContext share,
	Bool direct, const int *glxAttribs)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(!direct) return NULL;

		int eglAttribs[256], egli = 0;
		for(int i = 0; i < 256; i++) eglAttribs[i] = EGL_NONE;
		bool majorVerSpecified = false, forwardCompatSpecified = false;
		int majorVer = -1;

		if(glxAttribs && glxAttribs[0] != None)
		{
			for(int glxi = 0; glxAttribs[glxi] && egli < 256; glxi += 2)
			{
				switch(glxAttribs[glxi])
				{
					case GLX_CONTEXT_MAJOR_VERSION_ARB:
						eglAttribs[egli++] = EGL_CONTEXT_MAJOR_VERSION;
						eglAttribs[egli++] = majorVer = glxAttribs[glxi + 1];
						majorVerSpecified = true;
						break;
					case GLX_CONTEXT_MINOR_VERSION_ARB:
						eglAttribs[egli++] = EGL_CONTEXT_MINOR_VERSION;
						eglAttribs[egli++] = glxAttribs[glxi + 1];
						break;
					case GLX_CONTEXT_FLAGS_ARB:
					{
						int flags = glxAttribs[glxi + 1];
						if(flags & GLX_CONTEXT_DEBUG_BIT_ARB)
						{
							eglAttribs[egli++] = EGL_CONTEXT_OPENGL_DEBUG;
							eglAttribs[egli++] = EGL_TRUE;
						}
						if(flags & GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB)
						{
							eglAttribs[egli++] = EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE;
							eglAttribs[egli++] = EGL_TRUE;
							forwardCompatSpecified = true;
						}
						if(flags & GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB)
						{
							// For future expansion
							eglAttribs[egli++] = EGL_CONTEXT_OPENGL_ROBUST_ACCESS;
							eglAttribs[egli++] = EGL_TRUE;
						}
						if(flags & ~(GLX_CONTEXT_DEBUG_BIT_ARB |
							GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB))
						{
							vglfaker::sendGLXError(dpy, X_GLXCreateContextAttribsARB,
								BadValue, true);
							return NULL;
						}
						break;
					}
					case GLX_CONTEXT_PROFILE_MASK_ARB:
						// The mask bits are the same for GLX_ARB_create_context and EGL.
						eglAttribs[egli++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
						eglAttribs[egli++] = glxAttribs[glxi + 1];
						if(glxAttribs[glxi + 1] != GLX_CONTEXT_CORE_PROFILE_BIT_ARB
							&& glxAttribs[glxi + 1]
								!= GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB)
						{
							vglfaker::sendGLXError(dpy, X_GLXCreateContextAttribsARB,
								GLXBadProfileARB, false);
							return NULL;
						}
						break;
					case GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB:
						switch(glxAttribs[glxi + 1])
						{
							case GLX_NO_RESET_NOTIFICATION_ARB:
								eglAttribs[egli++] =
									EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY;
								eglAttribs[egli++] = EGL_NO_RESET_NOTIFICATION;
								break;
							case GLX_LOSE_CONTEXT_ON_RESET_ARB:
								eglAttribs[egli++] =
									EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY;
								eglAttribs[egli++] = EGL_LOSE_CONTEXT_ON_RESET;
								break;
						}
						break;
					default:
						if(glxAttribs[glxi] == GLX_RENDER_TYPE
							&& glxAttribs[glxi + 1] == GLX_COLOR_INDEX_TYPE)
						{
							vglfaker::sendGLXError(dpy, X_GLXCreateContextAttribsARB,
								BadMatch, true);
							return NULL;
						}
						else
						{
							vglfaker::sendGLXError(dpy, X_GLXCreateContextAttribsARB,
								BadValue, true);
						}
						return NULL;
				}
			}
		}

		CARD16 minorCode = egli ? X_GLXCreateContextAttribsARB :
			X_GLXCreateNewContext;
		if(majorVerSpecified && forwardCompatSpecified && majorVer < 3)
		{
			vglfaker::sendGLXError(dpy, minorCode, BadMatch, true);
			return NULL;
		}
		try
		{
			if(!VALID_CONFIG(config))
			{
				vglfaker::sendGLXError(dpy, minorCode, GLXBadFBConfig, false);
				return NULL;
			}
			if(!share)
			{
				vglfaker::getRBOContext(dpy).createContext();
				share = (GLXContext)vglfaker::getRBOContext(dpy).getContext();
			}
			if(!_eglBindAPI(EGL_OPENGL_API))
				THROW("Could not enable OpenGL API");
			GLXContext ctx = (GLXContext)_eglCreateContext(EDPY, (EGLConfig)0,
				(EGLContext)share, egli ? eglAttribs : NULL);
			EGLint eglError = _eglGetError();
			// Some implementations of eglCreateContext() return NULL but do not set
			// the EGL error if certain invalid OpenGL versions are passed to the
			// function.  This is why we can't have nice things.
			if(!ctx && eglError == EGL_SUCCESS && majorVerSpecified)
				eglError = EGL_BAD_MATCH;
			if(!ctx && eglError != EGL_SUCCESS)
				throw(vglfaker::EGLError("eglCreateContext()", __LINE__, eglError));
			if(ctx) ectxhash.add(ctx, config);
			return ctx;
		}
		CATCH_EGL(minorCode)
		return 0;
	}
	else
	#endif
	{
		if(glxAttribs && glxAttribs[0] != None)
			return _glXCreateContextAttribsARB(DPY3D, GLXFBC(config), share, direct,
				glxAttribs);
		else
			return _glXCreateNewContext(DPY3D, GLXFBC(config), GLX_RGBA_TYPE, share,
				direct);
	}
}


GLXPbuffer VGLCreatePbuffer(Display *dpy, VGLFBConfig config,
	const int *glxAttribs)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			vglfaker::VGLPbuffer *pb =
				new vglfaker::VGLPbuffer(dpy, config, glxAttribs);
			GLXDrawable id = pb->getID();
			if(id) vpbhash.add(id, pb);
			return id;
		}
		CATCH_EGL(X_GLXCreatePbuffer)
		return 0;
	}
	else
	#endif
		return _glXCreatePbuffer(DPY3D, GLXFBC(config), glxAttribs);
}


void VGLDestroyContext(Display *dpy, GLXContext ctx)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			if(!ctx) return;
			VGLFBConfig config = ectxhash.findConfig(ctx);
			ectxhash.remove(ctx);
			if(!_eglBindAPI(EGL_OPENGL_API))
				THROW("Could not enable OpenGL API");
			if(!_eglDestroyContext(EDPY, (EGLContext)ctx))
				THROW_EGL("eglDestroyContext()");
			if(!config)
				vglfaker::sendGLXError(dpy, X_GLXDestroyContext, GLXBadContext, false);
		}
		CATCH_EGL(X_GLXDestroyContext)
	}
	else
	#endif
		_glXDestroyContext(DPY3D, ctx);
}


void VGLDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			vpbhash.remove(pbuf);
		}
		CATCH_EGL(X_GLXDestroyPbuffer)
	}
	else
	#endif
		_glXDestroyPbuffer(DPY3D, pbuf);
}


void VGLDrawBuffer(GLenum mode)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		vglfaker::VGLPbuffer *pb = getCurrentVGLPbuffer(EGL_DRAW);
		if(pb)
		{
			pb->setDrawBuffer(mode, false);
			return;
		}
	}
	#endif
	_glDrawBuffer(mode);
}


void VGLDrawBuffers(GLsizei n, const GLenum *bufs)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		vglfaker::VGLPbuffer *pb = getCurrentVGLPbuffer(EGL_DRAW);
		if(pb)
		{
			pb->setDrawBuffers(n, bufs, false);
			return;
		}
	}
	#endif
	_glDrawBuffers(n, bufs);
}


GLXContext VGLGetCurrentContext(void)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		return (GLXContext)_eglGetCurrentContext();
	}
	else
	#endif
		return _glXGetCurrentContext();
}


Display *VGLGetCurrentDisplay(void)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		vglfaker::VGLPbuffer *pb = vpbhash.find(getCurrentDrawable());
		return pb ? pb->getDisplay() : NULL;
	}
	else
	#endif
		return _glXGetCurrentDisplay();
}


GLXDrawable VGLGetCurrentDrawable(void)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
		return getCurrentDrawable();
	else
	#endif
		return _glXGetCurrentDrawable();
}


GLXDrawable VGLGetCurrentReadDrawable(void)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
		return getCurrentReadDrawable();
	else
	#endif
		return _glXGetCurrentReadDrawable();
}


int VGLGetFBConfigAttrib(Display *dpy, VGLFBConfig config, int attribute,
	int *value)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(!value) return GLX_BAD_VALUE;

		switch(attribute)
		{
			case GLX_FBCONFIG_ID:
				*value = config->id;
				return Success;
			case GLX_BUFFER_SIZE:
				*value = config->attr.redSize + config->attr.greenSize +
					config->attr.blueSize + config->attr.alphaSize;
				return Success;
			case GLX_LEVEL:
				*value = 0;
				return Success;
			case GLX_DOUBLEBUFFER:
				*value = config->attr.doubleBuffer;
				return Success;
			case GLX_STEREO:
				*value = config->attr.stereo;
				return Success;
			case GLX_AUX_BUFFERS:
				*value = 0;
				return Success;
			case GLX_RED_SIZE:
				*value = config->attr.redSize;
				return Success;
			case GLX_GREEN_SIZE:
				*value = config->attr.greenSize;
				return Success;
			case GLX_BLUE_SIZE:
				*value = config->attr.blueSize;
				return Success;
			case GLX_ALPHA_SIZE:
				*value = config->attr.alphaSize;
				return Success;
			case GLX_DEPTH_SIZE:
				*value = config->attr.depthSize;
				return Success;
			case GLX_STENCIL_SIZE:
				*value = config->attr.stencilSize;
				return Success;
			case GLX_ACCUM_RED_SIZE:
			case GLX_ACCUM_GREEN_SIZE:
			case GLX_ACCUM_BLUE_SIZE:
			case GLX_ACCUM_ALPHA_SIZE:
				*value = 0;
				return Success;
			case GLX_RENDER_TYPE:
				*value = GLX_RGBA_BIT;
				return Success;
			case GLX_DRAWABLE_TYPE:
				*value = GLX_PBUFFER_BIT |
					(config->visualID ? GLX_WINDOW_BIT | GLX_PIXMAP_BIT : 0);
				return Success;
			case GLX_X_RENDERABLE:
				*value = !!config->visualID;
				return Success;
			case GLX_VISUAL_ID:
				*value = config->visualID;
				return Success;
			case GLX_X_VISUAL_TYPE:
				*value = config->c_class == TrueColor ?
					GLX_TRUE_COLOR : GLX_DIRECT_COLOR;
				return Success;
			case GLX_CONFIG_CAVEAT:
			case GLX_TRANSPARENT_TYPE:
				*value = GLX_NONE;
				return Success;
			case GLX_TRANSPARENT_INDEX_VALUE:
			case GLX_TRANSPARENT_RED_VALUE:
			case GLX_TRANSPARENT_GREEN_VALUE:
			case GLX_TRANSPARENT_BLUE_VALUE:
			case GLX_TRANSPARENT_ALPHA_VALUE:
				*value = 0;
				return Success;
			case GLX_MAX_PBUFFER_WIDTH:
			{
				*value = config->maxPBWidth;
				return EGL_TRUE;
			}
			case GLX_MAX_PBUFFER_HEIGHT:
			{
				*value = config->maxPBHeight;
				return EGL_TRUE;
			}
			case GLX_MAX_PBUFFER_PIXELS:
			{
				*value = config->maxPBWidth * config->maxPBHeight;
				return EGL_TRUE;
			}
			case GLX_SAMPLE_BUFFERS:
				*value = !!config->attr.samples;
				return Success;
			case GLX_SAMPLES:
				*value = config->attr.samples;
				return Success;
			default:
				return GLX_BAD_ATTRIBUTE;
		}
	}  // fconfig.egl
	else
	#endif
		return _glXGetFBConfigAttrib(DPY3D, GLXFBC(config), attribute, value);
}


void VGLGetIntegerv(GLenum pname, GLint *params)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		EGLContext ctx = _eglGetCurrentContext();
		VGLFBConfig config = ectxhash.findConfig(ctx);

		if(!params || !config)
		{
			_glGetIntegerv(pname, params);
			return;
		}
		else if(pname == GL_DOUBLEBUFFER)
		{
			*params = config->attr.doubleBuffer;
			return;
		}
		else if(pname == GL_DRAW_BUFFER)
		{
			vglfaker::VGLPbuffer *pb = getCurrentVGLPbuffer(EGL_DRAW);
			GLenum drawBuf = ectxhash.getDrawBuffer(ctx, 0);
			if(pb)
			{
				*params = drawBuf;
				return;
			}
		}
		else if(pname >= GL_DRAW_BUFFER0 && pname <= GL_DRAW_BUFFER15)
		{
			vglfaker::VGLPbuffer *pb = getCurrentVGLPbuffer(EGL_DRAW);
			int index = pname - GL_DRAW_BUFFER0;
			GLenum drawBuf = ectxhash.getDrawBuffer(ctx, index);
			if(pb)
			{
				*params = drawBuf;
				return;
			}
		}
		else if(pname == GL_DRAW_FRAMEBUFFER_BINDING)
		{
			*params = ectxhash.getDrawFBO(ctx);
			return;
		}
		else if(pname == GL_MAX_DRAW_BUFFERS)
		{
			*params = 16;
			return;
		}
		else if(pname == GL_READ_BUFFER)
		{
			vglfaker::VGLPbuffer *pb = getCurrentVGLPbuffer(EGL_READ);
			GLenum readBuf = ectxhash.getReadBuffer(ctx);
			if(pb)
			{
				*params = readBuf;
				return;
			}
		}
		else if(pname == GL_READ_FRAMEBUFFER_BINDING)
		{
			*params = ectxhash.getReadFBO(ctx);
			return;
		}
		else if(pname == GL_STEREO)
		{
			*params = config->attr.stereo;
			return;
		}
	}
	#endif
	_glGetIntegerv(pname, params);
}


Bool VGLIsDirect(GLXContext ctx)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
		return True;
	else
	#endif
		return _glXIsDirect(DPY3D, ctx);
}


Bool VGLMakeCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			if(!_eglBindAPI(EGL_OPENGL_API))
				THROW("Could not enable OpenGL API");
			EGLBoolean ret = (Bool)_eglMakeCurrent(EDPY, EGL_NO_SURFACE,
				EGL_NO_SURFACE, (EGLContext)ctx);
			if(!ret) THROW_EGL("eglMakeCurrent()");
			setCurrentDrawable(draw);
			setCurrentReadDrawable(read);
			if(!ctx) return True;

			vglfaker::VGLPbuffer *drawpb = NULL, *readpb = NULL;
			drawpb = vpbhash.find(draw);
			readpb = (read == draw ? drawpb : vpbhash.find(read));
			GLint drawFBO = -1, readFBO = -1;
			_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
			_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);

			if(drawpb) drawpb->createBuffer(false, true);
			if(readpb && readpb != drawpb) readpb->createBuffer(false, true);

			bool boundNewDrawFBO = false, boundNewReadFBO = false;
			if(drawpb && (ectxhash.getDrawFBO(ctx) == 0 || drawFBO == 0))
			{
				_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawpb->getFBO());
				boundNewDrawFBO = true;
			}
			if(readpb && (ectxhash.getReadFBO(ctx) == 0 || readFBO == 0))
			{
				_glBindFramebuffer(GL_READ_FRAMEBUFFER, readpb->getFBO());
				boundNewReadFBO = true;
			}

			VGLFBConfig config = ectxhash.findConfig(ctx);
			if(drawpb)
			{
				if(drawFBO == 0 && config)
				{
					drawpb->setDrawBuffer(config->attr.doubleBuffer ?
						GL_BACK : GL_FRONT, false);
					_glViewport(0, 0, drawpb->getWidth(), drawpb->getHeight());
				}
				else if(boundNewDrawFBO)
				{
					const GLenum *oldDrawBufs;  GLsizei nDrawBufs = 0;
					oldDrawBufs = ectxhash.getDrawBuffers(ctx, nDrawBufs);
					if(oldDrawBufs && nDrawBufs > 0)
					{
						drawpb->setDrawBuffers(nDrawBufs, oldDrawBufs, false);
						delete [] oldDrawBufs;
					}
				}
			}
			if(readpb)
			{
				if(readFBO == 0 && config)
					readpb->setReadBuffer(config->attr.doubleBuffer ?
						GL_BACK : GL_FRONT, false);
				else if(boundNewReadFBO)
				{
					GLenum oldReadBuf = ectxhash.getReadBuffer(ctx);
					if(oldReadBuf) readpb->setReadBuffer(oldReadBuf, false);
				}
			}

			return ret;
		}
		CATCH_EGL(X_GLXMakeContextCurrent)
		return 0;
	}
	else
	#endif
		return _glXMakeContextCurrent(DPY3D, draw, read, ctx);
}


#ifdef GL_VERSION_4_5

void VGLNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		vglfaker::VGLPbuffer *pb;
		if(framebuffer == 0 && (pb = getCurrentVGLPbuffer(EGL_DRAW)) != NULL)
		{
			pb->setDrawBuffer(buf, true);
			return;
		}
	}
	#endif
	_glNamedFramebufferDrawBuffer(framebuffer, buf);
}


void VGLNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n,
	const GLenum *bufs)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		vglfaker::VGLPbuffer *pb;
		if(framebuffer == 0 && (pb = getCurrentVGLPbuffer(EGL_DRAW)) != NULL)
		{
			pb->setDrawBuffers(n, bufs, true);
			return;
		}
	}
	#endif
	_glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
}


void VGLNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		vglfaker::VGLPbuffer *pb;
		if(framebuffer == 0 && (pb = getCurrentVGLPbuffer(EGL_READ)) != NULL)
		{
			pb->setReadBuffer(mode, true);
			return;
		}
	}
	#endif
	_glNamedFramebufferReadBuffer(framebuffer, mode);
}

#endif


int VGLQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		int retval = Success;
		VGLFBConfig config;

		if(!ctx || !(config = ectxhash.findConfig(ctx)))
		{
			vglfaker::sendGLXError(dpy, X_GLXQueryContext, GLXBadContext, false);
			return GLX_BAD_CONTEXT;
		}
		switch(attribute)
		{
			case GLX_FBCONFIG_ID:
				*value = config->id;
				retval = Success;
				break;
			case GLX_RENDER_TYPE:
				*value = GLX_RGBA_TYPE;
				retval = Success;
				break;
			case GLX_SCREEN:
				*value = config->screen;
				retval = Success;
				break;
			default:
				retval = GLX_BAD_ATTRIBUTE;
		}

		return retval;
	}
	else
	#endif
	{
		int retval = _glXQueryContext(DPY3D, ctx, attribute, value);
		if(fconfig.amdgpuHack && ctx && attribute == GLX_RENDER_TYPE && value
			&& *value == 0)
			*value = GLX_RGBA_TYPE;
		return retval;
	}
}


void VGLQueryDrawable(Display *dpy, GLXDrawable draw, int attribute,
	unsigned int *value)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		vglfaker::VGLPbuffer *pb = NULL;

		if(!value) return;

		if(!draw || (pb = vpbhash.find(draw)) == NULL)
		{
			vglfaker::sendGLXError(dpy, X_GLXGetDrawableAttributes, GLXBadDrawable,
				false);
			return;
		}

		switch(attribute)
		{
			case GLX_WIDTH:
				*value = pb->getWidth();
				return;
			case GLX_HEIGHT:
				*value = pb->getHeight();
				return;
			case GLX_PRESERVED_CONTENTS:
				*value = 1;
				return;
			case GLX_LARGEST_PBUFFER:
				*value = 0;
				return;
			case GLX_FBCONFIG_ID:
				*value = pb->getFBConfig() ? pb->getFBConfig()->id : 0;
				return;
			default:
				return;
		}
	}
	else
	#endif
		_glXQueryDrawable(DPY3D, draw, attribute, value);
}


Bool VGLQueryExtension(Display *dpy, int *majorOpcode, int *eventBase,
	int *errorBase)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		// If the 2D X server has a GLX extension, then we hijack its major opcode
		// and error base.
		Bool retval = _XQueryExtension(dpy, "GLX", majorOpcode, eventBase,
			errorBase);
		// Otherwise, there is no sane way for the EGL back end to operate, mostly
		// because of XCB.
		if(!retval)
			THROW("The EGL back end requires a 2D X server with a GLX extension.");
		return retval;
	}
	// When using the GLX back end, all GLX errors will come from the 3D X
	// server.
	else
	#endif
		return _XQueryExtension(DPY3D, "GLX", majorOpcode, eventBase, errorBase);
}


void VGLReadBuffer(GLenum mode)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		vglfaker::VGLPbuffer *pb = getCurrentVGLPbuffer(EGL_READ);
		if(pb)
		{
			pb->setReadBuffer(mode, false);
			return;
		}
	}
	#endif
	_glReadBuffer(mode);
}


void VGLReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
	GLenum format, GLenum type, void *data)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		bool fallthrough = true;
		VGLFBConfig config = ectxhash.findConfig(_eglGetCurrentContext());
		vglfaker::VGLPbuffer *readpb = getCurrentVGLPbuffer(EGL_READ);

		if(config && config->attr.samples > 1 && readpb)
		{
			GLuint fbo = 0, rbo = 0;
			_glGenFramebuffers(1, &fbo);
			if(fbo)
			{
				vglfaker::BufferState
					bs(BS_DRAWFBO | BS_READFBO | BS_DRAWBUFS | BS_READBUF);
				_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

				_glGenRenderbuffers(1, &rbo);
				if(rbo)
				{
					vglfaker::BufferState bsRBO(BS_RBO);
					_glBindRenderbuffer(GL_RENDERBUFFER, rbo);

					GLenum internalFormat = GL_RGB8;
					if(config->attr.redSize > 8) internalFormat = GL_RGB10_A2;
					else if(config->attr.alphaSize) internalFormat = GL_RGBA8;
					_glRenderbufferStorage(GL_RENDERBUFFER, internalFormat,
						readpb->getWidth(), readpb->getHeight());
					_glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						GL_RENDERBUFFER, rbo);

					GLenum status = _glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
					if(status == GL_FRAMEBUFFER_COMPLETE)
					{
						_glBlitFramebuffer(0, 0, readpb->getWidth(), readpb->getHeight(),
							0, 0, readpb->getWidth(), readpb->getHeight(),
							GL_COLOR_BUFFER_BIT, GL_NEAREST);
						_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bs.getOldReadFBO());
						_glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
						_glReadPixels(x, y, width, height, format, type, data);
						fallthrough = false;
					}
					_glDeleteRenderbuffers(1, &rbo);
				}
				_glDeleteFramebuffers(1, &fbo);
			}
		}
		if(!fallthrough) return;
	}
	#endif
	_glReadPixels(x, y, width, height, format, type, data);
}


void VGLSwapBuffers(Display *dpy, GLXDrawable drawable)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			if(pmhash.find(dpy, drawable)) return;

			vglfaker::VGLPbuffer *pb = NULL;

			if(drawable && (pb = vpbhash.find(drawable)) != NULL)
				pb->swap();
			else
				vglfaker::sendGLXError(dpy, X_GLXSwapBuffers, GLXBadDrawable, false);
		}
		CATCH_EGL(X_GLXSwapBuffers)
	}
	else
	#endif
		_glXSwapBuffers(DPY3D, drawable);
}
