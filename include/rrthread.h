/* Copyright (C)2004 Landmark Graphics Corporation
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

#ifndef __RRTHREAD_H__
#define __RRTHREAD_H__

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#include "rrerror.h"


inline unsigned long rrthread_id(void)
{
	#ifdef _WIN32
	return GetCurrentThreadId();
	#else
	return (unsigned long)pthread_self();
	#endif
}


// This class implements Java-like threads in C++

class Runnable
{
	public:
		Runnable(void) {}
		virtual ~Runnable(void) {}

	protected:
		virtual void run()=0;
		unsigned long _threadId;
		rrerror lasterror;
		friend class Thread;
};


class Thread
{
	public:

		Thread(Runnable *r) : _obj(r), _handle(0), _detached(false) {}

		void start(void)
		{
			if(!_obj) throw (rrerror("Thread::start()", "Unexpected NULL pointer"));
			#ifdef _WIN32
			DWORD tid;
			if((_handle=CreateThread(NULL, 0, threadfunc, _obj, 0, &tid))==NULL)
				throw (w32error("Thread::start()"));
			#else
			int err=0;
			if((err=pthread_create(&_handle, NULL, threadfunc, _obj))!=0)
				throw (rrerror("Thread::start()", strerror(err==-1?errno:err)));
			#endif
		}

		void stop(void)
		{
			#ifdef _WIN32
			if(_handle)
			{
				WaitForSingleObject(_handle, INFINITE);  CloseHandle(_handle);
			}
			#else
			if(_handle && !_detached) pthread_join(_handle, NULL);
			#endif
			_handle=0;
		}

		// This allows a Unix thread to kill itself.  It has no effect on Windows.
		void detach(void)
		{
			#ifndef _WIN32
			pthread_detach(_handle);
			_detached=true;
			#endif
		}

		void seterror(rrerror &e)
		{
			if(_obj) _obj->lasterror=e;
		}

		void checkerror(void)
		{
			if(_obj && _obj->lasterror) throw _obj->lasterror;
		}

	protected:

		Runnable *_obj;

		#ifdef _WIN32
		static DWORD WINAPI threadfunc(void *param)
		#else
		static void *threadfunc(void *param)
		#endif
		{
			try {
			((Runnable *)param)->_threadId=rrthread_id();
			((Runnable *)param)->run();
			}
			catch(rrerror &e)
			{
				((Runnable *)param)->lasterror=e;
			}
			return 0;
		}

		#ifdef _WIN32
		HANDLE _handle;
		#else
		pthread_t _handle;
		#endif
		bool _detached;
};


#endif // __RRTHREAD_H__
