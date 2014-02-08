/* Copyright (C)2007 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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

#include <X11/Xlib.h>
#include <unistd.h>
#include "Mutex.h"
#include "Thread.h"
#include "Log.h"


class vglconfigstart : public Runnable
{
	public:

		static vglconfigstart *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				CS::SafeLock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new vglconfigstart;
			}
			return _Instanceptr;
		}

		void popup(Display *dpy, int shmid)
		{
			if(!dpy || shmid==-1) _throw("Invalid argument");
			CS::SafeLock l(_Popupmutex);
			if(_t) return;
			_dpy=dpy;  _shmid=shmid;
			errifnot(_t=new Thread(this));
			_t->start();
		}

		void run(void)
		{
			try
			{
				char commandline[1024];
				unsetenv("LD_PRELOAD");
				unsetenv("LD_PRELOAD_32");
				unsetenv("LD_PRELOAD_64");
				sprintf(commandline, "%s -display %s -shmid %d -ppid %d",
					fconfig.config, DisplayString(_dpy), _shmid, getpid());
				if(system(commandline)==-1) _throwunix();
			}
			catch(Error &e)
			{
				vglout.println("Error invoking vglconfig--\n%s", e.getMessage());
			}
			CS::SafeLock l(_Popupmutex);
			_t->detach();  delete _t;  _t=NULL;
		}

	private:

		vglconfigstart(void): _t(NULL), _dpy(NULL), _shmid(-1)
		{
		}

		virtual ~vglconfigstart(void) {}

		int unsetenv(const char *name)
		{
			char *str=NULL;
			if(!name) {errno=EINVAL;  return -1;}
			if(strlen(name)<1 || strchr(name, '='))
				{errno=EINVAL;  return -1;}
			if(!getenv(name)) return -1;
			if((str=(char *)malloc(strlen(name)+2))==NULL)
				{errno=ENOMEM;  return -1;}
			sprintf(str, "%s=", name);
			putenv(str);
			strcpy(str, "=");
			putenv(str);
			return 0;
		}

		static vglconfigstart *_Instanceptr;
		static CS _Instancemutex;
		static CS _Popupmutex;
		Thread *_t;
		Display *_dpy;
		int _shmid;
};

#ifdef __VGLCONFIGSTART_STATICDEF__
vglconfigstart *vglconfigstart::_Instanceptr=NULL;
CS vglconfigstart::_Instancemutex;
CS vglconfigstart::_Popupmutex;
#endif

#define vglpopup(dpy, shmid) \
	((*(vglconfigstart::instance())).popup(dpy, shmid))
