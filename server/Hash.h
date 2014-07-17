/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014 D. R. Commander
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

#ifndef __HASH_H__
#define __HASH_H__

#include "Mutex.h"
#include "Error.h"


// Generic hash table template class

namespace vglserver
{
	template <class HashKeyType1, class HashKeyType2, class HashValueType>
	class Hash
	{
		public:

			typedef struct HashEntryStruct
			{
				HashKeyType1 key1;
				HashKeyType2 key2;
				HashValueType value;
				int refCount;
				struct HashEntryStruct *prev, *next;
			} HashEntry;

			void kill(void)
			{
				vglutil::CriticalSection::SafeLock l(mutex);
				while(start!=NULL) killEntry(start);
			}

		protected:

			Hash(void)
			{
				start=end=NULL;
				count=0;
			}

			virtual ~Hash(void)
			{
				kill();
			}

			int add(HashKeyType1 key1, HashKeyType2 key2, HashValueType value,
				bool useRef=false)
			{
				HashEntry *entry=NULL;

				if(!key1) _throw("Invalid argument");
				vglutil::CriticalSection::SafeLock l(mutex);

				if((entry=findEntry(key1, key2))!=NULL)
				{
					if(value) entry->value=value;
					if(useRef) entry->refCount++;  return 0;
				}
				_newcheck(entry=new HashEntry);
				memset(entry, 0, sizeof(HashEntry));
				entry->prev=end;  if(end) end->next=entry;
				if(!start) start=entry;
				end=entry;
				end->key1=key1;  end->key2=key2;  end->value=value;
				if(useRef) end->refCount=1;
				count++;
				return 1;
			}

			HashValueType find(HashKeyType1 key1, HashKeyType2 key2)
			{
				HashEntry *entry=NULL;
				vglutil::CriticalSection::SafeLock l(mutex);

				if((entry=findEntry(key1, key2))!=NULL)
				{
					if(!entry->value) entry->value=attach(key1, key2);
					return entry->value;
				}
				return (HashValueType)0;
			}

			void remove(HashKeyType1 key1, HashKeyType2 key2, bool useRef=false)
			{
				HashEntry *entry=NULL;
				vglutil::CriticalSection::SafeLock l(mutex);

				if((entry=findEntry(key1, key2))!=NULL)
				{
					if(useRef && entry->refCount>0) entry->refCount--;
					if(!useRef || entry->refCount<=0) killEntry(entry);
				}
			}

			int getCount(void) { return count; }

			HashEntry *findEntry(HashKeyType1 key1, HashKeyType2 key2)
			{
				HashEntry *entry=NULL;
				vglutil::CriticalSection::SafeLock l(mutex);

				entry=start;
				while(entry!=NULL)
				{
					if((entry->key1==key1 && entry->key2==key2)
						|| compare(key1, key2, entry))
					{
						return entry;
					}
					entry=entry->next;
				}
				return NULL;
			}

			void killEntry(HashEntry *entry)
			{
				vglutil::CriticalSection::SafeLock l(mutex);

				if(entry->prev) entry->prev->next=entry->next;
				if(entry->next) entry->next->prev=entry->prev;
				if(entry==start) start=entry->next;
				if(entry==end) end=entry->prev;
				if(entry->value) detach(entry);
				memset(entry, 0, sizeof(HashEntry));
				delete entry;
				count--;
			}

			virtual HashValueType attach(HashKeyType1 key1, HashKeyType2 key2)
			{
				return 0;
			}

			virtual void detach(HashEntry *entry)=0;
			virtual bool compare(HashKeyType1 key1, HashKeyType2 key2,
				HashEntry *entry)=0;

			int count;
			HashEntry *start, *end;
			vglutil::CriticalSection mutex;
	};
}

#endif // __HASH_H__
