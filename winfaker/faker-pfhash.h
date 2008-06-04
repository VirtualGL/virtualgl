/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2008 Sun Microsystems, Inc.
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

#include <windows.h>

#define _hashclass _pfhash
#define _hashkeytype1 HDC
#define _hashkeytype2 int
#define _hashvaluetype int
#define _hashclassstruct _pfhashstruct
#define __hashclassstruct __pfhashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct

// This maps a Pbuffer-compatible pixel format to the pixel format used by
// the window

class pfhash : public _pfhash
{
	public:

		static pfhash *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				rrcs::safelock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new pfhash;
			}
			return _Instanceptr;
		}

		static bool isalloc(void) {return (_Instanceptr!=NULL);}

		void add(HDC hdc, int winpf, int pbpf)
		{
			if(!hdc || winpf<1 || pbpf<1) _throw("Invalid argument");
			_pfhash::add(hdc, winpf, pbpf);
		}

		int find(HDC hdc, int winpf)
		{
			if(!hdc || winpf<1) return 0;
			return _pfhash::find(hdc, winpf);
		}

	private:

		~pfhash(void)
		{
			_pfhash::killhash();
		}

		int attach(HDC key1, int key2) {return 0;}

		void detach(_pfhashstruct *h) {return;}

		bool compare(HDC key1, int key2, _pfhashstruct *h)
		{
			return (key1==h->key1 && key2==h->key2);
		}

		static pfhash *_Instanceptr;
		static rrcs _Instancemutex;
};

#ifdef __FAKERHASH_STATICDEF__
pfhash *pfhash::_Instanceptr=NULL;
rrcs pfhash::_Instancemutex;
#endif

#define pfh (*(pfhash::instance()))
