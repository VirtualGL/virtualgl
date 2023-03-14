// Copyright (C)2019-2023 D. R. Commander
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

#include "backend.h"
#ifdef EGLBACKEND
#include "ContextHashEGL.h"
#include "PbufferHashEGL.h"
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


namespace backend {

#ifdef EGLBACKEND

#define CATCH_EGL(minorCode) \
	catch(EGLError &e) \
	{ \
		int glxError = e.getGLXError(); \
		bool isX11Error = e.isX11Error(); \
		if(glxError >= 0) \
		{ \
			if(fconfig.verbose) \
				vglout.print("[VGL] ERROR: in %s--\n[VGL]    %s\n", GET_METHOD(e), \
					e.what()); \
			faker::sendGLXError(dpy, minorCode, glxError, isX11Error); \
		} \
		else throw; \
	} \


VGL_THREAD_LOCAL(CurrentContextEGL, GLXContext, None)
VGL_THREAD_LOCAL(CurrentDrawableEGL, GLXDrawable, None)
VGL_THREAD_LOCAL(CurrentReadDrawableEGL, GLXDrawable, None)


static FakePbuffer *getCurrentFakePbuffer(EGLint readdraw)
{
	FakePbuffer *pb = pbhashegl.find(readdraw == EGL_READ ?
		getCurrentReadDrawableEGL() : getCurrentDrawableEGL());
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


void bindFramebuffer(GLenum target, GLuint framebuffer, bool ext)
{
	#ifdef EGLBACKEND
	const GLenum *oldDrawBufs = NULL;  GLsizei nDrawBufs = 0;
	GLenum oldReadBuf = GL_NONE;
	FakePbuffer *drawpb = NULL, *readpb = NULL;

	if(fconfig.egl)
	{
		if(framebuffer == 0)
		{
			if(target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
			{
				drawpb = pbhashegl.find(getCurrentDrawableEGL());
				if(drawpb)
				{
					oldDrawBufs =
						ctxhashegl.getDrawBuffers(_eglGetCurrentContext(), nDrawBufs);
					framebuffer = drawpb->getFBO();
					ctxhashegl.setDrawFBO(_eglGetCurrentContext(), 0);
				}
			}
			if(target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
			{
				readpb = pbhashegl.find(getCurrentReadDrawableEGL());
				if(readpb)
				{
					oldReadBuf = ctxhashegl.getReadBuffer(_eglGetCurrentContext());
					framebuffer = readpb->getFBO();
					ctxhashegl.setReadFBO(_eglGetCurrentContext(), 0);
				}
			}
		}
		else
		{
			if(target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
				ctxhashegl.setDrawFBO(_eglGetCurrentContext(), framebuffer);
			if(target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
				ctxhashegl.setReadFBO(_eglGetCurrentContext(), framebuffer);
		}
	}
	#endif
	if(ext) _glBindFramebufferEXT(target, framebuffer);
	else _glBindFramebuffer(target, framebuffer);
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(oldDrawBufs)
		{
			if(nDrawBufs == 1)
				drawpb->setDrawBuffer(oldDrawBufs[0], false);
			else if(nDrawBufs > 0)
				drawpb->setDrawBuffers(nDrawBufs, oldDrawBufs, false);
			delete [] oldDrawBufs;
		}
		if(oldReadBuf) readpb->setReadBuffer(oldReadBuf, false);
	}
	#endif
}


void deleteFramebuffers(GLsizei n, const GLuint *framebuffers)
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
					bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				if((GLint)framebuffers[i] == readFBO)
					bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			}
		}
	}
	#endif
	_glDeleteFramebuffers(n, framebuffers);
}


GLXContext createContext(Display *dpy, VGLFBConfig config, GLXContext share,
	Bool direct, const int *glxAttribs)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(!direct) return NULL;

		int eglAttribs[MAX_ATTRIBS + 1], egli = 0;
		for(int i = 0; i < MAX_ATTRIBS + 1; i++) eglAttribs[i] = EGL_NONE;
		bool majorVerSpecified = false, forwardCompatSpecified = false;
		int majorVer = -1;

