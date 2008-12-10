/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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
			event=CreateEvent(NULL, FALSE, TRUE, NULL);
			#else
			_ready=true;  _deadyet=false;
			pthread_mutex_init(&event, NULL);
			pthread_cond_init(&cond, NULL);
			#endif
		}

		~rrevent(void)
		{
			#ifdef _WIN32
			if(event) {SetEvent(event);  CloseHandle(event);  event=NULL;}
			#else
			pthread_mutex_lock(&event);
			_ready=true;  _deadyet=true;
			pthread_mutex_unlock(&event);
			pthread_cond_signal(&cond);
			pthread_mutex_destroy(&event);
			#endif
		}

		void wait(void)
		{
			#ifdef _WIN32
			if(WaitForSingleObject(event, INFINITE)==WAIT_FAILED)
				throw(w32error("rrevent::wait()"));
			#else
			int ret;
			if((ret=pthread_mutex_lock(&event))!=0)
				throw(rrerror("rrevent::wait()", strerror(ret)));
			while(!_ready && !_deadyet)
				if((ret=pthread_cond_wait(&cond, &event))!=0)
				{
					pthread_mutex_unlock(&event);
					throw(rrerror("rrevent::wait()", strerror(ret)));
				}
			_ready=false;
			if((ret=pthread_mutex_unlock(&event))!=0)
				throw(rrerror("rrevent::wait()", strerror(ret)));
			#endif
		}

		void signal(void)
		{
			#ifdef _WIN32
			if(!SetEvent(event)) throw(w32error("rrevent::signal()"));
			#else
			int ret;
			if((ret=pthread_mutex_lock(&event))!=0)
				throw(rrerror("rrevent::signal()", strerror(ret)));
			_ready=true;
			if((ret=pthread_mutex_unlock(&event))!=0)
				throw(rrerror("rrevent::signal()", strerror(ret)));
			if((ret=pthread_cond_signal(&cond))!=0)
				throw(rrerror("rrevent::signal()", strerror(ret)));
			#endif
		}

		bool locked(void)
		{
			bool islocked=true;
			#ifdef _WIN32
			DWORD dw=WaitForSingleObject(event, 0);
			if(dw==WAIT_FAILED) throw(w32error("rrevent::locked"));
			else if(dw==WAIT_OBJECT_0)
			{
				islocked=false;  SetEvent(event);
			}
			else if(dw==WAIT_TIMEOUT) islocked=true;
			#else
			int ret;
			if((ret=pthread_mutex_lock(&event))!=0)
				throw(rrerror("rrevent::locked()", strerror(ret)));
			islocked=!_ready;
			if((ret=pthread_mutex_unlock(&event))!=0)
				throw(rrerror("rrevent::signal()", strerror(ret)));
			#endif
			return islocked;
		}

	private:

		#ifdef _WIN32
		HANDLE event;
		#else
		pthread_mutex_t event;
		pthread_cond_t cond;
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
			cs=CreateMutex(NULL, FALSE, NULL);
			#else
			pthread_mutexattr_t ma;
			pthread_mutexattr_init(&ma);
			pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&cs, &ma);
			pthread_mutexattr_destroy(&ma);
			#endif
		}

		~rrcs(void)
		{
			#ifdef _WIN32
			if(cs) {ReleaseMutex(cs);  CloseHandle(cs);  cs=NULL;}
			#else
			pthread_mutex_unlock(&cs);
			pthread_mutex_destroy(&cs);
			#endif
		}

		void lock(bool errorcheck=true)
		{
			#ifdef _WIN32
			if(WaitForSingleObject(cs, INFINITE)==WAIT_FAILED && errorcheck)
				throw(w32error("rrcs::lock()"));
			#else
			int ret;
			if((ret=pthread_mutex_lock(&cs))!=0 && errorcheck)
				throw(rrerror("rrcs::lock()", strerror(ret)));
			#endif
		}

		void unlock(bool errorcheck=true)
		{
			#ifdef _WIN32
			if(!ReleaseMutex(cs) && errorcheck) throw(w32error("rrcs::unlock()"));
			#else
			int ret;
			if((ret=pthread_mutex_unlock(&cs))!=0 && errorcheck)
				throw(rrerror("rrcs::unlock()", strerror(ret)));
			#endif
		}

		class safelock
		{
			public:
				safelock(rrcs &cs) : _cs(cs) {cs.lock();}
				~safelock() {_cs.unlock();}
			private:
				rrcs &_cs;
		};

	private:

		#ifdef _WIN32
		HANDLE cs;
		#else
		pthread_mutex_t cs;
		#endif
};

