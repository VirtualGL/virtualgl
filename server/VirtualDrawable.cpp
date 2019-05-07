// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009-2015, 2017-2019 D. R. Commander
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

#include "VirtualDrawable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glxvisual.h"
#include "glext-vgl.h"
#include "TempContext.h"
#include "vglutil.h"
#include "faker.h"
#include "glpf.h"

using namespace vglcommon;
using namespace vglutil;
using namespace vglserver;


#define CHECKGL(m)  if(glError()) THROW("Could not " m);

// Generic OpenGL error checker (0 = no errors)
static int glError(void)
{
	int i, ret = 0;

	i = _glGetError();
	while(i != GL_NO_ERROR)
	{
		ret = 1;
		vglout.print("[VGL] ERROR: OpenGL error 0x%.4x\n", i);
		i = _glGetError();
	}
	return ret;
}


static Window create_window(Display *dpy, XVisualInfo *vis, int width,
	int height)
{
	Window win;
	XSetWindowAttributes wattrs;
	Colormap cmap;

	cmap = XCreateColormap(dpy, RootWindow(dpy, vis->screen), vis->visual,
		AllocNone);
	wattrs.background_pixel = 0;
	wattrs.border_pixel = 0;
	wattrs.colormap = cmap;
	wattrs.event_mask = 0;
	win = _XCreateWindow(dpy, RootWindow(dpy, vis->screen), 0, 0, width, height,
		1, vis->depth, InputOutput, vis->visual,
		CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &wattrs);
	return win;
}


// Pbuffer constructor

VirtualDrawable::OGLDrawable::OGLDrawable(int width_, int height_,
	GLXFBConfig config_) : cleared(false), stereo(false), glxDraw(0),
	width(width_), height(height_), depth(0), config(config_), glFormat(0),
	pm(0), win(0), isPixmap(false)
{
	if(!config_ || width_ < 1 || height_ < 1) THROW("Invalid argument");

	int pbattribs[] = { GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0,
		GLX_PRESERVED_CONTENTS, True, None };

	pbattribs[1] = width;  pbattribs[3] = height;
	glxDraw = _glXCreatePbuffer(DPY3D, config, pbattribs);
	if(!glxDraw) THROW("Could not create Pbuffer");

	setVisAttribs();
}


// Pixmap constructor

VirtualDrawable::OGLDrawable::OGLDrawable(int width_, int height_, int depth_,
	GLXFBConfig config_, const int *attribs) : cleared(false), stereo(false),
	glxDraw(0), width(width_), height(height_), depth(depth_), config(config_),
	glFormat(0), pm(0), win(0), isPixmap(true)
{
	if(!config_ || width_ < 1 || height_ < 1 || depth_ < 0)
		THROW("Invalid argument");

	XVisualInfo *vis = NULL;
	if((vis = _glXGetVisualFromFBConfig(DPY3D, config)) == NULL)
		goto bailout;
	win = create_window(DPY3D, vis, 1, 1);
	if(!win) goto bailout;
	pm = XCreatePixmap(DPY3D, win, width, height,
		depth > 0 ? depth : vis->depth);
	if(!pm) goto bailout;
	XFree(vis);
	glxDraw = _glXCreatePixmap(DPY3D, config, pm, attribs);
	if(!glxDraw) goto bailout;

	setVisAttribs();
	return;

	bailout:
	if(vis) XFree(vis);
	THROW("Could not create GLX pixmap");
}


void VirtualDrawable::OGLDrawable::setVisAttribs(void)
{
	if(glxvisual::visAttrib3D(config, GLX_STEREO))
		stereo = true;
	rgbSize = glxvisual::visAttrib3D(config, GLX_RED_SIZE) +
		glxvisual::visAttrib3D(config, GLX_GREEN_SIZE) +
		glxvisual::visAttrib3D(config, GLX_BLUE_SIZE);
	int pixelsize = rgbSize + glxvisual::visAttrib3D(config, GLX_ALPHA_SIZE);

	if(pixelsize == 32)
	{
		if(LittleEndian()) glFormat = GL_BGRA;
		else glFormat = GL_RGBA;
	}
	else
	{
		if(LittleEndian()) glFormat = GL_BGR;
		else glFormat = GL_RGB;
	}
}


