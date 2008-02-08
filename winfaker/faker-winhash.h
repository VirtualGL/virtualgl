/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005-2008 Sun Microsystems, Inc.
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
#define _hashkeytype1 HWND
#define _hashkeytype2 HDC
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

		bool findpb(HWND hwnd, HDC hdc, pbwin* &pbw)
		{
			pbwin *p;
			if(!hdc && !hwnd) return false;
			p=_winhash::find(hwnd, hdc, NULL);
			if(p==NULL) return false;
			else {pbw=p;  return true;}
		}

	private:

		~winhash(void)
		{
			_winhash::killhash();
		}

		pbwin *attach(HWND hwnd, HDC hdc) {return NULL;}

		void detach(_winhashstruct *h)
		{
			if(h)
			{
				pbwin *pb=h->value;
				if(pb) delete pb;
			}
		}

		bool compare(HWND key1, HDC key2, _winhashstruct *h)
		{
			pbwin *pb=h->value;
			return (
				(key1 && h->key1 && key1==h->key1)
				|| (pb && pb->gethdc() && key2==pb->gethdc())
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
