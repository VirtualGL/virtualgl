// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011-2012, 2014-2015, 2019-2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#ifndef __CONTEXTHASH_H__
#define __CONTEXTHASH_H__

#include "glxvisual.h"
#include "Hash.h"


typedef struct
{
	VGLFBConfig config;
	Bool direct;
} ContextAttribs;


#define HASH  Hash<GLXContext, void *, ContextAttribs *>

// This maps a GLXContext to a VGLFBConfig

namespace faker
{
	class ContextHash : public HASH
	{
		public:

			static ContextHash *getInstance(void)
			{
				if(instance == NULL)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new ContextHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(GLXContext ctx, VGLFBConfig config, Bool direct)
			{
				if(!ctx || !config) THROW("Invalid argument");
				ContextAttribs *attribs = NULL;
				attribs = new ContextAttribs;
				attribs->config = config;
				attribs->direct = direct;
				HASH::add(ctx, NULL, attribs);
			}

			VGLFBConfig findConfig(GLXContext ctx)
			{
				if(!ctx) THROW("Invalid argument");
				ContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs) return attribs->config;
				return 0;
			}

			Bool isDirect(GLXContext ctx)
			{
				if(ctx)
				{
					ContextAttribs *attribs = HASH::find(ctx, NULL);
					if(attribs) return attribs->direct;
				}
				return -1;
			}

			void remove(GLXContext ctx)
			{
				if(ctx) HASH::remove(ctx, NULL);
			}

		private:

			~ContextHash(void)
			{
				HASH::kill();
			}

			void detach(HashEntry *entry)
			{
				ContextAttribs *attribs = entry ? entry->value : NULL;
				delete attribs;
			}

			bool compare(GLXContext key1, void *key2, HashEntry *entry)
			{
				return false;
			}

			static ContextHash *instance;
			static util::CriticalSection instanceMutex;
	};
}

#undef HASH


#define ctxhash  (*(faker::ContextHash::getInstance()))

#endif  // __CONTEXTHASH_H__
