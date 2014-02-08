/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014 D. R. Commander
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


// This maps a window ID to an off-screen drawable instance

class winhash : public _winhash
{
	public:

		static winhash *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				CS::SafeLock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new winhash;
			}
			return _Instanceptr;
		}

		static bool isalloc(void) {return (_Instanceptr!=NULL);}

		void add(Display *dpy, Window win)
		{
			if(!dpy || !win) return;
			char *dpystring=strdup(DisplayString(dpy));
			if(!_winhash::add(dpystring, win, NULL))
				free(dpystring);
		}

		pbwin *findwin(Display *dpy, Window win)
		{
			if(!dpy || !win) return NULL;
			return _winhash::find(DisplayString(dpy), win);
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
			CS::SafeLock l(_mutex);
			if((ptr=findentry(DisplayString(dpy), win))!=NULL)
			{
				if(!ptr->value)
				{
					errifnot(ptr->value=new pbwin(dpy, win));
					pbwin *pbw=ptr->value;
					pbw->initfromwindow(config);
				}
				else
				{
					pbwin *pbw=ptr->value;
					pbw->checkconfig(config);
				}
				return ptr->value;
			}
			return NULL;
		}

		void setoverlay(Display *dpy, Window win)
		{
			if(!dpy || !win) return;
			_winhashstruct *ptr=NULL;
			CS::SafeLock l(_mutex);
			if((ptr=findentry(DisplayString(dpy), win))!=NULL)
			{
				if(!ptr->value) ptr->value=(pbwin *)-1;
			}
		}

		void remove(Display *dpy, GLXDrawable d)
		{
			if(!dpy || !d) return;
			_winhash::remove(DisplayString(dpy), d);
		}

		void remove(Display *dpy)
		{
			if(!dpy) return;
			_winhashstruct *ptr=NULL, *next=NULL;
			CS::SafeLock l(_mutex);
			ptr=_start;
			while(ptr!=NULL)
			{
				pbwin *pbw=ptr->value;
				next=ptr->next;
				if(pbw && pbw!=(pbwin *)-1 && dpy==pbw->get2ddpy()) killentry(ptr);
				ptr=next;
			}
		}

	private:

		~winhash(void)
		{
			_winhash::killhash();
		}

		pbwin *attach(char *key1, Window key2) {return NULL;}

		void detach(_winhashstruct *h)
		{
			pbwin *pbw=h->value;
			if(h && h->key1) free(h->key1);
			if(h && pbw && pbw!=(pbwin *)-1) delete pbw;
		}

		bool compare(char *key1, Window key2, _winhashstruct *h)
		{
			pbwin *pbw=h->value;
			return (
				// Match 2D X Server display string and Window ID stored in pbdrawable
				// instance
				(pbw && pbw!=(pbwin *)-1 && key1
					&& !strcasecmp(DisplayString(pbw->get2ddpy()), key1)
					&& key2==pbw->getx11drawable())
				||
				// If key1 is NULL, match off-screen drawable ID instead of X Window ID
				(pbw && pbw!=(pbwin *)-1 && key1==NULL && key2==pbw->getglxdrawable())
				||
				// Direct match
				(key1 && !strcasecmp(key1, h->key1) && key2==h->key2)
			);
		}

		static winhash *_Instanceptr;
		static CS _Instancemutex;
};

#ifdef __FAKERHASH_STATICDEF__
winhash *winhash::_Instanceptr=NULL;
CS winhash::_Instancemutex;
#endif

#define winh (*(winhash::instance()))
