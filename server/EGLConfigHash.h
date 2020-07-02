// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2014, 2016, 2019-2020 D. R. Commander
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

#ifndef __EGLCONFIGHASH_H__
#define __EGLCONFIGHASH_H__

#include "EGLRBOContext.h"
#include "Hash.h"


#define HASH  Hash<EGLConfig, void *, EGLRBOContext *>

// This maps an EGLConfig handle to an EGLRBOContext instance

namespace vglfaker
{
	class EGLConfigHash : public HASH
	{
		public:

			static EGLConfigHash *getInstance(void)
			{
				if(instance == NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new EGLConfigHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			EGLRBOContext *getRBOContext(EGLConfig config)
			{
				if(!config) THROW("Invalid argument");
				vglutil::CriticalSection::SafeLock l(mutex);
				EGLRBOContext *rboCtx = HASH::find(config, NULL);
				if(!rboCtx)
				{
					rboCtx = new EGLRBOContext(config);
					HASH::add(config, NULL, rboCtx);
				}
				return rboCtx;
			}

		private:

			~EGLConfigHash(void)
			{
				HASH::kill();
			}

			bool compare(EGLConfig key1, void *key2, HashEntry *entry)
			{
				return false;
			}

			void detach(HashEntry *entry)
			{
				if(entry) delete entry->value;
			}

			static EGLConfigHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define ecfghash  (*(vglfaker::EGLConfigHash::getInstance()))

#endif  // __EGLCONFIGHASH_H__
