// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011-2012, 2015, 2018-2022 D. R. Commander
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

// Interposed OpenGL functions

#include <math.h>
#include "ContextHash.h"
#include "WindowHash.h"
#ifdef EGLBACKEND
#include "EGLXWindowHash.h"
#endif
#include "faker.h"

static void doGLReadback(bool spoilLast, bool sync)
{
	GLXDrawable drawable = backend::getCurrentDrawable();
	if(!drawable) return;

	faker::VirtualWin *vw;
	if((vw = winhash.find(NULL, drawable)) != NULL)
	{
		if(DrawingToFront() || vw->dirty)
		{
			/////////////////////////////////////////////////////////////////////////
			OPENTRACE(doGLReadback);  PRARGX(vw->getGLXDrawable());  PRARGI(sync);
			PRARGI(spoilLast);  STARTTRACE();
			/////////////////////////////////////////////////////////////////////////

			vw->readback(GL_FRONT, spoilLast, sync);

			/////////////////////////////////////////////////////////////////////////
			STOPTRACE();  CLOSETRACE();
			/////////////////////////////////////////////////////////////////////////
		}
	}
}


extern "C" {

// VirtualGL reads back and transports the contents of the front buffer if
// something has been rendered to it since the last readback and one of the
// following functions is called to signal the end of a frame.

void glFinish(void)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glFinish();  return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	if(fconfig.trace) vglout.print("[VGL] glFinish()\n");
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	_glFinish();
	fconfig.flushdelay = 0.;
	doGLReadback(false, fconfig.sync);

	CATCH();
	ENABLE_FAKER();
}


void glFlush(void)
{
	static double lastTime = -1.;  double thisTime;

	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glFlush();  return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	if(fconfig.trace) vglout.print("[VGL] glFlush()\n");
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	_glFlush();
	if(lastTime < 0.) lastTime = GetTime();
	else
	{
		thisTime = GetTime() - lastTime;
		if(thisTime - lastTime < 0.01) fconfig.flushdelay = 0.01;
		else fconfig.flushdelay = 0.;
	}

	// See the notes regarding VGL_SPOILLAST and VGL_GLFLUSHTRIGGER in the
	// VirtualGL User's Guide.
	if(fconfig.glflushtrigger) doGLReadback(fconfig.spoillast, fconfig.sync);

	CATCH();
	ENABLE_FAKER();
}


void glXWaitGL(void)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glXWaitGL();  return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	if(fconfig.trace) vglout.print("[VGL] glXWaitGL()\n");
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	_glFinish();  // glXWaitGL() on some systems calls glFinish(), so we do this
	              // to avoid 2 readbacks
	fconfig.flushdelay = 0.;
	doGLReadback(false, fconfig.sync);

	CATCH();
	ENABLE_FAKER();
}


void glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glBindFramebuffer(target, framebuffer);
		return;
	}

	TRY();

	backend::bindFramebuffer(target, framebuffer);

	CATCH();
}

void glBindFramebufferEXT(GLenum target, GLuint framebuffer)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glBindFramebufferEXT(target, framebuffer);
		return;
	}

	TRY();

	backend::bindFramebuffer(target, framebuffer, true);

	CATCH();
}


void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glDeleteFramebuffers(n, framebuffers);
		return;
	}

	TRY();

	backend::deleteFramebuffers(n, framebuffers);

	CATCH();
}

void glDeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers)
{
	glDeleteFramebuffers(n, framebuffers);
}


// If the application is rendering to the front buffer and switches the draw
// buffer before calling glFlush()/glFinish()/glXWaitGL(), we set a lazy
// readback trigger to indicate that the front buffer needs to be read back
// upon the next call to glFlush()/glFinish()/glXWaitGL().

void glDrawBuffer(GLenum mode)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glDrawBuffer(mode);  return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glDrawBuffer);  PRARGX(mode);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = backend::getCurrentDrawable();

	if(drawable && (vw = winhash.find(NULL, drawable)) != NULL)
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		backend::drawBuffer(mode);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else backend::drawBuffer(mode);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(drawable && vw)
	{
		PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
	}
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


void glDrawBuffers(GLsizei n, const GLenum *bufs)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glDrawBuffers(n, bufs);  return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glDrawBuffers);  PRARGI(n);
	if(n && bufs)
	{
		for(GLsizei i = 0; i < n; i++) PRARGX(bufs[i]);
	}
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = backend::getCurrentDrawable();

	if(drawable && (vw = winhash.find(NULL, drawable)) != NULL)
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		backend::drawBuffers(n, bufs);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else backend::drawBuffers(n, bufs);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(drawable && vw)
	{
		PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
	}
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}

void glDrawBuffersARB(GLsizei n, const GLenum *bufs)
{
	glDrawBuffers(n, bufs);
}

