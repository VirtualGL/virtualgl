/* Copyright (C)2009-2010 D. R. Commander
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
#include "rrblitter.h"

static rrerror err;
char errstr[MAXSTR];

static FakerConfig *_fconfig=NULL;
static Display *_dpy=NULL;
static Window _win=0;

FakerConfig *fconfig_instance(void) {return _fconfig;}

/* This just wraps the rrblitter class in order to demonstrate how to
   build a custom transport plugin for VGL and also to serve as a sanity
   check for the plugin API */

extern "C" {

void *RRTransInit(Display *dpy, Window win, FakerConfig *fconfig)
{
	void *handle=NULL;
	try
	{
		_fconfig=fconfig;
		_dpy=dpy;
		_win=win;
		handle=(void *)(new rrblitter());
	}
	catch(rrerror &e)
	{
		err=e;  return NULL;
	}
	return handle;
}

int RRTransConnect(void *handle, char *receiver_name, int port)
{
	return 0;
}

RRFrame *RRTransGetFrame(void *handle, int width, int height, int stereo)
{
	try
	{
		rrblitter *rrb=(rrblitter *)handle;
		if(!rrb) _throw("Invalid handle");
		RRFrame *frame=new RRFrame;
		if(!frame) _throw("Memory allocation error");
		memset(frame, 0, sizeof(RRFrame));
		rrfb *f=rrb->getbitmap(_dpy, _win, width, height);
		f->_flags|=RRBMP_BOTTOMUP;
		frame->opaque=(void *)f;
		frame->w=f->_h.framew;
		frame->h=f->_h.frameh;
		frame->pitch=f->_pitch;
		frame->bits=f->_bits;
		for(int i=0; i<RRTRANS_FORMATOPT; i++)
		{
			if(rrtrans_bgr[i]==(f->_flags&RRBMP_BGR? 1:0)
				&& rrtrans_afirst[i]==(f->_flags&RRBMP_ALPHAFIRST? 1:0)
				&& rrtrans_ps[i]==f->_pixelsize)
				{frame->format=i;  break;}
		}
		return frame;
	}
	catch(rrerror &e)
	{
		err=e;  return NULL;
	}
}

int RRTransReady(void *handle)
{
	int ret=-1;
	try
	{
		rrblitter *rrb=(rrblitter *)handle;
		if(!rrb) _throw("Invalid handle");
		ret=(int)rrb->ready();
	}
	catch(rrerror &e)
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
		rrblitter *rrb=(rrblitter *)handle;
		if(!rrb) _throw("Invalid handle");
		rrb->synchronize();
	}
	catch(rrerror &e)
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
		rrblitter *rrb=(rrblitter *)handle;
		if(!rrb) _throw("Invalid handle");
		rrfb *f;
		if(!frame || (f=(rrfb *)frame->opaque)==NULL)
			_throw("Invalid frame handle");
		rrb->sendframe(f, (bool)sync);
		delete frame;
	}
	catch(rrerror &e)
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
		rrblitter *rrb=(rrblitter *)handle;
		if(!rrb) _throw("Invalid handle");
		delete rrb;
	}
	catch(rrerror &e)
	{
		err=e;  return -1;
	}
	return ret;
}

const char *RRTransGetError(void)
{
	snprintf(errstr, MAXSTR-1, "Error in %s -- %s",
		err.getMethod(), err.getMessage());
	return errstr;
}

}
