/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011 D. R. Commander
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

#ifndef __RRMUTEX_H__
#define __RRMUTEX_H__

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#endif
#include "rrerror.h"


class rrevent
{
	public:

		rrevent(void)
		{
			#ifdef _WIN32
			_event=CreateEvent(NULL, FALSE, TRUE, NULL);
			#else
			_ready=true;  _deadyet=false;
			pthread_mutex_init(&_event, NULL);
			pthread_cond_init(&_cond, NULL);
			#endif
		}

		~rrevent(void)
		{
			#ifdef _WIN32
			if(_event) {SetEvent(_event);  CloseHandle(_event);  _event=NULL;}
			#else
			pthread_mutex_lock(&_event);
			_ready=true;  _deadyet=true;
			pthread_mutex_unlock(&_event);
			pthread_cond_signal(&_cond);
			pthread_mutex_destroy(&_event);
			#endif
		}

		void wait(void)
		{
			#ifdef _WIN32
			if(WaitForSingleObject(_event, INFINITE)==WAIT_FAILED)
				throw(w32error("rrevent::wait()"));
			#else
			int ret;
			if((ret=pthread_mutex_lock(&_event))!=0)
				throw(rrerror("rrevent::wait()", strerror(ret)));
			while(!_ready && !_deadyet)
				if((ret=pthread_cond_wait(&_cond, &_event))!=0)
				{
					pthread_mutex_unlock(&_event);
					throw(rrerror("rrevent::wait()", strerror(ret)));
				}
			_ready=false;
			if((ret=pthread_mutex_unlock(&_event))!=0)
				throw(rrerror("rrevent::wait()", strerror(ret)));
			#endif
		}

		void signal(void)
		{
			#ifdef _WIN32
			if(!SetEvent(_event)) throw(w32error("rrevent::signal()"));
			#else
			int ret;
			if((ret=pthread_mutex_lock(&_event))!=0)
				throw(rrerror("rrevent::signal()", strerror(ret)));
			_ready=true;
			if((ret=pthread_mutex_unlock(&_event))!=0)
				throw(rrerror("rrevent::signal()", strerror(ret)));
			if((ret=pthread_cond_signal(&_cond))!=0)
				throw(rrerror("rrevent::signal()", strerror(ret)));
			#endif
		}

		bool locked(void)
		{
			bool islocked=true;
			#ifdef _WIN32
			DWORD dw=WaitForSingleObject(_event, 0);
			if(dw==WAIT_FAILED) throw(w32error("rrevent::locked"));
			else if(dw==WAIT_OBJECT_0)
			{
				islocked=false;  SetEvent(_event);
			}
			else if(dw==WAIT_TIMEOUT) islocked=true;
			#else
			int ret;
			if((ret=pthread_mutex_lock(&_event))!=0)
				throw(rrerror("rrevent::locked()", strerror(ret)));
			islocked=!_ready;
			if((ret=pthread_mutex_unlock(&_event))!=0)
				throw(rrerror("rrevent::signal()", strerror(ret)));
			#endif
			return islocked;
		}

	private:

		#ifdef _WIN32
		HANDLE _event;
		#else
		pthread_mutex_t _event;
		pthread_cond_t _cond;
		bool _ready, _deadyet;
		#endif
};


// Critical section (recursive mutex)

class rrcs
{
	public:

		rrcs(void)
		{
			#ifdef _WIN32
			_cs=CreateMutex(NULL, FALSE, NULL);
			#else
			pthread_mutexattr_t ma;
			pthread_mutexattr_init(&ma);
			pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&_cs, &ma);
			pthread_mutexattr_destroy(&ma);
			#endif
		}

		~rrcs(void)
		{
			#ifdef _WIN32
			if(_cs) {ReleaseMutex(_cs);  CloseHandle(_cs);  _cs=NULL;}
			#else
			pthread_mutex_unlock(&_cs);
			pthread_mutex_destroy(&_cs);
			#endif
		}

		void lock(bool errorcheck=true)
		{
			#ifdef _WIN32
			if(WaitForSingleObject(_cs, INFINITE)==WAIT_FAILED && errorcheck)
				throw(w32error("rrcs::lock()"));
			#else
			int ret;
			if((ret=pthread_mutex_lock(&_cs))!=0 && errorcheck)
				throw(rrerror("rrcs::lock()", strerror(ret)));
			#endif
		}

		void unlock(bool errorcheck=true)
		{
			#ifdef _WIN32
			if(!ReleaseMutex(_cs) && errorcheck) throw(w32error("rrcs::unlock()"));
			#else
			int ret;
			if((ret=pthread_mutex_unlock(&_cs))!=0 && errorcheck)
				throw(rrerror("rrcs::unlock()", strerror(ret)));
			#endif
		}

		class safelock
		{
			public:
				safelock(rrcs &cs, bool errorcheck=true) : _cs(cs),
					_errorcheck(errorcheck)
				{
					_cs.lock(errorcheck);
				}
				~safelock() {_cs.unlock(_errorcheck);}
			private:
				rrcs &_cs;
				bool _errorcheck;
		};

	protected:

		#ifdef _WIN32
		HANDLE _cs;
		#else
		pthread_mutex_t _cs;
		#endif
};


