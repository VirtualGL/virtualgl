// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011, 2014, 2019, 2021 D. R. Commander
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

#include "ClientWin.h"
#include "Error.h"
#include "Log.h"
#include "Profiler.h"
#include "GLFrame.h"

using namespace util;
using namespace common;
using namespace client;


extern Display *maindpy;


ClientWin::ClientWin(int dpynum_, Window window_, int drawMethod_,
	bool stereo_) : drawMethod(drawMethod_), reqDrawMethod(drawMethod_),
	fb(NULL), cfindex(0), deadYet(false), thread(NULL), stereo(stereo_)
{
	if(dpynum_ < 0 || dpynum_ > 65535 || !window_)
		throw(Error("ClientWin::ClientWin()", "Invalid argument"));
	dpynum = dpynum_;  window = window_;

	#ifdef USEXV
	for(int i = 0; i < NFRAMES; i++) xvframes[i] = NULL;
	#endif
	if(drawMethod == RR_DRAWAUTO) drawMethod = RR_DRAWX11;
	if(stereo) drawMethod = RR_DRAWOGL;
	initGL();
	initX11();

	thread = new Thread(this);
	thread->start();
}


ClientWin::~ClientWin(void)
{
	deadYet = true;
	q.release();
	if(thread) thread->stop();
	delete fb;  fb = NULL;
	#ifdef USEXV
	for(int i = 0; i < NFRAMES; i++)
	{
		if(xvframes[i])
		{
			xvframes[i]->signalComplete();  delete xvframes[i];  xvframes[i] = NULL;
		}
	}
	#endif
	for(int i = 0; i < NFRAMES; i++) cframes[i].signalComplete();
	delete thread;  thread = NULL;
}


void ClientWin::initGL(void)
{
	GLFrame *newfb = NULL;
	char dpystr[80];
	sprintf(dpystr, ":%d.0", dpynum);
	CriticalSection::SafeLock l(mutex);

	if(drawMethod == RR_DRAWOGL)
	{
		try
		{
			newfb = new GLFrame(dpystr, window);
			if(!newfb) throw("Could not allocate class instance");
		}
		catch(std::exception &e)
		{
			vglout.println("OpenGL error-- %s\nUsing X11 drawing instead", e.what());
			delete newfb;  newfb = NULL;
			drawMethod = RR_DRAWX11;
			vglout.PRINTLN("Stereo requires OpenGL drawing.  Disabling stereo.");
			stereo = false;
			return;
		}
	}
	if(newfb)
	{
		if(fb)
		{
			if(fb->isGL) delete ((GLFrame *)fb);
			else delete ((FBXFrame *)fb);
		}
		fb = (Frame *)newfb;
	}
}


void ClientWin::initX11(void)
{
	FBXFrame *newfb = NULL;
	char dpystr[80];
	sprintf(dpystr, ":%d.0", dpynum);
	CriticalSection::SafeLock l(mutex);

	if(drawMethod == RR_DRAWX11)
	{
		try
		{
			newfb = new FBXFrame(dpystr, window);
			if(!newfb) throw("Could not allocate class instance");
		}
		catch(...)
		{
			delete newfb;  newfb = NULL;
			throw;
		}
	}
	if(newfb)
	{
		if(fb)
		{
			if(fb->isGL) { delete ((GLFrame *)fb); }
			else delete ((FBXFrame *)fb);
		}
		fb = (Frame *)newfb;
	}
}


int ClientWin::match(int dpynum_, Window window_)
{
	return dpynum == dpynum_ && window == window_;
}


Frame *ClientWin::getFrame(bool useXV)
{
	Frame *f = NULL;

	if(thread) thread->checkError();
	cfmutex.lock();
	#ifdef USEXV
	if(useXV)
	{
		if(!xvframes[cfindex])
		{
			char dpystr[80];
			sprintf(dpystr, ":%d.0", dpynum);
			xvframes[cfindex] = new XVFrame(dpystr, window);
			if(!xvframes[cfindex]) THROW("Could not allocate class instance");
		}
		f = (Frame *)xvframes[cfindex];
	}
	else
	#endif
	f = (Frame *)&cframes[cfindex];
	cfindex = (cfindex + 1) % NFRAMES;
	cfmutex.unlock();
	f->waitUntilComplete();
	if(thread) thread->checkError();
	return f;
}


void ClientWin::drawFrame(Frame *f)
{
	if(thread) thread->checkError();
	if(!f->isXV)
	{
		CompressedFrame *c = (CompressedFrame *)f;
		if((c->rhdr.flags == RR_RIGHT || c->hdr.flags == RR_LEFT) && !stereo)
		{
			stereo = true;
			if(drawMethod != RR_DRAWOGL)
			{
				drawMethod = RR_DRAWOGL;
				initGL();
			}
		}
		if((c->hdr.flags == 0) && stereo)
		{
			stereo = false;
			drawMethod = reqDrawMethod;
			if(drawMethod == RR_DRAWAUTO) drawMethod = RR_DRAWX11;
			initX11();
		}
	}
	q.add(f);
}


void ClientWin::run(void)
{
	Profiler pt("Total     "), pb("Blit      "), pd("Decompress");
	Frame *f = NULL;  long bytes = 0;

	try
	{
		while(!deadYet)
		{
			void *ftemp = NULL;
			q.get(&ftemp);  f = (Frame *)ftemp;  if(deadYet) break;
			if(!f)
				throw(Error("ClientWin::run()", "Invalid image received from queue"));
			CriticalSection::SafeLock l(mutex);
			#ifdef USEXV
			if(f->isXV)
			{
				if(f->hdr.flags != RR_EOF)
				{
					pb.startFrame();
					((XVFrame *)f)->redraw();
					pb.endFrame(f->hdr.width * f->hdr.height, 0, 1);
					pt.endFrame(f->hdr.width * f->hdr.height, bytes, 1);
					bytes = 0;
					pt.startFrame();
				}
			}
			else
			#endif
			{
				if(f->hdr.flags == RR_EOF)
				{
					pb.startFrame();
					if(fb->isGL) ((GLFrame *)fb)->init(f->hdr, stereo);
					else ((FBXFrame *)fb)->init(f->hdr);
					if(fb->isGL) ((GLFrame *)fb)->redraw();
					else ((FBXFrame *)fb)->redraw();
					pb.endFrame(fb->hdr.framew * fb->hdr.frameh, 0, 1);
					pt.endFrame(fb->hdr.framew * fb->hdr.frameh, bytes, 1);
					bytes = 0;
					pt.startFrame();
				}
				else
				{
					pd.startFrame();
					if(fb->isGL) *((GLFrame *)fb) = *((CompressedFrame *)f);
					else *((FBXFrame *)fb) = *((CompressedFrame *)f);
					pd.endFrame(f->hdr.width * f->hdr.height, 0,
						(double)(f->hdr.width * f->hdr.height) /
							(double)(f->hdr.framew * f->hdr.frameh));
					bytes += f->hdr.size;
				}
			}
			f->signalComplete();
		}

	}
	catch(std::exception &e)
	{
		if(thread) thread->setError(e);
		if(f) f->signalComplete();
		throw;
	}
}