VirtualDrawable::OGLDrawable::~OGLDrawable(void)
{
	if(isPixmap)
	{
		if(glxDraw)
		{
			_glXDestroyPixmap(DPY3D, glxDraw);
			glxDraw = 0;
		}
		if(pm) { XFreePixmap(DPY3D, pm);  pm = 0; }
		if(win) { _XDestroyWindow(DPY3D, win);  win = 0; }
	}
	else
	{
		_glXDestroyPbuffer(DPY3D, glxDraw);
		glxDraw = 0;
	}
}


XVisualInfo *VirtualDrawable::OGLDrawable::getVisual(void)
{
	return _glXGetVisualFromFBConfig(DPY3D, config);
}


void VirtualDrawable::OGLDrawable::clear(void)
{
	if(cleared) return;
	cleared = true;
	GLfloat params[4];
	_glGetFloatv(GL_COLOR_CLEAR_VALUE, params);
	_glClearColor(0, 0, 0, 0);
	_glClear(GL_COLOR_BUFFER_BIT);
	_glClearColor(params[0], params[1], params[2], params[3]);
}


void VirtualDrawable::OGLDrawable::swap(void)
{
	_glXSwapBuffers(DPY3D, glxDraw);
}


// This class encapsulates the relationship between an X11 drawable and the
// 3D off-screen drawable that backs it.

VirtualDrawable::VirtualDrawable(Display *dpy_, Drawable x11Draw_)
{
	if(!dpy_ || !x11Draw_) THROW("Invalid argument");
	dpy = dpy_;
	x11Draw = x11Draw_;
	oglDraw = NULL;
	profReadback.setName("Readback  ");
	autotestFrameCount = 0;
	config = 0;
	ctx = 0;
	direct = -1;
	pbo = 0;
	numSync = numFrames = 0;
	lastFormat = -1;
	usePBO = (fconfig.readback == RRREAD_PBO);
	alreadyPrinted = alreadyWarned = alreadyWarnedRenderMode = false;
	ext = NULL;
}


VirtualDrawable::~VirtualDrawable(void)
{
	mutex.lock(false);
	if(oglDraw) { delete oglDraw;  oglDraw = NULL; }
	if(ctx) { _glXDestroyContext(DPY3D, ctx);  ctx = 0; }
	mutex.unlock(false);
}


int VirtualDrawable::init(int width, int height, GLXFBConfig config_)
{
	static bool alreadyPrintedDrawableType = false;
	if(!config_ || width < 1 || height < 1) THROW("Invalid argument");

	CriticalSection::SafeLock l(mutex);
	if(oglDraw && oglDraw->getWidth() == width && oglDraw->getHeight() == height
		&& FBCID(oglDraw->getConfig()) == FBCID(config_))
		return 0;
	if(fconfig.drawable == RRDRAWABLE_PIXMAP)
	{
		if(!alreadyPrintedDrawableType && fconfig.verbose)
		{
			vglout.println("[VGL] Using Pixmaps for rendering");
			alreadyPrintedDrawableType = true;
		}
		NEWCHECK(oglDraw = new OGLDrawable(width, height, 0, config_, NULL));
	}
	else
	{
		if(!alreadyPrintedDrawableType && fconfig.verbose)
		{
			vglout.println("[VGL] Using Pbuffers for rendering");
			alreadyPrintedDrawableType = true;
		}
		NEWCHECK(oglDraw = new OGLDrawable(width, height, config_));
	}
	if(config && FBCID(config_) != FBCID(config) && ctx)
	{
		_glXDestroyContext(DPY3D, ctx);  ctx = 0;
	}
	config = config_;
	return 1;
}


