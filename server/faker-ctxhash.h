/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011-2012, 2014 D. R. Commander
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

typedef struct
{
	GLXFBConfig config;
	Bool direct;
} ctxattribs;

#define _hashclass _ctxhash
#define _hashkeytype1 GLXContext
#define _hashkeytype2 void*
#define _hashvaluetype ctxattribs*
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

		static ctxhash *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				CS::SafeLock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new ctxhash;
			}
			return _Instanceptr;
		}

		static bool isalloc(void) {return (_Instanceptr!=NULL);}

		void add(GLXContext ctx, GLXFBConfig config, Bool direct)
		{
			if(!ctx || !config) _throw("Invalid argument");
			ctxattribs *attribs=NULL;
			newcheck(attribs=new ctxattribs);
			attribs->config=config;
			attribs->direct=direct;
			_ctxhash::add(ctx, NULL, attribs);
		}

		GLXFBConfig findconfig(GLXContext ctx)
		{
			if(!ctx) _throw("Invalid argument");
			ctxattribs *attribs=_ctxhash::find(ctx, NULL);
			if(attribs) return attribs->config;
			return 0;
		}

		bool isoverlay(GLXContext ctx)
		{
			if(ctx && findconfig(ctx)==(GLXFBConfig)-1) return true;
			return false;
		}

		Bool isdirect(GLXContext ctx)
		{
			if(ctx)
			{
				ctxattribs *attribs=_ctxhash::find(ctx, NULL);
				if(attribs) return attribs->direct;
			}
			return -1;
		}

		bool overlaycurrent(void)
		{
			return isoverlay(glXGetCurrentContext());
		}

		void remove(GLXContext ctx)
		{
			if(ctx) _ctxhash::remove(ctx, NULL);
		}

	private:

		~ctxhash(void)
		{
			_ctxhash::killhash();
		}

		ctxattribs *attach(GLXContext key1, void *key2) {return NULL;}

		void detach(_ctxhashstruct *h)
		{
			ctxattribs *attribs=h? (ctxattribs *)h->value:NULL;
			if(attribs) delete attribs;
		}

		bool compare(GLXContext key1, void *key2, _ctxhashstruct *h) {return false;}

		static ctxhash *_Instanceptr;
		static CS _Instancemutex;
};

#ifdef __FAKERHASH_STATICDEF__
ctxhash *ctxhash::_Instanceptr=NULL;
CS ctxhash::_Instancemutex;
#endif

#define ctxh (*(ctxhash::instance()))