void glDrawBuffersATI(GLsizei n, const GLenum *bufs)
{
	glDrawBuffers(n, bufs);
}


void glFramebufferDrawBufferEXT(GLuint framebuffer, GLenum mode)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glFramebufferDrawBufferEXT(framebuffer, mode);
		return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glFramebufferDrawBufferEXT);  PRARGI(framebuffer);  PRARGX(mode);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = 0;

	if(framebuffer == 0 && (drawable = backend::getCurrentDrawable()) != 0
		&& (vw = winhash.find(NULL, drawable)) != NULL)
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		backend::namedFramebufferDrawBuffer(framebuffer, mode, true);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else backend::namedFramebufferDrawBuffer(framebuffer, mode, true);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(drawable && vw)
	{
		PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
	}
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


void glFramebufferDrawBuffersEXT(GLuint framebuffer, GLsizei n,
	const GLenum *bufs)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glFramebufferDrawBuffersEXT(framebuffer, n, bufs);
		return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glFramebufferDrawBuffersEXT);  PRARGI(framebuffer);  PRARGI(n);
	if(n && bufs)
	{
		for(GLsizei i = 0; i < n; i++) PRARGX(bufs[i]);
	}
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = 0;

	if(framebuffer == 0 && (drawable = backend::getCurrentDrawable()) != 0
		&& (vw = winhash.find(NULL, drawable)) != NULL)
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		backend::namedFramebufferDrawBuffers(framebuffer, n, bufs, true);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else backend::namedFramebufferDrawBuffers(framebuffer, n, bufs, true);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(drawable && vw)
	{
		PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
	}
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


void glFramebufferReadBufferEXT(GLuint framebuffer, GLenum mode)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glFramebufferReadBufferEXT(framebuffer, mode);
		return;
	}

	TRY();

	backend::namedFramebufferReadBuffer(framebuffer, mode, true);

	CATCH();
}


void glGetBooleanv(GLenum pname, GLboolean *data)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent() || !data
		|| !fconfig.egl)
	{
		_glGetBooleanv(pname, data);  return;
	}

	TRY();

	switch(pname)
	{
		case GL_DOUBLEBUFFER:
		case GL_DRAW_BUFFER:
		case GL_DRAW_BUFFER0:
		case GL_DRAW_FRAMEBUFFER_BINDING:
		case GL_MAX_DRAW_BUFFERS:
		case GL_READ_BUFFER:
		case GL_READ_FRAMEBUFFER_BINDING:
		case GL_STEREO:
		{
			GLint val = -1;
			backend::getIntegerv(pname, &val);
			*data = (val == 0 ? GL_FALSE : GL_TRUE);
			break;
		}
		default:
			_glGetBooleanv(pname, data);
	}

	CATCH();
}


void glGetDoublev(GLenum pname, GLdouble *data)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent() || !data
		|| !fconfig.egl)
	{
		_glGetDoublev(pname, data);  return;
	}

	TRY();

	switch(pname)
	{
		case GL_DOUBLEBUFFER:
		case GL_DRAW_BUFFER:
		case GL_DRAW_BUFFER0:
		case GL_DRAW_FRAMEBUFFER_BINDING:
		case GL_MAX_DRAW_BUFFERS:
		case GL_READ_BUFFER:
		case GL_READ_FRAMEBUFFER_BINDING:
		case GL_STEREO:
		{
			GLint val = -1;
			backend::getIntegerv(pname, &val);
			*data = (GLdouble)val;
			break;
		}
		default:
			_glGetDoublev(pname, data);
	}

	CATCH();
}


void glGetFloatv(GLenum pname, GLfloat *data)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent() || !data
		|| !fconfig.egl)
	{
		_glGetFloatv(pname, data);  return;
	}

	TRY();

	switch(pname)
	{
		case GL_DOUBLEBUFFER:
		case GL_DRAW_BUFFER:
		case GL_DRAW_BUFFER0:
		case GL_DRAW_FRAMEBUFFER_BINDING:
		case GL_MAX_DRAW_BUFFERS:
		case GL_READ_BUFFER:
		case GL_READ_FRAMEBUFFER_BINDING:
		case GL_STEREO:
		{
			GLint val = -1;
			backend::getIntegerv(pname, &val);
			*data = (GLfloat)val;
			break;
		}
		default:
			_glGetFloatv(pname, data);
	}

	CATCH();
}


void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment,
	GLenum pname, GLint *params)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
		return;
	}

	TRY();

	backend::getFramebufferAttachmentParameteriv(target, attachment, pname, params);

	CATCH();
}


void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glGetFramebufferParameteriv(target, pname, params);  return;
	}

	TRY();

	backend::getFramebufferParameteriv(target, pname, params);

	CATCH();
}


