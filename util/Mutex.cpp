/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014 D. R. Commander
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

#include "Mutex.h"
#ifndef _WIN32
#include <string.h>
#endif
#include "Error.h"

using namespace vglutil;


Event::Event(void)
{
	#ifdef _WIN32

	event=CreateEvent(NULL, FALSE, TRUE, NULL);

	#else

	ready=true;  deadYet=false;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	#endif
}


Event::~Event(void)
{
	#ifdef _WIN32

	if(event)
	{
		SetEvent(event);  CloseHandle(event);  event=NULL;
	}

	#else

	pthread_mutex_lock(&mutex);
	ready=true;  deadYet=true;
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
	pthread_mutex_destroy(&mutex);

	#endif
}


void Event::wait(void)
{
	#ifdef _WIN32

	if(WaitForSingleObject(event, INFINITE)==WAIT_FAILED)
		throw(W32Error("Event::wait()"));

	#else

	int ret;
	if((ret=pthread_mutex_lock(&mutex))!=0)
		throw(Error("Event::wait()", strerror(ret)));
	while(!ready && !deadYet)
	{
		if((ret=pthread_cond_wait(&cond, &mutex))!=0)
		{
			pthread_mutex_unlock(&mutex);
			throw(Error("Event::wait()", strerror(ret)));
		}
	}
	ready=false;
	if((ret=pthread_mutex_unlock(&mutex))!=0)
		throw(Error("Event::wait()", strerror(ret)));

	#endif
}


void Event::signal(void)
{
	#ifdef _WIN32

	if(!SetEvent(event)) throw(W32Error("Event::signal()"));

	#else

	int ret;
	if((ret=pthread_mutex_lock(&mutex))!=0)
		throw(Error("Event::signal()", strerror(ret)));
	ready=true;
	if((ret=pthread_mutex_unlock(&mutex))!=0)
		throw(Error("Event::signal()", strerror(ret)));
	if((ret=pthread_cond_signal(&cond))!=0)
		throw(Error("Event::signal()", strerror(ret)));

	#endif
}


bool Event::isLocked(void)
{
	bool ret=true;

	#ifdef _WIN32

	DWORD dw=WaitForSingleObject(event, 0);
	if(dw==WAIT_FAILED) throw(W32Error("Event::isLocked"));
	else if(dw==WAIT_OBJECT_0)
	{
		ret=false;  SetEvent(event);
	}
	else if(dw==WAIT_TIMEOUT) ret=true;

	#else

	int err;
	if((err=pthread_mutex_lock(&mutex))!=0)
		throw(Error("Event::isLocked()", strerror(err)));
	ret=!ready;
	if((err=pthread_mutex_unlock(&mutex))!=0)
		throw(Error("Event::isLocked()", strerror(err)));

	#endif

	return ret;
}


CriticalSection::CriticalSection(void)
{
	#ifdef _WIN32

	mutex=CreateMutex(NULL, FALSE, NULL);

	#else

	pthread_mutexattr_t ma;
	pthread_mutexattr_init(&ma);
	pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &ma);
	pthread_mutexattr_destroy(&ma);

	#endif
}


CriticalSection::~CriticalSection(void)
{
	#ifdef _WIN32

	if(mutex)
	{
		ReleaseMutex(mutex);  CloseHandle(mutex);  mutex=NULL;
	}

	#else

	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);

	#endif
}


void CriticalSection::lock(bool errorCheck)
{
	#ifdef _WIN32

	if(WaitForSingleObject(mutex, INFINITE)==WAIT_FAILED && errorCheck)
		throw(W32Error("CriticalSection::lock()"));

	#else

	int ret;
	if((ret=pthread_mutex_lock(&mutex))!=0 && errorCheck)
		throw(Error("CriticalSection::lock()", strerror(ret)));

	#endif
}


void CriticalSection::unlock(bool errorCheck)
{
	#ifdef _WIN32

	if(!ReleaseMutex(mutex) && errorCheck)
		throw(W32Error("CriticalSection::unlock()"));

	#else

	int ret;
	if((ret=pthread_mutex_unlock(&mutex))!=0 && errorCheck)
		throw(Error("CriticalSection::unlock()", strerror(ret)));

	#endif
}


Semaphore::Semaphore(long initialCount)
{
	#ifdef _WIN32

	sem=CreateSemaphore(NULL, initialCount, MAXLONG, NULL);

	#elif defined (__APPLE__)

	semName=tmpnam(0);
	int oflag=O_CREAT|O_EXCL;
	mode_t mode=0644;
	sem=sem_open(semName, oflag, mode, (unsigned int)initialCount);

	#else

	sem_init(&sem, 0, (int)initialCount);

	#endif
}


Semaphore::~Semaphore(void)
{
	#ifdef _WIN32

	if(sem) CloseHandle(sem);

	#elif defined (__APPLE__)

	int ret=0, err=0;
	do
	{
		ret=sem_close(sem);  err=errno;  sem_post(sem);
	} while(ret==-1 && err==EBUSY);

	#else

	int ret=0, err=0;
	do
	{
		ret=sem_destroy(&sem);  err=errno;  sem_post(&sem);
	} while(ret==-1 && err==EBUSY);

	#endif
}


void Semaphore::wait(void)
{
	#ifdef _WIN32

	if(WaitForSingleObject(sem, INFINITE)==WAIT_FAILED)
		throw(W32Error("Semaphore::wait()"));

	#elif defined (__APPLE__)

	int err=0;
	do
	{
		err=sem_wait(sem);
	} while(err<0 && errno==EINTR);
	if(err<0) throw(UnixError("Semaphore::wait()"));

	#else

	int err=0;
	do
	{
		err=sem_wait(&sem);
	} while(err<0 && errno==EINTR);
	if(err<0) throw(UnixError("Semaphore::wait()"));

	#endif
}


bool Semaphore::tryWait()
{
	#ifdef _WIN32

	DWORD err=WaitForSingleObject(sem, 0);
	if(err==WAIT_FAILED) throw(W32Error("Semaphore::tryWait()"));
	else if(err==WAIT_TIMEOUT) return false;

	#elif defined (__APPLE__)

	int err=0;
	do
	{
		err=sem_trywait(sem);
	} while(err<0 && errno==EINTR);
	if(err<0)
	{
		if(errno==EAGAIN) return false;
		else throw(UnixError("Semaphore::tryWait()"));
	}

	#else

	int err=0;
	do
	{
		err=sem_trywait(&sem);
	} while(err<0 && errno==EINTR);
	if(err<0)
	{
		if(errno==EAGAIN) return false;
		else throw(UnixError("Semaphore::tryWait()"));
	}

	#endif

	return true;
}


void Semaphore::post(void)
{
	#ifdef _WIN32

	if(!ReleaseSemaphore(sem, 1, NULL)) throw(W32Error("Semaphore::post()"));

	#elif defined (__APPLE__)

	if(sem_post(sem)==-1) throw(UnixError("Semaphore::post()"));

	#else

	if(sem_post(&sem)==-1) throw(UnixError("Semaphore::post()"));

	#endif
}


long Semaphore::getValue(void)
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
