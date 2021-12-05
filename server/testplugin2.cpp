// Copyright (C)2009-2010, 2014, 2017-2021 D. R. Commander
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

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "rrtransport.h"
#include "X11Trans.h"
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif

extern "C" void _vgl_disableFaker(void);
extern "C" void _vgl_enableFaker(void);

using namespace common;
using namespace server;


static __thread char errStr[MAXSTR + 14];

static FakerConfig *fconfig = NULL;
static __thread Display *dpy = NULL;
static __thread Window win = 0;

static const int pf2trans[PIXELFORMATS] =
{
	RRTRANS_RGB, RRTRANS_RGBA, -1, RRTRANS_BGR, RRTRANS_BGRA, -1, RRTRANS_ABGR,
	-1, RRTRANS_ARGB, -1, -1
};


FakerConfig *fconfig_getinstance(void)
{
	#ifdef USEHELGRIND
	ANNOTATE_BENIGN_RACE_SIZED(&fconfig, sizeof(FakerConfig *), );
	#endif
	return fconfig;
}


// This just wraps the X11Trans class in order to demonstrate how to
// build a custom transport plugin for VGL and also to serve as a sanity
// check for the plugin API

extern "C" {

void *RRTransInit(Display *dpy_, Window win_, FakerConfig *fconfig_)
{
	_vgl_disableFaker();

	void *handle = NULL;
	try
	{
		#ifdef USEHELGRIND
		ANNOTATE_BENIGN_RACE_SIZED(&fconfig, sizeof(FakerConfig *), );
		#endif
		fconfig = fconfig_;
		dpy = dpy_;
		win = win_;
		handle = (void *)(new X11Trans());
	}
	catch(std::exception &e)
	{
		snprintf(errStr, MAXSTR + 14, "Error in %s -- %s", GET_METHOD(e),
			e.what());
		handle = NULL;
	}

	_vgl_enableFaker();

	return handle;
}


int RRTransConnect(void *handle, char *receiver_name, int port)
{
	return 0;
}


RRFrame *RRTransGetFrame(void *handle, int width, int height, int format,
	int stereo)
{
	_vgl_disableFaker();

	RRFrame *frame = NULL;
	try
	{
		X11Trans *trans = (X11Trans *)handle;
		if(!trans) THROW("Invalid handle");
		frame = new RRFrame;
		memset(frame, 0, sizeof(RRFrame));
		FBXFrame *f = trans->getFrame(dpy, win, width, height);
		f->flags |= FRAME_BOTTOMUP;
		frame->opaque = (void *)f;
		frame->w = f->hdr.framew;
		frame->h = f->hdr.frameh;
		frame->pitch = f->pitch;
		frame->bits = f->bits;
		frame->format = pf2trans[f->pf->id];
		if(frame->format < 0) THROW("Unsupported pixel format");
	}
	catch(std::exception &e)
	{
		snprintf(errStr, MAXSTR + 14, "Error in %s -- %s", GET_METHOD(e),
			e.what());
		delete frame;
		frame = NULL;
	}

	_vgl_enableFaker();

	return frame;
}


int RRTransReady(void *handle)
{
	_vgl_disableFaker();

	int ret = -1;
	try
	{
		X11Trans *trans = (X11Trans *)handle;
		if(!trans) THROW("Invalid handle");
		ret = (int)trans->isReady();
	}
	catch(std::exception &e)
	{
		snprintf(errStr, MAXSTR + 14, "Error in %s -- %s", GET_METHOD(e),
			e.what());
		ret = -1;
	}

	_vgl_enableFaker();

	return ret;
}


int RRTransSynchronize(void *handle)
{
	_vgl_disableFaker();

	int ret = 0;
	try
	{
		X11Trans *trans = (X11Trans *)handle;
		if(!trans) THROW("Invalid handle");
		trans->synchronize();
	}
	catch(std::exception &e)
	{
		snprintf(errStr, MAXSTR + 14, "Error in %s -- %s", GET_METHOD(e),
			e.what());
		ret = -1;
	}

	_vgl_enableFaker();

	return ret;
}


int RRTransSendFrame(void *handle, RRFrame *frame, int sync)
{
	_vgl_disableFaker();

	int ret = 0;
	try
	{
		X11Trans *trans = (X11Trans *)handle;
		if(!trans) THROW("Invalid handle");
		FBXFrame *f;
		if(!frame || (f = (FBXFrame *)frame->opaque) == NULL)
			THROW("Invalid frame handle");
		trans->sendFrame(f, (bool)sync);
		delete frame;
	}
	catch(std::exception &e)
	{
		snprintf(errStr, MAXSTR + 14, "Error in %s -- %s", GET_METHOD(e),
			e.what());
		ret = -1;
	}

	_vgl_enableFaker();

	return ret;
}


int RRTransDestroy(void *handle)
{
	_vgl_disableFaker();

	int ret = 0;
	try
	{
		X11Trans *trans = (X11Trans *)handle;
		if(!trans) THROW("Invalid handle");
		delete trans;
	}
	catch(std::exception &e)
	{
		snprintf(errStr, MAXSTR + 14, "Error in %s -- %s", GET_METHOD(e),
			e.what());
		ret = -1;
	}

	_vgl_enableFaker();

	return ret;
}


const char *RRTransGetError(void)
{
	return errStr;
}


}  // extern "C"
