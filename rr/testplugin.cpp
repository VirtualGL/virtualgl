/* Copyright (C)2009 D. R. Commander
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
#include "rrtransport.h"
#include "rrdisplayclient.h"

static rrerror err;
char errstr[MAXSTR];

static FakerConfig *_fconfig=NULL;

FakerConfig *fconfig_instance(void) {return _fconfig;}

/* This just wraps the rrdisplayclient class in order to demonstrate how to
   build a custom transport plugin for VGL and also to serve as a sanity
   check for the plugin API */

extern "C" {

void *RRTransInit(FakerConfig *fconfig)
{
	void *handle=NULL;
	try
	{
		_fconfig=fconfig;
		handle=(void *)(new rrdisplayclient());
	}
	catch(rrerror &e)
	{
		err=e;  return NULL;
	}
	return handle;
}

int RRTransConnect(void *handle, char *receiver_name, int port)
{
	int ret=0;
	try
	{
		rrdisplayclient *rrdpy=(rrdisplayclient *)handle;
		if(!rrdpy) _throw("Invalid handle");
		rrdpy->connect(receiver_name, port);
	}
	catch(rrerror &e)
	{
		err=e;  return -1;
	}
	return ret;
}

RRFrame *RRTransGetFrame(void *handle, int width, int height, int stereo,
	int spoil)
{
	try
	{
		rrdisplayclient *rrdpy=(rrdisplayclient *)handle;
		if(!rrdpy) _throw("Invalid handle");
		RRFrame *frame=new RRFrame;
		if(!frame) _throw("Memory allocation error");
		memset(frame, 0, sizeof(RRFrame));
		int compress=_fconfig->compress;
		if(compress<RRCOMP_JPEG || compress>RRCOMP_RGB) compress=RRCOMP_JPEG;
		int flags=RRBMP_BOTTOMUP;
		if(littleendian() && compress!=RRCOMP_RGB) flags|=RRBMP_BGR;
		rrframe *f=rrdpy->getbitmap(width, height, 3, flags, (bool)stereo,
			(bool)spoil);
		f->_h.compress=compress;
		frame->opaque=(void *)f;
		frame->w=f->_h.framew;
		frame->h=f->_h.frameh;
		frame->pitch=f->_pitch;
		frame->bits=f->_bits;
		frame->rbits=f->_rbits;
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

int RRTransFrameReady(void *handle)
{
	int ret=-1;
	try
	{
		rrdisplayclient *rrdpy=(rrdisplayclient *)handle;
		if(!rrdpy) _throw("Invalid handle");
		ret=(int)rrdpy->frameready();
	}
	catch(rrerror &e)
	{
		err=e;  return -1;
	}
	return ret;
}

int RRTransSendFrame(void *handle, RRFrame *frame)
{
	int ret=0;
	try
	{
		rrdisplayclient *rrdpy=(rrdisplayclient *)handle;
		if(!rrdpy) _throw("Invalid handle");
		rrframe *f;
		if(!frame || (f=(rrframe *)frame->opaque)==NULL)
			_throw("Invalid frame handle");
		f->_h.qual=_fconfig->qual;
		f->_h.subsamp=_fconfig->subsamp;
		f->_h.winid=frame->winid;
		rrdpy->sendframe(f);
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
		rrdisplayclient *rrdpy=(rrdisplayclient *)handle;
		if(!rrdpy) _throw("Invalid handle");
		delete rrdpy;
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
