/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009-2015 D. R. Commander
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

#include "VirtualDrawable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glxvisual.h"
#include "glext-vgl.h"
#include "TempContext.h"
#include "vglutil.h"
#include "faker.h"

using namespace vglutil;
using namespace vglserver;


#define CHECKGL(m) if(glError()) _throw("Could not "m);

// Generic OpenGL error checker (0 = no errors)
static int glError(void)
{
	int i, ret=0;

	i=glGetError();
	while(i!=GL_NO_ERROR)
	{
		ret=1;
		vglout.print("[VGL] ERROR: OpenGL error 0x%.4x\n", i);
		i=glGetError();
	}
	return ret;
}


static Window create_window(Display *dpy, XVisualInfo *vis, int width,
	int height)
{
	Window win;
	XSetWindowAttributes wattrs;
	Colormap cmap;

	cmap=XCreateColormap(dpy, RootWindow(dpy, vis->screen), vis->visual,
		AllocNone);
	wattrs.background_pixel=0;
	wattrs.border_pixel=0;
	wattrs.colormap=cmap;
	wattrs.event_mask=0;
	win=_XCreateWindow(dpy, RootWindow(dpy, vis->screen), 0, 0, width, height,
		1, vis->depth, InputOutput, vis->visual,
		CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &wattrs);
	return win;
}


// Pbuffer constructor

VirtualDrawable::OGLDrawable::OGLDrawable(int width_, int height_,
	GLXFBConfig config_) : cleared(false), stereo(false), glxDraw(0),
	width(width_), height(height_), depth(0), config(config_), format(0), pm(0),
	win(0), isPixmap(false)
{
	if(!config_ || width_<1 || height_<1) _throw("Invalid argument");

	int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0,
		GLX_PRESERVED_CONTENTS, True, None};

	pbattribs[1]=width;  pbattribs[3]=height;
	glxDraw=glXCreatePbuffer(_dpy3D, config, pbattribs);
	if(!glxDraw) _throw("Could not create Pbuffer");

	setVisAttribs();
}


// Pixmap constructor

VirtualDrawable::OGLDrawable::OGLDrawable(int width_, int height_, int depth_,
	GLXFBConfig config_, const int *attribs) : cleared(false), stereo(false),
	glxDraw(0), width(width_), height(height_), depth(depth_), config(config_),
	format(0), pm(0), win(0), isPixmap(true)
{
	if(!config_ || width_<1 || height_<1 || depth_<0)
		_throw("Invalid argument");

	XVisualInfo *vis=NULL;
	if((vis=_glXGetVisualFromFBConfig(_dpy3D, config))==NULL)
		goto bailout;
	win=create_window(_dpy3D, vis, 1, 1);
	if(!win) goto bailout;
	pm=XCreatePixmap(_dpy3D, win, width, height, depth>0? depth:vis->depth);
	if(!pm) goto bailout;
	glxDraw=glXCreatePixmap(_dpy3D, config, pm, attribs);
	if(!glxDraw) goto bailout;

	setVisAttribs();
	return;

	bailout:
	if(vis) XFree(vis);
	_throw("Could not create GLX pixmap");
}


void VirtualDrawable::OGLDrawable::setVisAttribs(void)
{
	if(glxvisual::visAttrib3D(config, GLX_STEREO))
		stereo=true;
	int pixelsize=glxvisual::visAttrib3D(config, GLX_RED_SIZE)
		+glxvisual::visAttrib3D(config, GLX_GREEN_SIZE)
		+glxvisual::visAttrib3D(config, GLX_BLUE_SIZE)
		+glxvisual::visAttrib3D(config, GLX_ALPHA_SIZE);

	if(pixelsize==32)
	{
		#ifdef GL_BGRA_EXT
		if(littleendian()) format=GL_BGRA_EXT;
		else
		#endif
		format=GL_RGBA;
	}
	else
	{
		#ifdef GL_BGR_EXT
		if(littleendian()) format=GL_BGR_EXT;
		else
		#endif
		format=GL_RGB;
	}
}


VirtualDrawable::OGLDrawable::~OGLDrawable(void)
{
	if(isPixmap)
	{
		if(glxDraw)
		{
			_glXDestroyPixmap(_dpy3D, glxDraw);
			glxDraw=0;
		}
		if(pm) { XFreePixmap(_dpy3D, pm);  pm=0; }
		if(win) { _XDestroyWindow(_dpy3D, win);  win=0; }
	}
	else
	{
		glXDestroyPbuffer(_dpy3D, glxDraw);
		glxDraw=0;
	}
}


