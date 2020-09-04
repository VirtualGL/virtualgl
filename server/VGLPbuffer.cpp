// Copyright (C)2019-2020 D. R. Commander
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

#include "VGLPbuffer.h"
#include "TempContextEGL.h"
#include "EGLContextHash.h"
#include <X11/Xlibint.h>

using namespace vglutil;
using namespace vglfaker;


VGLPbuffer::VGLPbuffer(Display *dpy_, VGLFBConfig config_,
	const int *glxAttribs) : dpy(dpy_), config(config_), eglpb(0), fbo(0),
	rbod(0), width(0), height(0)
{
	for(int i = 0; i < 4; i++) rboc[i] = 0;

	if(!dpy || !config) THROW("Invalid argument");

	if(glxAttribs && glxAttribs[0] != None)
	{
		for(int glxi = 0; glxAttribs[glxi]; glxi += 2)
		{
			switch(glxAttribs[glxi])
			{
				case GLX_PBUFFER_WIDTH:
					width = glxAttribs[glxi + 1];
					break;
				case GLX_PBUFFER_HEIGHT:
					height = glxAttribs[glxi + 1];
					break;
			}
		}
	}

	if(width < 1) width = 1;
	if(height < 1) height = 1;

	try
	{
		// Create a dummy 1x1 EGL Pbuffer surface so we have something to which to
		// bind the FBO.
		int eglAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		eglpb = _eglCreatePbufferSurface(EDPY, EGLFBC(config), eglAttribs);
		if(!eglpb) THROW_EGL("eglCreatePbufferSurface()");

		config->rboCtx->createContext();
		createBuffer(true);
	}
	catch(std::exception &e)
	{
		destroy(false);
		throw;
	}
}


VGLPbuffer::~VGLPbuffer(void)
{
	destroy(true);
}


void VGLPbuffer::createBuffer(bool useRBOContext)
{
	TempContextEGL *tc = NULL;
	GLint oldDrawFBO = -1, oldReadFBO = -1, oldRBO = -1;
	GLint oldDrawBuf = -1, oldReadBuf = -1;

	CriticalSection::SafeLock l(config->rboCtx->getMutex());

	try
	{
		if(useRBOContext)
			tc = new TempContextEGL(eglpb, eglpb, config->rboCtx->getContext());
		else
		{
			_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawFBO);
			_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFBO);
			_glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRBO);
			_glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuf);
			_glGetIntegerv(GL_READ_BUFFER, &oldReadBuf);
		}

		TRY_GL();
		if(fbo) _glDeleteFramebuffers(1, &fbo);
		_glGenFramebuffers(1, &fbo);
		_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		// 0 = front left, 1 = back left, 2 = front right, 3 = back right
		for(int i = 0; i < 2 * (!!config->attr.stereo + 1);
			i += (1 - !!config->attr.doubleBuffer + 1))
		{
			if(!rboc[i])
			{
				GLenum internalFormat = GL_RGB8;
				if(config->attr.redSize > 8) internalFormat = GL_RGB10_A2;
				else if(config->attr.alphaSize) internalFormat = GL_RGBA8;

				_glGenRenderbuffers(1, &rboc[i]);
				_glBindRenderbuffer(GL_RENDERBUFFER, rboc[i]);
				if(config->attr.samples > 1)
					_glRenderbufferStorageMultisample(GL_RENDERBUFFER,
						config->attr.samples, internalFormat, width, height);
				else
					_glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width,
						height);
			}
			else _glBindRenderbuffer(GL_RENDERBUFFER, rboc[i]);
			_glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
				GL_RENDERBUFFER, rboc[i]);
		}
		if(config->attr.stencilSize || config->attr.depthSize)
		{
			if(!rbod)
			{
				GLenum internalFormat = GL_DEPTH_COMPONENT24;
				if(config->attr.stencilSize && config->attr.depthSize)
					internalFormat = GL_DEPTH24_STENCIL8;
				else if(config->attr.stencilSize)
					internalFormat = GL_STENCIL_INDEX8;

				_glGenRenderbuffers(1, &rbod);
				_glBindRenderbuffer(GL_RENDERBUFFER, rbod);
				if(config->attr.samples > 1)
					_glRenderbufferStorageMultisample(GL_RENDERBUFFER,
						config->attr.samples, internalFormat, width, height);
				else
					_glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width,
						height);
			}
			else _glBindRenderbuffer(GL_RENDERBUFFER, rbod);

			GLenum attachment = GL_DEPTH_ATTACHMENT;
			if(config->attr.stencilSize && config->attr.depthSize)
				attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			else if(config->attr.stencilSize)
				attachment = GL_STENCIL_ATTACHMENT;

			_glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
				rbod);
		}
		CATCH_GL("Could not create FBO");
		GLenum status = _glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			vglout.print("[VGL] ERROR: glCheckFramebufferStatus() error 0x%.4x\n",
				status);
			THROW("FBO is incomplete");
		}
	}
	catch(...)
	{
		if(!useRBOContext)
		{
			if(oldDrawFBO >= 0) _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawFBO);
			if(oldReadFBO >= 0) _glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFBO);
			if(oldRBO >= 0) _glBindRenderbuffer(GL_RENDERBUFFER, oldRBO);
			if(oldDrawBuf >= 0) _glDrawBuffer(oldDrawBuf);
			if(oldReadBuf >= 0) _glReadBuffer(oldReadBuf);
		}
		delete tc;
		throw;
	}
	if(!useRBOContext)
	{
		if(oldDrawFBO >= 0) _glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawFBO);
		if(oldReadFBO >= 0) _glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFBO);
		if(oldRBO >= 0) _glBindRenderbuffer(GL_RENDERBUFFER, oldRBO);
		if(oldDrawBuf >= 0) _glDrawBuffer(oldDrawBuf);
		if(oldReadBuf >= 0) _glReadBuffer(oldReadBuf);
	}
	delete tc;
}


