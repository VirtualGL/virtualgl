/* Copyright (C)2004 Landmark Graphics
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

#ifndef __RRFRAME_H
#define __RRFRAME_H

#include "rr.h"
#ifdef _WIN32
#define XDK
#endif
#include "fbx.h"
#include "hpjpeg.h"
#include "rrmutex.h"
#include "rrlog.h"
#include "rrutil.h"
#include <string.h>

static int jpegsub[RR_SUBSAMPOPT]={HPJ_444, HPJ_422, HPJ_411};

// Bitmap flags
#define RRBMP_BOTTOMUP   1  // Bottom-up bitmap (as opposed to top-down)
#define RRBMP_BGR        2  // BGR or BGRA pixel order
#define RRBMP_ALPHAFIRST 4  // BGR buffer is really ABGR, and RGB buffer is really ARGB

// Uncompressed bitmap

class rrframe
{
	public:

	rrframe(bool primary=true) : _bits(NULL), _pitch(0), _pixelsize(0), _flags(0),
		_strip_height(RR_DEFAULTSTRIPHEIGHT), _primary(primary)
	{
		memset(&_h, 0, sizeof(rrframeheader));
		_ready.lock();
	}

	virtual ~rrframe(void)
	{
		if(_bits && _primary) delete [] _bits;
	}

	void init(rrframeheader *h, int pixelsize, int flags=RRBMP_BGR)
	{
		if(!h) throw(rrerror("rrframe::init", "Invalid argument"));
		_flags=flags;
		h->size=h->framew*h->frameh*pixelsize;
		checkheader(h);
		if(pixelsize<3 || pixelsize>4) _throw("Only true color bitmaps are supported");
		if(h->framew!=_h.framew || h->frameh!=_h.frameh || pixelsize!=_pixelsize
		|| !_bits)
		{
			if(_bits) delete [] _bits;
			errifnot(_bits=new unsigned char[h->framew*h->frameh*pixelsize+1]);
			_pixelsize=pixelsize;  _pitch=pixelsize*h->framew;
		}
		memcpy(&_h, h, sizeof(rrframeheader));
	}

	rrframe *getstrip(int startline, int endline)
	{
		rrframe *f;
		if(!_bits || !_pitch || !_pixelsize) _throw("Frame not initialized");
		if(startline<0 || startline>_h.height-1 || endline<=0 || endline>_h.height)
			_throw("Invalid argument");
		errifnot(f=new rrframe(false));
		f->_h=_h;
		f->_h.height=endline-startline;
		f->_h.y+=startline;
		f->_pixelsize=_pixelsize;
		f->_flags=_flags;
		f->_pitch=_pitch;
		bool bu=(_flags&RRBMP_BOTTOMUP);
		f->_bits=&_bits[_pitch*(bu? _h.height-endline:startline)];
		return f;
	}

	void zero(void)
	{
		if(!_h.frameh || !_pitch) return;
		memset(_bits, 0, _pitch*_h.frameh);
	}

	bool stripequals(rrframe *last, int startline, int endline)
	{
		bool bu=(_flags&RRBMP_BOTTOMUP);
		if(last && _h.width==last->_h.width && _h.height==last->_h.height
		&& _h.framew==last->_h.framew && _h.frameh==last->_h.frameh
		&& _h.qual==last->_h.qual && _h.subsamp==last->_h.subsamp
		&& _pixelsize==last->_pixelsize && _h.winid==last->_h.winid
		&& _h.dpynum==last->_h.dpynum && _bits && last->_pitch==_pitch
		&& startline>=0 && startline<=_h.height-1 && endline>0 && endline<=_h.height
		&& _h.flags==last->_h.flags  // left & right eye can't be compared
		&& last->_bits && !memcmp(&_bits[_pitch*(bu? _h.height-endline:startline)],
			&last->_bits[_pitch*(bu? _h.height-endline:startline)],
			_pitch*(endline-startline))) return true;
		return false;
	}

	void ready(void) {_ready.unlock();}
	void waituntilready(void) {_ready.lock();}
	void complete(void) {_complete.unlock();}
	void waituntilcomplete(void) {_complete.lock();}

	rrframe& operator= (rrframe& f)
	{
		if(this!=&f && f._bits)
		{
			if(f._bits)
			{
				init(&f._h, f._pixelsize);
				memcpy(_bits, f._bits, f._h.framew*f._h.frameh*f._pixelsize);
			}
		}
		return *this;
	}

	rrframeheader _h;
	unsigned char *_bits;
	int _pitch, _pixelsize, _flags, _strip_height;

	protected:

	void dumpheader(rrframeheader *h)
	{
		rrout.print("h->size    = %lu\n", h->size);
		rrout.print("h->winid   = 0x%.8x\n", h->winid);
		rrout.print("h->dpynum  = %d\n", h->dpynum);
		rrout.print("h->framew  = %d\n", h->framew);
		rrout.print("h->frameh  = %d\n", h->frameh);
		rrout.print("h->width   = %d\n", h->width);
		rrout.print("h->height  = %d\n", h->height);
		rrout.print("h->x       = %d\n", h->x);
		rrout.print("h->y       = %d\n", h->y);
		rrout.print("h->qual    = %d\n", h->qual);
		rrout.print("h->subsamp = %d\n", h->subsamp);
		rrout.print("h->flags   = %d\n", h->flags);
	}

	void checkheader(rrframeheader *h)
	{
		if(!h || h->framew<1 || h->frameh<1 || h->width<1 || h->height<1
		|| h->x+h->width>h->framew || h->y+h->height>h->frameh)
			throw(rrerror("rrframe::checkheader", "Invalid header"));
	}

	rrmutex _ready;
	rrmutex _complete;
	friend class rrjpeg;
	bool _primary;
};

// Compressed JPEG

class rrjpeg : public rrframe
{
	public:

	rrjpeg(void) : rrframe(), _hpjhnd(NULL)
	{
		if(!(_hpjhnd=hpjInitCompress())) _throw(hpjGetErrorStr());
		_pixelsize=3;
	}

	~rrjpeg(void)
	{
		if(_hpjhnd) hpjDestroy(_hpjhnd);
	}

	rrjpeg& operator= (rrframe& b)
	{
		int hpjflags=0;
		if(!b._bits) _throw("Bitmap not initialized");
		if(b._pixelsize<3 || b._pixelsize>4) _throw("Only true color bitmaps are supported");
		if(b._h.qual>100 || b._h.subsamp>RR_SUBSAMPOPT-1)
			throw(rrerror("JPEG compressor", "Invalid argument"));
		init(&b._h);
		if(b._flags&RRBMP_BOTTOMUP) hpjflags|=HPJ_BOTTOMUP;
		if(b._flags&RRBMP_BGR) hpjflags|=HPJ_BGR;
		unsigned long size;
		hpj(hpjCompress(_hpjhnd, b._bits, b._h.width, b._pitch, b._h.height, b._pixelsize,
			_bits, &size, jpegsub[b._h.subsamp], b._h.qual, hpjflags));
		_h.size=(unsigned int)size;
		return *this;
	}

	void init(rrframeheader *h, int pixelsize)
	{
		init(h);
	}

	void init(rrframeheader *h)
	{
		checkheader(h);
		if(h->framew!=_h.framew || h->frameh!=_h.frameh || !_bits)
		{
			if(_bits) delete [] _bits;
			errifnot(_bits=new unsigned char[HPJBUFSIZE(h->framew, h->frameh)]);
		}
		memcpy(&_h, h, sizeof(rrframeheader));
	}

	private:

	hpjhandle _hpjhnd;
	friend class rrfb;
};

// Bitmap created from shared graphics memory

class rrfb : public rrframe
{
	public:

	rrfb(Display *dpy, Window win) : rrframe()
	{
		if(!dpy || !win) throw(rrerror("rrfb::rrfb", "Invalid argument"));
		XFlush(dpy);
		init(DisplayString(dpy), win);
	}

	rrfb(char *dpystring, Window win) : rrframe()
	{
		init(dpystring, win);
	}

	void init(char *dpystring, Window win)
	{
		_hpjhnd=NULL;
		memset(&_fb, 0, sizeof(fbx_struct));
		if(!dpystring || !win) throw(rrerror("rrfb::init", "Invalid argument"));
		if(!(_wh.dpy=XOpenDisplay(dpystring)))
			throw(rrerror("rrfb::init", "Could not open display"));
		_wh.win=win;
	}

	~rrfb(void)
	{
		if(_fb.bits) fbx_term(&_fb);  if(_bits) _bits=NULL;
		if(_hpjhnd) hpjDestroy(_hpjhnd);
		if(_wh.dpy) XCloseDisplay(_wh.dpy);
	}

	void init(rrframeheader *h)
	{
		checkheader(h);
		fbx(fbx_init(&_fb, _wh, h->framew, h->frameh, 1));
		if(h->framew>_fb.width || h->frameh>_fb.height)
		{
			XSync(_wh.dpy, False);
			fbx(fbx_init(&_fb, _wh, h->framew, h->frameh, 1));
		}
		memcpy(&_h, h, sizeof(rrframeheader));
		if(_h.framew>_fb.width) _h.framew=_fb.width;
		if(_h.frameh>_fb.height) _h.frameh=_fb.height;
		_pixelsize=fbx_ps[_fb.format];  _pitch=_fb.pitch;  _bits=(unsigned char *)_fb.bits;
		_flags=0;
		if(fbx_bgr[_fb.format]) _flags|=RRBMP_BGR;
		if(fbx_alphafirst[_fb.format]) _flags|=RRBMP_ALPHAFIRST;
	}

	rrfb& operator= (rrjpeg& f)
	{
		int hpjflags=0;
		if(!f._bits || f._h.size<1)
			_throw("JPEG not initialized");
		init(&f._h);
		if(!_fb.xi) _throw("Bitmap not initialized");
		if(fbx_bgr[_fb.format]) hpjflags|=HPJ_BGR;
		if(fbx_alphafirst[_fb.format]) hpjflags|=HPJ_ALPHAFIRST;
		int width=min(f._h.width, _fb.width-f._h.x);
		int height=min(f._h.height, _fb.height-f._h.y);
		if(width>0 && height>0 && f._h.width<=width && f._h.height<=height)
		{
			if(!_hpjhnd)
			{
				if((_hpjhnd=hpjInitDecompress())==NULL) throw(rrerror("rrfb::decompressor", hpjGetErrorStr()));
			}
			hpj(hpjDecompress(_hpjhnd, f._bits, f._h.size, (unsigned char *)&_fb.bits[_fb.pitch*f._h.y+f._h.x*fbx_ps[_fb.format]],
				width, _fb.pitch, height, fbx_ps[_fb.format], hpjflags));
		}
		return *this;
	}

	void redraw(void)
	{
		if(_flags&RRBMP_BOTTOMUP)
		{
			for(int i=0; i<_fb.height; i++)
				fbx(fbx_awrite(&_fb, 0, _fb.height-i-1, 0, i, 0, 1));
			fbx(fbx_sync(&_fb));
		}
		else
			fbx(fbx_write(&_fb, 0, 0, 0, 0, _fb.width, _fb.height));
	}

	void draw(void)
	{
		int w=_h.width, h=_h.height;
		XWindowAttributes xwa;
		if(!XGetWindowAttributes(_wh.dpy, _wh.win, &xwa))
		{
			rrout.print("Failed to get window attributes\n");
			return;
		}
		if(_h.x+_h.width>xwa.width || _h.y+_h.height>xwa.height)
		{
			rrout.print("WARNING: bitmap (%d,%d) at (%d,%d) extends beyond window (%d,%d)\n",
				_h.width, _h.height, _h.x, _h.y, xwa.width, xwa.height);
			w=min(_h.width, xwa.width-_h.x);
			h=min(_h.height, xwa.height-_h.y);
		}
		if(_h.x+_h.width<=_fb.width && _h.y+_h.height<=_fb.height)
		fbx(fbx_write(&_fb, _h.x, _h.y, _h.x, _h.y, w, h));
	}

	void drawstrip(int startline, int endline)
	{
		if(startline<0 || startline>_h.frameh-1 || endline<0 || endline>_h.frameh)
			return;
		if(_flags&RRBMP_BOTTOMUP)
		{
			for(int i=startline; i<endline; i++)
				fbx(fbx_awrite(&_fb, 0, _fb.height-i-1, 0, i, 0, 1));
		}
		else
		fbx(fbx_awrite(&_fb, 0, startline, 0, startline, 0, endline-startline));
	}

	void sync(void) {fbx(fbx_sync(&_fb));}

	private:

	fbx_wh _wh;
	fbx_struct _fb;
	hpjhandle _hpjhnd;
};

// Bitmap drawn using OpenGL

#ifdef USEGL
#include <GL/glx.h>

class rrglframe : public rrframe
{
	public:

	rrglframe(char *dpystring, Window win) : rrframe(), _rbits(NULL),
		_madecurrent(false), _stereo(false), _dpy(NULL), _win(win), _ctx(0),
		_hpjhnd(NULL)
	{
		XVisualInfo *v=NULL;
		try
		{
			if(!dpystring || !win) throw(rrerror("rrglframe::rrglframe", "Invalid argument"));
			if(!(_dpy=XOpenDisplay(dpystring))) _throw("Could not open display");
			_pixelsize=3;
			XWindowAttributes xwa;
			XGetWindowAttributes(_dpy, _win, &xwa);
			XVisualInfo vtemp;  int n=0;
			if(!xwa.visual) _throw("Could not get window attributes");
			vtemp.visualid=xwa.visual->visualid;
			int maj_opcode=-1, first_event=-1, first_error=-1;
			if(!XQueryExtension(_dpy, "GLX", &maj_opcode, &first_event, &first_error)
			|| maj_opcode<0 || first_event<0 || first_error<0)
				_throw("GLX extension not available");
			if(!(v=XGetVisualInfo(_dpy, VisualIDMask, &vtemp, &n)) || n==0)
				_throw("Could not obtain visual");
			if(!(_ctx=glXCreateContext(_dpy, v, NULL, True)))
				_throw("Could not create GLX context");
			XFree(v);  v=NULL;
		}
		catch(...)
		{
			if(_dpy && _ctx) {glXMakeCurrent(_dpy, 0, 0);  glXDestroyContext(_dpy, _ctx);  _ctx=0;}
			if(v) XFree(v);
			if(_dpy) {XCloseDisplay(_dpy);  _dpy=NULL;}
			throw;
		}
	}

	~rrglframe(void)
	{
		if(_ctx && _dpy) {glXMakeCurrent(_dpy, 0, 0);  glXDestroyContext(_dpy, _ctx);  _ctx=0;}
		if(_dpy) {XCloseDisplay(_dpy);  _dpy=NULL;}
		if(_hpjhnd) {hpjDestroy(_hpjhnd);  _hpjhnd=NULL;}
		if(_rbits) {delete [] _rbits;  _rbits=NULL;}
	}

	void init(rrframeheader *h)
	{
		_flags=RRBMP_BOTTOMUP;
		#ifdef GL_BGR_EXT
		if(littleendian()) _flags|=RRBMP_BGR;
		#endif
		if(!h) throw(rrerror("rrglframe::init", "Invalid argument"));
		_pixelsize=3;  _pitch=_pixelsize*h->framew;
		checkheader(h);
		if(h->framew!=_h.framew || h->frameh!=_h.frameh)
		{
			if(_bits) delete [] _bits;
			errifnot(_bits=new unsigned char[_pitch*h->frameh+1]);
			if(h->flags==RR_LEFT || h->flags==RR_RIGHT)
			{
				if(_rbits) delete [] _rbits;
				errifnot(_rbits=new unsigned char[_pitch*h->frameh+1]);
			}
		}
		memcpy(&_h, h, sizeof(rrframeheader));
	}

	rrglframe& operator= (rrjpeg& f)
	{
		int hpjflags=HPJ_BOTTOMUP;
		if(!f._bits || f._h.size<1) _throw("JPEG not initialized");
		init(&f._h);
		if(!_bits) _throw("Bitmap not initialized");
		if(_flags&RRBMP_BGR) hpjflags|=HPJ_BGR;
		int width=min(f._h.width, _h.framew-f._h.x);
		int height=min(f._h.height, _h.frameh-f._h.y);
		if(width>0 && height>0 && f._h.width<=width && f._h.height<=height)
		{
			if(!_hpjhnd)
			{
				if((_hpjhnd=hpjInitDecompress())==NULL) throw(rrerror("rrglframe::decompressor", hpjGetErrorStr()));
			}
			int y=max(0, _h.frameh-f._h.y-height);
			unsigned char *dstbuf=_bits;
			if(f._h.flags==RR_RIGHT)
			{
				_stereo=true;  dstbuf=_rbits;
			}
			hpj(hpjDecompress(_hpjhnd, f._bits, f._h.size, (unsigned char *)&dstbuf[_pitch*y+f._h.x*_pixelsize],
				width, _pitch, height, _pixelsize, hpjflags));
		}
		return *this;
	}

	void redraw(void)
	{
		int format=GL_RGB;
		#ifdef GL_BGR_EXT
		if(littleendian()) format=GL_BGR_EXT;
		#endif
		if(!_madecurrent)
		{
			if(!glXMakeCurrent(_dpy, _win, _ctx))
				_throw("Could not make context current");
			_madecurrent=true;
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		int oldbuf=-1;
		glGetIntegerv(GL_DRAW_BUFFER, &oldbuf);
		if(_stereo) glDrawBuffer(GL_BACK_LEFT);
		glDrawPixels(_h.framew, _h.frameh, format, GL_UNSIGNED_BYTE, _bits);
		if(_stereo)
		{
			glDrawBuffer(GL_BACK_RIGHT);
			glDrawPixels(_h.framew, _h.frameh, format, GL_UNSIGNED_BYTE, _rbits);
			glDrawBuffer(oldbuf);
			_stereo=false;
		}
		glXSwapBuffers(_dpy, _win);
	}

	unsigned char *_rbits;

	private:

	bool _madecurrent, _stereo;
	Display *_dpy;  Window _win;
	GLXContext _ctx;
	hpjhandle _hpjhnd;
};
#endif

#endif
