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

#include "pbwin.h"

#define _hashclass _winhash
#define _hashkeytype1 char*
#define _hashkeytype2 Window
#define _hashvaluetype pbwin*
#define _hashclassstruct _winhashstruct
#define __hashclassstruct __winhashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct

// This maps a window ID to a Pbuffer instance

class winhash : public _winhash
{
	public:

		~winhash(void)
		{
			_winhash::killhash();
		}

		void add(Display *dpy, Window win)
		{
			if(!dpy || !win) _throw("Invalid argument");
			char *dpystring=strdup(DisplayString(dpy));
			if(!_winhash::add(dpystring, win, NULL))
				free(dpystring);
		}

		pbwin *findpb(Display *dpy, GLXDrawable d)
		{
			if(!dpy || !d) return NULL;
			return _winhash::find(DisplayString(dpy), d);
		}

		pbwin *findpb(GLXDrawable d)
		{
			if(!d) return NULL;
			return _winhash::find(NULL, d);
		}

		pbwin *setpb(Display *dpy, Window win, GLXFBConfig config)
		{
			if(!dpy || !win || !config) _throw("Invalid argument");
			_winhashstruct *ptr=NULL;
			rrcs::safelock l(mutex);
			if((ptr=findentry(DisplayString(dpy), win))!=NULL)
			{
				if(!ptr->value)
				{
					errifnot(ptr->value=new pbwin(dpy, win));
					pbwin *pb=(pbwin *)ptr->value;
					pb->initfromwindow(config);
				}
				return (pbwin *)ptr->value;
			}
			return NULL;
		}

		void remove(Display *dpy, GLXDrawable d)
		{
			if(!dpy || !d) _throw("Invalid argument");
			_winhash::remove(DisplayString(dpy), d);
		}

	private:

		pbwin *attach(char *key1, Window key2) {return NULL;}

		void detach(_winhashstruct *h)
		{
			if(h && h->key1) free(h->key1);
			if(h && h->value) delete((pbwin *)h->value);
		}

		bool compare(char *key1, Window key2, _winhashstruct *h)
		{
			pbwin *pb=h->value;
			return (
				(pb && key1 && !strcasecmp(DisplayString(pb->getwindpy()), key1) && key2==pb->getwin()) ||
				(pb && key1==NULL && key2==pb->getdrawable()) ||
				(key1 && !strcasecmp(key1, h->key1) && key2==h->key2)
			);
		}
};
