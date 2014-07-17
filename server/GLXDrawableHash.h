/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
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

#ifndef __GLXDRAWABLEHASH_H__
#define __GLXDRAWABLEHASH_H__

#include "glx.h"
#include <X11/Xlib.h>
#include "Hash.h"


#define HASH Hash<GLXDrawable, void *, Display *>

// This maps a GLXDrawable instance to a (remote) Display handle.
// Used primarily to make glXGetCurrentDisplay() work properly :/

namespace vglserver
{
	class GLXDrawableHash : public HASH
	{
		public:

			static GLXDrawableHash *getInstance(void)
			{
				if(instance==NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance==NULL) instance=new GLXDrawableHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return (instance!=NULL); }

			void add(GLXDrawable draw, Display *dpy)
			{
				if(!draw || !dpy) _throw("Invalid argument");
				HASH::add(draw, NULL, dpy);
			}

			Display *getCurrentDisplay(GLXDrawable draw)
			{
				if(!draw) _throw("Invalid argument");
				return HASH::find(draw, NULL);
			}

			void remove(GLXDrawable draw)
			{
				if(!draw) _throw("Invalid argument");
				HASH::remove(draw, NULL);
			}

		private:

			~GLXDrawableHash(void)
			{
				HASH::kill();
			}

			Display *attach(GLXDrawable key1, void *key2) { return NULL; }

			bool compare(GLXDrawable key1, void *key2, HashEntry *entry)
			{
				return false;
			}

			void detach(HashEntry *entry) {}

			static GLXDrawableHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define glxdhash (*(GLXDrawableHash::getInstance()))

#endif // __GLXDRAWABLEHASH_H__
