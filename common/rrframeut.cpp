/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2011 D. R. Commander
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

#include "rrthread.h"
#include "rrframe.h"
#include "../client/rrglframe.h"
#include "rrtimer.h"
#include "bmp.h"

#define ITER 50
#define NB 2
#define MINW 1
#define MAXW 400
#define BORDER 0
#define NUMWIN 1

bool usegl=false, usexv=false, pctest=false, usergb=false;

void resizewindow(Display *dpy, Window win, int w, int h, int myid)
{
	XWindowAttributes xwa;
	XGetWindowAttributes(dpy, win, &xwa);
	if(w!=xwa.width || h!=xwa.height)
	{
		XLockDisplay(dpy);
		XWindowChanges xwc;
		xwc.width=w;  xwc.height=h;
		xwc.x=(w+BORDER*2)*myid;  xwc.y=0;
		XConfigureWindow(dpy, win, CWWidth|CWHeight|CWX|CWY, &xwc);
		XFlush(dpy);
		XSync(dpy, False);
		XUnlockDisplay(dpy);
	}
}

class blitter : public Runnable
{
	public:

	blitter(Display *dpy, Window win, int myid) : _indx(0), _deadyet(false),
		_dpy(dpy), _win(win), _t(NULL), _myid(myid)
	{
		for(int i=0; i<NB; i++)
		{
			if(usegl) {errifnot(_fb[i]=new rrglframe(_dpy, _win));}
			#ifdef USEXV
			else if(usexv) {errifnot(_fb[i]=new rrxvframe(_dpy, _win));}
			#endif
			else {errifnot(_fb[i]=new rrfb(_dpy, _win));}
		}
		errifnot(_t=new Thread(this));
		_t->start();
	}

	virtual ~blitter(void)
	{
		shutdown();
		for(int i=0; i<NB; i++)
		{
			if(_fb[i])
			{
				if(_fb[i]->_isgl) delete (rrglframe *)_fb[i];
				#ifdef USEXV
				else if(_fb[i]->_isxv) delete (rrxvframe *)_fb[i];
				#endif
				else delete (rrfb *)_fb[i];
				_fb[i]=NULL;
			}
		}
	}

	rrframe *get(void)
	{
		rrframe *b=_fb[_indx];  _indx=(_indx+1)%NB;
		if(_t) _t->checkerror();  b->waituntilcomplete();  if(_t) _t->checkerror();
		return b;
	}

	void put(rrframe *b)
	{
		if(_t) _t->checkerror();
		b->ready();
	}

	void shutdown(void)
	{
		_deadyet=true;
		int i;
		for(i=0; i<NB; i++) _fb[i]->ready();  // Release my thread
		if(_t) {_t->stop();  delete _t;  _t=NULL;}
		for(i=0; i<NB; i++) _fb[i]->complete();  // Release decompressor
	}

	private:

	void run(void)
	{
		rrtimer timer;
		double mpixels=0., totaltime=0.;
		int buf=0;  rrframe *b;
		try
		{
			while(!_deadyet)
			{
				b=_fb[buf];  buf=(buf+1)%NB;
				b->waituntilready();  if(_deadyet) break;
				if(usexv) resizewindow(_dpy, _win, b->_h.width, b->_h.height, _myid);
				timer.start();
				if(b->_isgl) ((rrglframe *)b)->redraw();
				#ifdef USEXV
				else if(b->_isxv) ((rrxvframe *)b)->redraw();
				#endif
				else ((rrfb *)b)->redraw();
				mpixels+=(double)(b->_h.width*b->_h.height)/1000000.;
				totaltime+=timer.elapsed();
				b->complete();
			}
			fprintf(stderr, "Average blitter performance = %f Mpixels/sec\n", mpixels/totaltime);
		}
		catch (...)
		{
			for(int i=0; i<NB; i++) if(_fb[i]) _fb[i]->complete();  throw;
		}
		fprintf(stderr, "Blitter exiting ...\n");
	}

	int _indx;  bool _deadyet;  
	rrframe *_fb[NB];
	Display *_dpy;  Window _win;
	Thread *_t;
	int _myid;
};

class decompressor : public Runnable
{
	public:

