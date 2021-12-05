// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011, 2014, 2019-2021 D. R. Commander
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

#ifndef __WINDOWHASH_H__
#define __WINDOWHASH_H__

#include "VirtualWin.h"
#include "Hash.h"


#define HASH  Hash<char *, Window, VirtualWin *>

// This maps a window ID to an off-screen drawable instance

namespace faker
{
	class WindowHash : public HASH
	{
		public:

			static WindowHash *getInstance(void)
			{
				if(instance == NULL)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new WindowHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(Display *dpy, Window win)
			{
				if(!dpy || !win) return;
				char *dpystring = strdup(DisplayString(dpy));
				if(!HASH::add(dpystring, win, NULL))
					free(dpystring);
			}

			// dpy == NULL: search for VirtualWin instance by off-screen drawable ID
			// dpy == non-NULL: search for VirtualWin instance by Window ID
			VirtualWin *find(Display *dpy, GLXDrawable glxd)
			{
				if(!glxd) return NULL;
				return HASH::find(dpy ? DisplayString(dpy) : NULL, glxd);
			}

			VirtualWin *initVW(Display *dpy, Window win, VGLFBConfig config)
			{
				if(!dpy || !win || !config) THROW("Invalid argument");
				HashEntry *ptr = NULL;
				util::CriticalSection::SafeLock l(mutex);
				if((ptr = HASH::findEntry(DisplayString(dpy), win)) != NULL)
				{
					if(!ptr->value)
					{
						ptr->value = new VirtualWin(dpy, win);
						VirtualWin *vw = ptr->value;
						vw->initFromWindow(config);
					}
					else
					{
						VirtualWin *vw = ptr->value;
						vw->checkConfig(config);
					}
					return ptr->value;
				}
				return NULL;
			}

			void remove(Display *dpy, GLXDrawable glxd)
			{
				if(!dpy || !glxd) return;
				HASH::remove(DisplayString(dpy), glxd);
			}

			void remove(Display *dpy)
			{
				if(!dpy) return;
				HashEntry *ptr = NULL, *next = NULL;
				util::CriticalSection::SafeLock l(mutex);
				ptr = start;
				while(ptr != NULL)
				{
					VirtualWin *vw = ptr->value;
					next = ptr->next;
					if(vw && dpy == vw->getX11Display())
						HASH::killEntry(ptr);
					ptr = next;
				}
			}

		private:

			~WindowHash(void)
			{
				WindowHash::kill();
			}

			void detach(HashEntry *entry)
			{
				if(entry)
				{
					free(entry->key1);
					delete entry->value;
				}
			}

			bool compare(char *key1, Window key2, HashEntry *entry)
			{
				VirtualWin *vw = entry->value;
				return (
					// Match 2D X Server display string and Window ID stored in
					// VirtualDrawable instance
					(vw && key1 && !strcasecmp(DisplayString(vw->getX11Display()), key1)
						&& key2 == vw->getX11Drawable())
					||
					// If key1 is NULL, match off-screen drawable ID instead of X Window
					// ID
					(vw && key1 == NULL && key2 == vw->getGLXDrawable())
					||
					// Direct match
					(key1 && !strcasecmp(key1, entry->key1) && key2 == entry->key2)
				);
			}

			static WindowHash *instance;
			static util::CriticalSection instanceMutex;
	};
}

#undef HASH


#define winhash  (*(faker::WindowHash::getInstance()))

#endif  // __WINDOWHASH_H__
