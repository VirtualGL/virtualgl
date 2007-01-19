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

#define _hashclass _vishash
#define _hashkeytype1 char*
#define _hashkeytype2 XVisualInfo*
#define _hashvaluetype GLXFBConfig
#define _hashclassstruct _vishashstruct
#define __hashclassstruct __vishashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct

// This maps a XVisualInfo * to a GLXFBConfig

class vishash : public _vishash
{
	public:

		~vishash(void)
		{
			_vishash::killhash();
		}

		void add(Display *dpy, XVisualInfo *vis, GLXFBConfig _localc)
		{
			if(!dpy || !vis || !_localc) _throw("Invalid argument");
			char *dpystring=strdup(DisplayString(dpy));
			if(!_vishash::add(dpystring, vis, _localc))
				free(dpystring);
		}

		GLXFBConfig getpbconfig(Display *dpy, XVisualInfo *vis)
		{
			if(!dpy || !vis) _throw("Invalid argument");
			return vishash::find(DisplayString(dpy), vis);
		}

		void remove(Display *dpy, XVisualInfo *vis)
		{
			if(!vis) _throw("Invalid argument");
			_vishash::remove(dpy? DisplayString(dpy):NULL, vis);
		}

		GLXFBConfig mostrecentcfg(Display *dpy, XVisualInfo *vis)
		{
			if(!dpy || !vis) _throw("Invalid argument");
			_vishashstruct *ptr=NULL;
			rrcs::safelock l(mutex);
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

		GLXFBConfig attach(char *key1, XVisualInfo *key2) {return NULL;}

		bool compare(char *key1, XVisualInfo *key2, _vishashstruct *h)
		{
			return(key2==h->key2 && (!key1 || !strcasecmp(key1, h->key1)));
		}

		void detach(_vishashstruct *h)
		{
			if(h && h->key1) free(h->key1);
		}
};
