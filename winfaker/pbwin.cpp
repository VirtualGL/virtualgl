/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005-2008 Sun Microsystems, Inc.
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include "pbwin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fakerconfig.h"

#define checkgl(m) if(glerror()) _throw("Could not "m);

#define _isright(drawbuf) (drawbuf==GL_RIGHT || drawbuf==GL_FRONT_RIGHT \
	|| drawbuf==GL_BACK_RIGHT)
#define leye(buf) (buf==GL_BACK? GL_BACK_LEFT: \
	(buf==GL_FRONT? GL_FRONT_LEFT:buf))
#define reye(buf) (buf==GL_BACK? GL_BACK_RIGHT: \
	(buf==GL_FRONT? GL_FRONT_RIGHT:buf))

static inline int _drawingtoright(void)
{
	GLint drawbuf=GL_LEFT;
	glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return _isright(drawbuf);
}

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

#ifndef min
 #define min(a,b) ((a)<(b)?(a):(b))
#endif

extern PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;
extern PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB;
extern PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;
extern PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
extern PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;
extern BOOL (WINAPI *__SwapBuffers)(HDC);

pbuffer::pbuffer(HDC hdc, int w, int h, int pixelformat)
{
	PIXELFORMATDESCRIPTOR pfd;
	if(!hdc || !pixelformat || w<1 || h<1) _throw("Invalid argument");

	_cleared=false;  _stereo=false;

	int pbattribs[]={0};
	_w=w;  _h=h;
	_drawable=wglCreatePbufferARB(hdc, pixelformat, w, h, pbattribs);
	if(DescribePixelFormat(hdc, pixelformat, sizeof(PIXELFORMATDESCRIPTOR),
		&pfd) && pfd.dwFlags&PFD_STEREO) _stereo=true;
	if(!_drawable) _throww32m("Could not create Pbuffer");
	_hdc=wglGetPbufferDCARB(_drawable);
	if(!_hdc) _throww32m("Could not obtain Pbuffer device context");
}

pbuffer::~pbuffer(void)
{
	if(_drawable && _hdc)
	{
		wglReleasePbufferDCARB(_drawable, _hdc);  _hdc=0;
	}
	if(_drawable)
	{
		wglDestroyPbufferARB(_drawable);  _drawable=0;
	}
}

void pbuffer::clear(void)
{
	if(_cleared) return;
	_cleared=true;
	GLfloat params[4];
	glGetFloatv(GL_COLOR_CLEAR_VALUE, params);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(params[0], params[1], params[2], params[3]);
}

void pbuffer::swap(void)
{
	__SwapBuffers(_hdc);
}


// This class encapsulates the Pbuffer, its most recent ancestor, and
// information specific to its corresponding X window

pbwin::pbwin(HWND hwnd, HDC hdc, int pixelformat)
{
	if(!hwnd || !hdc || pixelformat<1) _throw("Invalid argument");
	_force=false;
	_pb=NULL;
	_hwnd=hwnd;
	_blitter=NULL;
	_prof_rb.setname("Readback  ");
	_prof_gamma.setname("Gamma     ");
	_prof_anaglyph.setname("Anaglyph  ");
	_dirty=false;
	_rdirty=false;
	_autotestframecount=0;
	_pixelformat=pixelformat;
	int attrib=WGL_DRAW_TO_PBUFFER_ARB, value=0;
	if(!wglGetPixelFormatAttribivARB(hdc, _pixelformat, 0, 1, &attrib, &value)
		|| value==0)
		_throw("Pixel format does not support Pbuffers");
	initfromwindow(hdc);
}

pbwin::~pbwin(void)
{
	_mutex.lock(false);
	if(_pb) {delete _pb;  _pb=NULL;}
	if(_blitter) {delete _blitter;  _blitter=NULL;}
	_mutex.unlock(false);
}

int pbwin::init(HDC hdc, int w, int h)
{
	if(!hdc || w<1 || h<1) _throw("Invalid argument");

	rrcs::safelock l(_mutex);
	if(_pb && _pb->width()==w && _pb->height()==h) return 0;
	if((_pb=new pbuffer(hdc, w, h, _pixelformat))==NULL)
		_throw("Could not create Pbuffer");
	_force=true;
	return 1;
}

void pbwin::clear(void)
{
	rrcs::safelock l(_mutex);
	if(_pb) _pb->clear();
}

void pbwin::initfromwindow(HDC hdc)
{
	RECT rect={0, 0, 0, 0};
	if(!GetClientRect(_hwnd, &rect))
		_throww32m("Could not get client rectangle");
	if(rect.right<=rect.left && rect.bottom<=rect.top)
		_throww32m("Invalid client rectangle");
	init(hdc, rect.right-rect.left, rect.bottom-rect.top);
}

// Get the current Pbuffer DC

HDC pbwin::gethdc(void)
{
	HDC retval=0;
	rrcs::safelock l(_mutex);
	retval=_pb->hdc();
	return retval;
}

// Get the current Pbuffer DC, but resize the Pbuffer first if necessary

HDC pbwin::updatehdc(void)
{
	HDC retval=0;
	rrcs::safelock l(_mutex);
	HDC hdc=GetDC(_hwnd);
	initfromwindow(hdc);
	DeleteDC(hdc);
	retval=_pb->hdc();
	return retval;
}

void pbwin::swapbuffers(void)
{
	rrcs::safelock l(_mutex);
	if(_pb) _pb->swap();
}

