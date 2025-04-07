// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2014, 2019, 2025 D. R. Commander
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

#ifndef __ERROR_H__
#define __ERROR_H__

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <new>
#include "vglutil.h"


namespace vglutil
{
	class Error
	{
		public:

			Error(const char *method_, char *message_)
			{
				init(method_, message_, -1);
			}

			Error(const char *method_, const char *message_)
			{
				init(method_, (char *)message_, -1);
			}

			Error(const char *method_, char *message_, int line)
			{
				init(method_, message_, line);
			}

			Error(const char *method_, const char *message_, int line)
			{
				init(method_, (char *)message_, line);
			}

			void init(const char *method_, char *message_, int line)
			{
				message[0] = 0;
				if(line >= 1) snprintf(message, MLEN + 1, "%d: ", line);
				if(!method_) method_ = "(Unknown error location)";
				method = method_;
				if(message_)
					strncpy(&message[strlen(message)], message_, MLEN - strlen(message));
			}

			Error(void) : method(NULL) { message[0] = 0; }

			operator bool() { return method != NULL && message[0] != 0; }

			const char *getMethod(void) { return method; }
			char *getMessage(void) { return message; }

		protected:

			static const int MLEN = 256;
			const char *method;  char message[MLEN + 1];
	};
}


#if defined(sgi)
#define __FUNCTION__  __FILE__
#endif
#define THROW(m)  throw(vglutil::Error(__FUNCTION__, m, __LINE__))
#define ERRIFNOT(f)  { if(!(f)) THROW("Unexpected NULL condition"); }
#define NEWCHECK(f) \
	try \
	{ \
		if(!(f)) THROW("Memory allocation error"); \
	} \
	catch(std::bad_alloc &e) \
	{ \
		THROW(e.what()); \
	}


#ifdef _WIN32

namespace vglutil
{
	class W32Error : public Error
	{
		public:

			W32Error(const char *method) : Error(method, (char *)NULL)
			{
				if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), message, MLEN, NULL))
					strncpy(message, "Error in FormatMessage()", MLEN);
			}

			W32Error(const char *method, int line) :
				Error(method, (char *)NULL, line)
			{
				if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &message[strlen(message)],
					MLEN - (DWORD)strlen(message), NULL))
					strncpy(message, "Error in FormatMessage()", MLEN);
			}
	};
}

#define THROW_W32()  throw(vglutil::W32Error(__FUNCTION__, __LINE__))
#define TRY_W32(f)  { if(!(f)) THROW_W32(); }

#endif  // _WIN32


namespace vglutil
{
	class UnixError : public Error
	{
		public:

			UnixError(const char *method_) : Error(method_, strerror(errno)) {}
			UnixError(const char *method_, int line) :
				Error(method_, strerror(errno), line) {}
	};
}

#define THROW_UNIX()  throw(vglutil::UnixError(__FUNCTION__, __LINE__))
#define TRY_UNIX(f)  { if((f) == -1) THROW_UNIX(); }


#define TRY_FBX(f) \
{ \
	if((f) == -1) \
		throw(vglutil::Error("FBX", fbx_geterrmsg(), fbx_geterrline())); \
}
#define TRY_FBXV(f) \
{ \
	if((f) == -1) \
		throw(vglutil::Error("FBXV", fbxv_geterrmsg(), fbxv_geterrline())); \
}
#define TRY_TJ(f) \
{ \
	if((f) == -1) \
		throw(vglutil::Error(__FUNCTION__, tjGetErrorStr(), __LINE__)); \
}

#endif  // __ERROR_H__