		if(glxAttribs && glxAttribs[0] != None)
		{
			for(int glxi = 0; glxAttribs[glxi] && egli < MAX_ATTRIBS; glxi += 2)
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
							faker::sendGLXError(dpy, X_GLXCreateContextAttribsARB, BadValue,
								true);
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
							faker::sendGLXError(dpy, X_GLXCreateContextAttribsARB,
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
							faker::sendGLXError(dpy, X_GLXCreateContextAttribsARB, BadMatch,
								true);
							return NULL;
						}
						else if(glxAttribs[glxi] != GLX_RENDER_TYPE
							|| glxAttribs[glxi + 1] != GLX_RGBA_TYPE)
						{
							faker::sendGLXError(dpy, X_GLXCreateContextAttribsARB, BadValue,
								true);
							return NULL;
						}
				}
			}
		}

		CARD16 minorCode = egli ? X_GLXCreateContextAttribsARB :
			X_GLXCreateNewContext;
		if(majorVerSpecified && forwardCompatSpecified && majorVer < 3)
		{
			faker::sendGLXError(dpy, minorCode, BadMatch, true);
			return NULL;
		}
		try
		{
			if(!VALID_CONFIG(config))
			{
				faker::sendGLXError(dpy, minorCode, GLXBadFBConfig, false);
				return NULL;
			}
			getRBOContext(dpy).createContext(RBOContext::REFCOUNT_CONTEXT);
			if(!share)
				share = (GLXContext)getRBOContext(dpy).getContext();
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
				throw(EGLError("eglCreateContext()", __LINE__, eglError));
			if(ctx) ctxhashegl.add(ctx, config);
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


GLXPbuffer createPbuffer(Display *dpy, VGLFBConfig config,
	const int *glxAttribs)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			FakePbuffer *pb = new FakePbuffer(dpy, config, glxAttribs);
			GLXDrawable id = pb->getID();
			if(id) pbhashegl.add(id, pb);
			return id;
		}
		CATCH_EGL(X_GLXCreatePbuffer)
		return 0;
	}
	else
	#endif
		return _glXCreatePbuffer(DPY3D, GLXFBC(config), glxAttribs);
}


void destroyContext(Display *dpy, GLXContext ctx)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			if(!ctx) return;
			VGLFBConfig config = ctxhashegl.findConfig(ctx);
			ctxhashegl.remove(ctx);
			getRBOContext(dpy).destroyContext(RBOContext::REFCOUNT_CONTEXT);
			if(!_eglBindAPI(EGL_OPENGL_API))
				THROW("Could not enable OpenGL API");
			if(!_eglDestroyContext(EDPY, (EGLContext)ctx))
				THROW_EGL("eglDestroyContext()");
			if(!config)
				faker::sendGLXError(dpy, X_GLXDestroyContext, GLXBadContext, false);
		}
		CATCH_EGL(X_GLXDestroyContext)
	}
	else
	#endif
		_glXDestroyContext(DPY3D, ctx);
}


void destroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			pbhashegl.remove(pbuf);
		}
		CATCH_EGL(X_GLXDestroyPbuffer)
	}
	else
	#endif
		_glXDestroyPbuffer(DPY3D, pbuf);
}


void drawBuffer(GLenum mode)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		FakePbuffer *pb = getCurrentFakePbuffer(EGL_DRAW);
		if(pb)
		{
			pb->setDrawBuffer(mode, false);
			return;
		}
	}
	#endif
	_glDrawBuffer(mode);
}


void drawBuffers(GLsizei n, const GLenum *bufs)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		FakePbuffer *pb = getCurrentFakePbuffer(EGL_DRAW);
		if(pb)
		{
			pb->setDrawBuffers(n, bufs, false);
			return;
		}
	}
	#endif
	_glDrawBuffers(n, bufs);
}


GLXContext getCurrentContext(void)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
		return getCurrentContextEGL();
	else
	#endif
		return _glXGetCurrentContext();
}


Display *getCurrentDisplay(void)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		FakePbuffer *pb = pbhashegl.find(getCurrentDrawableEGL());
		return pb ? pb->getDisplay() : NULL;
	}
	else
	#endif
		return _glXGetCurrentDisplay();
}


GLXDrawable getCurrentDrawable(void)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
		return getCurrentDrawableEGL();
	else
	#endif
		return _glXGetCurrentDrawable();
}


GLXDrawable getCurrentReadDrawable(void)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
		return getCurrentReadDrawableEGL();
	else
	#endif
		return _glXGetCurrentReadDrawable();
}


