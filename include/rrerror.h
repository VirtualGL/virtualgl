/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005 Sun Microsystems, Inc.
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

#ifndef __RRERROR_H__
#define __RRERROR_H__

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <new>

class rrerror
{
	public:

		rrerror(const char *method, char *message)
		{
			init(method, message, -1);
		}

		rrerror(const char *method, const char *message)
		{
			init(method, (char *)message, -1);
		}

		rrerror(const char *method, char *message, int line)
		{
			init(method, message, line);
		}

		rrerror(const char *method, const char *message, int line)
		{
			init(method, (char *)message, line);
		}

		void init(const char *method, char *message, int line)
		{
			_message[0]=0;
			if(line>=1) sprintf(_message, "%d: ", line);
			if(!method) method="(Unknown error location)";
			_method=method;
			if(message) strncpy(&_message[strlen(_message)], message, MLEN-strlen(_message));
		}
			
		rrerror(void) : _method(NULL) {_message[0]=0;}

		operator bool() {return (_method!=NULL && _message[0]!=0);}

		const char *getMethod(void) {return _method;}
		char *getMessage(void) {return _message;}

	protected:

		static const int MLEN=256;
		const char *_method;  char _message[MLEN+1];
};

#if defined(sgi)||defined(sun)
#define __FUNCTION__ __FILE__
#endif
#define _throw(m) throw(rrerror(__FUNCTION__, m, __LINE__))
#define errifnot(f) {if(!(f)) _throw("Unexpected NULL condition");}
#define newcheck(f) \
	try {if(!(f)) _throw("Memory allocation error");} \
	catch(std::bad_alloc& e) {_throw(e.what());}

#ifdef _WIN32
class w32error : public rrerror
{
	public:

		w32error(const char *method) : rrerror(method, (char *)NULL)
		{
			if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), _message, MLEN, NULL))
				strncpy(_message, "Error in FormatMessage()", MLEN);
		}

		w32error(const char *method, DWORD lasterr) : rrerror(method, (char *)NULL)
		{
			if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lasterr,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), _message, MLEN, NULL))
				strncpy(_message, "Error in FormatMessage()", MLEN);
		}

		w32error(const char *method, int line) : rrerror(method, (char *)NULL, line)
		{
			if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &_message[strlen(_message)],
				MLEN-strlen(_message), NULL))
				strncpy(_message, "Error in FormatMessage()", MLEN);
		}

		w32error(const char *method, int line, char *message) :
			rrerror(method, (char *)NULL, line)
		{
			if(strlen(_message)<MLEN-2)
			{
				strncpy(&_message[strlen(_message)], message, MLEN-strlen(_message));
				strcat(_message, ". ");
			}
			if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &_message[strlen(_message)],
				MLEN-strlen(_message), NULL))
				strncpy(_message, "Error in FormatMessage()", MLEN);
		}
};

#define _throww32() throw(w32error(__FUNCTION__, __LINE__))
#define _throww32m(message) throw(w32error(__FUNCTION__, __LINE__, message))
#define tryw32(f) {if(!(f)) _throww32();}

#endif

class unixerror : public rrerror
{
	public:
		unixerror(const char *method) : rrerror(method, strerror(errno)) {}
		unixerror(const char *method, int line) : rrerror(method, strerror(errno), line) {}
};

#define _throwunix() throw(unixerror(__FUNCTION__, __LINE__))
#define tryunix(f) {if((f)==-1) _throwunix();}

#define fbx(f) {if((f)==-1) throw(rrerror("FBX", fbx_geterrmsg(), fbx_geterrline()));}
#define tj(f) {if((f)==-1) throw(rrerror(__FUNCTION__, tjGetErrorStr(), __LINE__));}

#endif

