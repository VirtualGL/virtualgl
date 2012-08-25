/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009-2012 D. R. Commander
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

#include "pbdrawable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glxvisual.h"
#include "glext-vgl.h"
#include "tempctx.h"
#include "fakerconfig.h"
#include "rrutil.h"


extern Display *_localdpy;


#define checkgl(m) if(glerror()) _throw("Could not "m);

// Generic OpenGL error checker (0 = no errors)
static int glerror(void)
{
	int i, ret=0;
	i=glGetError();
	while(i!=GL_NO_ERROR)
	{
		ret=1;
		rrout.print("[VGL] ERROR: OpenGL error 0x%.4x\n", i);
		i=glGetError();
	}
	return ret;
}


Window create_window(Display *dpy, GLXFBConfig config, int w, int h)
{
	XVisualInfo *vis;
	Window win;
	XSetWindowAttributes wattrs;
	Colormap cmap;

	if((vis=_glXGetVisualFromFBConfig(dpy, config))==NULL) return 0;
	cmap=XCreateColormap(dpy, RootWindow(dpy, vis->screen), vis->visual,
		AllocNone);
	wattrs.background_pixel = 0;
	wattrs.border_pixel = 0;
	wattrs.colormap = cmap;
	wattrs.event_mask = ExposureMask | StructureNotifyMask;
	win = XCreateWindow(dpy, RootWindow(dpy, vis->screen), 0, 0, w, h, 1,
		vis->depth, InputOutput, vis->visual,
		CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &wattrs);
	XMapWindow(dpy, win);
	return win;
}


pbuffer::pbuffer(int w, int h, GLXFBConfig config)
{
	if(!config || w<1 || h<1) _throw("Invalid argument");

	_cleared=false;  _stereo=false;  _format=0;

	int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0,
		GLX_PRESERVED_CONTENTS, True, None};

	_w=w;  _h=h;  _config=config;
	pbattribs[1]=w;  pbattribs[3]=h;
	if(fconfig.usewindow) _drawable=create_window(_localdpy, config, w, h);
	else _drawable=glXCreatePbuffer(_localdpy, config, pbattribs);
	if(__vglServerVisualAttrib(config, GLX_STEREO)) _stereo=true;
	int pixelsize=__vglServerVisualAttrib(config, GLX_RED_SIZE)
		+__vglServerVisualAttrib(config, GLX_GREEN_SIZE)
		+__vglServerVisualAttrib(config, GLX_BLUE_SIZE)
		+__vglServerVisualAttrib(config, GLX_ALPHA_SIZE);
	if(pixelsize==32)
	{
		#ifdef GL_BGRA_EXT
		if(littleendian()) _format=GL_BGRA_EXT;
		else
		#endif
		_format=GL_RGBA;
	}
	else
	{
		#ifdef GL_BGR_EXT
		if(littleendian()) _format=GL_BGR_EXT;
		else
		#endif
		_format=GL_RGB;
	}
	if(!_drawable) _throw("Could not create Pbuffer");
}


pbuffer::~pbuffer(void)
{
	if(_drawable)
	{
		if(fconfig.usewindow) XDestroyWindow(_localdpy, _drawable);
		else glXDestroyPbuffer(_localdpy, _drawable);
	}
}


void pbuffer::clear(void)
{
	if(_cleared) return;
	_cleared=true;
	GLfloat params[4];
	_glGetFloatv(GL_COLOR_CLEAR_VALUE, params);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(params[0], params[1], params[2], params[3]);
}


void pbuffer::swap(void)
{
	_glXSwapBuffers(_localdpy, _drawable);
}


// This class encapsulates the relationship between an X11 drawable and the
// Pbuffer that backs it.

pbdrawable::pbdrawable(Display *dpy, Drawable drawable)
{
	if(!dpy || !drawable) _throw("Invalid argument");
	_dpy=dpy;  _drawable=drawable;
	_pb=NULL;
	_prof_rb.setname("Readback  ");
	_autotestframecount=0;
	_config=0;
	_ctx=0;
	_direct=-1;
}


