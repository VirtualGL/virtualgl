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

#include "rrdisplayclient.h"

// C wrappers

#define checkhandle(h,s) rrdisplayclient *rrc;  if((rrc=(rrdisplayclient *)h)==NULL) \
	{_throwlasterror("Invalid handle in "s);  return RR_ERROR;}

#define _setlasterror(f,l,m) {__lasterror.file=f;  __lasterror.line=l;  \
	__lasterror.message=m;}
#define _throwlasterror(m) _setlasterror(__FILE__, __LINE__, m)

#define _catch() catch(RRError e) {__lasterror=e;}

extern RRError __lasterror;

extern "C" {

DLLEXPORT RRDisplay DLLCALL
	RROpenDisplay(char *servername, unsigned short port, int ssl)
{
	hputil_log(1, 0);
	try
	{
		rrdisplayclient *rrc=new rrdisplayclient(servername, port, ssl);
		if(!rrc) {_throwlasterror("Could not allocate memory for class");  return NULL;}
		return (RRDisplay)rrc;
	}
	_catch()
	return NULL;
}

DLLEXPORT int DLLCALL
	RRCloseDisplay(RRDisplay rrdpy)
{
	checkhandle(rrdpy, "RRCloseDisplay()");
	try
	{
		delete rrc;
		return RR_SUCCESS;
	}
	_catch()
	return RR_ERROR;
}

DLLEXPORT rrbmp* DLLCALL
	RRGetBitmap(RRDisplay rrdpy, int width, int height, int pixelsize)
{
	rrdisplayclient *rrc;
	if((rrc=(rrdisplayclient *)rrdpy)==NULL)
		{_throwlasterror("Invalid handle in RRGetBitmap()");  return NULL;}
	try
	{
		return rrc->getbitmap(width, height, pixelsize);
	}
	_catch()
	return NULL;
}

DLLEXPORT int DLLCALL
	RRFrameReady(RRDisplay rrdpy)
{
	checkhandle(rrdpy, "RRFrameReady()");
	try
	{
		return rrc->frameready()?1:0;
	}
	_catch()
	return -1;
}

DLLEXPORT int DLLCALL
	RRSendFrame(RRDisplay rrdpy, rrbmp *frame)
{
	checkhandle(rrdpy, "RRSendFrame()");
	try
	{
		rrc->sendframe(frame);
		return RR_SUCCESS;
	}
	_catch()
	return RR_ERROR;
}

}