void VirtualDrawable::setDirect(Bool direct_)
{
	if(direct_ != True && direct_ != False) return;
	if(direct_ != direct && ctx)
	{
		_glXDestroyContext(DPY3D, ctx);  ctx = 0;
	}
	direct = direct_;
}


void VirtualDrawable::clear(void)
{
	CriticalSection::SafeLock l(mutex);
	if(oglDraw) oglDraw->clear();
}


// Get the current 3D off-screen drawable

GLXDrawable VirtualDrawable::getGLXDrawable(void)
{
	GLXDrawable retval = 0;
	CriticalSection::SafeLock l(mutex);
	if(oglDraw) retval = oglDraw->getGLXDrawable();
	return retval;
}


Display *VirtualDrawable::getX11Display(void)
{
	return dpy;
}


Drawable VirtualDrawable::getX11Drawable(void)
{
	return x11Draw;
}


static const char *formatString(int glFormat)
{
	switch(glFormat)
	{
		case GL_RGB:       return "RGB";
		case GL_RGBA:      return "RGBA";
		case GL_BGR:       return "BGR";
		case GL_BGRA:      return "BGRA";
		#ifdef GL_ABGR_EXT
		case GL_ABGR_EXT:  return "ABGR";
		#endif
		case GL_RED:       return "COMPONENT";
		default:           return "????";
	}
}


