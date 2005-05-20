/* Copyright (C)2004 Landmark Graphics
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

#include "rr.h"

#define _hashclass _dpyhash
#define _hashkeytype1 char*
#define _hashkeytype2 void*
#define _hashvaluetype RRDisplay
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

		RRDisplay findrrdpy(Display *dpy)
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

		RRDisplay attach(char *key, void *key2)
		{
			RRDisplay rrdpy=NULL;
			char *dpystring=NULL, *ptr=NULL;

			if(fconfig.client) dpystring=strdup(fconfig.client);
			else dpystring=strdup(key);

			if((ptr=strchr(dpystring, ':'))!=NULL)
				*ptr='\0';
			if(!strlen(dpystring)) {free(dpystring);  dpystring=strdup("localhost");}
			if(!(rrdpy=RROpenDisplay(dpystring, fconfig.port, fconfig.ssl)))
			throw(rrerror(RRErrorLocation(), RRErrorString()));
			free(dpystring);
			return rrdpy;
		}

		void detach(_dpyhashstruct *h)
		{
			if(h && h->key1) free(h->key1);
			if(h && h->value) RRCloseDisplay(h->value);
		}

		// We can't compare display handles, because the program may open
		// multiple instances of the same display
		bool compare(char *key1, void *key2, _dpyhashstruct *h)
		{
			return !strcasecmp(key1, h->key1);
		}
};
