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

// Thread-safe queue implementation using singly-linked lists
#include <string.h>
#include <errno.h>
#include "GenericQ.h"
#include "Error.h"
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif

using namespace util;


GenericQ::GenericQ(void)
{
	start = NULL;  end = NULL;
	deadYet = 0;
	#ifdef USEHELGRIND
	ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(int), );
	#endif
}


GenericQ::~GenericQ(void)
{
	deadYet = 1;
	release();
	mutex.lock(false);
	if(start != NULL)
	{
		Entry *temp;
		do
		{
			temp = start->next;  delete start;  start = temp;
		} while(start != NULL);
	}
	mutex.unlock(false);
}


void GenericQ::release(void)
{
	deadYet = 1;
	hasItem.post();
}


void GenericQ::spoil(void *item, SpoilCallback spoilCallback)
{
	if(deadYet) return;
	if(item == NULL) THROW("NULL argument in GenericQ::spoil()");
	CriticalSection::SafeLock l(mutex);
	if(deadYet) return;
	void *dummy = NULL;
	while(1)
	{
		get(&dummy, true);   if(!dummy) break;
		spoilCallback(dummy);
	}
	add(item);
}


void GenericQ::add(void *item)
{
	if(deadYet) return;
	if(item == NULL) THROW("NULL argument in GenericQ::add()");
	CriticalSection::SafeLock l(mutex);
	if(deadYet) return;
	Entry *temp = new Entry;
	if(temp == NULL) THROW("Alloc error");
	if(start == NULL) start = temp;
	else end->next = temp;
	temp->item = item;  temp->next = NULL;
	end = temp;
	hasItem.post();
}


// This will block until there is something in the queue
void GenericQ::get(void **item, bool nonBlocking)
{
	if(deadYet) return;
	if(item == NULL) THROW("NULL argument in GenericQ::get()");
	if(nonBlocking)
	{
		if(!hasItem.tryWait())
		{
			*item = NULL;  return;
		}
	}
	else hasItem.wait();
	if(!deadYet)
	{
		CriticalSection::SafeLock l(mutex);
		if(deadYet) return;
		if(start == NULL) THROW("Nothing in the queue");
		*item = start->item;
		Entry *temp = start->next;
		delete start;  start = temp;
	}
}


int GenericQ::items(void)
{
	int retval = 0;
	retval = hasItem.getValue();
	return retval;
}