void VGLPbuffer::destroy(bool errorCheck)
{
	try
	{
		if(eglpb)
		{
			CriticalSection::SafeLock l(config->rboCtx->getMutex());
			TempContextEGL tc(eglpb, eglpb, config->rboCtx->getContext());

			_glBindFramebuffer(GL_FRAMEBUFFER, 0);
			_glBindRenderbuffer(GL_RENDERBUFFER, 0);
			for(int i = 0; i < 4; i++)
			{
				if(rboc[i]) { _glDeleteRenderbuffers(1, &rboc[i]);  rboc[i] = 0; }
			}
			if(rbod) { _glDeleteRenderbuffers(1, &rbod);  rbod = 0; }
			if(fbo) { _glDeleteFramebuffers(1, &fbo);  fbo = 0; }

			if(!_eglBindAPI(EGL_OPENGL_API))
				THROW("Could not enable OpenGL API");
			if(!_eglDestroySurface(EDPY, eglpb))
				THROW_EGL("eglDestroySurface()");
			eglpb = 0;

			config->rboCtx->destroyContext();
		}
	}
	catch(std::exception &e)
	{
		if(errorCheck) throw;
	}
}


void VGLPbuffer::swap(void)
{
	bool changed = false;

	if(_eglGetCurrentContext()) _glFlush();

	CriticalSection::SafeLock l(config->rboCtx->getMutex());

	// 0 = front left, 1 = back left, 2 = front right, 3 = back right
	if(rboc[0] && rboc[1])
	{
		GLuint tmp = rboc[0];
		rboc[0] = rboc[1];
		rboc[1] = tmp;
		changed = true;
	}
	if(rboc[2] && rboc[3])
	{
		GLuint tmp = rboc[2];
		rboc[2] = rboc[3];
		rboc[3] = tmp;
		changed = true;
	}
	if(changed && _eglGetCurrentContext())
	{
		GLint drawFBO = -1, readFBO = -1;
		_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
		_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
		GLuint oldFBO = fbo;

		if(_eglGetCurrentSurface(EGL_DRAW) == eglpb
			|| _eglGetCurrentSurface(EGL_READ) == eglpb)
			createBuffer(false);

		if(_eglGetCurrentSurface(EGL_DRAW) == eglpb && drawFBO == (GLint)oldFBO)
			_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		if(_eglGetCurrentSurface(EGL_READ) == eglpb && readFBO == (GLint)oldFBO)
			_glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	}
}


