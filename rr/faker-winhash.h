/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

		static winhash *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				rrcs::safelock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new winhash;
			}
			return _Instanceptr;
		}

		static bool isalloc(void) {return (_Instanceptr!=NULL);}

		void add(Display *dpy, Window win)
		{
			if(!dpy || !win) _throw("Invalid argument");
			char *dpystring=strdup(DisplayString(dpy));
			if(!_winhash::add(dpystring, win, NULL))
				free(dpystring);
		}

		bool findpb(Display *dpy, GLXDrawable d, pbwin* &pbw)
		{
			pbwin *p;
			if(!dpy || !d) return false;
			p=_winhash::find(DisplayString(dpy), d);
			if(p==NULL || p==(pbwin *)-1) return false;
			else {pbw=p;  return true;}
		}

		bool isoverlay(Display *dpy, GLXDrawable d)
		{
			pbwin *p;
			if(!dpy || !d) return false;
			p=_winhash::find(DisplayString(dpy), d);
			if(p==(pbwin *)-1) return true;
			return false;
		}

		bool findpb(GLXDrawable d, pbwin* &pbw)
		{
			pbwin *p;
			if(!d) return false;
			p=_winhash::find(NULL, d);
			if(p==NULL || p==(pbwin *)-1) return false;
			else {pbw=p;  return true;}
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

		void setoverlay(Display *dpy, Window win)
		{
			if(!dpy || !win) _throw("Invalid argument");
			_winhashstruct *ptr=NULL;
			rrcs::safelock l(mutex);
			if((ptr=findentry(DisplayString(dpy), win))!=NULL)
			{
				if(!ptr->value) ptr->value=(pbwin *)-1;
			}
		}

		void remove(Display *dpy, GLXDrawable d)
		{
			if(!dpy || !d) _throw("Invalid argument");
			_winhash::remove(DisplayString(dpy), d);
		}

	private:

		~winhash(void)
		{
			_winhash::killhash();
		}

		pbwin *attach(char *key1, Window key2) {return NULL;}

		void detach(_winhashstruct *h)
		{
			pbwin *pb=h->value;
			if(h && h->key1) free(h->key1);
			if(h && pb && pb!=(pbwin *)-1) delete pb;
		}

		bool compare(char *key1, Window key2, _winhashstruct *h)
		{
			pbwin *pb=h->value;
			return (
				(pb && pb!=(pbwin *)-1 && key1 && !strcasecmp(DisplayString(pb->getwindpy()), key1) && key2==pb->getwin()) ||
				(pb && pb!=(pbwin *)-1 && key1==NULL && key2==pb->getdrawable()) ||
				(key1 && !strcasecmp(key1, h->key1) && key2==h->key2)
			);
		}

		static winhash *_Instanceptr;
		static rrcs _Instancemutex;
};

#ifdef __FAKERHASH_STATICDEF__
winhash *winhash::_Instanceptr=NULL;
rrcs winhash::_Instancemutex;
#endif

#define winh (*(winhash::instance()))