int getFBConfigAttrib(Display *dpy, VGLFBConfig config, int attribute,
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
			case GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT:
				*value = !!(config->attr.redSize + config->attr.greenSize +
					config->attr.blueSize == 24);
				return Success;
			default:
				return GLX_BAD_ATTRIBUTE;
		}
	}  // fconfig.egl
	else
	#endif
		return _glXGetFBConfigAttrib(DPY3D, GLXFBC(config), attribute, value);
}


void getFramebufferAttachmentParameteriv(GLenum target, GLenum attachment,
	GLenum pname, GLint *params)
{
	#ifdef EGLBACKEND
	bool isDefault = false;

	if(fconfig.egl)
	{
		if(!params)
		{
			_glGetFramebufferAttachmentParameteriv(target, attachment, pname,
				params);
			return;
		}
		else if((attachment >= GL_FRONT_LEFT && attachment <= GL_BACK_RIGHT)
			|| (attachment >= GL_DEPTH && attachment <= GL_STENCIL))
		{
			FakePbuffer *pb;
			if(((target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
					&& (pb = getCurrentFakePbuffer(EGL_DRAW)) != NULL)
				|| (target == GL_READ_FRAMEBUFFER
					&& (pb = getCurrentFakePbuffer(EGL_READ)) != NULL))
			{
				switch(attachment)
				{
					case GL_FRONT_LEFT:
						attachment = GL_COLOR_ATTACHMENT0;  isDefault = true;  break;
					case GL_FRONT_RIGHT:
						attachment = GL_COLOR_ATTACHMENT2;  isDefault = true;  break;
					case GL_BACK_LEFT:
						attachment = GL_COLOR_ATTACHMENT1;  isDefault = true;  break;
					case GL_BACK_RIGHT:
						attachment = GL_COLOR_ATTACHMENT3;  isDefault = true;  break;
					case GL_DEPTH:
					{
						VGLFBConfig config = pb->getFBConfig();
						if(config->attr.stencilSize && config->attr.depthSize)
							attachment = GL_DEPTH_STENCIL_ATTACHMENT;
						else
							attachment = GL_DEPTH_ATTACHMENT;
					  isDefault = true;  break;
					}
					case GL_STENCIL:
					{
						VGLFBConfig config = pb->getFBConfig();
						if(config->attr.stencilSize && config->attr.depthSize)
							attachment = GL_DEPTH_STENCIL_ATTACHMENT;
						else
							attachment = GL_STENCIL_ATTACHMENT;
					  isDefault = true;  break;
					}
				}
			}
		}
	}
	#endif
	_glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(isDefault && *params == GL_RENDERBUFFER)
			*params = GL_FRAMEBUFFER_DEFAULT;
	}
	#endif
}


void getFramebufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(!params)
		{
			_glGetFramebufferParameteriv(target, pname, params);
			return;
		}
		FakePbuffer *pb;
		if(((target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
				&& (pb = getCurrentFakePbuffer(EGL_DRAW)) != NULL)
			|| (target == GL_READ_FRAMEBUFFER
				&& (pb = getCurrentFakePbuffer(EGL_READ)) != NULL))
		{
			if(pname == GL_DOUBLEBUFFER)
			{
				*params = pb->getFBConfig()->attr.doubleBuffer;
				return;
			}
			else if(pname == GL_STEREO)
			{
				*params = pb->getFBConfig()->attr.stereo;
				return;
			}
		}
	}
	#endif
	_glGetFramebufferParameteriv(target, pname, params);
}


