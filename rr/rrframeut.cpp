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

#include "rrframe.h"

#define ITER 50
#define NB 2
#define MINW 1
#define MAXW 400
#define BORDER 0
#define NUMWIN 3

class rrframeut
{
	public:

	rrframeut(Display *dpy, int myid)
	{
		int i;  buf=1;
		w=-1;  h=-1;
		this->dpy=dpy;  this->myid=myid;
		cthnd=dthnd=bthnd=0;
		deadyet=0;
		errifnot(win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), myid*(MINW+BORDER*2), 0, MINW+BORDER, MINW+BORDER, 0,
			WhitePixel(dpy, DefaultScreen(dpy)), BlackPixel(dpy, DefaultScreen(dpy))));
		errifnot(XMapRaised(dpy, win));

		for(i=0; i<NB; i++)
		{
			errifnot(b[i]=new rrbitmap(dpy, win));
			memset(&bmp[i], 0, sizeof(rrbmp));
			tryunix(pthread_mutex_init(&_ready[i], NULL));  tryunix(pthread_mutex_lock(&_ready[i]));
			tryunix(pthread_mutex_init(&_complete[i], NULL));
		}

		tryunix(pthread_create(&cthnd, NULL, compressor, this));
		tryunix(pthread_create(&dthnd, NULL, decompressor, this));
		tryunix(pthread_create(&bthnd, NULL, blitter, this));
	}

	~rrframeut(void)
	{
		shutdown();
	}

	void dotest(int w, int h)
	{
		buf=(buf+1)%NB;
		tryunix(pthread_mutex_lock(&_complete[buf]));
		#if 0
		if(w!=this->w || h!=this->h)
		{
			XLockDisplay(dpy);
			XWindowChanges xwc;
			xwc.width=w;  xwc.height=h;  xwc.x=(w+BORDER*2)*myid;  xwc.y=0;
			XConfigureWindow(dpy, win, CWWidth|CWHeight|CWX|CWY, &xwc);
			XFlush(dpy);
			XSync(dpy, False);
			XWindowAttributes xwa;
			do XGetWindowAttributes(dpy, win, &xwa); while(xwa.width<w || xwa.height<h);
			this->w=w;  this->h=h;
			XUnlockDisplay(dpy);
		}
		#endif
		allocbmp(buf, w, h, 3);
		bmp[buf].h.winid=win;
		bmp[buf].h.winw=w+BORDER;
		bmp[buf].h.winh=h+BORDER;
		bmp[buf].h.bmpx=BORDER;
		bmp[buf].h.bmpy=BORDER;
		bmp[buf].h.qual=80;
		bmp[buf].h.subsamp=RR_422;
		bmp[buf].flags=0;
		initbmp(buf);
		pthread_mutex_unlock(&_ready[buf]);
	}

	private:

	void allocbmp(int buf, int w, int h, int pixelsize)
	{
		if(buf<0 || buf>NB-1 || w<1 || h<1 || pixelsize<3 || pixelsize>4)
			throw("Invalid argument to allocbmp()");
		if(bmp[buf].h.bmpw==w && bmp[buf].h.bmph==h && bmp[buf].pixelsize==pixelsize
			&& bmp[buf].bits) return;
		if(bmp[buf].bits) {free(bmp[buf].bits);  memset(&bmp[buf], 0, sizeof(rrbmp));}
		errifnot(bmp[buf].bits=(unsigned char *)malloc(w*h*pixelsize));
		bmp[buf].h.bmpw=w;  bmp[buf].h.bmph=h;
		bmp[buf].pixelsize=pixelsize;
	}

	void initbmp(int buf)
	{
		int i, j, pitch=bmp[buf].h.bmpw*bmp[buf].pixelsize;
		for(i=0; i<bmp[buf].h.bmph; i++)
		{
			for(j=0; j<bmp[buf].h.bmpw; j++)
			{
				bmp[buf].bits[i*pitch+j*bmp[buf].pixelsize]=i%256;
				bmp[buf].bits[i*pitch+j*bmp[buf].pixelsize+1]=j%256;
				bmp[buf].bits[i*pitch+j*bmp[buf].pixelsize+2]=(i+j)%256;
			}	
		}
	}

	void shutdown(void)
	{
		int i;
		deadyet=1;
		for(i=0; i<NB; i++) {if(b[i]) b[i]->ready();}
		if(bthnd) {pthread_join(bthnd, NULL);  bthnd=0;}

		for(i=0; i<NB; i++) {if(b[i]) b[i]->complete();  j[i].ready();}
		if(dthnd) {pthread_join(dthnd, NULL);  dthnd=0;}

		for(i=0; i<NB; i++) {j[i].complete();  pthread_mutex_unlock(&_ready[i]);}
		if(cthnd) {pthread_join(cthnd, NULL);  cthnd=0;}

		for(i=0; i<NB; i++) {delete b[i];  b[i]=NULL;}
	}

	FILE *f;
	Display *dpy;
	int myid, buf, w, h;
	rrbmp bmp[NB];
	pthread_mutex_t _ready[NB], _complete[NB];
	rrjpeg j[NB];
	rrbitmap *b[NB];
	int deadyet;
	pthread_t cthnd, dthnd, bthnd;
	Window win;
	friend void *compressor(void *);
	friend void *decompressor(void *);
	friend void *blitter(void *);
};