XVisualInfo *VirtualDrawable::OGLDrawable::getVisual(void)
{
	return _glXGetVisualFromFBConfig(_dpy3D, config);
}


void VirtualDrawable::OGLDrawable::clear(void)
{
	if(cleared) return;
	cleared=true;
	GLfloat params[4];
	_glGetFloatv(GL_COLOR_CLEAR_VALUE, params);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(params[0], params[1], params[2], params[3]);
}


void VirtualDrawable::OGLDrawable::swap(void)
{
	_glXSwapBuffers(_dpy3D, glxDraw);
}


// This class encapsulates the relationship between an X11 drawable and the
// 3D off-screen drawable that backs it.

VirtualDrawable::VirtualDrawable(Display *dpy_, Drawable x11Draw_)
{
	if(!dpy_ || !x11Draw_) _throw("Invalid argument");
	dpy=dpy_;
	x11Draw=x11Draw_;
	oglDraw=NULL;
	profReadback.setName("Readback  ");
	autotestFrameCount=0;
	config=0;
	ctx=0;
	direct=-1;
}


VirtualDrawable::~VirtualDrawable(void)
{
	mutex.lock(false);
	if(oglDraw) { delete oglDraw;  oglDraw=NULL; }
	if(ctx) { _glXDestroyContext(_dpy3D, ctx);  ctx=0; }
	mutex.unlock(false);
}


int VirtualDrawable::init(int width, int height, GLXFBConfig config_)
{
	static bool alreadyPrinted=false;
	if(!config_ || width<1 || height<1) _throw("Invalid argument");

	CriticalSection::SafeLock l(mutex);
	if(oglDraw && oglDraw->getWidth()==width && oglDraw->getHeight()==height
		&& _FBCID(oglDraw->getConfig())==_FBCID(config_))
		return 0;
	if(fconfig.drawable==RRDRAWABLE_PIXMAP)
	{
		if(!alreadyPrinted && fconfig.verbose)
		{
			vglout.println("[VGL] Using Pixmaps for rendering");
			alreadyPrinted=true;
		}
		_newcheck(oglDraw=new OGLDrawable(width, height, 0, config_, NULL));
	}
	else
	{
		if(!alreadyPrinted && fconfig.verbose)
		{
			vglout.println("[VGL] Using Pbuffers for rendering");
			alreadyPrinted=true;
		}
		_newcheck(oglDraw=new OGLDrawable(width, height, config_));
	}
	if(config && _FBCID(config_)!=_FBCID(config) && ctx)
	{
		_glXDestroyContext(_dpy3D, ctx);  ctx=0;
	}
	config=config_;
	return 1;
}


void VirtualDrawable::setDirect(Bool direct_)
{
	if(direct_!=True && direct_!=False) return;
	if(direct_!=direct && ctx)
	{
		_glXDestroyContext(_dpy3D, ctx);  ctx=0;
	}
	direct=direct_;
}


void VirtualDrawable::clear(void)
{
	CriticalSection::SafeLock l(mutex);
	if(oglDraw) oglDraw->clear();
}


// Get the current 3D off-screen drawable

