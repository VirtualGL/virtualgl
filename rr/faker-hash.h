/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
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

// Generic hash table

#include <pthread.h>

#include "rrmutex.h"

typedef struct __hashclassstruct
{
	_hashkeytype1 key1;
	_hashkeytype2 key2;
	_hashvaluetype value;
	int refcount;
	struct __hashclassstruct *prev, *next;
} _hashclassstruct;

class _hashclass
{
	public:

		void killhash(void)
		{
			rrcs::safelock l(mutex);
			while(start!=NULL) killentry(start);
		}

		int add(_hashkeytype1 key1, _hashkeytype2 key2, _hashvaluetype value, bool useref=false)
		{
			_hashclassstruct *ptr=NULL;
			if(!key1) _throw("Invalid argument");
			rrcs::safelock l(mutex);
			if((ptr=findentry(key1, key2))!=NULL)
			{
				if(value) ptr->value=value;
				if(useref) ptr->refcount++;  return 0;
			}
			errifnot(ptr=new _hashclassstruct);
			memset(ptr, 0, sizeof(_hashclassstruct));
			ptr->prev=end;  if(end) end->next=ptr;
			if(!start) start=ptr;
			end=ptr;
			end->key1=key1;  end->key2=key2;  end->value=value;
			if(useref) end->refcount=1;
			entries++;
			return 1;
		}

		_hashvaluetype find(_hashkeytype1 key1, _hashkeytype2 key2)
		{
			_hashclassstruct *ptr=NULL;
//			if(!key1) _throw("Invalid argument");
			rrcs::safelock l(mutex);
			if((ptr=findentry(key1, key2))!=NULL)
			{
				if(!ptr->value) ptr->value=attach(key1, key2);
				return ptr->value;
			}
			return (_hashvaluetype)0;
		}

		void remove(_hashkeytype1 key1, _hashkeytype2 key2, bool useref=false)
		{
			_hashclassstruct *ptr=NULL;
//			if(!key1) _throw("Invalid argument");
			rrcs::safelock l(mutex);
			if((ptr=findentry(key1, key2))!=NULL)
			{
				if(useref && ptr->refcount>0) ptr->refcount--;
				if(!useref || ptr->refcount<=0) killentry(ptr);
			}
		}

		int numEntries(void) {return entries;}

	protected:

		_hashclass(void)
		{
			start=end=NULL;
			entries=0;
		}

		virtual ~_hashclass(void)
		{
			killhash();
		}

		_hashclassstruct *findentry(_hashkeytype1 key1, _hashkeytype2 key2)
		{
			_hashclassstruct *ptr=NULL;
//			if(!key1) _throw("Invalid argument");
			rrcs::safelock l(mutex);
			ptr=start;
			while(ptr!=NULL)
			{
				if((ptr->key1==key1 && ptr->key2==key2) || compare(key1, key2, ptr))
				{
					return ptr;
				}
				ptr=ptr->next;
			}
			return NULL;
		}

		void killentry(_hashclassstruct *ptr)
		{
			rrcs::safelock l(mutex);
			if(ptr->prev) ptr->prev->next=ptr->next;
			if(ptr->next) ptr->next->prev=ptr->prev;
			if(ptr==start) start=ptr->next;
			if(ptr==end) end=ptr->prev;
			if(ptr->value) detach(ptr);
			memset(ptr, 0, sizeof(_hashclassstruct));
			delete ptr;
			entries--;
		}

		int entries;
		virtual _hashvaluetype attach(_hashkeytype1, _hashkeytype2)=0;
		virtual void detach(_hashclassstruct *h)=0;
		virtual bool compare(_hashkeytype1, _hashkeytype2, _hashclassstruct *h)=0;
		_hashclassstruct *start, *end;
		rrcs mutex;
};