void glGetIntegerv(GLenum pname, GLint *params)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glGetIntegerv(pname, params);  return;
	}

	TRY();

	backend::getIntegerv(pname, params);

	CATCH();
}


void glGetInteger64v(GLenum pname, GLint64 *data)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent() || !data
		|| !fconfig.egl)
	{
		_glGetInteger64v(pname, data);  return;
	}

	TRY();

	switch(pname)
	{
		case GL_DOUBLEBUFFER:
		case GL_DRAW_BUFFER:
		case GL_DRAW_BUFFER0:
		case GL_DRAW_FRAMEBUFFER_BINDING:
		case GL_MAX_DRAW_BUFFERS:
		case GL_READ_BUFFER:
		case GL_READ_FRAMEBUFFER_BINDING:
		case GL_STEREO:
		{
			GLint val = -1;
			backend::getIntegerv(pname, &val);
			*data = (GLint64)val;
			break;
		}
		default:
			_glGetInteger64v(pname, data);
	}

	CATCH();
}


void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname,
	GLint *param)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glGetNamedFramebufferParameteriv(framebuffer, pname, param);  return;
	}

	TRY()

	backend::getNamedFramebufferParameteriv(framebuffer, pname, param);

	CATCH();
}


const GLubyte *glGetString(GLenum name)
{
	char *string = NULL;

	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
		return _glGetString(name);

	TRY();

	string = (char *)_glGetString(name);
	if(name == GL_EXTENSIONS && string
		&& strstr(string, "GL_EXT_x11_sync_object") != NULL)
	{
		if(!faker::glExtensions)
		{
			faker::GlobalCriticalSection::SafeLock l(globalMutex);
			if(!faker::glExtensions)
			{
				faker::glExtensions = strdup(string);
				if(!faker::glExtensions) THROW("strdup() failed");
				char *ptr =
					strstr((char *)faker::glExtensions, "GL_EXT_x11_sync_object");
				if(ptr)
				{
					if(ptr[22] == ' ') memmove(ptr, &ptr[23], strlen(&ptr[23]) + 1);
					else *ptr = 0;
				}
			}
		}
		string = faker::glExtensions;
	}

	CATCH();

	return (GLubyte *)string;
}


const GLubyte *glGetStringi(GLenum name, GLuint index)
{
	const GLubyte *string = NULL;

	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
		return _glGetStringi(name, index);

	TRY();

	string = _glGetStringi(name, index);
	if(name == GL_EXTENSIONS && string
		&& !strcmp((char *)string, "GL_EXT_x11_sync_object"))
	{
		// This is a hack to avoid interposing the various flavors of
		// glGetInteger*() and modifying the value returned for GL_NUM_EXTENSIONS.
		string = (const GLubyte *)"";
	}

	CATCH();

	return string;
}


void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glNamedFramebufferDrawBuffer(framebuffer, buf);
		return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glNamedFramebufferDrawBuffer);  PRARGI(framebuffer);  PRARGX(buf);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = 0;

	if(framebuffer == 0 && (drawable = backend::getCurrentDrawable()) != 0
		&& (vw = winhash.find(NULL, drawable)) != NULL)
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		backend::namedFramebufferDrawBuffer(framebuffer, buf);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else backend::namedFramebufferDrawBuffer(framebuffer, buf);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(drawable && vw)
	{
		PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
	}
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n,
	const GLenum *bufs)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
		return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glNamedFramebufferDrawBuffers);  PRARGI(framebuffer);  PRARGI(n);
	if(n && bufs)
	{
		for(GLsizei i = 0; i < n; i++) PRARGX(bufs[i]);
	}
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = 0;

	if(framebuffer == 0 && (drawable = backend::getCurrentDrawable()) != 0
		&& (vw = winhash.find(NULL, drawable)) != NULL)
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		backend::namedFramebufferDrawBuffers(framebuffer, n, bufs);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else backend::namedFramebufferDrawBuffers(framebuffer, n, bufs);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(drawable && vw)
	{
		PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
	}
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glNamedFramebufferReadBuffer(framebuffer, mode);
		return;
	}

	TRY();

	backend::namedFramebufferReadBuffer(framebuffer, mode);

	CATCH();
}


// glPopAttrib() can change the draw buffer state as well :|

void glPopAttrib(void)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glPopAttrib();  return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glPopAttrib);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = backend::getCurrentDrawable();

	if(drawable && (vw = winhash.find(NULL, drawable)) != NULL)
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		_glPopAttrib();
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else _glPopAttrib();

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(drawable && vw)
	{
		PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
	}
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


void glReadBuffer(GLenum mode)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glReadBuffer(mode);  return;
	}

	TRY();

	backend::readBuffer(mode);

	CATCH();
}


