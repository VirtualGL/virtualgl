// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011, 2014, 2021 D. R. Commander
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

#ifndef __MUTEX_H__
#define __MUTEX_H__

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif


namespace util
{
	class Event
	{
		public:

			Event(void);
			~Event(void);
			void wait(void);
			void signal(void);
			bool isLocked(void);

		private:

			#ifdef _WIN32
			HANDLE event;
			#else
			pthread_mutex_t mutex;
			pthread_cond_t cond;
			bool ready, deadYet;
			#endif
	};


	// Critical section (recursive mutex)
	class CriticalSection
	{
		public:

			CriticalSection(void);
			~CriticalSection(void);
			void lock(bool errorCheck = true);
			void unlock(bool errorCheck = true);

			class SafeLock
			{
				public:

					SafeLock(CriticalSection &cs_, bool errorCheck_ = true) : cs(cs_),
						errorCheck(errorCheck_)
					{
						cs.lock(errorCheck);
					}
					~SafeLock() { cs.unlock(errorCheck); }

				private:

					CriticalSection &cs;
					bool errorCheck;
			};

		protected:

			#ifdef _WIN32
			HANDLE mutex;
			#else
			pthread_mutex_t mutex;
			#endif
	};


	class Semaphore
	{
		public:

			Semaphore(long initialCount = 0);
			~Semaphore(void);
			void wait(void);
			bool tryWait();
			void post(void);
			long getValue(void);

		private:

			#ifdef _WIN32
			HANDLE sem;
			#elif defined(__APPLE__)
			char *semName;
			sem_t *sem;
			#else
			sem_t sem;
			#endif
	};
}

#endif  // __MUTEX_H__