	decompressor(blitter *bobj, Display *dpy, Window win, int myid) : _bobj(bobj),
		_indx(0), _deadyet(false), _dpy(dpy), _win(win), _myid(myid), _t(NULL)
	{
		errifnot(_t=new Thread(this));
		_t->start();
	}

	virtual ~decompressor(void) {shutdown();}

	rrcompframe &get(void)
	{
		rrcompframe &c=_cf[_indx];  _indx=(_indx+1)%NB;
		if(_deadyet) return c;
		if(_t) _t->checkerror();  c.waituntilcomplete();  if(_t) _t->checkerror();
		return c;
	}

	void put(rrcompframe &c)
	{
		if(_t) _t->checkerror();
		c.ready();
	}

	void shutdown(void)
	{
		_deadyet=true;
		int i;
		for(i=0; i<NB; i++) _cf[i].ready();  // Release my thread
		if(_t) {_t->stop();  delete _t;  _t=NULL;}
		for(i=0; i<NB; i++) _cf[i].complete();  // Release compressor
	}

	private:

	void run(void)
	{
		int buf=0;  rrframe *b=NULL;
		try
		{
			while(!_deadyet)
			{
				rrcompframe &c=_cf[buf];  buf=(buf+1)%NB;
				c.waituntilready();  if(_deadyet) break;
				b=_bobj->get();  if(_deadyet) break;
				resizewindow(_dpy, _win, c._h.width, c._h.height, _myid);
				if(b->_isgl) *((rrglframe *)b)=c;
				#ifdef USEXV
				else if(b->_isxv) *((rrxvframe *)b)=c;
				#endif
				else *((rrfb *)b)=c;
				_bobj->put(b);
				c.complete();
			}
		}
		catch (...)
		{
			for(int i=0; i<NB; i++) _cf[i].complete();  throw;
		}
		fprintf(stderr, "Decompressor exiting ...\n");
	}

	blitter *_bobj;
	int _indx;  bool _deadyet;
	Display *_dpy;  Window _win;
	rrcompframe _cf[NB];
	int _myid;  Thread *_t;
};

class compressor : public Runnable
{
	public:

	compressor(decompressor *dobj, blitter *bobj) : _indx(0), _deadyet(false),
		_t(NULL), _dobj(dobj), _bobj(bobj)
	{
		errifnot(_t=new Thread(this));
		_t->start();
	}

	virtual ~compressor(void)
	{
		if(_t) _t->stop();
	}

	rrframe &get(int w, int h)
	{
		rrframe &f=_bmp[_indx];  _indx=(_indx+1)%NB;
		if(_t) _t->checkerror();  f.waituntilcomplete();  if(_t) _t->checkerror();
		rrframeheader hdr;
		hdr.framew=hdr.width=w+BORDER;
		hdr.frameh=hdr.height=h+BORDER;
		hdr.x=hdr.y=BORDER;
		hdr.qual=80;
		hdr.subsamp=2;
		hdr.compress=usergb? RRCOMP_RGB:RRCOMP_JPEG;
		if(usexv) hdr.compress=RRCOMP_YUV;
		f.init(hdr, 3, (littleendian() && !usergb && !usexv)? RRBMP_BGR:0);
		return f;
	}

	void put(rrframe &f)
	{
		if(_t) _t->checkerror();
		f.ready();
	}

	void shutdown(void)
	{
		_deadyet=true;
		int i;
		for(i=0; i<NB; i++) _bmp[i].ready();  // Release my thread
		if(_t) {_t->stop();  delete _t;  _t=NULL;}
		for(i=0; i<NB; i++) _bmp[i].complete();  // Release main thread
	}

	private:

	void run(void)
	{
		int buf=0;
		try
		{
			while(!_deadyet)
			{
				rrframe &f=_bmp[buf];  buf=(buf+1)%NB;
				f.waituntilready();  if(_deadyet) break;
				#ifdef USEXV
				if(usexv)
				{
					rrxvframe *c=(rrxvframe *)_bobj->get();  if(_deadyet) break;
					*c=f;
					_bobj->put(c);
				}
				else
				#endif
				{
					rrcompframe &c=_dobj->get();  if(_deadyet) break;
					c=f;
					_dobj->put(c);
				}
				f.complete();
			}
		}
		catch (...)
		{
			for(int i=0; i<NB; i++) _bmp[i].complete();  throw;
		}
		fprintf(stderr, "Compressor exiting ...\n");
	}

