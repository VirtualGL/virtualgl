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

#define _hashclass _ctxhash
#define _hashkeytype1 GLXContext
#define _hashkeytype2 void*
#define _hashvaluetype GLXFBConfig
#define _hashclassstruct _ctxhashstruct
#define __hashclassstruct __ctxhashstruct
#include "faker-hash.h"
#undef _hashclass
#undef _hashkeytype1
#undef _hashkeytype2
#undef _hashvaluetype
#undef _hashclassstruct
#undef __hashclassstruct


// This maps a GLXContext to a GLXFBConfig

class ctxhash : public _ctxhash
{
	public:

		~ctxhash(void)
		{
			_ctxhash::killhash();
		}

		void add(GLXContext ctx, GLXFBConfig config)
		{
			if(!ctx || !config) _throw("Invalid argument");
			_ctxhash::add(ctx, NULL, config);
		}

		GLXFBConfig findconfig(GLXContext ctx)
		{
			if(!ctx) _throw("Invalid argument");
			return _ctxhash::find(ctx, NULL);
		}

		bool isoverlay(GLXContext ctx)
		{
			if(ctx && findconfig(ctx)==(GLXFBConfig)-1) return true;
			return false;
		}

		bool overlaycurrent(void)
		{
			return isoverlay(_glXGetCurrentContext());
		}

		void remove(GLXContext ctx)
		{
			if(!ctx) _throw("Invalid argument");
			_ctxhash::remove(ctx, NULL);
		}

	private:

		GLXFBConfig attach(GLXContext key1, void *key2) {return NULL;}

		void detach(_ctxhashstruct *h) {}

		bool compare(GLXContext key1, void *key2, _ctxhashstruct *h) {return false;}
};
