// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2011, 2014, 2017-2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include "Thread.h"
#include "Frame.h"
#include "../client/GLFrame.h"
#include "vglutil.h"
#include "Timer.h"
#include "bmp.h"
#include "vgllogo.h"
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif

using namespace util;
using namespace common;

#define ITER  50
#define NFRAMES  2
#define MINW  1
#define MAXW  400
#define BORDER  0
#define NUMWIN  1

bool useGL = false, useXV = false, doRgbBench = false, useRGB = false,
	addLogo = false, anaglyph = false, check = false;


void resizeWindow(Display *dpy, Window win, int width, int height, int myID)
{
	XWindowAttributes xwa;
	XGetWindowAttributes(dpy, win, &xwa);
	if(width != xwa.width || height != xwa.height)
	{
		XLockDisplay(dpy);
		XWindowChanges xwc;
		xwc.x = (width + BORDER * 2) * myID;  xwc.y = 0;
		xwc.width = width;  xwc.height = height;
		XConfigureWindow(dpy, win, CWWidth | CWHeight | CWX | CWY, &xwc);
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
			for(int i = 0; i < NFRAMES; i++)
			{
				if(useGL) { frames[i] = new GLFrame(dpy, win); }
				#ifdef USEXV
				else if(useXV) { frames[i] = new XVFrame(dpy, win); }
				#endif
				else { frames[i] = new FBXFrame(dpy, win); }
			}
			thread = new Thread(this);
			thread->start();
			#ifdef USEHELGRIND
			ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
			#endif
		}

		virtual ~Blitter(void)
		{
			shutdown();
			for(int i = 0; i < NFRAMES; i++)
			{
				if(frames[i])
				{
					if(frames[i]->isGL) delete (GLFrame *)frames[i];
					#ifdef USEXV
					else if(frames[i]->isXV) delete (XVFrame *)frames[i];
					#endif
					else delete (FBXFrame *)frames[i];
					frames[i] = NULL;
				}
			}
		}

		Frame *get(void)
		{
			Frame *frame = frames[findex];
			findex = (findex + 1) % NFRAMES;
			if(thread) thread->checkError();
			if(!deadYet) frame->waitUntilComplete();
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
			deadYet = true;
			int i;
			for(i = 0; i < NFRAMES; i++)
				frames[i]->signalReady();  // Release my thread
			if(thread)
			{
				thread->stop();  delete thread;  thread = NULL;
			}
			for(i = 0; i < NFRAMES; i++)
				frames[i]->signalComplete();  // Release Decompressor
		}

	private:

		void run(void)
		{
			Timer timer;
			double mpixels = 0., totalTime = 0.;
			int index = 0;  Frame *frame;
			try
			{
				while(!deadYet)
				{
					frame = frames[index];
					index = (index + 1) % NFRAMES;
					frame->waitUntilReady();  if(deadYet) break;
					if(useXV)
						resizeWindow(dpy, win, frame->hdr.width, frame->hdr.height, myID);
					timer.start();
					if(frame->isGL) ((GLFrame *)frame)->redraw();
					#ifdef USEXV
					else if(frame->isXV) ((XVFrame *)frame)->redraw();
					#endif
					else ((FBXFrame *)frame)->redraw();
					mpixels += (double)(frame->hdr.width * frame->hdr.height) / 1000000.;
					totalTime += timer.elapsed();
					if(check && !checkFrame(frame))
						THROW("Pixel data is bogus");
					frame->signalComplete();
				}
				fprintf(stderr, "Average Blitter performance = %f Mpixels/sec%s\n",
					mpixels / totalTime, check ? " [PASSED]" : "");
			}
			catch(std::exception &e)
			{
				if(thread) thread->setError(e);
				for(int i = 0; i < NFRAMES; i++)
					if(frames[i]) frames[i]->signalComplete();
				throw;
			}
			fprintf(stderr, "Blitter exiting ...\n");
		}

		bool checkFrame(Frame *frame)
		{
			int i, j, _j, pitch = frame->pitch, seed = frame->hdr.winid;
			unsigned char *ptr = frame->bits, *pixel;
			PF *pf = pf_get(frame->hdr.dpynum);
			int maxRGB = (1 << pf->bpc);

			for(_j = 0; _j < frame->hdr.height; _j++, ptr += pitch)
			{
				j = frame->flags & FRAME_BOTTOMUP ? frame->hdr.height - _j - 1 : _j;
				for(i = 0, pixel = ptr; i < frame->hdr.width;
					i++, pixel += frame->pf->size)
				{
					int r, g, b;
					frame->pf->getRGB(pixel, &r, &g, &b);
					if(frame->pf->bpc == 10 && pf->bpc != 10)
					{
						r >>= 2;  g >>= 2;  b >>= 2;
					}
					if(addLogo && !useXV)
					{
						int lw = min(VGLLOGO_WIDTH, frame->hdr.width - 1);
						int lh = min(VGLLOGO_HEIGHT, frame->hdr.height - 1);
						int li = i - (frame->hdr.width - lw - 1);
						int lj = j - (frame->hdr.height - lh - 1);
						if(lw > 0 && lh > 0 && li >= 0 && lj >= 0 && li < lw && lj < lh
							&& vgllogo[lj * VGLLOGO_WIDTH + li])
						{
							r ^= 113;  g ^= 162;  b ^= 117;
						}
					}
					if(r != (i + seed) % maxRGB || g != (j + seed) % maxRGB
						|| b != (i + j + seed) % maxRGB)
						return false;
				}
			}

			return true;
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
			thread = new Thread(this);
			thread->start();
			#ifdef USEHELGRIND
			ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
			#endif
		}

		virtual ~Decompressor(void) { shutdown(); }

		CompressedFrame &get(void)
		{
			CompressedFrame &cframe = cframes[findex];
			findex = (findex + 1) % NFRAMES;
			if(deadYet) return cframe;
			if(thread) thread->checkError();
			if(!deadYet) cframe.waitUntilComplete();
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
			deadYet = true;
			int i;
			for(i = 0; i < NFRAMES; i++)
				cframes[i].signalReady();  // Release my thread
			if(thread)
			{
				thread->stop();  delete thread;  thread = NULL;
			}
			for(i = 0; i < NFRAMES; i++)
				cframes[i].signalComplete();  // Release compressor
		}

	private:

		void run(void)
		{
			int index = 0;  Frame *frame = NULL;
			try
			{
				while(!deadYet)
				{
					CompressedFrame &cframe = cframes[index];
					index = (index + 1) % NFRAMES;
					cframe.waitUntilReady();  if(deadYet) break;
					frame = blitter->get();  if(deadYet) break;
					resizeWindow(dpy, win, cframe.hdr.width, cframe.hdr.height, myID);
					if(frame->isGL) *((GLFrame *)frame) = cframe;
					#ifdef USEXV
					else if(frame->isXV) *((XVFrame *)frame) = cframe;
					#endif
					else *((FBXFrame *)frame) = cframe;
					blitter->put(frame);
					cframe.signalComplete();
				}
			}
			catch(std::exception &e)
			{
				if(thread) thread->setError(e);
				for(int i = 0; i < NFRAMES; i++) cframes[i].signalComplete();
				throw;
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
			deadYet(false), thread(NULL), decompressor(decompressor_)
			#ifdef USEXV
			, blitter(blitter_)
			#endif
		{
			thread = new Thread(this);
			thread->start();
			#ifdef USEHELGRIND
			ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
			#endif
		}

		virtual ~Compressor(void)
		{
			if(thread) thread->stop();
		}

		Frame &get(int width, int height, int pixelFormat)
		{
			Frame &frame = frames[findex];
			findex = (findex + 1) % NFRAMES;
			if(thread) thread->checkError();
			if(!deadYet) frame.waitUntilComplete();
			if(thread) thread->checkError();
			rrframeheader hdr;
			memset(&hdr, 0, sizeof(rrframeheader));
			hdr.framew = hdr.width = width + BORDER;
			hdr.frameh = hdr.height = height + BORDER;
			hdr.x = hdr.y = BORDER;
			hdr.qual = 80;
			hdr.subsamp = 2;
			hdr.compress = useRGB ? RRCOMP_RGB : RRCOMP_JPEG;
			if(useXV) hdr.compress = RRCOMP_YUV;
			frame.init(hdr, pixelFormat, 0);
			return frame;
		}

		void put(Frame &frame)
		{
			if(thread) thread->checkError();
			frame.signalReady();
		}

		void shutdown(void)
		{
			deadYet = true;
			int i;
			for(i = 0; i < NFRAMES; i++)
				frames[i].signalReady();  // Release my thread
			if(thread)
			{
				thread->stop();  delete thread;  thread = NULL;
			}
			for(i = 0; i < NFRAMES; i++)
				frames[i].signalComplete();  // Release main thread
		}

	private:

		void run(void)
		{
			int index = 0;
			try
			{
				while(!deadYet)
				{
					Frame &frame = frames[index];
					index = (index + 1) % NFRAMES;
					frame.waitUntilReady();  if(deadYet) break;
					#ifdef USEXV
					if(useXV)
					{
						XVFrame *xvframe = (XVFrame *)blitter->get();  if(deadYet) break;
						*xvframe = frame;
						blitter->put(xvframe);
					}
					else
					#endif
					{
						CompressedFrame &cframe = decompressor->get();  if(deadYet) break;
						cframe = frame;
						decompressor->put(cframe);
					}
					frame.signalComplete();
				}
			}
			catch(std::exception &e)
			{
				if(thread) thread->setError(e);
				for(int i = 0; i < NFRAMES; i++) frames[i].signalComplete();
				throw;
			}
			fprintf(stderr, "Compressor exiting ...\n");
		}

		int findex;  bool deadYet;
		Thread *thread;
		Frame frames[NFRAMES];
		Decompressor *decompressor;
		#ifdef USEXV
		Blitter *blitter;
		#endif
};


class FrameTest
{
	public:

		FrameTest(Display *dpy_, int myID_) : dpy(dpy_), compressor(NULL),
			decompressor(NULL), blitter(NULL), myID(myID_)
		{
			ERRIFNOT(win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
				myID * (MINW + BORDER * 2), 0, MINW + BORDER, MINW + BORDER, 0,
				WhitePixel(dpy, DefaultScreen(dpy)),
				BlackPixel(dpy, DefaultScreen(dpy))));
			ERRIFNOT(XMapRaised(dpy, win));

			blitter = new Blitter(dpy, win, myID);
			if(!useXV)
				decompressor = new Decompressor(blitter, dpy, win, myID);
			compressor = new Compressor(decompressor, blitter);
		}

		~FrameTest(void) { shutdown();  XDestroyWindow(dpy, win); }

		void dotest(int width, int height, int seed, PF *pf)
		{
			if(pf->bpc == 8)
			{
				Frame &frame = compressor->get(width, height, pf->id);
				if(anaglyph) makeAnaglyph(frame, seed);
				else initFrame(frame, seed);
				// This unit test doesn't use winid, so use it to track the seed.
				frame.hdr.winid = seed;
				// This unit test doesn't use dpynum, so use it to track the source
				// pixel format.
				frame.hdr.dpynum = pf->id;
				if(addLogo) frame.addLogo();
				compressor->put(frame);
			}
			else
			{
				FBXFrame *frame = (FBXFrame *)blitter->get();
				rrframeheader hdr;
				memset(&hdr, 0, sizeof(rrframeheader));
				hdr.width = hdr.framew = width;
				hdr.height = hdr.frameh = height;
				frame->init(hdr);
				if(anaglyph) makeAnaglyph(*frame, seed);
				else initFrame(*frame, seed);
				// This unit test doesn't use winid, so use it to track the seed.
				frame->hdr.winid = seed;
				// This unit test doesn't use dpynum, so use it to track the source
				// pixel format.
				frame->hdr.dpynum = pf->id;
				if(addLogo) frame->addLogo();
				resizeWindow(dpy, win, width, height, myID);
				blitter->put(frame);
			}
		}

	private:

		void initFrame(Frame &frame, int seed)
		{
			int i, j, pitch = frame.pitch;
			unsigned char *ptr = frame.bits, *pixel;
			int maxRGB = (1 << frame.pf->bpc);

			for(j = 0; j < frame.hdr.height; j++, ptr += pitch)
			{
				for(i = 0, pixel = ptr; i < frame.hdr.width;
					i++, pixel += frame.pf->size)
					frame.pf->setRGB(pixel, (i + seed) % maxRGB, (j + seed) % maxRGB,
						(i + j + seed) % maxRGB);
			}
		}

		void makeAnaglyph(Frame &frame, int seed)
		{
			Frame rFrame, gFrame, bFrame;
			int i, j;
			unsigned char *ptr;

			rFrame.init(frame.hdr, PF_COMP, frame.flags, false);
			for(j = 0, ptr = rFrame.bits; j < frame.hdr.height;
				j++, ptr += rFrame.pitch)
			{
				for(i = 0; i < frame.hdr.width; i++) ptr[i] = (i + seed) % 256;
			}
			gFrame.init(frame.hdr, PF_COMP, frame.flags, false);
			for(j = 0, ptr = gFrame.bits; j < frame.hdr.height;
				j++, ptr += gFrame.pitch)
			{
				memset(ptr, (j + seed) % 256, frame.hdr.width);
			}
			bFrame.init(frame.hdr, PF_COMP, frame.flags, false);
			for(j = 0, ptr = bFrame.bits; j < frame.hdr.height;
				j++, ptr += bFrame.pitch)
			{
				for(i = 0; i < frame.hdr.width; i++) ptr[i] = (i + j + seed) % 256;
			}

			frame.makeAnaglyph(rFrame, gFrame, bFrame);
		}

		void shutdown(void)
		{
			fprintf(stderr, "Shutting down....\n");  fflush(stderr);
			if(compressor) compressor->shutdown();
			if(decompressor) decompressor->shutdown();
			if(blitter) blitter->shutdown();
			delete compressor;  compressor = NULL;
			delete decompressor;  decompressor = NULL;
			delete blitter;  blitter = NULL;
		}

		Display *dpy;  Window win;
		Compressor *compressor;  Decompressor *decompressor;  Blitter *blitter;
		int myID;
};


int cmpFrame(unsigned char *buf, int width, int height, Frame &dst)
{
	int _i;  int pitch = width * 3;
	bool dstbu = (dst.flags & FRAME_BOTTOMUP);
	for(int i = 0; i < height; i++)
	{
		_i = dstbu ? i : height - i - 1;
		for(int j = 0; j < height; j++)
		{
			int r, g, b;
			dst.pf->getRGB(&dst.bits[dst.pitch * i + j * dst.pf->size], &r, &g, &b);
			if(dst.pf->bpc == 10)
			{
				r >>= 2;  g >>= 2;  b >>= 2;
			}
			if(r != buf[pitch * _i + j * 3] || g != buf[pitch * _i + j * 3 + 1]
				|| b != buf[pitch * _i + j * 3 + 2])
				return 1;
		}
	}
	return 0;
}


void rgbBench(char *filename)
{
	unsigned char *buf;  int width, height, dstbu;
	CompressedFrame src;  Frame dst;  int dstformat;

	for(dstformat = 0; dstformat < PIXELFORMATS - 1; dstformat++)
	{
		PF *dstpf = pf_get(dstformat);
		for(dstbu = 0; dstbu < 2; dstbu++)
		{
			if(bmp_load(filename, &buf, &width, 1, &height, PF_RGB,
				BMPORN_BOTTOMUP) == -1)
				THROW(bmp_geterr());
			rrframeheader hdr;
			memset(&hdr, 0, sizeof(hdr));
			hdr.width = hdr.framew = width;
			hdr.height = hdr.frameh = height;
			hdr.compress = RRCOMP_RGB;  hdr.size = width * 3 * height;
			src.init(hdr, hdr.flags);
			memcpy(src.bits, buf, width * 3 * height);
			dst.init(hdr, dstpf->id, dstbu ? FRAME_BOTTOMUP : 0);
			memset(dst.bits, 0, dst.pitch * dst.hdr.frameh);
			fprintf(stderr, "RGB (BOTTOM-UP) -> %s (%s)\n", dstpf->name,
				dstbu ? "BOTTOM-UP" : "TOP-DOWN");
			double tStart, tTotal = 0.;  int iter = 0;
			do
			{
				tStart = GetTime();
				dst.decompressRGB(src, width, height, false);
				tTotal += GetTime() - tStart;  iter++;
			} while(tTotal < 1.);
			fprintf(stderr, "%f Mpixels/sec - ", (double)width * (double)height *
				(double)iter / 1000000. / tTotal);
			if(cmpFrame(buf, width, height, dst))
				fprintf(stderr, "FAILED!\n");
			else fprintf(stderr, "Passed.\n");
			free(buf);
		}
		fprintf(stderr, "\n");
	}
}


void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-gl = Use OpenGL instead of X11 for blitting\n");
	fprintf(stderr, "-xv = Test X Video encoding/display\n");
	fprintf(stderr, "-rgb = Use RGB encoding instead of JPEG compression\n");
	fprintf(stderr, "-logo = Add VirtualGL logo\n");
	fprintf(stderr, "-anaglyph = Test anaglyph creation\n");
	fprintf(stderr, "-rgbbench <filename> = Benchmark the decoding of RGB-encoded frames.\n");
	fprintf(stderr, "                       <filename> should be a BMP or PPM file.\n");
	fprintf(stderr, "-v = Verbose output (may affect benchmark results)\n");
	fprintf(stderr, "-check = Check correctness of pixel paths (implies -rgb)\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	Display *dpy = NULL;
	FrameTest *test[NUMWIN];
	int i, j, w, h;
	char *fileName = NULL;
	bool verbose = false;

	if(argc > 1) for(i = 1; i < argc; i++)
	{
		if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
		else if(!stricmp(argv[i], "-gl"))
		{
			fprintf(stderr, "Using OpenGL for blitting ...\n");
			useGL = true;
		}
		else if(!stricmp(argv[i], "-logo")) addLogo = true;
		else if(!stricmp(argv[i], "-anaglyph")) anaglyph = true;
		#ifdef USEXV
		else if(!stricmp(argv[i], "-xv"))
		{
			fprintf(stderr, "Using X Video ...\n");
			useXV = true;
		}
		#endif
		else if(!stricmp(argv[i], "-rgb"))
		{
			fprintf(stderr, "Using RGB encoding ...\n");
			useRGB = true;
		}
		else if(!stricmp(argv[i], "-rgbbench") && i < argc - 1)
		{
			fileName = argv[++i];  doRgbBench = true;
		}
		else if(!stricmp(argv[i], "-v")) verbose = true;
		else if(!stricmp(argv[i], "-check")) { check = true;  useRGB = true; }
		else usage(argv);
	}

	try
	{
		if(doRgbBench) { rgbBench(fileName);  exit(0); }

		ERRIFNOT(XInitThreads());
		if(!(dpy = XOpenDisplay(0)))
		{
			fprintf(stderr, "Could not open display %s\n", XDisplayName(0));
			exit(1);
		}

		for(int format = 0; format < PIXELFORMATS - 1; format++)
		{
			PF *pf = pf_get(format);

			if((useXV || anaglyph || useGL) && pf->bpc != 8) continue;
			if(DefaultDepth(dpy, DefaultScreen(dpy)) != 30 && pf->bpc == 10)
				continue;

			fprintf(stderr, "Pixel format: %s\n", pf->name);

			for(i = 0; i < NUMWIN; i++)
			{
				test[i] = new FrameTest(dpy, i);
			}

			for(w = MINW; w <= MAXW; w += 33)
			{
				h = 1;
				if(verbose) fprintf(stderr, "%.4d x %.4d: ", w, h);
				for(i = 0; i < ITER; i++)
				{
					if(verbose) fprintf(stderr, ".");
					for(j = 0; j < NUMWIN; j++) test[j]->dotest(w, h, i, pf);
				}
				if(verbose) fprintf(stderr, "\n");
			}

			for(h = MINW; h <= MAXW; h += 33)
			{
				w = 1;
				if(verbose) fprintf(stderr, "%.4d x %.4d: ", w, h);
				for(i = 0; i < ITER; i++)
				{
					if(verbose) fprintf(stderr, ".");
					for(j = 0; j < NUMWIN; j++) test[j]->dotest(w, h, i, pf);
				}
				if(verbose) fprintf(stderr, "\n");
			}

			for(w = MINW; w <= MAXW; w += 33)
			{
				h = w;
				if(verbose) fprintf(stderr, "%.4d x %.4d: ", w, h);
				for(i = 0; i < ITER; i++)
				{
					if(verbose) fprintf(stderr, ".");
					for(j = 0; j < NUMWIN; j++) test[j]->dotest(w, h, i, pf);
				}
				if(verbose) fprintf(stderr, "\n");
			}

			for(i = 0; i < NUMWIN; i++)
			{
				delete test[i];
			}
			fprintf(stderr, "\n");
		}
	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s\n%s\n", GET_METHOD(e), e.what());
		if(dpy) XCloseDisplay(dpy);
		exit(1);
	}
	if(dpy) XCloseDisplay(dpy);
	return 0;
}
