// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2011, 2014, 2019-2020 D. R. Commander
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

#ifndef __EGLPBUFFERHASH_H__
#define __EGLPBUFFERHASH_H__

#include "VGLPbuffer.h"
#include "Hash.h"


#define HASH  Hash<EGLSurface, void *, VGLPbuffer *>

// This maps an EGLSurface handle to a VGLPbuffer instance

namespace vglfaker
{
	class EGLPbufferHash : public HASH
	{
		public:

			static EGLPbufferHash *getInstance(void)
			{
				if(instance == NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new EGLPbufferHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(EGLSurface eglpb, VGLPbuffer *pb)
			{
				if(!eglpb || !pb) THROW("Invalid argument");
				HASH::add(eglpb, NULL, pb);
			}

			VGLPbuffer *find(EGLSurface eglpb)
			{
				if(!eglpb) return NULL;
				return HASH::find(eglpb, NULL);
			}

			void remove(EGLSurface eglpb)
			{
				if(!eglpb) THROW("Invalid argument");
				HASH::remove(eglpb, NULL);
			}

		private:

			~EGLPbufferHash(void)
			{
				HASH::kill();
			}

			void detach(HashEntry *entry)
			{
				if(entry) delete entry->value;
			}

			bool compare(EGLSurface key1, void *key2, HashEntry *entry)
			{
				return false;
			}

			static EGLPbufferHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define epbhash  (*(vglfaker::EGLPbufferHash::getInstance()))

#endif  // __EGLPBUFFERHASH_H__