class rrsem
{
	public:

		rrsem(long initialCount=0)
		{
			#ifdef _WIN32
			_sem=CreateSemaphore(NULL, initialCount, MAXLONG, NULL);
			#elif defined (__APPLE__)
			_sem_name=tmpnam(0);
			int oflag=O_CREAT|O_EXCL;
			mode_t mode=0644;
			_sem=sem_open(_sem_name, oflag, mode, (unsigned int)initialCount);
			#else
			sem_init(&_sem, 0, (int)initialCount);
			#endif
		}

		~rrsem(void)
		{
			#ifdef _WIN32
			if(_sem) CloseHandle(_sem);
			#elif defined (__APPLE__)
			int ret=0, err=0;
			do {ret=sem_close(_sem);  err=errno;  sem_post(_sem);}
				while(ret==-1 && err==EBUSY);
			#else
			int ret=0, err=0;
			do {ret=sem_destroy(&_sem);  err=errno;  sem_post(&_sem);}
				while(ret==-1 && err==EBUSY);
			#endif
		}

		void wait(void)
		{
			#ifdef _WIN32
			if(WaitForSingleObject(_sem, INFINITE)==WAIT_FAILED)
				throw(w32error("rrsem::wait()"));
			#elif defined (__APPLE__)
			int err=0;
			do {err=sem_wait(_sem);} while(err<0 && errno==EINTR);
			if(err<0) throw(unixerror("rrsem::wait()"));
			#else
			int err=0;
			do {err=sem_wait(&_sem);} while(err<0 && errno==EINTR);
			if(err<0) throw(unixerror("rrsem::wait()"));
			#endif
		}

		bool trywait()
		{
			#ifdef _WIN32
			DWORD err=WaitForSingleObject(_sem, 0);
			if(err==WAIT_FAILED) throw(w32error("rrsem::trywait()"));
			else if(err==WAIT_TIMEOUT) return false;
			#elif defined (__APPLE__)
			int err=0;
			do {err=sem_trywait(_sem);} while(err<0 && errno==EINTR);
			if(err<0)
			{
				if(errno==EAGAIN) return false;
				else throw(unixerror("rrsem::trywait()"));
			}
			#else
			int err=0;
			do {err=sem_trywait(&_sem);} while(err<0 && errno==EINTR);
			if(err<0)
			{
				if(errno==EAGAIN) return false;
				else throw(unixerror("rrsem::trywait()"));
			}
			#endif
			return true;
		}

		void post(void)
		{
			#ifdef _WIN32
			if(!ReleaseSemaphore(_sem, 1, NULL)) throw(w32error("rrsem::post()"));
			#elif defined (__APPLE__)
			if(sem_post(_sem)==-1) throw(unixerror("rrsem::post()"));
			#else
			if(sem_post(&_sem)==-1) throw(unixerror("rrsem::post()"));
			#endif
		}

		long getvalue(void)
		{
			#ifdef _WIN32
			long count=0;
			if(WaitForSingleObject(_sem, 0)!=WAIT_TIMEOUT)
			{
				ReleaseSemaphore(_sem, 1, &count);
				count++;
			}
			#elif defined (__APPLE__)
			int count=0;
			sem_getvalue(_sem, &count);
			#else
			int count=0;
			sem_getvalue(&_sem, &count);
			#endif
			return (long)count;
		}

	private:

		#ifdef _WIN32
		HANDLE _sem;
		#elif defined(__APPLE__)
		char *_sem_name;
		sem_t *_sem;
		#else
		sem_t _sem;
		#endif
};


#endif //__RRMUTEX_H__
