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


#define _Hash Hash<GLXDrawable, void *, Display *>

// This maps a GLXDrawable instance to a (remote) Display handle.
// Used primarily to make glXGetCurrentDisplay() work properly :/

namespace vglserver
{
	class GLXDrawableHash : public _Hash
	{
		public:

			static GLXDrawableHash *instance(void)
			{
				if(instancePtr==NULL)
				{
					vglutil::CS::SafeLock l(instanceMutex);
					if(instancePtr==NULL) instancePtr=new GLXDrawableHash;
				}
				return instancePtr;
			}

			static bool isAlloc(void) { return (instancePtr!=NULL); }

			void add(GLXDrawable draw, Display *dpy)
			{
				if(!draw || !dpy) _throw("Invalid argument");
				_Hash::add(draw, NULL, dpy);
			}

			Display *getCurrentDisplay(GLXDrawable draw)
			{
				if(!draw) _throw("Invalid argument");
				return _Hash::find(draw, NULL);
			}

			void remove(GLXDrawable draw)
			{
				if(!draw) _throw("Invalid argument");
				_Hash::remove(draw, NULL);
			}

		private:

			~GLXDrawableHash(void)
			{
				_Hash::kill();
			}

			Display *attach(GLXDrawable key1, void *key2) { return NULL; }

			bool compare(GLXDrawable key1, void *key2, HashEntry *entry)
			{
				return false;
			}

			void detach(HashEntry *entry) {}

			static GLXDrawableHash *instancePtr;
			static vglutil::CS instanceMutex;
	};
}

#undef _Hash


#define glxdhash (*(GLXDrawableHash::instance()))

#endif // __GLXDRAWABLEHASH_H__
