/* Copyright (C)2007 Sun Microsystems, Inc.
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

#include <X11/Xlib.h>
#include <unistd.h>
#include "rrmutex.h"
#include "rrthread.h"
#include "rrlog.h"

class vglconfigstart : public Runnable
{
	public:

		static vglconfigstart *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				rrcs::safelock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new vglconfigstart;
			}
			return _Instanceptr;
		}

		void popup(Display *dpy, int shmid)
		{
			if(!dpy || shmid==-1) _throw("Invalid argument");
			rrcs::safelock l(_Popupmutex);
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
				sprintf(commandline, "%s -display %s -shmid %d",
					(char *)fconfig.config, DisplayString(_dpy), _shmid);
				if(system(commandline)==-1) _throwunix();
			}
			catch(rrerror &e)
			{
				rrout.println("Error invoking vglconfig--\n%s", e.getMessage());
			}
			rrcs::safelock l(_Popupmutex);
			_t->detach();  delete _t;  _t=NULL;
		}

	private:

		vglconfigstart(void): _t(NULL), _dpy(NULL), _shmid(-1)
		{
		}

		virtual ~vglconfigstart(void) {}

		static vglconfigstart *_Instanceptr;
		static rrcs _Instancemutex;
		static rrcs _Popupmutex;
		Thread *_t;
		Display *_dpy;
		int _shmid;
};

#ifdef __VGLCONFIGSTART_STATICDEF__
vglconfigstart *vglconfigstart::_Instanceptr=NULL;
rrcs vglconfigstart::_Instancemutex;
rrcs vglconfigstart::_Popupmutex;
#endif

#define vglpopup(dpy, shmid) \
	((*(vglconfigstart::instance())).popup(dpy, shmid))
