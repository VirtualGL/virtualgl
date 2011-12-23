/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#include "glx.h"
#include <X11/Xlib.h>

#define _hashclass _cfghash
#define _hashkeytype1 char*
#define _hashkeytype2 int
#define _hashvaluetype VisualID
#define _hashclassstruct _cfghashstruct
#define __hashclassstruct __cfghashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct
#include "glxvisual.h"


// This maps a GLXFBConfig to an X Visual ID

class cfghash : public _cfghash
{
	public:

		static cfghash *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				rrcs::safelock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new cfghash;
			}
			return _Instanceptr;
		}

		static bool isalloc(void) {return (_Instanceptr!=NULL);}

		void add(Display *dpy, GLXFBConfig c, VisualID vid)
		{
			if(!dpy || !vid || !c) _throw("Invalid argument");
			char *dpystring=strdup(DisplayString(dpy));
			if(!_cfghash::add(dpystring, _FBCID(c), vid))
				free(dpystring);
		}

		VisualID getvisual(Display *dpy, GLXFBConfig c)
		{
			if(!dpy || !c) _throw("Invalid argument");
			return cfghash::find(DisplayString(dpy), _FBCID(c));
		}

		VisualID getvisual(Display *dpy, int fbcid)
		{
			if(!dpy || !fbcid) _throw("Invalid argument");
			return cfghash::find(DisplayString(dpy), fbcid);
		}

		void remove(Display *dpy, GLXFBConfig c)
		{
			if(!dpy || !c) _throw("Invalid argument");
			_cfghash::remove(DisplayString(dpy), _FBCID(c));
		}

	private:

		~cfghash(void)
		{
			_cfghash::killhash();
		}

		VisualID attach(char *key1, int cfgid) {return 0;}

		bool compare(char *key1, int key2, _cfghashstruct *h)
		{
			return(key2==h->key2 && !strcasecmp(key1, h->key1));
		}

		void detach(_cfghashstruct *h)
		{
			if(h && h->key1) free(h->key1);
		}

		static cfghash *_Instanceptr;
		static rrcs _Instancemutex;
};

#ifdef __FAKERHASH_STATICDEF__
cfghash *cfghash::_Instanceptr=NULL;
rrcs cfghash::_Instancemutex;
#endif

#define cfgh (*(cfghash::instance()))
