// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
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

#ifndef __VISUALHASH_H__
#define __VISUALHASH_H__

#include "glxvisual.h"
#include <X11/Xlib.h>
#include "Hash.h"


#define HASH  Hash<char *, XVisualInfo *, VGLFBConfig>

// This maps a XVisualInfo * to a VGLFBConfig

namespace faker
{
	class VisualHash : public HASH
	{
		public:

			static VisualHash *getInstance(void)
			{
				if(instance == NULL)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new VisualHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(Display *dpy, XVisualInfo *vis, VGLFBConfig config)
			{
				if(!dpy || !vis || !config) THROW("Invalid argument");
				char *dpystring = strdup(DisplayString(dpy));
				if(!HASH::add(dpystring, vis, config))
					free(dpystring);
			}

			VGLFBConfig getConfig(Display *dpy, XVisualInfo *vis)
			{
				if(!dpy || !vis) THROW("Invalid argument");
				return HASH::find(DisplayString(dpy), vis);
			}

			void remove(Display *dpy, XVisualInfo *vis)
			{
				if(!vis) THROW("Invalid argument");
				HASH::remove(dpy ? DisplayString(dpy) : NULL, vis);
			}

		private:

			~VisualHash(void)
			{
				HASH::kill();
			}

			bool compare(char *key1, XVisualInfo *key2, HashEntry *entry)
			{
				return key2 == entry->key2
					&& (!key1 || !strcasecmp(key1, entry->key1));
			}

			void detach(HashEntry *entry)
			{
				if(entry) free(entry->key1);
			}

			static VisualHash *instance;
			static util::CriticalSection instanceMutex;
	};
}

#undef HASH


#define vishash  (*(faker::VisualHash::getInstance()))

#endif  // __VISUALHASH_H__