void VirtualDrawable::readPixels(GLint x, GLint y, GLint width, GLint pitch,
	GLint height, GLenum glFormat, PF *pf, GLubyte *bits, GLint readBuf,
	bool stereo)
{
	double t0 = 0.0, tRead, tTotal;
	GLenum type = GL_UNSIGNED_BYTE;

	// Compute OpenGL format from pixel format of frame
	if(glFormat == GL_NONE)
	{
		glFormat = pf_glformat[pf->id];  type = pf_gldatatype[pf->id];
	}
	if(glFormat == GL_NONE) THROW("Unsupported pixel format");

	// Whenever the readback format changes (perhaps due to switching
	// compression or transports), then reset the PBO synchronicity detector
	int currentFormat =
		(glFormat == GL_GREEN || glFormat == GL_BLUE) ? GL_RED : glFormat;
	if(lastFormat >= 0 && lastFormat != currentFormat)
	{
		usePBO = (fconfig.readback == RRREAD_PBO);
		numSync = numFrames = 0;
		alreadyPrinted = alreadyWarned = false;
	}
	lastFormat = currentFormat;

	GLXDrawable read = _glXGetCurrentDrawable();
	GLXDrawable draw = _glXGetCurrentDrawable();
	if(read == 0 || readBuf == GL_BACK) read = getGLXDrawable();
	if(draw == 0 || readBuf == GL_BACK) draw = getGLXDrawable();

	// VirtualGL has to create a temporary context when performing pixel
	// readback, because the current context may not be using the same drawable
	// for rendering and readback, and the values of certain parameters within
	// that context might not be suitable for pixel readback.  However, the
	// use of this temporary context triggers a GLXBadContextState error if the
	// render mode is not GL_RENDER (it is illegal to call
	// glXMake[Context]Current() with a render mode of GL_SELECT|GL_FEEDBACK.)
	// Temporarily switching the render mode to GL_RENDER is impossible without
	// breaking OpenGL/GLX conformance, because calling glRenderMode(GL_RENDER)
	// resets the state of the select or feedback buffer, and there is no way to
	// restore that state to its previous value.  Thus, we have no choice but to
	// skip pixel readback if the render mode != GL_RENDER.  Although this is not
	// known to break any existing applications, our behavior in this regard is
	// non-standard, so we print a warning if VGL_VERBOSE=1.
	int renderMode = 0;
	_glGetIntegerv(GL_RENDER_MODE, &renderMode);
	if(renderMode != GL_RENDER && renderMode != 0)
	{
		if(!alreadyWarnedRenderMode && fconfig.verbose)
		{
			vglout.println("[VGL] WARNING: One or more readbacks skipped because render mode != GL_RENDER.");
			alreadyWarnedRenderMode = true;
		}
		return;
	}

	if(!ctx)
	{
		if(!isInit())
			THROW("VirtualDrawable instance has not been fully initialized");
		if((ctx = _glXCreateNewContext(DPY3D, config, GLX_RGBA_TYPE, NULL,
			direct)) == 0)
			THROW("Could not create OpenGL context for readback");
	}
	TempContext tc(DPY3D, draw, read, ctx, config, GLX_RGBA_TYPE);

	_glReadBuffer(readBuf);

	if(pitch % 8 == 0) _glPixelStorei(GL_PACK_ALIGNMENT, 8);
	else if(pitch % 4 == 0) _glPixelStorei(GL_PACK_ALIGNMENT, 4);
	else if(pitch % 2 == 0) _glPixelStorei(GL_PACK_ALIGNMENT, 2);
	else if(pitch % 1 == 0) _glPixelStorei(GL_PACK_ALIGNMENT, 1);

	if(usePBO)
	{
		if(!ext)
		{
			ext = (const char *)_glGetString(GL_EXTENSIONS);
			if(!ext || !strstr(ext, "GL_ARB_pixel_buffer_object"))
				THROW("GL_ARB_pixel_buffer_object extension not available");
		}
		if(!pbo) _glGenBuffers(1, &pbo);
		if(!pbo) THROW("Could not generate pixel buffer object");
		if(!alreadyPrinted && fconfig.verbose)
		{
			vglout.println("[VGL] Using pixel buffer objects for readback (%s --> %s)",
				formatString(oglDraw->getFormat()), formatString(glFormat));
			alreadyPrinted = true;
		}
		_glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, pbo);
		int size = 0;
		_glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
		if(size != pitch * height)
			_glBufferData(GL_PIXEL_PACK_BUFFER_EXT, pitch * height, NULL,
				GL_STREAM_READ);
		_glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
		if(size != pitch * height)
			THROW("Could not set PBO size");
	}
	else
	{
		if(!alreadyPrinted && fconfig.verbose)
		{
			vglout.println("[VGL] Using synchronous readback (%s --> %s)",
				formatString(oglDraw->getFormat()), formatString(glFormat));
			alreadyPrinted = true;
		}
	}

	int e = _glGetError();
	while(e != GL_NO_ERROR) e = _glGetError();  // Clear previous error
	profReadback.startFrame();
	if(usePBO) t0 = GetTime();
	_glReadPixels(x, y, width, height, glFormat, type, usePBO ? NULL : bits);

	if(usePBO)
	{
		tRead = GetTime() - t0;
		unsigned char *pboBits = NULL;
		pboBits = (unsigned char *)_glMapBuffer(GL_PIXEL_PACK_BUFFER_EXT,
			GL_READ_ONLY);
		if(!pboBits) THROW("Could not map pixel buffer object");
		memcpy(bits, pboBits, pitch * height);
		if(!_glUnmapBuffer(GL_PIXEL_PACK_BUFFER_EXT))
			THROW("Could not unmap pixel buffer object");
		_glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
		tTotal = GetTime() - t0;
		numFrames++;
		if(tRead / tTotal > 0.5 && numFrames <= 10)
		{
			numSync++;
			if(numSync >= 10 && !alreadyWarned && fconfig.verbose)
			{
				vglout.println("[VGL] NOTICE: PBO readback is not behaving asynchronously.  Disabling PBOs.");
				if(glFormat != oglDraw->getFormat())
				{
					vglout.println("[VGL]    This could be due to a mismatch between the readback pixel format");
					vglout.println("[VGL]    (%s) and the Pbuffer pixel format (%s).",
						formatString(glFormat), formatString(oglDraw->getFormat()));
					if(((oglDraw->getFormat() == GL_BGRA && glFormat == GL_BGR)
						|| (oglDraw->getFormat() == GL_RGBA && glFormat == GL_RGB))
						&& fconfig.forcealpha)
						vglout.println("[VGL]    Try setting VGL_FORCEALPHA=0.");
					else if(((oglDraw->getFormat() == GL_BGR && glFormat == GL_BGRA)
						|| (oglDraw->getFormat() == GL_RGB && glFormat == GL_RGBA))
						&& !fconfig.forcealpha)
						vglout.println("[VGL]    Try setting VGL_FORCEALPHA=1.");
				}
				alreadyWarned = true;
			}
		}
	}

	profReadback.endFrame(width * height, 0, stereo ? 0.5 : 1);
	CHECKGL("Read Pixels");

	// If automatic faker testing is enabled, store the FB color in an
	// environment variable so the test program can verify it
	if(fconfig.autotest)
	{
		unsigned char *rowptr, *pixel;  int match = 1;
		int color = -1, i, j, k;

		color = -1;
		if(readBuf != GL_FRONT_RIGHT && readBuf != GL_BACK_RIGHT)
			autotestFrameCount++;
		for(j = 0, rowptr = bits; j < height && match; j++, rowptr += pitch)
			for(i = 1, pixel = &rowptr[pf->size]; i < width && match;
				i++, pixel += pf->size)
				for(k = 0; k < pf->size; k++)
				{
					if(pixel[k] != rowptr[k])
					{
						match = 0;  break;
					}
				}
		if(match)
		{
			unsigned char rgb[3];
			_glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);
			color = rgb[0] + (rgb[1] << 8) + (rgb[2] << 16);
		}
		if(readBuf == GL_FRONT_RIGHT || readBuf == GL_BACK_RIGHT)
			vglfaker::setAutotestRColor(color);
		else
			vglfaker::setAutotestColor(color);
		vglfaker::setAutotestFrame(autotestFrameCount);
		vglfaker::setAutotestDisplay(dpy);
		vglfaker::setAutotestDrawable(x11Draw);
	}
}