void pbwin::readback(GLint drawbuf, bool spoillast)
{
	fconfig.reloadenv();
	bool dostereo=false;  int stereomode=fconfig.stereo;

	rrcs::safelock l(_mutex);

	_dirty=false;

	if(stereo() && stereomode!=RRSTEREO_LEYE && stereomode!=RRSTEREO_REYE)
	{
		if(_drawingtoright() || _rdirty) dostereo=true;
		_rdirty=false;
	}

	sendw32(drawbuf, spoillast, dostereo, stereomode);
}

void pbwin::sendw32(GLint drawbuf, bool spoillast, bool dostereo,
	int stereomode)
{
	int pbw=_pb->width(), pbh=_pb->height();

	rrfb *b;
	if(!_blitter) errifnot(_blitter=new rrblitter());
	if(spoillast && fconfig.spoil && !_blitter->frameready()) return;
	errifnot(b=_blitter->getbitmap(_hwnd, pbw, pbh, fconfig.spoil));
	b->_flags|=RRBMP_BOTTOMUP;
	if(dostereo && stereomode==RRSTEREO_REDCYAN) makeanaglyph(b, drawbuf);
	else
	{
		int format;
		unsigned char *bits=b->_bits;
		switch(b->_pixelsize)
		{
			case 3:
				format=GL_RGB;
				#ifdef GL_BGR_EXT
				if(b->_flags&RRBMP_BGR) format=GL_BGR_EXT;
				#endif
				break;
			case 4:
				format=GL_RGBA;
				#ifdef GL_BGRA_EXT
				if(b->_flags&RRBMP_BGR && !(b->_flags&RRBMP_ALPHAFIRST))
					format=GL_BGRA_EXT;
				#endif
				break;
			default:
				_throw("Unsupported pixel format");
		}
		GLint buf=drawbuf;
		if(stereomode==RRSTEREO_REYE) buf=reye(drawbuf);
		else if(stereomode==RRSTEREO_LEYE) buf=leye(drawbuf);
		readpixels(0, 0, min(pbw, b->_h.framew), b->_pitch,
			min(pbh, b->_h.frameh), format, b->_pixelsize, bits, buf, true);
	}
	_blitter->sendframe(b);
}

void pbwin::makeanaglyph(rrframe *b, int drawbuf)
{
	_r.init(b->_h, 1, b->_flags, false);
	readpixels(0, 0, _r._h.framew, _r._pitch, _r._h.frameh, GL_RED,
		_r._pixelsize, _r._bits, leye(drawbuf), false);
	_g.init(b->_h, 1, b->_flags, false);
	readpixels(0, 0, _g._h.framew, _g._pitch, _g._h.frameh, GL_GREEN,
		_g._pixelsize, _g._bits, reye(drawbuf), false);
	_b.init(b->_h, 1, b->_flags, false);
	readpixels(0, 0, _b._h.framew, _b._pitch, _b._h.frameh, GL_BLUE,
		_b._pixelsize, _b._bits, reye(drawbuf), false);
	_prof_anaglyph.startframe();
	b->makeanaglyph(_r, _g, _b);
	_prof_anaglyph.endframe(b->_h.framew*b->_h.frameh, 0, 1);
}

void pbwin::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h,
	GLenum format, int ps, GLubyte *bits, GLint buf, bool stereo)
{

	GLint readbuf=GL_BACK;
	glGetIntegerv(GL_READ_BUFFER, &readbuf);

	glReadBuffer(buf);
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

	if(pitch%8==0) glPixelStorei(GL_PACK_ALIGNMENT, 8);
	else if(pitch%4==0) glPixelStorei(GL_PACK_ALIGNMENT, 4);
	else if(pitch%2==0) glPixelStorei(GL_PACK_ALIGNMENT, 2);
	else if(pitch%1==0) glPixelStorei(GL_PACK_ALIGNMENT, 1);

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	_prof_rb.startframe();
	glReadPixels(x, y, w, h, format, GL_UNSIGNED_BYTE, bits);
	_prof_rb.endframe(w*h, 0, stereo? 0.5 : 1);
	checkgl("Read Pixels");

	// Gamma correction
	if(fconfig.gamma!=0.0 && fconfig.gamma!=1.0 && fconfig.gamma!=-1.0)
	{
		_prof_gamma.startframe();
		static bool first=true;
		if(first)
		{
			first=false;
			if(fconfig.verbose)
				rrout.println("[VGL] Using software gamma correction (correction factor=%f)\n",
					(double)fconfig.gamma);
		}
		unsigned short *ptr1, *ptr2=(unsigned short *)(&bits[pitch*h]);
		for(ptr1=(unsigned short *)bits; ptr1<ptr2; ptr1++)
			*ptr1=fconfig.gamma._lut16[*ptr1];
		if((pitch*h)%2!=0) bits[pitch*h-1]=fconfig.gamma._lut[bits[pitch*h-1]];
		_prof_gamma.endframe(w*h, 0, stereo?0.5 : 1);
	}

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
			snprintf(_autotestrclr, 79, "__VGL_AUTOTESTRCLR%x=%d", (unsigned int)_hwnd, color);
			putenv(_autotestrclr);
		}
		else
		{
			snprintf(_autotestclr, 79, "__VGL_AUTOTESTCLR%x=%d", (unsigned int)_hwnd, color);
			putenv(_autotestclr);
		}
		snprintf(_autotestframe, 79, "__VGL_AUTOTESTFRAME%x=%d", (unsigned int)_hwnd, _autotestframecount);
		putenv(_autotestframe);
	}

	glPopClientAttrib();
	glReadBuffer(readbuf);
}

bool pbwin::stereo(void)
{
	return (_pb && _pb->stereo());
}