pbdrawable::~pbdrawable(void)
{
	_mutex.lock(false);
	if(_pb) {delete _pb;  _pb=NULL;}
	if(_ctx) {_glXDestroyContext(_localdpy, _ctx);  _ctx=0;}
	_mutex.unlock(false);
}


int pbdrawable::init(int w, int h, GLXFBConfig config)
{
	if(!config || w<1 || h<1) _throw("Invalid argument");

	rrcs::safelock l(_mutex);
	if(_pb && _pb->width()==w && _pb->height()==h
		&& _FBCID(_pb->config())==_FBCID(config)) return 0;
	if((_pb=new pbuffer(w, h, config))==NULL)
		_throw("Could not create Pbuffer");
	if(_config && _FBCID(config)!=_FBCID(_config) && _ctx)
	{
		_glXDestroyContext(_localdpy, _ctx);  _ctx=0;
	}
	_config=config;
	return 1;
}


void pbdrawable::setdirect(Bool direct)
{
	if(direct!=True && direct!=False) return;
	if(direct!=_direct && _ctx)
	{
		_glXDestroyContext(_localdpy, _ctx);  _ctx=0;
	}
	_direct=direct;
}


void pbdrawable::clear(void)
{
	rrcs::safelock l(_mutex);
	if(_pb) _pb->clear();
}


// Get the current Pbuffer drawable

GLXDrawable pbdrawable::getglxdrawable(void)
{
	GLXDrawable retval=0;
	rrcs::safelock l(_mutex);
	retval=_pb->drawable();
	return retval;
}


Display *pbdrawable::get2ddpy(void)
{
	return _dpy;
}


Drawable pbdrawable::getx11drawable(void)
{
	return _drawable;
}


