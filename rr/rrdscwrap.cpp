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

#include "rrdisplayserver.h"

// C wrappers

#define checkhandle(h,s) rrdisplayserver *rrs;  if((rrs=(rrdisplayserver *)h)==NULL) \
	{_throwlasterror("Invalid handle in "s);  return RR_ERROR;}

#define _setlasterror(f,l,m) {__lasterror.file=f;  __lasterror.line=l;  \
	__lasterror.message=m;}
#define _throwlasterror(m) _setlasterror(__FILE__, __LINE__, m)

#define _catch() catch(RRError e) {__lasterror=e;}

RRError __lasterror={"No File", 0, "No Error"};

extern "C" {

DLLEXPORT RRDisplay DLLCALL
	RRInitDisplay(unsigned short port, int dossl)
{
	try
	{
		rrdisplayserver *rrs=new rrdisplayserver(port, dossl!=0);
		if(!rrs) {_throwlasterror("Could not allocate memory for class");  return NULL;}
		return (RRDisplay)rrs;
	}
	_catch()
	return NULL;
}

DLLEXPORT int DLLCALL
	RRTerminateDisplay(RRDisplay h)
{
	checkhandle(h, "RRTerminateDisplay()");
	try
	{
		delete rrs;
		return RR_SUCCESS;
	}
	_catch()
	return RR_ERROR;
}

DLLEXPORT RRError DLLCALL
	RRGetError(void)
{
	return __lasterror;
}

}
