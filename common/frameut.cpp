/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014 D. R. Commander
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

#include "Thread.h"
#include "Frame.h"
#include "../client/GLFrame.h"
#include "vglutil.h"
#include "Timer.h"
#include "bmp.h"

using namespace vglcommon;

#define ITER 50
#define NFRAMES 2
#define MINW 1
#define MAXW 400
#define BORDER 0
#define NUMWIN 1

bool useGL=false, useXV=false, doRGBBench=false, useRGB=false;


void resizeWindow(Display *dpy, Window win, int w, int h, int myID)
{
	XWindowAttributes xwa;
	XGetWindowAttributes(dpy, win, &xwa);
	if(w!=xwa.width || h!=xwa.height)
	{
		XLockDisplay(dpy);
		XWindowChanges xwc;
		xwc.width=w;  xwc.height=h;
		xwc.x=(w+BORDER*2)*myID;  xwc.y=0;
		XConfigureWindow(dpy, win, CWWidth|CWHeight|CWX|CWY, &xwc);
		XFlush(dpy);
		XSync(dpy, False);
		XUnlockDisplay(dpy);
	}
}


class Blitter : public Runnable
{
	public:

		Blitter(Display *dpy_, Window win_, int myID_) : findex(0), deadYet(false),
			dpy(dpy_), win(win_), t(NULL), myID(myID_)
		{
			for(int i=0; i<NFRAMES; i++)
			{
				if(useGL) { errifnot(frames[i]=new GLFrame(dpy, win)); }
				#ifdef USEXV
				else if(useXV) { errifnot(frames[i]=new XVFrame(dpy, win)); }
				#endif
				else { errifnot(frames[i]=new FBXFrame(dpy, win)); }
			}
			errifnot(t=new Thread(this));
			t->start();
		}

		virtual ~Blitter(void)
		{
			shutdown();
			for(int i=0; i<NFRAMES; i++)
			{
				if(frames[i])
				{
					if(frames[i]->isGL) delete (GLFrame *)frames[i];
					#ifdef USEXV
					else if(frames[i]->isXV) delete (XVFrame *)frames[i];
					#endif
					else delete (FBXFrame *)frames[i];
					frames[i]=NULL;
				}
			}
		}

		Frame *get(void)
		{
			Frame *f=frames[findex];  findex=(findex+1)%NFRAMES;
			if(t) t->checkError();  f->waitUntilComplete();
			if(t) t->checkError();
			return f;
		}

		void put(Frame *f)
		{
			if(t) t->checkError();
			f->signalReady();
		}

		void shutdown(void)
		{
			deadYet=true;
			int i;
			for(i=0; i<NFRAMES; i++) frames[i]->signalReady();  // Release my thread
			if(t)
			{
				t->stop();  delete t;  t=NULL;
			}
			for(i=0; i<NFRAMES; i++)
				frames[i]->signalComplete();  // Release Decompressor
		}

	private:

		void run(void)
		{
			Timer timer;
			double mpixels=0., totalTime=0.;
			int index=0;  Frame *f;
			try
			{
				while(!deadYet)
				{
					f=frames[index];  index=(index+1)%NFRAMES;
					f->waitUntilReady();  if(deadYet) break;
					if(useXV) resizeWindow(dpy, win, f->hdr.width, f->hdr.height, myID);
					timer.start();
					if(f->isGL) ((GLFrame *)f)->redraw();
					#ifdef USEXV
					else if(f->isXV) ((XVFrame *)f)->redraw();
					#endif
					else ((FBXFrame *)f)->redraw();
					mpixels+=(double)(f->hdr.width*f->hdr.height)/1000000.;
					totalTime+=timer.elapsed();
					f->signalComplete();
				}
				fprintf(stderr, "Average Blitter performance = %f Mpixels/sec\n",
					mpixels/totalTime);
			}
			catch (...)
			{
				for(int i=0; i<NFRAMES; i++)
					if(frames[i]) frames[i]->signalComplete();
				throw;
			}
			fprintf(stderr, "Blitter exiting ...\n");
		}