	int _indx;  bool _deadyet;
	Thread *_t;
	rrframe _bmp[NB];
	decompressor *_dobj;
	blitter *_bobj;
};

class rrframeut
{
	public:

	rrframeut(Display *dpy, int myid) : c(NULL), d(NULL), b(NULL)
	{
		Window win;
		errifnot(win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), myid*(MINW+BORDER*2), 0, MINW+BORDER, MINW+BORDER, 0,
			WhitePixel(dpy, DefaultScreen(dpy)), BlackPixel(dpy, DefaultScreen(dpy))));
		errifnot(XMapRaised(dpy, win));

		errifnot(b=new blitter(dpy, win, myid));
		if(!usexv) errifnot(d=new decompressor(b, dpy, win, myid));
		errifnot(c=new compressor(d, b));
	}

	~rrframeut(void) {shutdown();}

	void dotest(int w, int h, int seed)
	{
		rrframe &f=c->get(w, h);
		initbmp(f, seed);
		c->put(f);
	}

	private:

	void initbmp(rrframe &f, int seed)
	{
		int i, j, pitch=f._pitch;
		for(i=0; i<f._h.height; i++)
		{
			for(j=0; j<f._h.width; j++)
			{
				f._bits[i*pitch+j*f._pixelsize]=(i+seed)%256;
				f._bits[i*pitch+j*f._pixelsize+1]=(j+seed)%256;
				f._bits[i*pitch+j*f._pixelsize+2]=(i+j+seed)%256;
			}	
		}
	}

	void shutdown(void)
	{
		fprintf(stderr, "Shutting down....\n");  fflush(stderr);
		if(b) b->shutdown();  if(d) d->shutdown();  if(c) c->shutdown();
		if(b) {delete b;  b=NULL;}
		if(d) {delete d;  d=NULL;}
		if(c) {delete c;  c=NULL;}
	}

	compressor *c;  decompressor *d;  blitter *b;
};

static const char *bmpformatname[BMPPIXELFORMATS]=
	{"RGB", "RGBA", "BGR", "BGRA", "ABGR", "ARGB"};
static const int bmpps[BMPPIXELFORMATS]={3, 4, 3, 4, 4, 4};
static const int bmproff[BMPPIXELFORMATS]={0, 0, 2, 2, 3, 1};
static const int bmpgoff[BMPPIXELFORMATS]={1, 1, 1, 1, 2, 2};
static const int bmpboff[BMPPIXELFORMATS]={2, 2, 0, 0, 1, 3};
static const int bmpflags[BMPPIXELFORMATS]=
	{0, 0, RRBMP_BGR, RRBMP_BGR, RRBMP_ALPHAFIRST|RRBMP_BGR, RRBMP_ALPHAFIRST};

int cmpframe(unsigned char *buf, int w, int h, rrframe &dst,
	BMPPIXELFORMAT dstpf)
{
	int _i;  int dstbu=((dst._flags&RRBMP_BOTTOMUP)!=0);  int pitch=w*3;
	for(int i=0; i<h; i++)
	{
		_i=dstbu? i:h-i-1;
		for(int j=0; j<h; j++)
		{
			if((buf[pitch*_i+j*3]
				!= dst._bits[dst._pitch*i+j*bmpps[dstpf]+bmproff[dstpf]])
				|| (buf[pitch*_i+j*3+1]
				!= dst._bits[dst._pitch*i+j*bmpps[dstpf]+bmpgoff[dstpf]])
				|| (buf[pitch*_i+j*3+2]
				!= dst._bits[dst._pitch*i+j*bmpps[dstpf]+bmpboff[dstpf]]))
				return 1;
		}
 	}
	return 0;
}

