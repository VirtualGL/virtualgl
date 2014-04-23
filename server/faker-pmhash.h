/* Copyright (C)2004 Landmark Graphics Corporation
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

#include "VirtualPixmap.h"

using namespace vglserver;

#define _hashclass _pmhash
#define _hashkeytype1 char*
#define _hashkeytype2 Pixmap
#define _hashvaluetype VirtualPixmap*
#define _hashclassstruct _pmhashstruct
#define __hashclassstruct __pmhashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct


// This maps a 2D pixmap ID on the 2D X Server to a VirtualPixmap instance, which
// encapsulates the corresponding 3D pixmap on the 3D X Server

class pmhash : public _pmhash
{
	public:

		static pmhash *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				CS::SafeLock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new pmhash;
			}
			return _Instanceptr;
		}

		static bool isalloc(void) {return (_Instanceptr!=NULL);}

		void add(Display *dpy, Pixmap pm, VirtualPixmap *pbp)
		{
			if(!dpy || !pm) _throw("Invalid argument");
			char *dpystring=strdup(DisplayString(dpy));
			if(!_pmhash::add(dpystring, pm, pbp))
				free(dpystring);
		}

		VirtualPixmap *find(Display *dpy, Pixmap pm)
		{
			if(!dpy || !pm) return NULL;
			return _pmhash::find(DisplayString(dpy), pm);
		}

		Pixmap reversefind(GLXDrawable pb)
		{
			if(!pb) return 0;
			_pmhashstruct *ptr=NULL;
			CS::SafeLock l(_mutex);
			if((ptr=findentry(NULL, pb))!=NULL)
				return ptr->key2;
			return 0;
		}

		void remove(Display *dpy, GLXPixmap pix)
		{
			if(!dpy || !pix) _throw("Invalid argument");
			_pmhash::remove(DisplayString(dpy), pix);
		}

	private:

		~pmhash(void)
		{
			_pmhash::killhash();
		}

		VirtualPixmap *attach(char *key1, Pixmap key2) {return NULL;}

		void detach(_pmhashstruct *h)
		{
			if(h && h->key1) free(h->key1);
			if(h && h->value) delete((VirtualPixmap *)h->value);
		}

		bool compare(char *key1, Pixmap key2, _pmhashstruct *h)
		{
			VirtualPixmap *pbp=h->value;
			return (
				(key1 && !strcasecmp(key1, h->key1) && (key2==h->key2
					|| (pbp && key2==pbp->getGLXDrawable())))
				|| (key1==NULL && key2==pbp->getGLXDrawable())
			);
		}

		static pmhash *_Instanceptr;
		static CS _Instancemutex;
};

#ifdef __FAKERHASH_STATICDEF__
pmhash *pmhash::_Instanceptr=NULL;
CS pmhash::_Instancemutex;
#endif

#define pmh (*(pmhash::instance()))
