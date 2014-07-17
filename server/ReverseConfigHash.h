/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2012, 2014 D. R. Commander
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

#ifndef __REVERSECONFIGHASH_H__
#define __REVERSECONFIGHASH_H__

#include "glx.h"
#include <X11/Xlib.h>
#include "Hash.h"


#define HASH Hash<char *, GLXFBConfig, VisualID>

// This maps a GLXFBConfig to an X Visual ID

namespace vglserver
{
	class ReverseConfigHash : public HASH
	{
		public:

			static ReverseConfigHash *getInstance(void)
			{
				if(instance==NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance==NULL) instance=new ReverseConfigHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return (instance!=NULL); }

			void add(Display *dpy, GLXFBConfig config)
			{
				if(!dpy || !config) _throw("Invalid argument");
				char *dpystring=strdup(DisplayString(dpy));
				if(!HASH::add(dpystring, config, (VisualID)-1))
					free(dpystring);
			}

			bool isOverlay(Display *dpy, GLXFBConfig config)
			{
				if(!dpy || !config) return false;
				VisualID vid=HASH::find(DisplayString(dpy), config);
				if(vid==(VisualID)-1) return true;
				else return false;
			}

			void remove(Display *dpy, GLXFBConfig config)
			{
				if(!dpy || !config) _throw("Invalid argument");
				HASH::remove(DisplayString(dpy), config);
			}

		private:

			~ReverseConfigHash(void)
			{
				HASH::kill();
			}

			VisualID attach(char *key1, GLXFBConfig config) { return 0; }

			bool compare(char *key1, GLXFBConfig key2, HashEntry *entry)
			{
				return(key2==entry->key2 && !strcasecmp(key1, entry->key1));
			}

			void detach(HashEntry *h)
			{
				if(h && h->key1) free(h->key1);
			}

			static ReverseConfigHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define rcfghash (*(ReverseConfigHash::getInstance()))

#endif // __REVERSECONFIGHASH_H__