void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
	GLenum format, GLenum type, GLvoid *pixels)
{
	if(faker::getOGLExcludeCurrent() || faker::getEGLXContextCurrent())
	{
		_glReadPixels(x, y, width, height, format, type, pixels);
		return;
	}

	TRY();

	backend::readPixels(x, y, width, height, format, type, pixels);

	CATCH();
}


// Sometimes XNextEvent() is called from a thread other than the
// rendering thread, so we wait until glViewport() is called and
// take that opportunity to resize the off-screen drawable.

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(faker::getOGLExcludeCurrent())
	{
		_glViewport(x, y, width, height);  return;
	}

	TRY();

	#ifdef EGLBACKEND
	if(faker::getEGLXContextCurrent())
	{
		///////////////////////////////////////////////////////////////////////////
		OPENTRACE(glViewport);  PRARGI(x);  PRARGI(y);  PRARGI(width);
		PRARGI(height);  STARTTRACE();
		///////////////////////////////////////////////////////////////////////////

		EGLContext ctx = _eglGetCurrentContext();
		EGLSurface draw = _eglGetCurrentSurface(EGL_DRAW);
		EGLSurface read = _eglGetCurrentSurface(EGL_READ);
		faker::EGLXDisplay *eglxdpy = faker::getCurrentEGLXDisplay();
		EGLSurface newRead = 0, newDraw = 0;

		if(eglxdpy && (draw || read) && ctx)
		{
			newRead = read, newDraw = draw;
			faker::EGLXVirtualWin *drawEGLXVW =
				eglxwinhash.findInternal(eglxdpy, draw);
			faker::EGLXVirtualWin *readEGLXVW =
				eglxwinhash.findInternal(eglxdpy, read);
			if(drawEGLXVW) drawEGLXVW->checkResize();
			if(readEGLXVW && readEGLXVW != drawEGLXVW) readEGLXVW->checkResize();
			if(drawEGLXVW) newDraw = (EGLSurface)drawEGLXVW->updateGLXDrawable();
			if(readEGLXVW) newRead = (EGLSurface)readEGLXVW->updateGLXDrawable();
			if(newRead != read || newDraw != draw)
			{
				_eglMakeCurrent(eglxdpy->edpy, 0, 0, 0);
				if(!_eglMakeCurrent(eglxdpy->edpy, newDraw, newRead, ctx))
					THROW_EGL("eglMakeCurrent()");
				if(drawEGLXVW) { drawEGLXVW->clear();  drawEGLXVW->cleanup(); }
				if(readEGLXVW) readEGLXVW->cleanup();
			}
		}
		_glViewport(x, y, width, height);

		///////////////////////////////////////////////////////////////////////////
		STOPTRACE();
		if(draw != newDraw) { PRARGX(draw);  PRARGX(newDraw); }
		if(read != newRead) { PRARGX(read);  PRARGX(newRead); }
		CLOSETRACE();
		///////////////////////////////////////////////////////////////////////////
	}
	else
	#endif
	{
		///////////////////////////////////////////////////////////////////////////
		OPENTRACE(glViewport);  PRARGI(x);  PRARGI(y);  PRARGI(width);
		PRARGI(height);  STARTTRACE();
		///////////////////////////////////////////////////////////////////////////

		GLXContext ctx = backend::getCurrentContext();
		GLXDrawable draw = backend::getCurrentDrawable();
		GLXDrawable read = backend::getCurrentReadDrawable();
		Display *dpy = backend::getCurrentDisplay();
		GLXDrawable newRead = 0, newDraw = 0;

		if(dpy && (draw || read) && ctx && ctxhash.findConfig(ctx))
		{
			newRead = read, newDraw = draw;
			faker::VirtualWin *drawVW = winhash.find(NULL, draw);
			faker::VirtualWin *readVW = winhash.find(NULL, read);
			if(drawVW) drawVW->checkResize();
			if(readVW && readVW != drawVW) readVW->checkResize();
			if(drawVW) newDraw = drawVW->updateGLXDrawable();
			if(readVW) newRead = readVW->updateGLXDrawable();
			if(newRead != read || newDraw != draw)
			{
				backend::makeCurrent(dpy, newDraw, newRead, ctx);
				if(drawVW) { drawVW->clear();  drawVW->cleanup(); }
				if(readVW) readVW->cleanup();
			}
		}
		_glViewport(x, y, width, height);

		///////////////////////////////////////////////////////////////////////////
		STOPTRACE();
		if(draw != newDraw) { PRARGX(draw);  PRARGX(newDraw); }
		if(read != newRead) { PRARGX(read);  PRARGX(newRead); }
		CLOSETRACE();
		///////////////////////////////////////////////////////////////////////////
	}

	CATCH();
}


}  // extern "C"
