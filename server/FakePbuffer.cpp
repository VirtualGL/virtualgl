// Copyright (C)2019-2022 D. R. Commander
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

#include <X11/Xlibint.h>
#include "FakePbuffer.h"
#include "TempContextEGL.h"
#include "BufferState.h"
#include "ContextHashEGL.h"

using namespace util;
using namespace backend;


CriticalSection FakePbuffer::idMutex;
GLXDrawable FakePbuffer::nextID = 1;


FakePbuffer::FakePbuffer(Display *dpy_, VGLFBConfig config_,
	const int *glxAttribs) : dpy(dpy_), config(config_), id(0), fbo(0),
	rbod(0), width(0), height(0)
{
	for(int i = 0; i < 4; i++) rboc[i] = 0;

	if(!dpy || !VALID_CONFIG(config)) THROW("Invalid argument");

	if(glxAttribs && glxAttribs[0] != None)
	{
		for(int glxi = 0; glxAttribs[glxi] && glxi < MAX_ATTRIBS; glxi += 2)
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
		getRBOContext(dpy).createContext(RBOContext::REFCOUNT_DRAWABLE);
		createBuffer(true);
		CriticalSection::SafeLock l(idMutex);
		id = nextID++;
	}
	catch(std::exception &e)
	{
		destroy(false);
		throw;
	}
}


FakePbuffer::~FakePbuffer(void)
{
	destroy(true);
}


void FakePbuffer::createBuffer(bool useRBOContext, bool ignoreReadDrawBufs,
	bool ignoreDrawFBO, bool ignoreReadFBO)
{
	TempContextEGL *tc = NULL;
	BufferState *bs = NULL;

	CriticalSection::SafeLock l(getRBOContext(dpy).getMutex());

	try
	{
		if(useRBOContext)
			tc = new TempContextEGL(getRBOContext(dpy).getContext());
		else
		{
			int saveMask = BS_RBO;
			if(!ignoreReadDrawBufs) saveMask |= BS_DRAWBUFS | BS_READBUF;
			if(!ignoreDrawFBO) saveMask |= BS_DRAWFBO;
			if(!ignoreReadFBO) saveMask |= BS_READFBO;

			bs = new BufferState(saveMask);
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
				GLenum internalFormat = GL_SRGB8;
				if(config->attr.redSize > 8) internalFormat = GL_RGB10_A2;
				else if(config->attr.alphaSize) internalFormat = GL_SRGB8_ALPHA8;

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
		delete bs;
		delete tc;
		throw;
	}
	delete bs;
	delete tc;
}


void FakePbuffer::destroy(bool errorCheck)
{
	try
	{
		CriticalSection::SafeLock l(getRBOContext(dpy).getMutex());

		{
			TempContextEGL tc(getRBOContext(dpy).getContext());

			_glBindFramebuffer(GL_FRAMEBUFFER, 0);
			_glBindRenderbuffer(GL_RENDERBUFFER, 0);
			for(int i = 0; i < 4; i++)
			{
				if(rboc[i]) { _glDeleteRenderbuffers(1, &rboc[i]);  rboc[i] = 0; }
			}
			if(rbod) { _glDeleteRenderbuffers(1, &rbod);  rbod = 0; }
			if(fbo) { _glDeleteFramebuffers(1, &fbo);  fbo = 0; }
		}

		getRBOContext(dpy).destroyContext(RBOContext::REFCOUNT_DRAWABLE);
	}
	catch(std::exception &e)
	{
		if(errorCheck) throw;
	}
}


void FakePbuffer::swap(void)
{
	bool changed = false;

	if(_eglGetCurrentContext()) _glFlush();

	CriticalSection::SafeLock l(getRBOContext(dpy).getMutex());

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

		if(getCurrentDrawable() == id || getCurrentReadDrawable() == id)
			createBuffer(false, false, drawFBO == (GLint)oldFBO,
				readFBO == (GLint)oldFBO);

		if(getCurrentDrawable() == id && drawFBO == (GLint)oldFBO)
		{
			BufferState bs(BS_DRAWBUFS);
			_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		}
		if(getCurrentReadDrawable() == id && readFBO == (GLint)oldFBO)
		{
			BufferState bs(BS_READBUF);
			_glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		}
	}
}


void FakePbuffer::setDrawBuffer(GLenum drawBuf, bool deferred)
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
	if(deferred)
		_glNamedFramebufferDrawBuffers(fbo, nActualBufs, actualBufs);
	else
		_glDrawBuffers(nActualBufs, actualBufs);
	ctxhashegl.setDrawBuffers(_eglGetCurrentContext(), 1, &drawBuf);
}


void FakePbuffer::setDrawBuffers(GLsizei n, const GLenum *bufs, bool deferred)
{
	if(n < 0)
	{
		// Trigger GL_INVALID_ENUM by passing n < 0 to the real glDrawBuffers()
		// function.
		_glDrawBuffers(n, bufs);
		return;
	}
	GLint maxDrawBufs = 16;
	_glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBufs);
	if(n > min(maxDrawBufs, 16))
	{
		// Trigger GL_INVALID_VALUE by passing n > the real value of
		// GL_MAX_DRAW_BUFFERS to the real glDrawBuffers() function.
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
	if(deferred)
		_glNamedFramebufferDrawBuffers(fbo, nActualBufs, actualBufs);
	else
		_glDrawBuffers(nActualBufs, actualBufs);
	ctxhashegl.setDrawBuffers(_eglGetCurrentContext(), n, bufs);
}


void FakePbuffer::setReadBuffer(GLenum readBuf, bool deferred)
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
	GLenum actualReadBuf = readBuf;
	if(readBuf == GL_FRONT_LEFT || readBuf == GL_FRONT || readBuf == GL_LEFT
		|| readBuf == GL_FRONT_AND_BACK)
		actualReadBuf = GL_COLOR_ATTACHMENT0;
	else if(readBuf == GL_FRONT_RIGHT || readBuf == GL_RIGHT)
		actualReadBuf = GL_COLOR_ATTACHMENT2;
	else if(readBuf == GL_BACK_LEFT || readBuf == GL_BACK)
		actualReadBuf = GL_COLOR_ATTACHMENT1;
	else if(readBuf == GL_BACK_RIGHT)
		actualReadBuf = GL_COLOR_ATTACHMENT3;
	if(deferred)
		_glNamedFramebufferReadBuffer(fbo, actualReadBuf);
	else
		_glReadBuffer(actualReadBuf);
	ctxhashegl.setReadBuffer(_eglGetCurrentContext(), readBuf);
}
