// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2011, 2014, 2019-2021 D. R. Commander
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

#ifndef __PBUFFERHASHEGL_H__
#define __PBUFFERHASHEGL_H__

#include "FakePbuffer.h"
#include "Hash.h"


#define HASH  faker::Hash<GLXDrawable, void *, backend::FakePbuffer *>

// This maps a GLX drawable ID to a backend::FakePbuffer instance

namespace backend
{
	class PbufferHashEGL : public HASH
	{
		public:

			static PbufferHashEGL *getInstance(void)
			{
				if(instance == NULL)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new PbufferHashEGL;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(GLXDrawable id, backend::FakePbuffer *pb)
			{
				if(!id || !pb) THROW("Invalid argument");
				HASH::add(id, NULL, pb);
			}

			backend::FakePbuffer *find(GLXDrawable id)
			{
				if(!id) return NULL;
				return HASH::find(id, NULL);
			}

			void remove(GLXDrawable id)
			{
				if(!id) THROW("Invalid argument");
				HASH::remove(id, NULL);
			}

		private:

			~PbufferHashEGL(void)
			{
				HASH::kill();
			}

			void detach(HashEntry *entry)
			{
				if(entry) delete entry->value;
			}

			bool compare(GLXDrawable key1, void *key2, HashEntry *entry)
			{
				return false;
			}

			static PbufferHashEGL *instance;
			static util::CriticalSection instanceMutex;
	};
}

#undef HASH


#define pbhashegl  (*(backend::PbufferHashEGL::getInstance()))

#endif  // __PBUFFERHASHEGL_H__
