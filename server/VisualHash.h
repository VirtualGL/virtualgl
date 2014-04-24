/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014 D. R. Commander
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

#ifndef __VISUALHASH_H__
#define __VISUALHASH_H__

#include "glx.h"
#include <X11/Xlib.h>
#include "Hash.h"


#define _Hash Hash<char *, XVisualInfo *, GLXFBConfig>

// This maps a XVisualInfo * to a GLXFBConfig

namespace vglserver
{
	class VisualHash : public _Hash
	{
		public:

			static VisualHash *instance(void)
			{
				if(instancePtr==NULL)
				{
					vglutil::CS::SafeLock l(instanceMutex);
					if(instancePtr==NULL) instancePtr=new VisualHash;
				}
				return instancePtr;
			}

			static bool isAlloc(void) { return (instancePtr!=NULL); }

			void add(Display *dpy, XVisualInfo *vis, GLXFBConfig config)
			{
				if(!dpy || !vis || !config) _throw("Invalid argument");
				char *dpystring=strdup(DisplayString(dpy));
				if(!_Hash::add(dpystring, vis, config))
					free(dpystring);
			}

			GLXFBConfig getConfig(Display *dpy, XVisualInfo *vis)
			{
				if(!dpy || !vis) _throw("Invalid argument");
				return _Hash::find(DisplayString(dpy), vis);
			}

			void remove(Display *dpy, XVisualInfo *vis)
			{
				if(!vis) _throw("Invalid argument");
				_Hash::remove(dpy? DisplayString(dpy):NULL, vis);
			}

			GLXFBConfig mostRecentConfig(Display *dpy, XVisualInfo *vis)
			{
				if(!dpy || !vis) _throw("Invalid argument");
				HashEntry *ptr=NULL;
				vglutil::CS::SafeLock l(mutex);
				ptr=end;
				while(ptr!=NULL)
				{
					if(ptr->key1 && !strcasecmp(DisplayString(dpy), ptr->key1)
						&& ptr->key2 && vis->visualid==ptr->key2->visualid)
						return ptr->value;
					ptr=ptr->prev;
				}
				return 0;
			}

		private:

			~VisualHash(void)
			{
				_Hash::kill();
			}

			bool compare(char *key1, XVisualInfo *key2, HashEntry *entry)
			{
				return(key2==entry->key2 && (!key1 || !strcasecmp(key1, entry->key1)));
			}

			void detach(HashEntry *entry)
			{
				if(entry && entry->key1) free(entry->key1);
			}

			static VisualHash *instancePtr;
			static vglutil::CS instanceMutex;
	};
}

#undef _Hash


#define vishash (*(VisualHash::instance()))

#endif // __VISUALHASH_H__
