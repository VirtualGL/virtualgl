/* Copyright (C)2006 Sun Microsystems, Inc.
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

#define _hashclass _rcfghash
#define _hashkeytype1 char*
#define _hashkeytype2 GLXFBConfig
#define _hashvaluetype VisualID
#define _hashclassstruct _rcfghashstruct
#define __hashclassstruct __rcfghashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct
#include "glxvisual.h"

// This maps a GLXFBConfig to an X Visual ID

class rcfghash : public _rcfghash
{
	public:

		~rcfghash(void)
		{
			_rcfghash::killhash();
		}

		void add(Display *dpy, GLXFBConfig c)
		{
			if(!dpy || !c) _throw("Invalid argument");
			char *dpystring=strdup(DisplayString(dpy));
			if(!_rcfghash::add(dpystring, c, (VisualID)-1))
				free(dpystring);
		}

		bool isoverlay(Display *dpy, GLXFBConfig c)
		{
			if(!dpy || !c) _throw("Invalid argument");
			VisualID vid=rcfghash::find(DisplayString(dpy), c);
			if(vid==(VisualID)-1) return true;
			else return false;
		}

		void remove(Display *dpy, GLXFBConfig c)
		{
			if(!dpy || !c) _throw("Invalid argument");
			_rcfghash::remove(DisplayString(dpy), c);
		}

	private:

		VisualID attach(char *key1, GLXFBConfig c) {return 0;}

		bool compare(char *key1, GLXFBConfig key2, _rcfghashstruct *h)
		{
			return(key2==h->key2 && !strcasecmp(key1, h->key1));
		}

		void detach(_rcfghashstruct *h)
		{
			if(h && h->key1) free(h->key1);
		}
};