GLXDrawable VirtualDrawable::getGLXDrawable(void)
{
	GLXDrawable retval=0;
	CriticalSection::SafeLock l(mutex);
	if(oglDraw) retval=oglDraw->getGLXDrawable();
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


static const char *formatString(int format)
{
	switch(format)
	{
		case GL_RGB:
			return "RGB";
		case GL_RGBA:
			return "RGBA";
		#ifdef GL_BGR_EXT
		case GL_BGR:
			return "BGR";
		#endif
		#ifdef GL_BGRA_EXT
		case GL_BGRA:
			return "BGRA";
		#endif
		#ifdef GL_ABGR_EXT
		case GL_ABGR_EXT:
			return "ABGR";
		#endif
		case GL_COLOR_INDEX:
			return "INDEX";
		case GL_RED:  case GL_GREEN:  case GL_BLUE:
			return "COMPONENT";
		default:
			return "????";
	}
}


#define CHECKPBOSYM(s) {  \
	if(!__##s) {  \
		__##s=(__##s##Type)glXGetProcAddressARB((const GLubyte *)#s);  \
		if(!__##s)  \
			_throw(#s" symbol not loaded");  \
	}  \
}


void VirtualDrawable::readPixels(GLint x, GLint y, GLint width, GLint pitch,
	GLint height, GLenum format, int ps, GLubyte *bits, GLint buf, bool stereo)
{
	#ifdef GL_VERSION_1_5
	static GLuint pbo=0;
	#endif
	double t0=0.0, tRead, tTotal;
	static int numSync=0, numFrames=0, lastFormat=-1;
	static bool usePBO=(fconfig.readback==RRREAD_PBO);
	static bool alreadyPrinted=false, alreadyWarned=false;
	static const char *ext=NULL;

	typedef void (*__glGenBuffersType)(GLsizei, GLuint *);
	static __glGenBuffersType __glGenBuffers=NULL;
	typedef void (*__glBindBufferType)(GLenum, GLuint);
	static __glBindBufferType __glBindBuffer=NULL;
	typedef void (*__glBufferDataType)(GLenum, GLsizeiptr, const GLvoid *,
		GLenum);
	static __glBufferDataType __glBufferData=NULL;
	typedef void (*__glGetBufferParameterivType)(GLenum, GLenum, GLint *);
	static __glGetBufferParameterivType __glGetBufferParameteriv=NULL;
	typedef void *(*__glMapBufferType)(GLenum, GLenum);
	static __glMapBufferType __glMapBuffer=NULL;
	typedef GLboolean (*__glUnmapBufferType)(GLenum);
	static __glUnmapBufferType __glUnmapBuffer=NULL;

	// Whenever the readback format changes (perhaps due to switching
	// compression or transports), then reset the PBO synchronicity detector
	int currentFormat=(format==GL_GREEN || format==GL_BLUE)? GL_RED:format;
	if(lastFormat>=0 && lastFormat!=currentFormat)
	{
		usePBO=(fconfig.readback==RRREAD_PBO);
		numSync=numFrames=0;
		alreadyPrinted=alreadyWarned=false;
	}
	lastFormat=currentFormat;

	GLXDrawable read=_glXGetCurrentDrawable();
	GLXDrawable draw=_glXGetCurrentDrawable();
	if(read==0 || buf==GL_BACK) read=getGLXDrawable();
	if(draw==0 || buf==GL_BACK) draw=getGLXDrawable();

	if(!ctx)
	{
		if(!isInit())
			_throw("VirtualDrawable instance has not been fully initialized");
		if((ctx=_glXCreateNewContext(_dpy3D, config, GLX_RGBA_TYPE, NULL,
			direct))==0)
			_throw("Could not create OpenGL context for readback");
	}
	TempContext tc(_dpy3D, draw, read, ctx, config, GLX_RGBA_TYPE);

	glReadBuffer(buf);

	if(pitch%8==0) glPixelStorei(GL_PACK_ALIGNMENT, 8);
	else if(pitch%4==0) glPixelStorei(GL_PACK_ALIGNMENT, 4);
	else if(pitch%2==0) glPixelStorei(GL_PACK_ALIGNMENT, 2);
	else if(pitch%1==0) glPixelStorei(GL_PACK_ALIGNMENT, 1);

	if(usePBO)
	{
		if(!ext)
		{
			ext=(const char *)glGetString(GL_EXTENSIONS);
			if(!ext || !strstr(ext, "GL_ARB_pixel_buffer_object"))
				_throw("GL_ARB_pixel_buffer_object extension not available");
		}
		CHECKPBOSYM(glGenBuffers);
		CHECKPBOSYM(glBindBuffer);
		CHECKPBOSYM(glBufferData);
		CHECKPBOSYM(glGetBufferParameteriv);
		CHECKPBOSYM(glMapBuffer);
		CHECKPBOSYM(glUnmapBuffer);
		#ifdef GL_VERSION_1_5
		if(!pbo) __glGenBuffers(1, &pbo);
		if(!pbo) _throw("Could not generate pixel buffer object");
		if(!alreadyPrinted && fconfig.verbose)
		{
			vglout.println("[VGL] Using pixel buffer objects for readback (%s --> %s)",
				formatString(oglDraw->getFormat()), formatString(format));
			alreadyPrinted=true;
		}
		__glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, pbo);
		int size=0;
		__glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
		if(size!=pitch*height)
			__glBufferData(GL_PIXEL_PACK_BUFFER_EXT, pitch*height, NULL,
				GL_STREAM_READ);
		__glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
		if(size!=pitch*height)
			_throw("Could not set PBO size");
		#else
		_throw("PBO support not compiled in.  Rebuild VGL on a system that has OpenGL 1.5 or later.");
		#endif
	}
	else
	{
		if(!alreadyPrinted && fconfig.verbose)
		{
			vglout.println("[VGL] Using synchronous readback (%s --> %s)",
				formatString(oglDraw->getFormat()), formatString(format));
			alreadyPrinted=true;
		}
	}

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	profReadback.startFrame();
	if(usePBO) t0=getTime();
	glReadPixels(x, y, width, height, format, GL_UNSIGNED_BYTE,
		usePBO? NULL:bits);

	if(usePBO)
	{
		tRead=getTime()-t0;
		#ifdef GL_VERSION_1_5
		unsigned char *pboBits=NULL;
		pboBits=(unsigned char *)__glMapBuffer(GL_PIXEL_PACK_BUFFER_EXT,
			GL_READ_ONLY);
		if(!pboBits) _throw("Could not map pixel buffer object");
		memcpy(bits, pboBits, pitch*height);
		if(!__glUnmapBuffer(GL_PIXEL_PACK_BUFFER_EXT))
			_throw("Could not unmap pixel buffer object");
		__glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
		#endif
		tTotal=getTime()-t0;
		numFrames++;
		if(tRead/tTotal>0.5 && numFrames<=10)
		{
			numSync++;
			if(numSync>=10 && !alreadyWarned && fconfig.verbose)
			{
				vglout.println("[VGL] NOTICE: PBO readback is not behaving asynchronously.  Disabling PBOs.");
				if(format!=oglDraw->getFormat())
				{
					vglout.println("[VGL]    This could be due to a mismatch between the readback pixel format");
					vglout.println("[VGL]    (%s) and the Pbuffer pixel format (%s).",
						formatString(format), formatString(oglDraw->getFormat()));
					if(((oglDraw->getFormat()==GL_BGRA && format==GL_BGR)
						|| (oglDraw->getFormat()==GL_RGBA && format==GL_RGB))
						&& fconfig.forcealpha)
						vglout.println("[VGL]    Try setting VGL_FORCEALPHA=0.");
					else if(((oglDraw->getFormat()==GL_BGR && format==GL_BGRA)
						|| (oglDraw->getFormat()==GL_RGB && format==GL_RGBA))
						&& !fconfig.forcealpha)
						vglout.println("[VGL]    Try setting VGL_FORCEALPHA=1.");
				}
				alreadyWarned=true;
			}
		}
	}

	profReadback.endFrame(width*height, 0, stereo? 0.5 : 1);
	CHECKGL("Read Pixels");

	// If automatic faker testing is enabled, store the FB color in an
	// environment variable so the test program can verify it
	if(fconfig.autotest)
	{
		unsigned char *rowptr, *pixel;  int match=1;
		int color=-1, i, j, k;

		color=-1;
		if(buf!=GL_FRONT_RIGHT && buf!=GL_BACK_RIGHT)
			autotestFrameCount++;
		for(j=0, rowptr=bits; j<height && match; j++, rowptr+=pitch)
			for(i=1, pixel=&rowptr[ps]; i<width && match; i++, pixel+=ps)
				for(k=0; k<ps; k++)
				{
					if(pixel[k]!=rowptr[k])
					{
						match=0;  break;
					}
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
			snprintf(autotestRColor, 79, "__VGL_AUTOTESTRCLR%x=%d",
				(unsigned int)x11Draw, color);
			putenv(autotestRColor);
		}
		else
		{
			snprintf(autotestColor, 79, "__VGL_AUTOTESTCLR%x=%d",
				(unsigned int)x11Draw, color);
			putenv(autotestColor);
		}
		snprintf(autotestFrame, 79, "__VGL_AUTOTESTFRAME%x=%d",
			(unsigned int)x11Draw, autotestFrameCount);
		putenv(autotestFrame);
	}
}


void VirtualDrawable::copyPixels(GLint srcX, GLint srcY, GLint width,
	GLint height, GLint destX, GLint destY, GLXDrawable draw)
{
	if(!ctx)
	{
		if(!isInit())
			_throw("VirtualDrawable instance has not been fully initialized");
		if((ctx=_glXCreateNewContext(_dpy3D, config, GLX_RGBA_TYPE, NULL,
			direct))==0)
			_throw("Could not create OpenGL context for readback");
	}
	TempContext tc(_dpy3D, draw, getGLXDrawable(), ctx, config,
		GLX_RGBA_TYPE);

	glReadBuffer(GL_FRONT);
	_glDrawBuffer(GL_FRONT_AND_BACK);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error

	_glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	for(GLint i=0; i<height; i++)
	{
		glRasterPos2i(destX, height-destY-i-1);
		glCopyPixels(srcX, height-srcY-i-1, width, 1, GL_COLOR);
	}
	CHECKGL("Copy Pixels");

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}
