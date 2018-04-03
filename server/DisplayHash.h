/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2015 D. R. Commander
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

#ifndef __DISPLAYHASH_H__
#define __DISPLAYHASH_H__

#include "Hash.h"
#include <X11/Xlib.h>


#define HASH  Hash<Display *, void *, bool>

// This maps a window ID to an off-screen drawable instance

namespace vglserver
{
	class DisplayHash : public HASH
	{
		public:

			static DisplayHash *getInstance(void)
			{
				if(instance == NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new DisplayHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(Display *dpy)
			{
				if(!dpy) return;
				HASH::add(dpy, NULL, true, true);
			}

			bool find(Display *dpy)
			{
				if(!dpy) return false;
				return HASH::find(dpy, NULL);
			}

			void remove(Display *dpy)
			{
				if(!dpy) return;
				HASH::remove(dpy, NULL);
			}

		private:

			~DisplayHash(void)
			{
				DisplayHash::kill();
			}

			void detach(HashEntry *entry)
			{
			}

			bool compare(Display *key1, void *key2, HashEntry *entry)
			{
				return key1 == entry->key1;
			}

			static DisplayHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define dpyhash  (*(DisplayHash::getInstance()))

#endif  // __DISPLAYHASH_H__