class rrsem
{
	public:

		rrsem(long initialCount=0)
		{
			#ifdef _WIN32
			sem=CreateSemaphore(NULL, initialCount, MAXLONG, NULL);
			#elif defined (__APPLE__)
			sem_name = tmpnam(0);
			int oflag = O_CREAT | O_EXCL;
			mode_t mode = 0644;
			sem = sem_open(sem_name, oflag, mode, (unsigned int)initialCount);
			#else
			sem_init(&sem, 0, (int)initialCount);
			#endif
		}

		~rrsem(void)
		{
			#ifdef _WIN32
			if(sem) CloseHandle(sem);
			#elif defined (__APPLE__)
			int ret=0, err=0;
			do {ret=sem_close(sem);  err=errno;  sem_post(sem);}
				while(ret==-1 && err==EBUSY);
			#else
			int ret=0, err=0;
			do {ret=sem_destroy(&sem);  err=errno;  sem_post(&sem);}
				while(ret==-1 && err==EBUSY);
			#endif
		}

		void wait(void)
		{
			#ifdef _WIN32
			if(WaitForSingleObject(sem, INFINITE)==WAIT_FAILED)
				throw(w32error("rrsem::wait()"));
			#elif defined (__APPLE__)
			int err=0;
			do {err=sem_wait(sem);} while(err<0 && errno==EINTR);
			if(err<0) throw(unixerror("rrsem::wait()"));
			#else
			int err=0;
			do {err=sem_wait(&sem);} while(err<0 && errno==EINTR);
			if(err<0) throw(unixerror("rrsem::wait()"));
			#endif
		}

		bool trywait()
		{
			#ifdef _WIN32
			DWORD err=WaitForSingleObject(sem, 0);
			if(err==WAIT_FAILED) throw(w32error("rrsem::trywait()"));
			else if(err==WAIT_TIMEOUT) return false;
			#elif defined (__APPLE__)
			int err=0;
			do {err=sem_trywait(sem);} while(err<0 && errno==EINTR);
			if(err<0)
			{
				if(errno==EAGAIN) return false;
				else throw(unixerror("rrsem::trywait()"));
			}
			#else
			int err=0;
			do {err=sem_trywait(&sem);} while(err<0 && errno==EINTR);
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
			if(!ReleaseSemaphore(sem, 1, NULL)) throw(w32error("rrsem::post()"));
			#elif defined (__APPLE__)
			if(sem_post(sem)==-1) throw(unixerror("rrsem::post()"));
			#else
			if(sem_post(&sem)==-1) throw(unixerror("rrsem::post()"));
			#endif
		}

		long getvalue(void)
		{
			#ifdef _WIN32
			long count=0;
			if(WaitForSingleObject(sem, 0)!=WAIT_TIMEOUT)
			{
				ReleaseSemaphore(sem, 1, &count);
				count++;
			}
			#elif defined (__APPLE__)
			int count=0;
			sem_getvalue(sem, &count);
			#else
			int count=0;
			sem_getvalue(&sem, &count);
			#endif
			return (long)count;
		}

	private:

		#ifdef _WIN32
		HANDLE sem;
		#elif defined(__APPLE__)
		char *sem_name;
		sem_t *sem;
		#else
		sem_t sem;
		#endif
};

#endif

