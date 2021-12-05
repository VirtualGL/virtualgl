// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2007 Sun Microsystems, Inc.
// Copyright (C)2011, 2014, 2021 D. R. Commander
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

// Thread-safe queue implementation using singly-linked lists

#ifndef __GENERICQ_H__
#define __GENERICQ_H__

#include "Mutex.h"


namespace util
{
	class GenericQ
	{
		public:

			typedef void (*SpoilCallback)(void *);

			GenericQ(void);
			~GenericQ(void);
			void add(void *item);
			void spoil(void *item, SpoilCallback spoilCallback);
			void get(void **item, bool nonBlocking = false);
			void release(void);
			int items(void);

		private:

			typedef struct EntryStruct
			{
				void *item;  struct EntryStruct *next;
			} Entry;

			Entry *start, *end;
			Semaphore hasItem;
			CriticalSection mutex;
			int deadYet;
	};
}

#endif  // __GENERICQ_H__