		int findex;  bool deadYet;  
		Frame *frames[NFRAMES];
		Display *dpy;  Window win;
		Thread *t;
		int myID;
};


class Decompressor : public Runnable
{
	public:

		Decompressor(Blitter *blitter_, Display *dpy_, Window win_, int myID_) :
			blitter(blitter_), findex(0), deadYet(false), dpy(dpy_), win(win_),
			myID(myID_), t(NULL)
		{
			errifnot(t=new Thread(this));
			t->start();
		}

		virtual ~Decompressor(void) { shutdown(); }

		CompressedFrame &get(void)
		{
			CompressedFrame &cf=cframes[findex];  findex=(findex+1)%NFRAMES;
			if(deadYet) return cf;
			if(t) t->checkError();  cf.waitUntilComplete();
			if(t) t->checkError();
			return cf;
		}

		void put(CompressedFrame &cf)
		{
			if(t) t->checkError();
			cf.signalReady();
		}

		void shutdown(void)
		{
			deadYet=true;
			int i;
			for(i=0; i<NFRAMES; i++) cframes[i].signalReady();  // Release my thread
			if(t)
			{
				t->stop();  delete t;  t=NULL;
			}
			for(i=0; i<NFRAMES; i++)
				cframes[i].signalComplete();  // Release compressor
		}

	private:

		void run(void)
		{
			int index=0;  Frame *f=NULL;
			try
			{
				while(!deadYet)
				{
					CompressedFrame &cf=cframes[index];  index=(index+1)%NFRAMES;
					cf.waitUntilReady();  if(deadYet) break;
					f=blitter->get();  if(deadYet) break;
					resizeWindow(dpy, win, cf.hdr.width, cf.hdr.height, myID);
					if(f->isGL) *((GLFrame *)f)=cf;
					#ifdef USEXV
					else if(f->isXV) *((XVFrame *)f)=cf;
					#endif
					else *((FBXFrame *)f)=cf;
					blitter->put(f);
					cf.signalComplete();
				}
			}
			catch (...)
			{
				for(int i=0; i<NFRAMES; i++) cframes[i].signalComplete();  throw;
			}
			fprintf(stderr, "Decompressor exiting ...\n");
		}

		Blitter *blitter;
		int findex;  bool deadYet;
		Display *dpy;  Window win;
		CompressedFrame cframes[NFRAMES];
		int myID;  Thread *t;
};


class Compressor : public Runnable
{
	public:

		Compressor(Decompressor *decompressor_, Blitter *blitter_) : findex(0),
			deadYet(false), t(NULL), decompressor(decompressor_), blitter(blitter_)
		{
			errifnot(t=new Thread(this));
			t->start();
		}

		virtual ~Compressor(void)
		{
			if(t) t->stop();
		}

		Frame &get(int w, int h)
		{
			Frame &f=frames[findex];  findex=(findex+1)%NFRAMES;
			if(t) t->checkError();  f.waitUntilComplete();
			if(t) t->checkError();
			rrframeheader hdr;
			hdr.framew=hdr.width=w+BORDER;
			hdr.frameh=hdr.height=h+BORDER;
			hdr.x=hdr.y=BORDER;
			hdr.qual=80;
			hdr.subsamp=2;
			hdr.compress=useRGB? RRCOMP_RGB:RRCOMP_JPEG;
			if(useXV) hdr.compress=RRCOMP_YUV;
			f.init(hdr, 3, (littleendian() && !useRGB && !useXV)? FRAME_BGR:0);
			return f;
		}

		void put(Frame &f)
		{
			if(t) t->checkError();
			f.signalReady();
		}

		void shutdown(void)
		{
			deadYet=true;
			int i;
			for(i=0; i<NFRAMES; i++) frames[i].signalReady();  // Release my thread
			if(t)
			{
				t->stop();  delete t;  t=NULL;
			}
			for(i=0; i<NFRAMES; i++)
				frames[i].signalComplete();  // Release main thread
		}

	private:

