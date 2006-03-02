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

#include "rrdisplayclient.h"

#define _hashclass _dpyhash
#define _hashkeytype1 char*
#define _hashkeytype2 void*
#define _hashvaluetype rrdisplayclient*
#define _hashclassstruct _dpyhashstruct
#define __hashclassstruct __dpyhashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct

// This maps a Display handle to a RRlib instance

class dpyhash : public _dpyhash
{
	public:

		~dpyhash(void)
		{
			_dpyhash::killhash();
		}

		void add(Display *dpy)
		{
			if(!dpy) _throw("Invalid argument");
			char *dpystring=strdup(DisplayString(dpy));
			if(!_dpyhash::add(dpystring, NULL, NULL, true))
				free(dpystring);
		}

		rrdisplayclient *findrrdpy(Display *dpy)
		{
			if(!dpy) _throw("Invalid argument");
			return _dpyhash::find(DisplayString(dpy), NULL);
		}

		void remove(Display *dpy)
		{
			if(!dpy) _throw("Invalid argument");
			_dpyhash::remove(DisplayString(dpy), NULL, true);
		}

	private:

		rrdisplayclient *attach(char *key, void *key2)
		{
			rrdisplayclient *rrdpy=NULL;
			errifnot(rrdpy=new rrdisplayclient(fconfig.client? fconfig.client:key));
			return rrdpy;
		}

		void detach(_dpyhashstruct *h)
		{
			if(h && h->key1) free(h->key1);
			if(h && h->value) delete((rrdisplayclient *)h->value);
		}

		// We can't compare display handles, because the program may open
		// multiple instances of the same display
		bool compare(char *key1, void *key2, _dpyhashstruct *h)
		{
			return !strcasecmp(key1, h->key1);
		}
};
