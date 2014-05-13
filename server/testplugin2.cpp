/* Copyright (C)2009-2010, 2014 D. R. Commander
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

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "rrtransport.h"
#include "X11Trans.h"

using namespace vglutil;
using namespace vglcommon;
using namespace vglserver;


static Error err;
char errStr[MAXSTR];

static FakerConfig *fconfig=NULL;
static Display *dpy=NULL;
static Window win=0;


FakerConfig *fconfig_instance(void) { return fconfig; }


/* This just wraps the X11Trans class in order to demonstrate how to
   build a custom transport plugin for VGL and also to serve as a sanity
   check for the plugin API */

extern "C" {

void *RRTransInit(Display *dpy_, Window win_, FakerConfig *fconfig_)
{
	void *handle=NULL;
	try
	{
		fconfig=fconfig_;
		dpy=dpy_;
		win=win_;
		_newcheck(handle=(void *)(new X11Trans()));
	}
	catch(Error &e)
	{
		err=e;  return NULL;
	}
	return handle;
}


int RRTransConnect(void *handle, char *receiver_name, int port)
{
	return 0;
}


RRFrame *RRTransGetFrame(void *handle, int width, int height, int format,
	int stereo)
{
	try
	{
		X11Trans *trans=(X11Trans *)handle;
		if(!trans) _throw("Invalid handle");
		RRFrame *frame;
		_newcheck(frame=new RRFrame);
		memset(frame, 0, sizeof(RRFrame));
		FBXFrame *f=trans->getFrame(dpy, win, width, height);
		f->flags|=FRAME_BOTTOMUP;
		frame->opaque=(void *)f;
		frame->w=f->hdr.framew;
		frame->h=f->hdr.frameh;
		frame->pitch=f->pitch;
		frame->bits=f->bits;
		for(int i=0; i<RRTRANS_FORMATOPT; i++)
		{
			if(rrtrans_bgr[i]==(f->flags&FRAME_BGR? 1:0)
				&& rrtrans_afirst[i]==(f->flags&FRAME_ALPHAFIRST? 1:0)
				&& rrtrans_ps[i]==f->pixelSize)
			{
				frame->format=i;  break;
			}
		}
		return frame;
	}
	catch(Error &e)
	{
		err=e;  return NULL;
	}
}


int RRTransReady(void *handle)
{
	int ret=-1;
	try
	{
		X11Trans *trans=(X11Trans *)handle;
		if(!trans) _throw("Invalid handle");
		ret=(int)trans->isReady();
	}
	catch(Error &e)
	{
		err=e;  return -1;
	}
	return ret;
}


int RRTransSynchronize(void *handle)
{
	int ret=0;
	try
	{
		X11Trans *trans=(X11Trans *)handle;
		if(!trans) _throw("Invalid handle");
		trans->synchronize();
	}
	catch(Error &e)
	{
		err=e;  return -1;
	}
	return ret;
}


int RRTransSendFrame(void *handle, RRFrame *frame, int sync)
{
	int ret=0;
	try
	{
		X11Trans *trans=(X11Trans *)handle;
		if(!trans) _throw("Invalid handle");
		FBXFrame *f;
		if(!frame || (f=(FBXFrame *)frame->opaque)==NULL)
			_throw("Invalid frame handle");
		trans->sendFrame(f, (bool)sync);
		delete frame;
	}
	catch(Error &e)
	{
		err=e;  return -1;
	}
	return ret;
}


int RRTransDestroy(void *handle)
{
	int ret=0;
	try
	{
		X11Trans *trans=(X11Trans *)handle;
		if(!trans) _throw("Invalid handle");
		delete trans;
	}
	catch(Error &e)
	{
		err=e;  return -1;
	}
	return ret;
}


const char *RRTransGetError(void)
{
	snprintf(errStr, MAXSTR-1, "Error in %s -- %s",
		err.getMethod(), err.getMessage());
	return errStr;
}


} // extern "C"