		void run(void)
		{
			int index=0;
			try
			{
				while(!deadYet)
				{
					Frame &f=frames[index];  index=(index+1)%NFRAMES;
					f.waitUntilReady();  if(deadYet) break;
					#ifdef USEXV
					if(useXV)
					{
						XVFrame *xvf=(XVFrame *)blitter->get();  if(deadYet) break;
						*xvf=f;
						blitter->put(xvf);
					}
					else
					#endif
					{
						CompressedFrame &cf=decompressor->get();  if(deadYet) break;
						cf=f;
						decompressor->put(cf);
					}
					f.signalComplete();
				}
			}
			catch (...)
			{
				for(int i=0; i<NFRAMES; i++) frames[i].signalComplete();  throw;
			}
			fprintf(stderr, "Compressor exiting ...\n");
		}

		int findex;  bool deadYet;
		Thread *t;
		Frame frames[NFRAMES];
		Decompressor *decompressor;
		Blitter *blitter;
};


class FrameTest
{
	public:

		FrameTest(Display *dpy, int myID) : compressor(NULL), decompressor(NULL),
			blitter(NULL)
		{
			Window win;
			errifnot(win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
				myID*(MINW+BORDER*2), 0, MINW+BORDER, MINW+BORDER, 0,
				WhitePixel(dpy, DefaultScreen(dpy)),
				BlackPixel(dpy, DefaultScreen(dpy))));
			errifnot(XMapRaised(dpy, win));

			errifnot(blitter=new Blitter(dpy, win, myID));
			if(!useXV)
				errifnot(decompressor=new Decompressor(blitter, dpy, win, myID));
			errifnot(compressor=new Compressor(decompressor, blitter));
		}

		~FrameTest(void) { shutdown(); }

		void dotest(int w, int h, int seed)
		{
			Frame &f=compressor->get(w, h);
			initFrame(f, seed);
			compressor->put(f);
		}

	private:

		void initFrame(Frame &f, int seed)
		{
			int i, j, pitch=f.pitch;
			for(i=0; i<f.hdr.height; i++)
			{
				for(j=0; j<f.hdr.width; j++)
				{
					f.bits[i*pitch+j*f.pixelSize]=(i+seed)%256;
					f.bits[i*pitch+j*f.pixelSize+1]=(j+seed)%256;
					f.bits[i*pitch+j*f.pixelSize+2]=(i+j+seed)%256;
				}	
			}
		}

		void shutdown(void)
		{
			fprintf(stderr, "Shutting down....\n");  fflush(stderr);
			if(blitter) blitter->shutdown();
			if(decompressor) decompressor->shutdown();
			if(compressor) compressor->shutdown();
			if(blitter) { delete blitter;  blitter=NULL; }
			if(decompressor) { delete decompressor;  decompressor=NULL; }
			if(compressor) { delete compressor;  compressor=NULL; }
		}

		Compressor *compressor;  Decompressor *decompressor;  Blitter *blitter;
};


static const char *formatName[BMPPIXELFORMATS]=
{
	"RGB", "RGBA", "BGR", "BGRA", "ABGR", "ARGB"
};

static const int ps[BMPPIXELFORMATS]={3, 4, 3, 4, 4, 4};

static const int roff[BMPPIXELFORMATS]={0, 0, 2, 2, 3, 1};

static const int goff[BMPPIXELFORMATS]={1, 1, 1, 1, 2, 2};

static const int boff[BMPPIXELFORMATS]={2, 2, 0, 0, 1, 3};

static const int flags[BMPPIXELFORMATS]=
{
	0, 0, FRAME_BGR, FRAME_BGR, FRAME_ALPHAFIRST|FRAME_BGR, FRAME_ALPHAFIRST
};


