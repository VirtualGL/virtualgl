/* Copyright (C)2004 Landmark Graphics
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

#define _hashclass _pmhash
#define _hashkeytype1 char*
#define _hashkeytype2 Pixmap
#define _hashvaluetype pbuffer*
#define _hashclassstruct _pmhashstruct
#define __hashclassstruct __pmhashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct

// This maps a pixmap ID on the remote display to a GLXPbuffer ID on the local
// display

class pmhash : public _pmhash
{
	public:

		static pmhash *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				rrcs::safelock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new pmhash;
			}
			return _Instanceptr;
		}

		static bool isalloc(void) {return (_Instanceptr!=NULL);}

		void add(Display *dpy, Pixmap pm, pbuffer *pb)
		{
			if(!dpy || !pm) _throw("Invalid argument");
			char *dpystring=strdup(DisplayString(dpy));
			if(!_pmhash::add(dpystring, pm, pb))
				free(dpystring);
		}

		pbuffer *find(Display *dpy, Pixmap pm)
		{
			if(!dpy || !pm) return NULL;
			return (pbuffer *)_pmhash::find(DisplayString(dpy), pm);
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

		pbuffer *attach(char *key1, Pixmap key2) {return NULL;}

		void detach(_pmhashstruct *h)
		{
			if(h && h->key1) free(h->key1);
			if(h && h->value) delete((pbuffer *)h->value);
		}

		bool compare(char *key1, Pixmap key2, _pmhashstruct *h)
		{
			pbuffer *pb=h->value;
			return (
				(!strcasecmp(key1, h->key1) && (key2==h->key2
					|| (pb && key2==pb->drawable())))
			);
		}

		static pmhash *_Instanceptr;
		static rrcs _Instancemutex;
};

#ifdef __FAKERHASH_STATICDEF__
pmhash *pmhash::_Instanceptr=NULL;
rrcs pmhash::_Instancemutex;
#endif

#define pmh (*(pmhash::instance()))
