/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005 Sun Microsystems, Inc.
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include "glx.h"
#include <X11/Xlib.h>

#define _hashclass _glxdhash
#define _hashkeytype1 GLXDrawable
#define _hashkeytype2 void*
#define _hashvaluetype Display*
#define _hashclassstruct _glxdhashstruct
#define __hashclassstruct __glxdhashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct

// This maps a GLXDrawable instance to a (remote) Display handle.
// Used primarily to make glXGetCurrentDisplay() work properly :/

class glxdhash : public _glxdhash
{
	public:

		~glxdhash(void)
		{
			_glxdhash::killhash();
		}

		void add(GLXDrawable d, Display *dpy)
		{
			if(!d || !dpy) _throw("Invalid argument");
			_glxdhash::add(d, NULL, dpy);
		}

		Display *getcurrentdpy(GLXDrawable d)
		{
			if(!d) _throw("Invalid argument");
			return _glxdhash::find(d, NULL);
		}

		void remove(GLXDrawable d)
		{
			if(!d) _throw("Invalid argument");
			_glxdhash::remove(d, NULL);
		}

	private:

		Display *attach(GLXDrawable key1, void *key2) {return NULL;}

		bool compare(GLXDrawable key1, void *key2, _glxdhashstruct *h)
		{
			return false;
		}

		void detach(_glxdhashstruct *h) {}
};
