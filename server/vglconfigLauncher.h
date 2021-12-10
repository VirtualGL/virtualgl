// Copyright (C)2007 Sun Microsystems, Inc.
// Copyright (C)2014, 2018-2021 D. R. Commander
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

#ifndef __VGLCONFIGLAUNCHER_H__
#define __VGLCONFIGLAUNCHER_H__

#include <X11/Xlib.h>
#include <unistd.h>
#include "Mutex.h"
#include "Thread.h"
#include "Log.h"
#include "fakerconfig.h"


namespace faker
{
	class vglconfigLauncher : public util::Runnable
	{
		public:

			static vglconfigLauncher *getInstance(void)
			{
				if(instance == NULL)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new vglconfigLauncher;
				}
				return instance;
			}

			void popup(Display *dpy_, int shmid_)
			{
				if(!dpy_ || shmid_ == -1) THROW("Invalid argument");
				util::CriticalSection::SafeLock l(popupMutex);
				if(thread) return;
				dpy = dpy_;  shmid = shmid_;
				thread = new util::Thread(this);
				thread->start();
			}

			void run(void)
			{
				try
				{
					char commandLine[1024];
					unsetenv("LD_PRELOAD");
					unsetenv("LD_PRELOAD_32");
					unsetenv("LD_PRELOAD_64");
					sprintf(commandLine, "%s -display %s -shmid %d -ppid %d",
						fconfig.config, DisplayString(dpy), shmid, (int)getpid());
					if(system(commandLine) == -1) THROW_UNIX();
				}
				catch(std::exception &e)
				{
					vglout.println("Error invoking vglconfig--\n%s", e.what());
				}
				util::CriticalSection::SafeLock l(popupMutex);
				thread->detach();  delete thread;  thread = NULL;
			}

		private:

			vglconfigLauncher(void) : thread(NULL), dpy(NULL), shmid(-1)
			{
			}

			virtual ~vglconfigLauncher(void) {}

			int unsetenv(const char *name)
			{
				char *str = NULL;
				if(!name) { errno = EINVAL;  return -1; }
				if(strlen(name) < 1 || strchr(name, '='))
				{
					errno = EINVAL;  return -1;
				}
				if(!getenv(name)) return -1;
				if((str = (char *)malloc(strlen(name) + 2)) == NULL)
				{
					errno = ENOMEM;  return -1;
				}
				sprintf(str, "%s=", name);
				putenv(str);
				strcpy(str, "=");
				putenv(str);
				return 0;
			}

			static vglconfigLauncher *instance;
			static util::CriticalSection instanceMutex;
			static util::CriticalSection popupMutex;
			util::Thread *thread;
			Display *dpy;
			int shmid;
	};
}


#define VGLPOPUP(dpy, shmid) \
	((*(faker::vglconfigLauncher::getInstance())).popup(dpy, shmid))

#endif  // __VGLCONFIGLAUNCHER_H__
