/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005 Sun Microsystems, Inc.
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

#include "rrthread.h"
#include "rrframe.h"
#undef USEGLP
#include "rrglframe.h"
#include "rrtimer.h"

#define ITER 50
#define NB 2
#define MINW 1
#define MAXW 400
#define BORDER 0
#define NUMWIN 3

class blitter : public Runnable
{
	public:

	blitter(Display *dpy, Window win, bool usegl=false) : _indx(0), _deadyet(false), _dpy(dpy),
		_win(win), _t(NULL)
	{
		for(int i=0; i<NB; i++)
		{
			if(usegl) {errifnot(_fb[i]=new rrglframe(_dpy, _win));}
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
				timer.start();
				if(b->_isgl) ((rrglframe *)b)->redraw();
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

	rrjpeg &get(void)
	{
		rrjpeg &j=_jpg[_indx];  _indx=(_indx+1)%NB;
		if(_deadyet) return j;
		if(_t) _t->checkerror();  j.waituntilcomplete();  if(_t) _t->checkerror();
		return j;
	}

	void put(rrjpeg &j)
	{
		if(_t) _t->checkerror();
		j.ready();
	}

	void shutdown(void)
	{
		_deadyet=true;
		int i;
		for(i=0; i<NB; i++) _jpg[i].ready();  // Release my thread
		if(_t) {_t->stop();  delete _t;  _t=NULL;}
		for(i=0; i<NB; i++) _jpg[i].complete();  // Release compressor
	}

	private:

	void run(void)
	{
		int buf=0;  rrframe *b=NULL;
		try
		{
			while(!_deadyet)
			{
				rrjpeg &j=_jpg[buf];  buf=(buf+1)%NB;
				j.waituntilready();  if(_deadyet) break;
				b=_bobj->get();  if(_deadyet) break;
				XWindowAttributes xwa;
				int w=j._h.width, h=j._h.height;
				XGetWindowAttributes(_dpy, _win, &xwa);
				if(w!=xwa.width || h!=xwa.height)
				{
					XLockDisplay(_dpy);
					XWindowChanges xwc;
					xwc.width=w;  xwc.height=h;
					xwc.x=(w+BORDER*2)*_myid;  xwc.y=0;
					XConfigureWindow(_dpy, _win, CWWidth|CWHeight|CWX|CWY, &xwc);
					XFlush(_dpy);
					XSync(_dpy, False);
//					XWindowAttributes xwa;
//					do XGetWindowAttributes(_dpy, _win, &xwa); while(xwa.width<w || xwa.height<h);
					XUnlockDisplay(_dpy);
				}
				if(b->_isgl) *((rrglframe *)b)=j;
				else *((rrfb *)b)=j;
				_bobj->put(b);
				j.complete();
			}
		}
		catch (...)
		{
			for(int i=0; i<NB; i++) _jpg[i].complete();  throw;
		}
		fprintf(stderr, "Decompressor exiting ...\n");
	}

	blitter *_bobj;
	int _indx;  bool _deadyet;
	Display *_dpy;  Window _win;
	rrjpeg _jpg[NB];
	int _myid;  Thread *_t;
};

class compressor : public Runnable
{
	public:

	compressor(decompressor *dobj) : _indx(0), _deadyet(false), _t(NULL), _dobj(dobj)
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
		hdr.subsamp=RR_422;
		f.init(&hdr, 3);
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
				rrjpeg &j=_dobj->get();  if(_deadyet) break;
				j=f;
				_dobj->put(j);
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
};

class rrframeut
{
	public:

	rrframeut(Display *dpy, int myid, bool usegl=false) : c(NULL), d(NULL), b(NULL)
	{
		Window win;
		errifnot(win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), myid*(MINW+BORDER*2), 0, MINW+BORDER, MINW+BORDER, 0,
			WhitePixel(dpy, DefaultScreen(dpy)), BlackPixel(dpy, DefaultScreen(dpy))));
		errifnot(XMapRaised(dpy, win));

		errifnot(b=new blitter(dpy, win, usegl));
		errifnot(d=new decompressor(b, dpy, win, myid));
		errifnot(c=new compressor(d));
	}

	~rrframeut(void) {shutdown();}

	void dotest(int w, int h)
	{
		rrframe &f=c->get(w, h);
		initbmp(f);
		c->put(f);
	}

	private:

	void initbmp(rrframe &f)
	{
		int i, j, pitch=f._pitch;
		for(i=0; i<f._h.height; i++)
		{
			for(j=0; j<f._h.width; j++)
			{
				f._bits[i*pitch+j*f._pixelsize]=i%256;
				f._bits[i*pitch+j*f._pixelsize+1]=j%256;
				f._bits[i*pitch+j*f._pixelsize+2]=(i+j)%256;
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

int main(int argc, char **argv)
{
	Display *dpy=NULL;
	rrframeut *test[NUMWIN];
	int i, j, w, h;
	bool usegl=false;

	if(argc>1) for(i=1; i<argc; i++) if(!stricmp(argv[i], "-gl")) usegl=true;

	try
	{
		errifnot(XInitThreads());
		#ifdef SUNOGL
		errifnot(glXInitThreadsSUN());
		#endif
		if(!(dpy=XOpenDisplay(0))) {fprintf(stderr, "Could not open display %s\n", XDisplayName(0));  exit(1);}

		for(i=0; i<NUMWIN; i++)
		{
			errifnot(test[i]=new rrframeut(dpy, i, usegl));
		}

		for(w=MINW; w<=MAXW; w+=33)
		{
			h=1;
			fprintf(stderr, "%.4d x %.4d: ", w, h);
			for(i=0; i<ITER; i++)
			{
				fprintf(stderr, ".");
				for(j=0; j<NUMWIN; j++) test[j]->dotest(w, h);
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
				for(j=0; j<NUMWIN; j++) test[j]->dotest(w, h);
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
				for(j=0; j<NUMWIN; j++) test[j]->dotest(w, h);
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