void getIntegerv(GLenum pname, GLint *params)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		EGLContext ctx = _eglGetCurrentContext();
		VGLFBConfig config = ctxhashegl.findConfig(ctx);

		if(!params || !config)
		{
			_glGetIntegerv(pname, params);
			return;
		}
		else if(pname == GL_DOUBLEBUFFER)
		{
			FakePbuffer *pb = getCurrentFakePbuffer(EGL_DRAW);
			if(pb)
			{
				*params = config->attr.doubleBuffer;
				return;
			}
		}
		else if(pname == GL_DRAW_BUFFER)
		{
			FakePbuffer *pb = getCurrentFakePbuffer(EGL_DRAW);
			GLenum drawBuf = ctxhashegl.getDrawBuffer(ctx, 0);
			if(pb)
			{
				*params = drawBuf;
				return;
			}
		}
		else if(pname >= GL_DRAW_BUFFER0 && pname <= GL_DRAW_BUFFER15)
		{
			FakePbuffer *pb = getCurrentFakePbuffer(EGL_DRAW);
			int index = pname - GL_DRAW_BUFFER0;
			GLenum drawBuf = ctxhashegl.getDrawBuffer(ctx, index);
			if(pb)
			{
				*params = drawBuf;
				return;
			}
		}
		else if(pname == GL_DRAW_FRAMEBUFFER_BINDING)
		{
			*params = ctxhashegl.getDrawFBO(ctx);
			return;
		}
		else if(pname == GL_MAX_DRAW_BUFFERS)
		{
			_glGetIntegerv(GL_MAX_DRAW_BUFFERS, params);
			if(*params > 16) *params = 16;
			return;
		}
		else if(pname == GL_READ_BUFFER)
		{
			FakePbuffer *pb = getCurrentFakePbuffer(EGL_READ);
			GLenum readBuf = ctxhashegl.getReadBuffer(ctx);
			if(pb)
			{
				*params = readBuf;
				return;
			}
		}
		else if(pname == GL_READ_FRAMEBUFFER_BINDING)
		{
			*params = ctxhashegl.getReadFBO(ctx);
			return;
		}
		else if(pname == GL_STEREO)
		{
			FakePbuffer *pb = getCurrentFakePbuffer(EGL_DRAW);
			if(pb)
			{
				*params = config->attr.stereo;
				return;
			}
		}
	}
	#endif
	_glGetIntegerv(pname, params);
}


void getNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname,
	GLint *param)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		if(!param)
		{
			_glGetNamedFramebufferParameteriv(framebuffer, pname, param);
			return;
		}
		FakePbuffer *pb;
		if(framebuffer == 0
			&& (pb = pbhashegl.find(getCurrentDrawableEGL())) != NULL)
		{
			if(pname == GL_DOUBLEBUFFER)
			{
				*param = pb->getFBConfig()->attr.doubleBuffer;
				return;
			}
			else if(pname == GL_STEREO)
			{
				*param = pb->getFBConfig()->attr.stereo;
				return;
			}
			else framebuffer = pb->getFBO();
		}
	}
	#endif
	_glGetNamedFramebufferParameteriv(framebuffer, pname, param);
}


Bool isDirect(GLXContext ctx)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
		return True;
	else
	#endif
		return _glXIsDirect(DPY3D, ctx);
}


Bool makeCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
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
			setCurrentContextEGL(ctx);
			setCurrentDrawableEGL(draw);
			setCurrentReadDrawableEGL(read);
			if(!ctx) return True;

			FakePbuffer *drawpb = NULL, *readpb = NULL;
			drawpb = pbhashegl.find(draw);
			readpb = (read == draw ? drawpb : pbhashegl.find(read));
			GLint drawFBO = -1, readFBO = -1;
			_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
			_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);

			if(drawpb)
				drawpb->createBuffer(false, true,
					(ctxhashegl.getDrawFBO(ctx) == 0 || drawFBO == 0),
					(ctxhashegl.getReadFBO(ctx) == 0 || readFBO == 0));
			if(readpb && readpb != drawpb)
				readpb->createBuffer(false, true,
					(ctxhashegl.getDrawFBO(ctx) == 0 || drawFBO == 0),
					(ctxhashegl.getReadFBO(ctx) == 0 || readFBO == 0));

			bool boundNewDrawFBO = false, boundNewReadFBO = false;
			if(drawpb && (ctxhashegl.getDrawFBO(ctx) == 0 || drawFBO == 0))
			{
				_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawpb->getFBO());
				boundNewDrawFBO = true;
			}
			if(readpb && (ctxhashegl.getReadFBO(ctx) == 0 || readFBO == 0))
			{
				_glBindFramebuffer(GL_READ_FRAMEBUFFER, readpb->getFBO());
				boundNewReadFBO = true;
			}

			VGLFBConfig config = ctxhashegl.findConfig(ctx);
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
					oldDrawBufs = ctxhashegl.getDrawBuffers(ctx, nDrawBufs);
					if(oldDrawBufs && nDrawBufs > 0)
					{
						if(nDrawBufs == 1)
							drawpb->setDrawBuffer(oldDrawBufs[0], false);
						else
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
					GLenum oldReadBuf = ctxhashegl.getReadBuffer(ctx);
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


void namedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf, bool ext)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		FakePbuffer *pb;
		if(framebuffer == 0
			&& (pb = pbhashegl.find(getCurrentDrawableEGL())) != NULL)
		{
			pb->setDrawBuffer(buf, true);
			return;
		}
	}
	#endif
	if(ext) _glFramebufferDrawBufferEXT(framebuffer, buf);
	else _glNamedFramebufferDrawBuffer(framebuffer, buf);
}


void namedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n,
	const GLenum *bufs, bool ext)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		FakePbuffer *pb;
		if(framebuffer == 0
			&& (pb = pbhashegl.find(getCurrentDrawableEGL())) != NULL)
		{
			pb->setDrawBuffers(n, bufs, true);
			return;
		}
	}
	#endif
	if(ext) _glFramebufferDrawBuffersEXT(framebuffer, n, bufs);
	else _glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
}


void namedFramebufferReadBuffer(GLuint framebuffer, GLenum mode, bool ext)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		FakePbuffer *pb;
		if(framebuffer == 0
			&& (pb = pbhashegl.find(getCurrentReadDrawableEGL())) != NULL)
		{
			pb->setReadBuffer(mode, true);
			return;
		}
	}
	#endif
	if(ext) _glFramebufferReadBufferEXT(framebuffer, mode);
	else _glNamedFramebufferReadBuffer(framebuffer, mode);
}


int queryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		int retval = Success;
		VGLFBConfig config;

		if(!ctx || !(config = ctxhashegl.findConfig(ctx)))
		{
			faker::sendGLXError(dpy, X_GLXQueryContext, GLXBadContext, false);
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


void queryDrawable(Display *dpy, GLXDrawable draw, int attribute,
	unsigned int *value)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		FakePbuffer *pb = NULL;

		if(!value) return;

		if(!draw || (pb = pbhashegl.find(draw)) == NULL)
		{
			faker::sendGLXError(dpy, X_GLXGetDrawableAttributes, GLXBadDrawable,
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


Bool queryExtension(Display *dpy, int *majorOpcode, int *eventBase,
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
		// because of XCB.  However, we don't throw a fatal error here, because the
		// 3D application is well within its rights to ask whether the GLX
		// extension is present and then not use it.  (It could, for instance, use
		// EGL/X11 instead.)
		if(!retval)
		{
			static bool alreadyWarned = false;
			if(!alreadyWarned)
			{
				if(fconfig.verbose)
					vglout.print("[VGL] WARNING: The EGL back end requires a 2D X server with a GLX extension.\n");
				alreadyWarned = true;
			}
		}
		return retval;
	}
	// When using the GLX back end, all GLX errors will come from the 3D X
	// server.
	else
	#endif
		return _XQueryExtension(DPY3D, "GLX", majorOpcode, eventBase, errorBase);
}


void readBuffer(GLenum mode)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		FakePbuffer *pb = getCurrentFakePbuffer(EGL_READ);
		if(pb)
		{
			pb->setReadBuffer(mode, false);
			return;
		}
	}
	#endif
	_glReadBuffer(mode);
}


void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
	GLenum type, void *data)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		bool fallthrough = true;
		VGLFBConfig config = ctxhashegl.findConfig(_eglGetCurrentContext());
		FakePbuffer *readpb = getCurrentFakePbuffer(EGL_READ);

		if(config && config->attr.samples > 1 && readpb)
		{
			GLuint fbo = 0, rbo = 0;
			_glGenFramebuffers(1, &fbo);
			if(fbo)
			{
				BufferState bs(BS_DRAWFBO | BS_READFBO | BS_DRAWBUFS | BS_READBUF);
				_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

				_glGenRenderbuffers(1, &rbo);
				if(rbo)
				{
					BufferState bsRBO(BS_RBO);
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


void swapBuffers(Display *dpy, GLXDrawable drawable)
{
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		try
		{
			if(pmhash.find(dpy, drawable)) return;

			FakePbuffer *pb = NULL;

			if(drawable && (pb = pbhashegl.find(drawable)) != NULL)
				pb->swap();
			else
				faker::sendGLXError(dpy, X_GLXSwapBuffers, GLXBadDrawable, false);
		}
		CATCH_EGL(X_GLXSwapBuffers)
	}
	else
	#endif
		_glXSwapBuffers(DPY3D, drawable);
}

}  // namespace
