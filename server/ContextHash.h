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

#ifndef __CONTEXTHASH_H__
#define __CONTEXTHASH_H__

#include "glx.h"
#include "Hash.h"


typedef struct
{
	GLXFBConfig config;
	Bool direct;
	bool colorIndex;  // This context appears as a color index context to the
                    // application (VGL uses CI emulation with it)
} ContextAttribs;


#define HASH Hash<GLXContext, void *, ContextAttribs *>

// This maps a GLXContext to a GLXFBConfig

namespace vglserver
{
	class ContextHash : public HASH
	{
		public:

			static ContextHash *getInstance(void)
			{
				if(instance==NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance==NULL) instance=new ContextHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return (instance!=NULL); }

			void add(GLXContext ctx, GLXFBConfig config, Bool direct,
				bool colorIndex)
			{
				if(!ctx || !config) _throw("Invalid argument");
				ContextAttribs *attribs=NULL;
				_newcheck(attribs=new ContextAttribs);
				attribs->config=config;
				attribs->direct=direct;
				attribs->colorIndex=colorIndex;
				HASH::add(ctx, NULL, attribs);
			}

			GLXFBConfig findConfig(GLXContext ctx)
			{
				if(!ctx) _throw("Invalid argument");
				ContextAttribs *attribs=HASH::find(ctx, NULL);
				if(attribs) return attribs->config;
				return 0;
			}

			bool isOverlay(GLXContext ctx)
			{
				if(ctx && findConfig(ctx)==(GLXFBConfig)-1) return true;
				return false;
			}

			Bool isDirect(GLXContext ctx)
			{
				if(ctx)
				{
					ContextAttribs *attribs=HASH::find(ctx, NULL);
					if(attribs) return attribs->direct;
				}
				return -1;
			}

			bool isColorIndex(GLXContext ctx)
			{
				if(ctx)
				{
					ContextAttribs *attribs=HASH::find(ctx, NULL);
					if(attribs) return attribs->colorIndex;
				}
				return false;
			}

			bool overlayCurrent(void)
			{
				return isOverlay(glXGetCurrentContext());
			}

			bool colorIndexCurrent(void)
			{
				return isColorIndex(glXGetCurrentContext());
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
				ContextAttribs *attribs=entry? (ContextAttribs *)entry->value:NULL;
				if(attribs) delete attribs;
			}

			bool compare(GLXContext key1, void *key2, HashEntry *entry)
			{
				return false;
			}

			static ContextHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define ctxhash (*(ContextHash::getInstance()))

#endif // __CONTEXTHASH_H__