void VGLPbuffer::setDrawBuffer(GLenum drawBuf)
{
	if(((drawBuf == GL_FRONT_RIGHT || drawBuf == GL_RIGHT)
			&& !config->attr.stereo)
		|| ((drawBuf == GL_BACK_LEFT || drawBuf == GL_BACK)
			&& !config->attr.doubleBuffer)
		|| (drawBuf == GL_BACK_RIGHT
			&& (!config->attr.stereo || !config->attr.doubleBuffer))
		|| (drawBuf >= GL_COLOR_ATTACHMENT0 && drawBuf <= GL_DEPTH_ATTACHMENT))
	{
		// Trigger GL_INVALID_OPERATION by passing an FBO-incompatible value to
		// the real glDrawBuffer() function.
		_glDrawBuffer(GL_FRONT_LEFT);
		return;
	}

	GLenum actualBufs[4] = { 0, 0, 0, 0 };
	int nActualBufs = 0;

	// 0 = front left, 1 = back left, 2 = front right, 3 = back right
	if(drawBuf == GL_FRONT_LEFT || drawBuf == GL_FRONT || drawBuf == GL_LEFT
		|| drawBuf == GL_FRONT_AND_BACK)
		actualBufs[nActualBufs++] = GL_COLOR_ATTACHMENT0;
	if(drawBuf == GL_FRONT_RIGHT
		|| (drawBuf == GL_FRONT && config->attr.stereo)
		|| (drawBuf == GL_RIGHT && config->attr.stereo)
		|| (drawBuf == GL_FRONT_AND_BACK && config->attr.stereo))
		actualBufs[nActualBufs++] = GL_COLOR_ATTACHMENT2;
	if(drawBuf == GL_BACK_LEFT || drawBuf == GL_BACK
		|| (drawBuf == GL_LEFT && config->attr.doubleBuffer)
		|| (drawBuf == GL_FRONT_AND_BACK && config->attr.doubleBuffer))
		actualBufs[nActualBufs++] = GL_COLOR_ATTACHMENT1;
	if(drawBuf == GL_BACK_RIGHT
		|| (drawBuf == GL_BACK && config->attr.stereo && config->attr.doubleBuffer)
		|| (drawBuf == GL_RIGHT && config->attr.stereo
			&& config->attr.doubleBuffer)
		|| (drawBuf == GL_FRONT_AND_BACK && config->attr.stereo
			&& config->attr.doubleBuffer))
		actualBufs[nActualBufs++] = GL_COLOR_ATTACHMENT3;
	if(nActualBufs == 0)
		actualBufs[nActualBufs++] = drawBuf;
	_glDrawBuffers(nActualBufs, actualBufs);
	ectxhash.setDrawBuffers(_eglGetCurrentContext(), 1, &drawBuf);
}