void VirtualDrawable::copyPixels(GLint srcX, GLint srcY, GLint width,
	GLint height, GLint destX, GLint destY, GLXDrawable draw)
{
	if(!ctx)
	{
		if(!isInit())
			THROW("VirtualDrawable instance has not been fully initialized");
		if((ctx = _glXCreateNewContext(DPY3D, config, GLX_RGBA_TYPE, NULL,
			direct)) == 0)
			THROW("Could not create OpenGL context for readback");
	}
	TempContext tc(DPY3D, draw, getGLXDrawable(), ctx, config, GLX_RGBA_TYPE);

	_glReadBuffer(GL_FRONT);
	_glDrawBuffer(GL_FRONT_AND_BACK);

	_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	_glPixelStorei(GL_PACK_ALIGNMENT, 1);

	int e = _glGetError();
	while(e != GL_NO_ERROR) e = _glGetError();  // Clear previous error

	_glViewport(0, 0, width, height);
	_glMatrixMode(GL_PROJECTION);
	_glPushMatrix();
	_glLoadIdentity();
	_glOrtho(0, width, 0, height, -1, 1);
	_glMatrixMode(GL_MODELVIEW);
	_glPushMatrix();
	_glLoadIdentity();

	for(GLint i = 0; i < height; i++)
	{
		_glRasterPos2i(destX, height - destY - i - 1);
		_glCopyPixels(srcX, height - srcY - i - 1, width, 1, GL_COLOR);
	}
	CHECKGL("Copy Pixels");

	_glMatrixMode(GL_MODELVIEW);
	_glPopMatrix();
	_glMatrixMode(GL_PROJECTION);
	_glPopMatrix();
}
