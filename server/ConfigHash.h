// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2014, 2016, 2019 D. R. Commander
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

#ifndef __CONFIGHASH_H__
#define __CONFIGHASH_H__

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <X11/Xlib.h>
#include "glxvisual.h"
#include "Hash.h"


#define HASH  Hash<char *, int, XVisualInfo *>

namespace vglserver
{
	class ConfigHash : public HASH
	{
		public:

			static ConfigHash *getInstance(void)
			{
				if(instance == NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new ConfigHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(Display *dpy, int screen, GLXFBConfig config, VisualID vid)
			{
				if(!dpy || screen < 0 || !config || !vid)
					THROW("Invalid argument");
				char *dpystring = strdup(DisplayString(dpy));
				XVisualInfo *vis;
				vis = (XVisualInfo *)calloc(1, sizeof(XVisualInfo));
				vis->screen = screen;
				vis->visualid = vid;
				remove(dpy, config);
				if(!HASH::add(dpystring, FBCID(config), vis))
				{
					free(dpystring);
					XFree(vis);
				}
			}

			VisualID getVisual(Display *dpy, GLXFBConfig config, int &screen)
			{
				if(!dpy || !config) THROW("Invalid argument");
				XVisualInfo *vis = HASH::find(DisplayString(dpy), FBCID(config));
				if(!vis) return 0;
				screen = vis->screen;
				return vis->visualid;
			}

			void remove(Display *dpy, GLXFBConfig config)
			{
				if(!dpy || !config) THROW("Invalid argument");
				HASH::remove(DisplayString(dpy), FBCID(config));
			}

		private:

			~ConfigHash(void)
			{
				HASH::kill();
			}

			bool compare(char *key1, int key2, HashEntry *entry)
			{
				return key2 == entry->key2 && !strcasecmp(key1, entry->key1);
			}

			void detach(HashEntry *entry)
			{
				if(entry) free(entry->key1);
				if(entry && entry->value) XFree(entry->value);
			}

			static ConfigHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define cfghash  (*(ConfigHash::getInstance()))

#endif  // __CONFIGHASH_H__
