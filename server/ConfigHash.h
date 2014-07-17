/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#ifndef __CONFIGHASH_H__
#define __CONFIGHASH_H__

#include "glx.h"
#include <X11/Xlib.h>
#include "glxvisual.h"
#include "Hash.h"


#define HASH Hash<char *, int, VisualID>

namespace vglserver
{
	class ConfigHash : public HASH
	{
		public:

			static ConfigHash *getInstance(void)
			{
				if(instance==NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance==NULL) instance=new ConfigHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return (instance!=NULL); }

			void add(Display *dpy, GLXFBConfig config, VisualID vid)
			{
				if(!dpy || !vid || !config) _throw("Invalid argument");
				char *dpystring=strdup(DisplayString(dpy));
				if(!HASH::add(dpystring, _FBCID(config), vid))
					free(dpystring);
			}

			VisualID getVisual(Display *dpy, GLXFBConfig config)
			{
				if(!dpy || !config) _throw("Invalid argument");
				return HASH::find(DisplayString(dpy),
					_FBCID(config));
			}

			VisualID getVisual(Display *dpy, int fbcid)
			{
				if(!dpy || !fbcid) _throw("Invalid argument");
				return HASH::find(DisplayString(dpy), fbcid);
			}

			void remove(Display *dpy, GLXFBConfig config)
			{
				if(!dpy || !config) _throw("Invalid argument");
				HASH::remove(DisplayString(dpy), _FBCID(config));
			}

		private:

			~ConfigHash(void)
			{
				HASH::kill();
			}

			bool compare(char *key1, int key2, HashEntry *entry)
			{
				return(key2==entry->key2 && !strcasecmp(key1, entry->key1));
			}

			void detach(HashEntry *entry)
			{
				if(entry && entry->key1) free(entry->key1);
			}

			static ConfigHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define cfghash (*(ConfigHash::getInstance()))

#endif // __CONFIGHASH_H__