int cmpframe(unsigned char *buf, int w, int h, Frame &dst,
	BMPPIXELFORMAT dstpf)
{
	int _i;  int dstbu=((dst.flags&FRAME_BOTTOMUP)!=0);  int pitch=w*3;
	for(int i=0; i<h; i++)
	{
		_i=dstbu? i:h-i-1;
		for(int j=0; j<h; j++)
		{
			if((buf[pitch*_i+j*3]
					!= dst.bits[dst.pitch*i+j*ps[dstpf]+roff[dstpf]]) ||
				(buf[pitch*_i+j*3+1]
					!= dst.bits[dst.pitch*i+j*ps[dstpf]+goff[dstpf]]) ||
				(buf[pitch*_i+j*3+2]
					!= dst.bits[dst.pitch*i+j*ps[dstpf]+boff[dstpf]]))
				return 1;
		}
	}
	return 0;
}


void rgbBench(char *filename)
{
	unsigned char *buf;  int w, h, dstbu;
	CompressedFrame src;  Frame dst;  int dstpf;

	for(dstpf=0; dstpf<BMPPIXELFORMATS; dstpf++)
	{
		int dstflags=flags[dstpf];
		for(dstbu=0; dstbu<2; dstbu++)
		{
			if(dstbu) dstflags|=FRAME_BOTTOMUP;
			if(loadbmp(filename, &buf, &w, &h, BMP_RGB, 1, 1)==-1)
				throw(bmpgeterr());
			rrframeheader hdr;
			memset(&hdr, 0, sizeof(hdr));
			hdr.width=hdr.framew=w;  hdr.height=hdr.frameh=h;  hdr.x=hdr.y=0;
			hdr.compress=RRCOMP_RGB;  hdr.flags=0;  hdr.size=w*3*h;
			src.init(hdr, hdr.flags);
			memcpy(src.bits, buf, w*3*h);
			dst.init(hdr, ps[dstpf], dstflags);
			memset(dst.bits, 0, dst.pitch*dst.hdr.frameh);
			fprintf(stderr, "RGB (BOTTOM-UP) -> %s (%s)\n", formatName[dstpf],
				dstbu? "BOTTOM-UP":"TOP-DOWN");
			double tstart, ttotal=0.;  int iter=0;
			do
			{
				tstart=getTime();
				dst.decompressRGB(src, w, h, false);
				ttotal+=getTime()-tstart;  iter++;
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


void usage(char *programName)
{
	fprintf(stderr, "\nUSAGE: %s [-gl] [-xv] [-rgb] [-rgbbench <filename>]\n\n",
		programName);
	fprintf(stderr, "-gl = Use OpenGL instead of X11 for blitting\n");
	fprintf(stderr, "-xv = Test X Video encoding/display\n");
	fprintf(stderr, "-rgb = Use RGB encoding instead of JPEG compression\n");
	fprintf(stderr, "-rgbbench <filename> = Benchmark the decoding of RGB-encoded images.\n");
	fprintf(stderr, "                       <filename> should be a BMP or PPM file.\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	Display *dpy=NULL;
	FrameTest *test[NUMWIN];
	int i, j, w, h;
	char *bmpfile=NULL;

	if(argc>1)
	{
		for(i=1; i<argc; i++)
		{
			if(!stricmp(argv[i], "-gl")) useGL=true;
			#ifdef USEXV
			else if(!stricmp(argv[i], "-xv")) useXV=true;
			#endif
			else if(!stricmp(argv[i], "-rgb")) useRGB=true;
			else if(!stricmp(argv[i], "-rgbbench"))
			{
				if(i>=argc-1) usage(argv[0]);
				bmpfile=argv[++i];  doRGBBench=true;
			}
			else if(!strnicmp(argv[i], "-h", 2) || !strcmp(argv[i], "-?"))
				usage(argv[0]);
		}
	}

	try
	{
		if(doRGBBench) { rgbBench(bmpfile);  exit(0); }

		errifnot(XInitThreads());
		if(!(dpy=XOpenDisplay(0)))
		{
			fprintf(stderr, "Could not open display %s\n", XDisplayName(0));
			exit(1);
		}

		for(i=0; i<NUMWIN; i++)
		{
			errifnot(test[i]=new FrameTest(dpy, i));
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
	catch (Error &e)
	{
		fprintf(stderr, "%s\n%s\n", e.getMethod(), e.getMessage());
		exit(1);
	}	
	return 0;
}
