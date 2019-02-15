/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011-2012, 2015, 2018-2019 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

// Interposed OpenGL functions

#include <math.h>
#include "ContextHash.h"
#include "WindowHash.h"
#include "faker.h"

using namespace vglserver;


#define ROUND(f)  ((f) >= 0 ? (long)((f) + 0.5) : (long)((f) - 0.5))


static void doGLReadback(bool spoilLast, bool sync)
{
	VirtualWin *vw;  GLXDrawable drawable;

	drawable = _glXGetCurrentDrawable();
	if(!drawable) return;

	if(winhash.find(drawable, vw))
	{
		if(DrawingToFront() || vw->dirty)
		{
				OPENTRACE(doGLReadback);  PRARGX(vw->getGLXDrawable());
				PRARGI(sync);  PRARGI(spoilLast);  STARTTRACE();

			vw->readback(GL_FRONT, spoilLast, sync);

				STOPTRACE();  CLOSETRACE();
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

		if(fconfig.trace) vglout.print("[VGL] glFinish()\n");

	_glFinish();
	fconfig.flushdelay = 0.;
	doGLReadback(false, fconfig.sync);

	CATCH();
}


void glFlush(void)
{
	static double lastTime = -1.;  double thisTime;

	if(vglfaker::getExcludeCurrent()) { _glFlush();  return; }

	TRY();

		if(fconfig.trace) vglout.print("[VGL] glFlush()\n");

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
}


void glXWaitGL(void)
{
	if(vglfaker::getExcludeCurrent()) { _glXWaitGL();  return; }

	TRY();

		if(fconfig.trace) vglout.print("[VGL] glXWaitGL()\n");

	_glFinish();  // glXWaitGL() on some systems calls glFinish(), so we do this
	              // to avoid 2 readbacks
	fconfig.flushdelay = 0.;
	doGLReadback(false, fconfig.sync);

	CATCH();
}


// If the application is rendering to the front buffer and switches the draw
// buffer before calling glFlush()/glFinish()/glXWaitGL(), we set a lazy
// readback trigger to indicate that the front buffer needs to be read back
// upon the next call to glFlush()/glFinish()/glXWaitGL().

void glDrawBuffer(GLenum mode)
{
	if(vglfaker::getExcludeCurrent()) { _glDrawBuffer(mode);  return; }

	TRY();

		OPENTRACE(glDrawBuffer);  PRARGX(mode);  STARTTRACE();

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

		STOPTRACE();
		if(drawable && vw)
		{
			PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
		}
		CLOSETRACE();

	CATCH();
}


// glPopAttrib() can change the draw buffer state as well :|

void glPopAttrib(void)
{
	if(vglfaker::getExcludeCurrent()) { _glPopAttrib();  return; }

	TRY();

		OPENTRACE(glPopAttrib);  STARTTRACE();

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

		STOPTRACE();
		if(drawable && vw)
		{
			PRARGI(vw->dirty);  PRARGI(vw->rdirty);  PRARGX(vw->getGLXDrawable());
		}
		CLOSETRACE();

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

		OPENTRACE(glViewport);  PRARGI(x);  PRARGI(y);  PRARGI(width);
		PRARGI(height);  STARTTRACE();

	GLXContext ctx = _glXGetCurrentContext();
	GLXDrawable draw = _glXGetCurrentDrawable();
	GLXDrawable read = _glXGetCurrentReadDrawable();
	Display *dpy = _glXGetCurrentDisplay();
	GLXDrawable newRead = 0, newDraw = 0;

	if(dpy && (draw || read) && ctx)
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

		STOPTRACE();
		if(draw != newDraw) { PRARGX(draw);  PRARGX(newDraw); }
		if(read != newRead) { PRARGX(read);  PRARGX(newRead); }
		CLOSETRACE();

	CATCH();
}


}  // extern "C"
