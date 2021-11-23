// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011, 2014-2015, 2018, 2021 D. R. Commander
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

#ifndef __EGLXDISPLAYHASH_H__
#define __EGLXDISPLAYHASH_H__

#include "Hash.h"
#include <X11/Xlib.h>
#include "faker-sym.h"


#define HASH  Hash<Display *, int, EGLXDisplay *>

// This tracks which EGLDisplay handles belong to the X11 platform, i.e. which
// EGLDisplay handles were allocated by VirtualGL and not by the underlying
// EGL implementation.

namespace faker
{
	class EGLXDisplayHash : public HASH
	{
		public:

			static EGLXDisplayHash *getInstance(void)
			{
				if(instance == NULL)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new EGLXDisplayHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(Display *dpy, int screen, EGLXDisplay *eglxdpy)
			{
				if(!dpy || !eglxdpy) THROW("Invalid argument");
				HASH::add(dpy, screen, eglxdpy);
			}

			EGLXDisplay *find(Display *dpy, int screen)
			{
				if(!dpy) return NULL;
				return HASH::find(dpy, screen);
			}

			bool find(EGLDisplay edpy)
			{
				if(!edpy) return false;

				HashEntry *entry = NULL;
				util::CriticalSection::SafeLock l(mutex);

				entry = start;
				while(entry != NULL)
				{
					if((EGLDisplay)entry->value == edpy)
						return true;
					entry = entry->next;
				}
				return false;
			}

			void remove(Display *dpy, int screen)
			{
				if(!dpy) return;
				HASH::remove(dpy, screen);
			}

		private:

			~EGLXDisplayHash(void)
			{
				EGLXDisplayHash::kill();
			}

			void detach(HashEntry *entry)
			{
				EGLXDisplay *eglxdpy = entry ? entry->value : NULL;
				if(eglxdpy->isDefault) _XCloseDisplay(eglxdpy->x11dpy);
				delete eglxdpy;
			}

			bool compare(Display *key1, int key2, HashEntry *entry)
			{
				return false;
			}

			static EGLXDisplayHash *instance;
			static util::CriticalSection instanceMutex;
	};
}

#undef HASH


#define eglxdpyhash  (*(faker::EGLXDisplayHash::getInstance()))

#endif  // __EGLXDISPLAYHASH_H__
