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

#include "rrthread.h"
#include "rrframe.h"
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

	blitter(Display *_dpy, Window _win) : indx(0), deadyet(false), dpy(_dpy),
		win(_win), t(NULL)
	{
		for(int i=0; i<NB; i++) errifnot(fb[i]=new rrfb(dpy, win));
		errifnot(t=new Thread(this));
		t->start();
	}

	virtual ~blitter(void)
	{
		shutdown();
		for(int i=0; i<NB; i++) {if(fb[i]) {delete fb[i];  fb[i]=NULL;}}
	}

	rrfb *get(void)
	{
		rrfb *b=fb[indx];  indx=(indx+1)%NB;
		if(t) t->checkerror();  b->waituntilcomplete();  if(t) t->checkerror();
		return b;
	}

	void put(rrfb *b)
	{
		if(t) t->checkerror();
		b->ready();
	}

	void shutdown(void)
	{
		deadyet=true;
		int i;
		for(i=0; i<NB; i++) fb[i]->ready();  // Release my thread
		if(t) {t->stop();  delete t;  t=NULL;}
		for(i=0; i<NB; i++) fb[i]->complete();  // Release decompressor
	}

	private:

	void run(void)
	{
		rrtimer timer;
		double mpixels=0., totaltime=0.;
		int buf=0;  rrfb *b;
		try
		{
			while(!deadyet)
			{
				b=fb[buf];  buf=(buf+1)%NB;
				b->waituntilready();  if(deadyet) break;
				timer.start();
				b->redraw();
				mpixels+=(double)(b->h.width*b->h.height)/1000000.;
				totaltime+=timer.elapsed();
				b->complete();
			}
			fprintf(stderr, "Average blitter performance = %f Mpixels/sec\n", mpixels/totaltime);
		}
		catch (...)
		{
			for(int i=0; i<NB; i++) if(fb[i]) fb[i]->complete();  throw;
		}
		fprintf(stderr, "Blitter exiting ...\n");
	}

	int indx;  bool deadyet;  
	rrfb *fb[NB];
	Display *dpy;  Window win;
	Thread *t;
};

class decompressor : public Runnable
{
	public:

	decompressor(blitter *_bobj, Display *_dpy, Window _win, int _myid) : bobj(_bobj),
		indx(0), deadyet(false), dpy(_dpy), win(_win), myid(_myid), t(NULL)
	{
		errifnot(t=new Thread(this));
		t->start();
	}

	virtual ~decompressor(void) {shutdown();}

	rrjpeg &get(void)
	{
		rrjpeg &j=jpg[indx];  indx=(indx+1)%NB;
		if(deadyet) return j;
		if(t) t->checkerror();  j.waituntilcomplete();  if(t) t->checkerror();
		return j;
	}

	void put(rrjpeg &j)
	{
		if(t) t->checkerror();
		j.ready();
	}

	void shutdown(void)
	{
		deadyet=true;
		int i;
		for(i=0; i<NB; i++) jpg[i].ready();  // Release my thread
		if(t) {t->stop();  delete t;  t=NULL;}
		for(i=0; i<NB; i++) jpg[i].complete();  // Release compressor
	}

	private:

	void run(void)
	{
		int buf=0;  rrfb *b=NULL;
		try
		{
			while(!deadyet)
			{
				rrjpeg &j=jpg[buf];  buf=(buf+1)%NB;
				j.waituntilready();  if(deadyet) break;
				b=bobj->get();  if(deadyet) break;
				XWindowAttributes xwa;
				int w=j.h.width, h=j.h.height;
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
					XWindowAttributes xwa;
					do XGetWindowAttributes(dpy, win, &xwa); while(xwa.width<w || xwa.height<h);
					XUnlockDisplay(dpy);
				}
				(*b)=j;
				bobj->put(b);
				j.complete();
			}
		}
		catch (...)
		{
			for(int i=0; i<NB; i++) jpg[i].complete();  throw;
		}
		fprintf(stderr, "Decompressor exiting ...\n");
	}

	blitter *bobj;
	int indx;  bool deadyet;
	Display *dpy;  Window win;
	rrjpeg jpg[NB];
	int myid;  Thread *t;
};

class compressor : public Runnable
{
	public:

	compressor(decompressor *_dobj) : indx(0), deadyet(false), t(NULL), dobj(_dobj)
	{
		errifnot(t=new Thread(this));
		t->start();
	}

	virtual ~compressor(void)
	{
		if(t) t->stop();
	}

	rrframe &get(int w, int h)
	{
		rrframe &f=bmp[indx];  indx=(indx+1)%NB;
		if(t) t->checkerror();  f.waituntilcomplete();  if(t) t->checkerror();
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
		if(t) t->checkerror();
		f.ready();
	}

	void shutdown(void)
	{
		deadyet=true;
		int i;
		for(i=0; i<NB; i++) bmp[i].ready();  // Release my thread
		if(t) {t->stop();  delete t;  t=NULL;}
		for(i=0; i<NB; i++) bmp[i].complete();  // Release main thread
	}

	private:

	void run(void)
	{
		int buf=0;
		try
		{
			while(!deadyet)
			{
				rrframe &f=bmp[buf];  buf=(buf+1)%NB;
				f.waituntilready();  if(deadyet) break;
				rrjpeg &j=dobj->get();  if(deadyet) break;
				j=f;
				dobj->put(j);
				f.complete();
			}
		}
		catch (...)
		{
			for(int i=0; i<NB; i++) bmp[i].complete();  throw;
		}
		fprintf(stderr, "Compressor exiting ...\n");
	}

	int indx;  bool deadyet;
	Thread *t;
	rrframe bmp[NB];
	decompressor *dobj;
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

		errifnot(b=new blitter(dpy, win));
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
		int i, j, pitch=f.pitch;
		for(i=0; i<f.h.height; i++)
		{
			for(j=0; j<f.h.width; j++)
			{
				f.bits[i*pitch+j*f.pixelsize]=i%256;
				f.bits[i*pitch+j*f.pixelsize+1]=j%256;
				f.bits[i*pitch+j*f.pixelsize+2]=(i+j)%256;
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

int main(void)
{
	Display *dpy=NULL;
	rrframeut *test[NUMWIN];
	int i, j, w, h;

	try
	{
		errifnot(XInitThreads());
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
