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

using namespace vglutil;
using namespace vglcommon;

#define ITER 50
#define NFRAMES 2
#define MINW 1
#define MAXW 400
#define BORDER 0
#define NUMWIN 1

bool useGL=false, useXV=false, doRgbBench=false, useRGB=false;


void resizeWindow(Display *dpy, Window win, int width, int height, int myID)
{
	XWindowAttributes xwa;
	XGetWindowAttributes(dpy, win, &xwa);
	if(width!=xwa.width || height!=xwa.height)
	{
		XLockDisplay(dpy);
		XWindowChanges xwc;
		xwc.x=(width+BORDER*2)*myID;  xwc.y=0;
		xwc.width=width;  xwc.height=height;
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
			dpy(dpy_), win(win_), thread(NULL), myID(myID_)
		{
			for(int i=0; i<NFRAMES; i++)
			{
				if(useGL) { _newcheck(frames[i]=new GLFrame(dpy, win)); }
				#ifdef USEXV
				else if(useXV) { _newcheck(frames[i]=new XVFrame(dpy, win)); }
				#endif
				else { _newcheck(frames[i]=new FBXFrame(dpy, win)); }
			}
			_newcheck(thread=new Thread(this));
			thread->start();
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
			Frame *frame=frames[findex];
			findex=(findex+1)%NFRAMES;
			if(thread) thread->checkError();  frame->waitUntilComplete();
			if(thread) thread->checkError();
			return frame;
		}

		void put(Frame *frame)
		{
			if(thread) thread->checkError();
			frame->signalReady();
		}

		void shutdown(void)
		{
			deadYet=true;
			int i;
			for(i=0; i<NFRAMES; i++) frames[i]->signalReady();  // Release my thread
			if(thread)
			{
				thread->stop();  delete thread;  thread=NULL;
			}
			for(i=0; i<NFRAMES; i++)
				frames[i]->signalComplete();  // Release Decompressor
		}

	private:

		void run(void)
		{
			Timer timer;
			double mpixels=0., totalTime=0.;
			int index=0;  Frame *frame;
			try
			{
				while(!deadYet)
				{
					frame=frames[index];
					index=(index+1)%NFRAMES;
					frame->waitUntilReady();  if(deadYet) break;
					if(useXV)
						resizeWindow(dpy, win, frame->hdr.width, frame->hdr.height, myID);
					timer.start();
					if(frame->isGL) ((GLFrame *)frame)->redraw();
					#ifdef USEXV
					else if(frame->isXV) ((XVFrame *)frame)->redraw();
					#endif
					else ((FBXFrame *)frame)->redraw();
					mpixels+=(double)(frame->hdr.width*frame->hdr.height)/1000000.;
					totalTime+=timer.elapsed();
					frame->signalComplete();
				}
				fprintf(stderr, "Average Blitter performance = %f Mpixels/sec\n",
					mpixels/totalTime);
			}
			catch(Error &e)
			{
				if(thread) thread->setError(e);
				for(int i=0; i<NFRAMES; i++)
					if(frames[i]) frames[i]->signalComplete();
				throw;
			}
			fprintf(stderr, "Blitter exiting ...\n");
		}

		int findex;  bool deadYet;
		Frame *frames[NFRAMES];
		Display *dpy;  Window win;
		Thread *thread;
		int myID;
};


class Decompressor : public Runnable
{
	public:

		Decompressor(Blitter *blitter_, Display *dpy_, Window win_, int myID_) :
			blitter(blitter_), findex(0), deadYet(false), dpy(dpy_), win(win_),
			myID(myID_), thread(NULL)
		{
			_newcheck(thread=new Thread(this));
			thread->start();
		}

		virtual ~Decompressor(void) { shutdown(); }

		CompressedFrame &get(void)
		{
			CompressedFrame &cframe=cframes[findex];
			findex=(findex+1)%NFRAMES;
			if(deadYet) return cframe;
			if(thread) thread->checkError();  cframe.waitUntilComplete();
			if(thread) thread->checkError();
			return cframe;
		}

		void put(CompressedFrame &cframe)
		{
			if(thread) thread->checkError();
			cframe.signalReady();
		}

		void shutdown(void)
		{
			deadYet=true;
			int i;
			for(i=0; i<NFRAMES; i++) cframes[i].signalReady();  // Release my thread
			if(thread)
			{
				thread->stop();  delete thread;  thread=NULL;
			}
			for(i=0; i<NFRAMES; i++)
				cframes[i].signalComplete();  // Release compressor
		}

	private:

		void run(void)
		{
			int index=0;  Frame *frame=NULL;
			try
			{
				while(!deadYet)
				{
					CompressedFrame &cframe=cframes[index];
					index=(index+1)%NFRAMES;
					cframe.waitUntilReady();  if(deadYet) break;
					frame=blitter->get();  if(deadYet) break;
					resizeWindow(dpy, win, cframe.hdr.width, cframe.hdr.height, myID);
					if(frame->isGL) *((GLFrame *)frame)=cframe;
					#ifdef USEXV
					else if(frame->isXV) *((XVFrame *)frame)=cframe;
					#endif
					else *((FBXFrame *)frame)=cframe;
					blitter->put(frame);
					cframe.signalComplete();
				}
			}
			catch(Error &e)
			{
				if(thread) thread->setError(e);
				for(int i=0; i<NFRAMES; i++) cframes[i].signalComplete();  throw;
			}
			fprintf(stderr, "Decompressor exiting ...\n");
		}

		Blitter *blitter;
		int findex;  bool deadYet;
		Display *dpy;  Window win;
		CompressedFrame cframes[NFRAMES];
		int myID;  Thread *thread;
};


class Compressor : public Runnable
{
	public:

		Compressor(Decompressor *decompressor_, Blitter *blitter_) : findex(0),
			deadYet(false), thread(NULL), decompressor(decompressor_),
			blitter(blitter_)
		{
			_newcheck(thread=new Thread(this));
			thread->start();
		}

		virtual ~Compressor(void)
		{
			if(thread) thread->stop();
		}

		Frame &get(int width, int height)
		{
			Frame &frame=frames[findex];
			findex=(findex+1)%NFRAMES;
			if(thread) thread->checkError();  frame.waitUntilComplete();
			if(thread) thread->checkError();
			rrframeheader hdr;
			hdr.framew=hdr.width=width+BORDER;
			hdr.frameh=hdr.height=height+BORDER;
			hdr.x=hdr.y=BORDER;
			hdr.qual=80;
			hdr.subsamp=2;
			hdr.compress=useRGB? RRCOMP_RGB:RRCOMP_JPEG;
			if(useXV) hdr.compress=RRCOMP_YUV;
			frame.init(hdr, 3, (littleendian() && !useRGB && !useXV)? FRAME_BGR:0);
			return frame;
		}

		void put(Frame &frame)
		{
			if(thread) thread->checkError();
			frame.signalReady();
		}

		void shutdown(void)
		{
			deadYet=true;
			int i;
			for(i=0; i<NFRAMES; i++) frames[i].signalReady();  // Release my thread
			if(thread)
			{
				thread->stop();  delete thread;  thread=NULL;
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
					Frame &frame=frames[index];
					index=(index+1)%NFRAMES;
					frame.waitUntilReady();  if(deadYet) break;
					#ifdef USEXV
					if(useXV)
					{
						XVFrame *xvframe=(XVFrame *)blitter->get();  if(deadYet) break;
						*xvframe=frame;
						blitter->put(xvframe);
					}
					else
					#endif
					{
						CompressedFrame &cframe=decompressor->get();  if(deadYet) break;
						cframe=frame;
						decompressor->put(cframe);
					}
					frame.signalComplete();
				}
			}
			catch(Error &e)
			{
				if(thread) thread->setError(e);
				for(int i=0; i<NFRAMES; i++) frames[i].signalComplete();  throw;
			}
			fprintf(stderr, "Compressor exiting ...\n");
		}

		int findex;  bool deadYet;
		Thread *thread;
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
			_errifnot(win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
				myID*(MINW+BORDER*2), 0, MINW+BORDER, MINW+BORDER, 0,
				WhitePixel(dpy, DefaultScreen(dpy)),
				BlackPixel(dpy, DefaultScreen(dpy))));
			_errifnot(XMapRaised(dpy, win));

			_newcheck(blitter=new Blitter(dpy, win, myID));
			if(!useXV)
				_newcheck(decompressor=new Decompressor(blitter, dpy, win, myID));
			_newcheck(compressor=new Compressor(decompressor, blitter));
		}

		~FrameTest(void) { shutdown(); }

		void dotest(int width, int height, int seed)
		{
			Frame &frame=compressor->get(width, height);
			initFrame(frame, seed);
			compressor->put(frame);
		}

	private:

		void initFrame(Frame &frame, int seed)
		{
			int i, j, pitch=frame.pitch;
			for(i=0; i<frame.hdr.height; i++)
			{
				for(j=0; j<frame.hdr.width; j++)
				{
					frame.bits[i*pitch+j*frame.pixelSize]=(i+seed)%256;
					frame.bits[i*pitch+j*frame.pixelSize+1]=(j+seed)%256;
					frame.bits[i*pitch+j*frame.pixelSize+2]=(i+j+seed)%256;
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


static const char *formatName[BMP_NUMPF]=
{
	"RGB", "RGBA", "BGR", "BGRA", "ABGR", "ARGB"
};

static const int ps[BMP_NUMPF]={ 3, 4, 3, 4, 4, 4 };

static const int roffset[BMP_NUMPF]={ 0, 0, 2, 2, 3, 1 };

static const int goffset[BMP_NUMPF]={ 1, 1, 1, 1, 2, 2 };

static const int boffset[BMP_NUMPF]={ 2, 2, 0, 0, 1, 3 };

static const int flags[BMP_NUMPF]=
{
	0, 0, FRAME_BGR, FRAME_BGR, FRAME_ALPHAFIRST|FRAME_BGR, FRAME_ALPHAFIRST
};


int cmpFrame(unsigned char *buf, int width, int height, Frame &dst,
	BMPPF dstpf)
{
	int _i;  int dstbu=((dst.flags&FRAME_BOTTOMUP)!=0);  int pitch=width*3;
	for(int i=0; i<height; i++)
	{
		_i=dstbu? i:height-i-1;
		for(int j=0; j<height; j++)
		{
			if((buf[pitch*_i+j*3]
					!= dst.bits[dst.pitch*i+j*ps[dstpf]+roffset[dstpf]]) ||
				(buf[pitch*_i+j*3+1]
					!= dst.bits[dst.pitch*i+j*ps[dstpf]+goffset[dstpf]]) ||
				(buf[pitch*_i+j*3+2]
					!= dst.bits[dst.pitch*i+j*ps[dstpf]+boffset[dstpf]]))
				return 1;
		}
	}
	return 0;
}


void rgbBench(char *filename)
{
	unsigned char *buf;  int width, height, dstbu;
	CompressedFrame src;  Frame dst;  int dstpf;

	for(dstpf=0; dstpf<BMP_NUMPF; dstpf++)
	{
		int dstflags=flags[dstpf];
		for(dstbu=0; dstbu<2; dstbu++)
		{
			if(dstbu) dstflags|=FRAME_BOTTOMUP;
			if(bmp_load(filename, &buf, &width, 1, &height, BMPPF_RGB,
				BMPORN_BOTTOMUP)==-1)
				throw(bmp_geterr());
			rrframeheader hdr;
			memset(&hdr, 0, sizeof(hdr));
			hdr.x=hdr.y=0;
			hdr.width=hdr.framew=width;
			hdr.height=hdr.frameh=height;
			hdr.compress=RRCOMP_RGB;  hdr.flags=0;  hdr.size=width*3*height;
			src.init(hdr, hdr.flags);
			memcpy(src.bits, buf, width*3*height);
			dst.init(hdr, ps[dstpf], dstflags);
			memset(dst.bits, 0, dst.pitch*dst.hdr.frameh);
			fprintf(stderr, "RGB (BOTTOM-UP) -> %s (%s)\n", formatName[dstpf],
				dstbu? "BOTTOM-UP":"TOP-DOWN");
			double tStart, tTotal=0.;  int iter=0;
			do
			{
				tStart=getTime();
				dst.decompressRGB(src, width, height, false);
				tTotal+=getTime()-tStart;  iter++;
			} while(tTotal<1.);
			fprintf(stderr, "%f Mpixels/sec - ", (double)width*(double)height
				*(double)iter/1000000./tTotal);
			if(cmpFrame(buf, width, height, dst, (BMPPF)dstpf))
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
	char *fileName=NULL;

	if(argc>1)
	{
		for(i=1; i<argc; i++)
		{
			if(!stricmp(argv[i], "-gl"))
			{
				fprintf(stderr, "Using OpenGL for blitting ...\n");
				useGL=true;
			}
			#ifdef USEXV
			else if(!stricmp(argv[i], "-xv"))
			{
				fprintf(stderr, "Using X Video ...\n");
				useXV=true;
			}
			#endif
			else if(!stricmp(argv[i], "-rgb"))
			{
				fprintf(stderr, "Using RGB encoding ...\n");
				useRGB=true;
			}
			else if(!stricmp(argv[i], "-rgbbench"))
			{
				if(i>=argc-1) usage(argv[0]);
				fileName=argv[++i];  doRgbBench=true;
			}
			else if(!strnicmp(argv[i], "-h", 2) || !strcmp(argv[i], "-?"))
				usage(argv[0]);
		}
	}

	try
	{
		if(doRgbBench) { rgbBench(fileName);  exit(0); }

		_errifnot(XInitThreads());
		if(!(dpy=XOpenDisplay(0)))
		{
			fprintf(stderr, "Could not open display %s\n", XDisplayName(0));
			exit(1);
		}

		for(i=0; i<NUMWIN; i++)
		{
			_newcheck(test[i]=new FrameTest(dpy, i));
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
