// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011-2012, 2015, 2018-2021 D. R. Commander
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
#include "faker.h"

using namespace vglfaker;
using namespace vglserver;


static void doGLReadback(bool spoilLast, bool sync)
{
	VirtualWin *vw;  GLXDrawable drawable;

	drawable = _glXGetCurrentDrawable();
	if(!drawable) return;

	if(winhash.find(drawable, vw))
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
	if(vglfaker::getExcludeCurrent()) { _glFinish();  return; }

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

	if(vglfaker::getExcludeCurrent()) { _glFlush();  return; }

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
	if(vglfaker::getExcludeCurrent()) { _glXWaitGL();  return; }

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


// If the application is rendering to the front buffer and switches the draw
// buffer before calling glFlush()/glFinish()/glXWaitGL(), we set a lazy
// readback trigger to indicate that the front buffer needs to be read back
// upon the next call to glFlush()/glFinish()/glXWaitGL().

void glDrawBuffer(GLenum mode)
{
	if(vglfaker::getExcludeCurrent()) { _glDrawBuffer(mode);  return; }

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glDrawBuffer);  PRARGX(mode);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = _glXGetCurrentDrawable();

	if(drawable && winhash.find(drawable, vw))
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		_glDrawBuffer(mode);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else _glDrawBuffer(mode);

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
	if(vglfaker::getExcludeCurrent()) { _glDrawBuffers(n, bufs);  return; }

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glDrawBuffers);  PRARGI(n);
	if(n && bufs)
	{
		for(GLsizei i = 0; i < n; i++) PRARGX(bufs[i]);
	}
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = _glXGetCurrentDrawable();

	if(drawable && winhash.find(drawable, vw))
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		_glDrawBuffers(n, bufs);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else _glDrawBuffers(n, bufs);

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
	if(vglfaker::getExcludeCurrent())
	{
		_glFramebufferDrawBufferEXT(framebuffer, mode);
		return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glFramebufferDrawBufferEXT);  PRARGI(framebuffer);  PRARGX(mode);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = 0;

	if(framebuffer == 0 && (drawable = _glXGetCurrentDrawable()) != 0
		&& winhash.find(drawable, vw))
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		_glFramebufferDrawBufferEXT(framebuffer, mode);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else _glFramebufferDrawBufferEXT(framebuffer, mode);

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
	if(vglfaker::getExcludeCurrent())
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

	VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = 0;

	if(framebuffer == 0 && (drawable = _glXGetCurrentDrawable()) != 0
		&& winhash.find(drawable, vw))
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		_glFramebufferDrawBuffersEXT(framebuffer, n, bufs);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else _glFramebufferDrawBuffersEXT(framebuffer, n, bufs);

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


const GLubyte *glGetString(GLenum name)
{
	char *string = NULL;

	if(vglfaker::getExcludeCurrent()) { return _glGetString(name); }

	TRY();

	string = (char *)_glGetString(name);
	if(name == GL_EXTENSIONS && string
		&& strstr(string, "GL_EXT_x11_sync_object") != NULL)
	{
		if(!glExtensions)
		{
			GlobalCriticalSection::SafeLock l(globalMutex);
			if(!glExtensions)
			{
				glExtensions = strdup(string);
				if(!glExtensions) THROW("strdup() failed");
				char *ptr = strstr((char *)glExtensions, "GL_EXT_x11_sync_object");
				if(ptr)
				{
					if(ptr[22] == ' ') memmove(ptr, &ptr[23], strlen(&ptr[23]) + 1);
					else *ptr = 0;
				}
			}
		}
		string = glExtensions;
	}

	CATCH();

	return (GLubyte *)string;
}


const GLubyte *glGetStringi(GLenum name, GLuint index)
{
	const GLubyte *string = NULL;

	if(vglfaker::getExcludeCurrent()) { return _glGetStringi(name, index); }

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
	if(vglfaker::getExcludeCurrent())
	{
		_glNamedFramebufferDrawBuffer(framebuffer, buf);
		return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glNamedFramebufferDrawBuffer);  PRARGI(framebuffer);  PRARGX(buf);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = 0;

	if(framebuffer == 0 && (drawable = _glXGetCurrentDrawable()) != 0
		&& winhash.find(drawable, vw))
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		_glNamedFramebufferDrawBuffer(framebuffer, buf);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else _glNamedFramebufferDrawBuffer(framebuffer, buf);

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
	if(vglfaker::getExcludeCurrent())
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

	VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = 0;

	if(framebuffer == 0 && (drawable = _glXGetCurrentDrawable()) != 0
		&& winhash.find(drawable, vw))
	{
		before = DrawingToFront();
		rbefore = DrawingToRight();
		_glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
		after = DrawingToFront();
		rafter = DrawingToRight();
		if(before && !after) vw->dirty = true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty = true;
	}
	else _glNamedFramebufferDrawBuffers(framebuffer, n, bufs);

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


// glPopAttrib() can change the draw buffer state as well :|

void glPopAttrib(void)
{
	if(vglfaker::getExcludeCurrent()) { _glPopAttrib();  return; }

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glPopAttrib);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	VirtualWin *vw = NULL;
	int before = -1, after = -1, rbefore = -1, rafter = -1;
	GLXDrawable drawable = _glXGetCurrentDrawable();

	if(drawable && winhash.find(drawable, vw))
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


// Sometimes XNextEvent() is called from a thread other than the
// rendering thread, so we wait until glViewport() is called and
// take that opportunity to resize the off-screen drawable.

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(vglfaker::getExcludeCurrent())
	{
		_glViewport(x, y, width, height);  return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glViewport);  PRARGI(x);  PRARGI(y);  PRARGI(width);
	PRARGI(height);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	GLXContext ctx = _glXGetCurrentContext();
	GLXDrawable draw = _glXGetCurrentDrawable();
	GLXDrawable read = _glXGetCurrentReadDrawable();
	Display *dpy = _glXGetCurrentDisplay();
	GLXDrawable newRead = 0, newDraw = 0;

	if(dpy && (draw || read) && ctx && ctxhash.findConfig(ctx))
	{
		newRead = read, newDraw = draw;
		VirtualWin *drawVW = NULL, *readVW = NULL;
		winhash.find(draw, drawVW);
		winhash.find(read, readVW);
		if(drawVW) drawVW->checkResize();
		if(readVW && readVW != drawVW) readVW->checkResize();
		if(drawVW) newDraw = drawVW->updateGLXDrawable();
		if(readVW) newRead = readVW->updateGLXDrawable();
		if(newRead != read || newDraw != draw)
		{
			_glXMakeContextCurrent(dpy, newDraw, newRead, ctx);
			if(drawVW) { drawVW->clear();  drawVW->cleanup(); }
			if(readVW) readVW->cleanup();
		}
	}
	_glViewport(x, y, width, height);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(draw != newDraw) { PRARGX(draw);  PRARGX(newDraw); }
	if(read != newRead) { PRARGX(read);  PRARGX(newRead); }
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


}  // extern "C"
