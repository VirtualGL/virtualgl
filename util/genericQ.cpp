/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2011 D. R. Commander
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

// Thread-safe queue implementation using singly linked lists
#include <string.h>
#include <errno.h>
#include "genericQ.h"
#include "rrerror.h"


genericQ::genericQ(void)
{
	_start=NULL;  _end=NULL;
	_deadyet=0;
}


genericQ::~genericQ(void)
{
	_deadyet=1;
	release();
	_qmutex.lock(false);
	if(_start!=NULL)
	{
		qstruct *temp;
		do {temp=_start->next;  delete _start;  _start=temp;} while(_start!=NULL);
	}
	_qmutex.unlock(false);
}


void genericQ::release(void)
{
	_deadyet=1;
	_qhasitem.post();
}


void genericQ::spoil(void *myval, qspoilfct f)
{
	if(_deadyet) return;
	if(myval==NULL) _throw("NULL argument in genericQ::spoil()");
	rrcs::safelock l(_qmutex);
	if(_deadyet) return;
	void *dummy;
	while(1)
	{
		get(&dummy, true);   if(!dummy) break;
		f(dummy);
	}
	add(myval);
}

void genericQ::add(void *myval)
{
	if(_deadyet) return;
	if(myval==NULL) _throw("NULL argument in genericQ::add()");
	rrcs::safelock l(_qmutex);
	if(_deadyet) return;
	qstruct *temp=new qstruct;
	if(temp==NULL) _throw("Alloc error");
	if(_start==NULL) _start=temp;
	else _end->next=temp;
	temp->value=myval;  temp->next=NULL;
	_end=temp;
	_qhasitem.post();
}


// This will block until there is something in the queue
void genericQ::get(void **myval, bool nonblocking)
{
	if(_deadyet) return;
	if(myval==NULL) _throw("NULL argument in genericQ::get()");
	if(nonblocking)
	{
		if(!_qhasitem.trywait())
		{
			*myval=NULL;  return;
		}
	}
	else _qhasitem.wait();
	if(!_deadyet)
	{
		rrcs::safelock l(_qmutex);
		if(_deadyet) return;
		if(_start==NULL) _throw("Nothing in the queue");
		*myval=_start->value;
		qstruct *temp=_start->next;
		delete _start;  _start=temp;
	}
}


int genericQ::items(void)
{
	int retval=0;
	retval=_qhasitem.getvalue();
	return retval;
}