void VGLPbuffer::setDrawBuffers(GLsizei n, const GLenum *bufs)
{
	if(n < 0)
	{
		// Trigger GL_INVALID_ENUM by passing n < 0 to the real glDrawBuffers()
		// function.
		_glDrawBuffers(n, bufs);
		return;
	}
	if(n > 16)
	{
		// Trigger GL_INVALID_VALUE by passing n > the real value of
		// GL_MAX_DRAW_BUFFERS to the real glDrawBuffers() function.
		GLint maxDrawBufs = 0;
		_glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBufs);
		_glDrawBuffers(maxDrawBufs + 1, bufs);
		return;
	}

	GLenum actualBufs[4] = { 0, 0, 0, 0 };
	int nActualBufs = 0;

	for(GLsizei i = 0; i < n; i++)
	{
		if((bufs[i] >= GL_COLOR_ATTACHMENT0 && bufs[i] <= GL_DEPTH_ATTACHMENT)
			|| (bufs[i] == GL_BACK && n > 1))
		{
			// Trigger GL_INVALID_OPERATION by passing an FBO-incompatible value to
			// the real glDrawBuffer() function.
			_glDrawBuffer(GL_FRONT_LEFT);
			return;
		}
		if((bufs[i] < GL_FRONT_LEFT || bufs[i] > GL_BACK_RIGHT)
			&& bufs[i] != GL_NONE && (bufs[i] != GL_BACK || n != 1))
		{
			// Trigger GL_INVALID_ENUM by passing an invalid value to the real
			// glDrawBuffers() function.
			GLenum buf = 0xFFFF;
			_glDrawBuffers(1, &buf);
			return;
		}
		if(i > 0)
		{
			for(GLsizei j = 0; j < i; j++)
			{
				if(bufs[j] == bufs[i] && bufs[i] != GL_NONE)
				{
					// Trigger GL_INVALID_OPERATION by passing the FBO-incompatible value
					// to the real glDrawBuffer() function.
					_glDrawBuffer(bufs[i]);
					return;
				}
			}
		}
		if(((bufs[i] == GL_FRONT_RIGHT || bufs[i] == GL_RIGHT)
				&& !config->attr.stereo)
			|| ((bufs[i] == GL_BACK_LEFT || bufs[i] == GL_BACK)
				&& !config->attr.doubleBuffer)
			|| (bufs[i] == GL_BACK_RIGHT
				&& (!config->attr.stereo || !config->attr.doubleBuffer)))
		{
			// Trigger GL_INVALID_OPERATION by passing the FBO-incompatible value
			// to the real glDrawBuffer() function.
			_glDrawBuffer(bufs[i]);
			return;
		}
		// 0 = front left, 1 = back left, 2 = front right, 3 = back right
		if(bufs[i] == GL_FRONT_LEFT || bufs[i] == GL_FRONT || bufs[i] == GL_LEFT
			|| bufs[i] == GL_FRONT_AND_BACK)
			actualBufs[nActualBufs++] = GL_COLOR_ATTACHMENT0;
		if(bufs[i] == GL_FRONT_RIGHT
			|| (bufs[i] == GL_FRONT && config->attr.stereo)
			|| (bufs[i] == GL_RIGHT && config->attr.stereo)
			|| (bufs[i] == GL_FRONT_AND_BACK && config->attr.stereo))
			actualBufs[nActualBufs++] = GL_COLOR_ATTACHMENT2;
		if(bufs[i] == GL_BACK_LEFT || bufs[i] == GL_BACK
			|| (bufs[i] == GL_LEFT && config->attr.doubleBuffer)
			|| (bufs[i] == GL_FRONT_AND_BACK && config->attr.doubleBuffer))
			actualBufs[nActualBufs++] = GL_COLOR_ATTACHMENT1;
		if(bufs[i] == GL_BACK_RIGHT
			|| (bufs[i] == GL_BACK && config->attr.stereo
				&& config->attr.doubleBuffer)
			|| (bufs[i] == GL_RIGHT && config->attr.stereo
				&& config->attr.doubleBuffer)
			|| (bufs[i] == GL_FRONT_AND_BACK && config->attr.stereo
				&& config->attr.doubleBuffer))
			actualBufs[nActualBufs++] = GL_COLOR_ATTACHMENT3;
		if(bufs[i] == GL_NONE)
			actualBufs[nActualBufs++] = bufs[i];
	}
	_glDrawBuffers(nActualBufs, actualBufs);
	ectxhash.setDrawBuffers(_eglGetCurrentContext(), n, bufs);
}


void VGLPbuffer::setReadBuffer(GLenum readBuf)
{
	if(((readBuf == GL_FRONT_RIGHT || readBuf == GL_RIGHT)
			&& !config->attr.stereo)
		|| ((readBuf == GL_BACK_LEFT || readBuf == GL_BACK)
			&& !config->attr.doubleBuffer)
		|| (readBuf == GL_BACK_RIGHT
			&& (!config->attr.stereo || !config->attr.doubleBuffer))
		|| (readBuf >= GL_COLOR_ATTACHMENT0 && readBuf <= GL_DEPTH_ATTACHMENT))
	{
		// Trigger GL_INVALID_OPERATION by passing an FBO-incompatible value to
		// the real glReadBuffer() function.
		_glReadBuffer(GL_FRONT_LEFT);
		return;
	}

	// 0 = front left, 1 = back left, 2 = front right, 3 = back right
	if(readBuf == GL_FRONT_LEFT || readBuf == GL_FRONT || readBuf == GL_LEFT)
		_glReadBuffer(GL_COLOR_ATTACHMENT0);
	else if(readBuf == GL_FRONT_RIGHT || readBuf == GL_RIGHT)
		_glReadBuffer(GL_COLOR_ATTACHMENT2);
	else if(readBuf == GL_BACK_LEFT || readBuf == GL_BACK)
		_glReadBuffer(GL_COLOR_ATTACHMENT1);
	else if(readBuf == GL_BACK_RIGHT)
		_glReadBuffer(GL_COLOR_ATTACHMENT3);
	else
		_glReadBuffer(readBuf);
	ectxhash.setReadBuffer(_eglGetCurrentContext(), readBuf);
}