void dopctest(char *filename)
{
	unsigned char *buf;  int w, h, dstbu;
	rrcompframe src;  rrframe dst;  int dstpf;

	for(dstpf=0; dstpf<BMPPIXELFORMATS; dstpf++)
	{
		int dstflags=bmpflags[dstpf];
		for(dstbu=0; dstbu<2; dstbu++)
		{
			if(dstbu) dstflags|=RRBMP_BOTTOMUP;
			if(loadbmp(filename, &buf, &w, &h, BMP_RGB, 1, 1)==-1)
				_throw(bmpgeterr());
			rrframeheader _h;
			memset(&_h, 0, sizeof(_h));
			_h.width=_h.framew=w;  _h.height=_h.frameh=h;  _h.x=_h.y=0;
			_h.compress=RRCOMP_RGB;  _h.flags=0;  _h.size=w*3*h;
			src.init(_h, _h.flags);
			memcpy(src._bits, buf, w*3*h);
			dst.init(_h, bmpps[dstpf], dstflags);
			memset(dst._bits, 0, dst._pitch*dst._h.frameh);
			fprintf(stderr, "RGB (BOTTOM-UP) -> %s (%s)\n", bmpformatname[dstpf],
				dstbu? "BOTTOM-UP":"TOP-DOWN");
			double tstart, ttotal=0.;  int iter=0;
			do
			{
				tstart=rrtime();
				dst.decompressrgb(src, w, h, false);
				ttotal+=rrtime()-tstart;  iter++;
			} while(ttotal<1.);
			fprintf(stderr, "%f Mpixels/sec - ", (double)w*(double)h
				*(double)iter/1000000./ttotal);
			if(cmpframe(buf, w, h, dst, (BMPPIXELFORMAT)dstpf))
				fprintf(stderr, "FAILED!\n");
			else fprintf(stderr, "Passed.\n");
			free(buf);
		}
		fprintf(stderr, "\n");
	}

}

int main(int argc, char **argv)
{
	Display *dpy=NULL;
	rrframeut *test[NUMWIN];
	int i, j, w, h;
	char *bmpfile=NULL;

	if(argc>1)
	{
		for(i=1; i<argc; i++)
		{
			if(!stricmp(argv[i], "-gl")) usegl=true;
			#ifdef USEXV
			else if(!stricmp(argv[i], "-xv")) usexv=true;
			#endif
			else if(!stricmp(argv[i], "-rgb")) usergb=true;
			else if(!stricmp(argv[i], "-pc"))
			{
				if(i>=argc-1)
				{
					fprintf(stderr, "USAGE: %s -pc <filename>\n", argv[0]);
					exit(1);
				}
				bmpfile=argv[++i];  pctest=true;
			}
		}
	}

	try
	{
		if(pctest) {dopctest(bmpfile);  exit(0);}

		errifnot(XInitThreads());
		#ifdef SUNOGL
		errifnot(glXInitThreadsSUN());
		#endif
		if(!(dpy=XOpenDisplay(0))) {fprintf(stderr, "Could not open display %s\n", XDisplayName(0));  exit(1);}

		for(i=0; i<NUMWIN; i++)
		{
			errifnot(test[i]=new rrframeut(dpy, i));
		}

		for(w=MINW; w<=MAXW; w+=33)
		{
			h=1;
			fprintf(stderr, "%.4d x %.4d: ", w, h);
			for(i=0; i<ITER; i++)
			{
				fprintf(stderr, ".");
				for(j=0; j<NUMWIN; j++) test[j]->dotest(w, h, i);
			}
			fprintf(stderr, "\n");
		}

		for(h=MINW; h<=MAXW; h+=33)
		{
			w=1;
			fprintf(stderr, "%.4d x %.4d: ", w, h);
			for(i=0; i<ITER; i++)
			{
				fprintf(stderr, ".");
				for(j=0; j<NUMWIN; j++) test[j]->dotest(w, h, i);
			}
			fprintf(stderr, "\n");
		}

		for(w=MINW; w<=MAXW; w+=33)
		{
			h=w;
			fprintf(stderr, "%.4d x %.4d: ", w, h);
			for(i=0; i<ITER; i++)
			{
				fprintf(stderr, ".");
				for(j=0; j<NUMWIN; j++) test[j]->dotest(w, h, i);
			}
			fprintf(stderr, "\n");
		}

		for(i=0; i<NUMWIN; i++)
		{
			delete test[i];
		}
	}
	catch (rrerror &e)
	{
		fprintf(stderr, "%s\n%s\n", e.getMethod(), e.getMessage());
		exit(1);
	}	
	return 0;
}
