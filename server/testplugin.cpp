/* Copyright (C)2009-2011, 2014 D. R. Commander
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
#include "VGLTrans.h"

using namespace vglutil;
using namespace vglcommon;
using namespace vglserver;


static Error err;
char errStr[MAXSTR];

static FakerConfig *fconfig=NULL;
static Window win=0;


FakerConfig *fconfig_instance(void) { return fconfig; }


/* This just wraps the VGLTrans class in order to demonstrate how to build a
   custom transport plugin for VGL and also to serve as a sanity check for the
   plugin API */

extern "C" {

void *RRTransInit(Display *dpy, Window win_, FakerConfig *fconfig_)
{
	void *handle=NULL;
	try
	{
		fconfig=fconfig_;
		win=win_;
		_newcheck(handle=(void *)(new VGLTrans()));
	}
	catch(Error &e)
	{
		err=e;  return NULL;
	}
	return handle;
}


int RRTransConnect(void *handle, char *receiverName, int port)
{
	int ret=0;
	try
	{
		VGLTrans *vglconn=(VGLTrans *)handle;
		if(!vglconn) _throw("Invalid handle");
		vglconn->connect(receiverName, port);
	}
	catch(Error &e)
	{
		err=e;  return -1;
	}
	return ret;
}


RRFrame *RRTransGetFrame(void *handle, int width, int height, int format,
	int stereo)
{
	try
	{
		VGLTrans *vglconn=(VGLTrans *)handle;
		if(!vglconn) _throw("Invalid handle");
		RRFrame *frame;
		_newcheck(frame=new RRFrame);
		memset(frame, 0, sizeof(RRFrame));
		int compress=fconfig->compress;
		if(compress==RRCOMP_PROXY || compress==RRCOMP_RGB) compress=RRCOMP_RGB;
		else compress=RRCOMP_JPEG;
		int flags=FRAME_BOTTOMUP, pixelsize=3;
		if(compress!=RRCOMP_RGB)
		{
			switch(format)
			{
				case RRTRANS_BGR:
					flags|=FRAME_BGR;  break;
				case RRTRANS_RGBA:
					pixelsize=4;  break;
				case RRTRANS_BGRA:
					flags|=FRAME_BGR;  pixelsize=4;  break;
				case RRTRANS_ABGR:
					flags|=(FRAME_BGR|FRAME_ALPHAFIRST);  pixelsize=4;  break;
				case RRTRANS_ARGB:
					flags|=FRAME_ALPHAFIRST;  pixelsize=4;  break;
			}
		}
		Frame *f=vglconn->getFrame(width, height, pixelsize, flags, (bool)stereo);
		f->hdr.compress=compress;
		frame->opaque=(void *)f;
		frame->w=f->hdr.framew;
		frame->h=f->hdr.frameh;
		frame->pitch=f->pitch;
		frame->bits=f->bits;
		frame->rbits=f->rbits;
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
		VGLTrans *vglconn=(VGLTrans *)handle;
		if(!vglconn) _throw("Invalid handle");
		ret=(int)vglconn->isReady();
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
		VGLTrans *vglconn=(VGLTrans *)handle;
		if(!vglconn) _throw("Invalid handle");
		vglconn->synchronize();
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
		VGLTrans *vglconn=(VGLTrans *)handle;
		if(!vglconn) _throw("Invalid handle");
		Frame *f;
		if(!frame || (f=(Frame *)frame->opaque)==NULL)
			_throw("Invalid frame handle");
		f->hdr.qual=fconfig->qual;
		f->hdr.subsamp=fconfig->subsamp;
		f->hdr.winid=win;
		vglconn->sendFrame(f);
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
		VGLTrans *vglconn=(VGLTrans *)handle;
		if(!vglconn) _throw("Invalid handle");
		delete vglconn;
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