void *compressor(void *p)
{
	rrframeut *test=(rrframeut *)p;
	int buf=1;
	try
	{
		while(!test->deadyet)
		{
			buf=(buf+1)%NB;
			tryunix(pthread_mutex_lock(&test->_ready[buf]));
			test->j[buf].waituntilcomplete();
			test->j[buf]=test->bmp[buf];
			pthread_mutex_unlock(&test->_complete[buf]);
			test->j[buf].ready();
		}
	}
	catch (char *e) {hpprintf("Compressor- %s\n", e);  test->cthnd=0;  exit(1);}
	catch (const char *e) {hpprintf("Compressor- %s\n", e);  test->cthnd=0;  exit(1);}
	hpprintf("Compressor exiting ...\n");
	return 0;
}

void *decompressor(void *p)
{
	rrframeut *test=(rrframeut *)p;
	int buf=1, w=-1, h=-1;
	try
	{
		while(!test->deadyet)
		{
			buf=(buf+1)%NB;
			test->j[buf].waituntilready();
			test->b[buf]->waituntilcomplete();
			w=test->j[buf].h.bmpw;  h=test->j[buf].h.bmph;
		if(w!=test->w || h!=test->h)
		{
			XLockDisplay(test->dpy);
			XWindowChanges xwc;
			xwc.width=w;  xwc.height=h;  xwc.x=(w+BORDER*2)*test->myid;  xwc.y=0;
			XConfigureWindow(test->dpy, test->win, CWWidth|CWHeight|CWX|CWY, &xwc);
			XFlush(test->dpy);
			XSync(test->dpy, False);
			XWindowAttributes xwa;
			do XGetWindowAttributes(test->dpy, test->win, &xwa); while(xwa.width<w || xwa.height<h);
			test->w=w;  test->h=h;
			XUnlockDisplay(test->dpy);
		}
			*(test->b[buf])=test->j[buf];
			test->j[buf].complete();
			test->b[buf]->ready();
		}
	}
	catch (char *e) {hpprintf("Decompressor- %s\n", e);  test->dthnd=0;  exit(1);}
	catch (const char *e) {hpprintf("Decompressor- %s\n", e);  test->dthnd=0;  exit(1);}
	hpprintf("Decompressor exiting ...\n");
	return 0;
}

void *blitter(void *p)
{
	rrframeut *test=(rrframeut *)p;
	hptimer_init();
	double mpixels=0., totaltime=0.;
	int iter=0;
	int buf=1;
	try
	{
		while(!test->deadyet)
		{
			buf=(buf+1)%NB;
			test->b[buf]->waituntilready();
			double _time=hptime();
			test->b[buf]->draw();
			mpixels+=(double)(test->w*test->h)/1000000.;
			totaltime+=hptime()-_time;
			test->b[buf]->complete();
		}
		hpprintf("Average blitter performance = %f Mpixels/sec\n", mpixels/totaltime);
	}
	catch (char *e) {hpprintf("Blitter- %s\n", e);  test->bthnd=0;  exit(1);}
	catch (const char *e) {hpprintf("Blitter- %s\n", e);  test->bthnd=0;  exit(1);}
	hpprintf("Blitter exiting ...\n");
	return 0;
}

int main(void)
{
	Display *dpy=NULL;
	rrframeut *test[NUMWIN];
	int i, j, w, h;

	hputil_log(1, 0);
	try
	{
		errifnot(XInitThreads());
		if(!(dpy=XOpenDisplay(0))) {hpprintf("Could not open display %s\n", XDisplayName(0));  exit(1);}

		for(i=0; i<NUMWIN; i++)
		{
			errifnot(test[i]=new rrframeut(dpy, i));
		}

		for(w=MINW; w<=MAXW; w+=33)
		{
			h=w;
			hpprintf("%.4d x %.4d: ", w, h);
			for(i=0; i<ITER; i++)
			{
				hpprintf(".");
				for(j=0; j<NUMWIN; j++) test[j]->dotest(w, h);
			}
			hpprintf("\n");
		}

		for(i=0; i<NUMWIN; i++)
		{
			delete test[i];
		}
	}
	catch (char *e) {hpprintf("Main- %s\n", e);  exit(1);}	
	catch (const char *e) {hpprintf("Main- %s\n", e);  exit(1);}	
	return 0;
}