void pbdrawable::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h,
	GLenum format, int ps, GLubyte *bits, GLint buf, bool usepbo, bool stereo)
{
	#ifdef GL_VERSION_1_5
	GLuint pbo=0;
	#endif
	static bool alreadyprinted=false;
	static const char *ext=NULL;

	GLXDrawable read=_glXGetCurrentDrawable();
	GLXDrawable draw=_glXGetCurrentDrawable();
	if(read==0) read=getglxdrawable();
	if(draw==0) draw=getglxdrawable();

	if(!_ctx)
	{
		if(_direct!=True && _direct!=False)
			_throw("pbdrawable instance has not been fully initialized");
		if((_ctx=_glXCreateNewContext(_localdpy, _config, GLX_RGBA_TYPE, NULL,
			_direct))==0)
			_throw("Could not create OpenGL context for readback");
	}
	tempctx tc(_localdpy, draw, read, _ctx, _config, GLX_RGBA_TYPE);

	glReadBuffer(buf);

	if(pitch%8==0) glPixelStorei(GL_PACK_ALIGNMENT, 8);
	else if(pitch%4==0) glPixelStorei(GL_PACK_ALIGNMENT, 4);
	else if(pitch%2==0) glPixelStorei(GL_PACK_ALIGNMENT, 2);
	else if(pitch%1==0) glPixelStorei(GL_PACK_ALIGNMENT, 1);

	if(usepbo)
	{
		if(!ext)
		{
			ext=(const char *)glGetString(GL_EXTENSIONS);
			if(!ext || !strstr(ext, "GL_ARB_pixel_buffer_object"))
				_throw("GL_ARB_pixel_buffer_object extension not available");
		}
		#ifdef GL_VERSION_1_5
		glGenBuffers(1, &pbo);
		if(!pbo) _throw("Could not generate pixel buffer object");
		if(!alreadyprinted && fconfig.verbose)
		{
			rrout.println("[VGL] Using pixel buffer objects for readback (GL format = 0x%.4x)",
				format);
			alreadyprinted=true;
		}
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, pbo);
		int size=0;
		glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
		if(size!=pitch*h)
			glBufferData(GL_PIXEL_PACK_BUFFER_EXT, pitch*h, NULL, GL_STREAM_READ);
		glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
		if(size!=pitch*h)
			_throw("Could not set PBO size");
		#else
		_throw("PBO support not compiled in.  Rebuild VGL on a system that has OpenGL 1.5.");
		#endif
	}
	else
	{
		if(!alreadyprinted && fconfig.verbose)
		{
			rrout.println("[VGL] Using synchronous readback (GL format = 0x%.4x)",
				format);
			alreadyprinted=true;
		}
	}

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	_prof_rb.startframe();
	glReadPixels(x, y, w, h, format, GL_UNSIGNED_BYTE, usepbo? NULL:bits);

	if(usepbo)
	{
		#ifdef GL_VERSION_1_5
		unsigned char *pbobits=NULL;
		pbobits=(unsigned char *)glMapBuffer(GL_PIXEL_PACK_BUFFER_EXT,
			GL_READ_ONLY);
		if(!pbobits) _throw("Could not map pixel buffer object");
		memcpy(bits, pbobits, pitch*h);
		if(!glUnmapBuffer(GL_PIXEL_PACK_BUFFER_EXT))
			_throw("Could not unmap pixel buffer object");
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
		glDeleteBuffers(1, &pbo);
		#endif
	}

	_prof_rb.endframe(w*h, 0, stereo? 0.5 : 1);
	checkgl("Read Pixels");

	// If automatic faker testing is enabled, store the FB color in an
	// environment variable so the test program can verify it
	if(fconfig.autotest)
	{
		unsigned char *rowptr, *pixel;  int match=1;
		int color=-1, i, j, k;
		color=-1;
		if(buf!=GL_FRONT_RIGHT && buf!=GL_BACK_RIGHT) _autotestframecount++;
		for(j=0, rowptr=bits; j<h && match; j++, rowptr+=pitch)
			for(i=1, pixel=&rowptr[ps]; i<w && match; i++, pixel+=ps)
				for(k=0; k<ps; k++)
				{
					if(pixel[k]!=rowptr[k]) {match=0;  break;}
				}
		if(match)
		{
			if(format==GL_COLOR_INDEX)
			{
				unsigned char index;
				glReadPixels(0, 0, 1, 1, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, &index);
				color=index;
			}
			else
			{
				unsigned char rgb[3];
				glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);
				color=rgb[0]+(rgb[1]<<8)+(rgb[2]<<16);
			}
		}
		if(buf==GL_FRONT_RIGHT || buf==GL_BACK_RIGHT)
		{
			snprintf(_autotestrclr, 79, "__VGL_AUTOTESTRCLR%x=%d",
				(unsigned int)_drawable, color);
			putenv(_autotestrclr);
		}
		else
		{
			snprintf(_autotestclr, 79, "__VGL_AUTOTESTCLR%x=%d",
				(unsigned int)_drawable, color);
			putenv(_autotestclr);
		}
		snprintf(_autotestframe, 79, "__VGL_AUTOTESTFRAME%x=%d",
			(unsigned int)_drawable, _autotestframecount);
		putenv(_autotestframe);
	}
}


void pbdrawable::copypixels(GLint src_x, GLint src_y, GLint w, GLint h,
	GLint dest_x, GLint dest_y, GLXDrawable draw)
{
	if(!_ctx)
	{
		if(_direct!=True && _direct!=False)
			_throw("pbdrawable instance has not been fully initialized");
		if((_ctx=_glXCreateNewContext(_localdpy, _config, GLX_RGBA_TYPE, NULL,
			_direct))==0)
			_throw("Could not create OpenGL context for readback");
	}
	tempctx tc(_localdpy, draw, getglxdrawable(), _ctx, _config, GLX_RGBA_TYPE);

	glReadBuffer(GL_FRONT);
	_glDrawBuffer(GL_FRONT_AND_BACK);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error

	_glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	for(GLint i=0; i<h; i++)
	{
		glRasterPos2i(dest_x, h-dest_y-i-1);
		glCopyPixels(src_x, h-src_y-i-1, w, 1, GL_COLOR);
	}
	checkgl("Copy Pixels");

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}
