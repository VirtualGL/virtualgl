/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
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
	startptr=NULL;  endptr=NULL;
	deadyet=0;
}


genericQ::~genericQ(void)
{
	deadyet=1;
	release();
	qmutex.lock(false);
	if(startptr!=NULL)
	{
		qstruct *temp;
		do {temp=startptr->next;  delete startptr;  startptr=temp;} while(startptr!=NULL);
	}
	qmutex.unlock(false);
}


void genericQ::release(void)
{
	deadyet=1;
	qhasitem.post();
}


void genericQ::spoil(void *myval, qspoilfct f)
{
	if(deadyet) return;
	if(myval==NULL) _throw("NULL argument in genericQ::spoil()");
	rrcs::safelock l(qmutex);
	if(deadyet) return;
	void *dummy;
	for(int i=0; i<items(); i++)
	{
		get(&dummy);
		f(dummy);
	}
	add(myval);
}

void genericQ::add(void *myval)
{
	if(deadyet) return;
	if(myval==NULL) _throw("NULL argument in genericQ::add()");
	qstruct *temp=new qstruct;
	if(temp==NULL) _throw("Alloc error");
	if(startptr==NULL) startptr=temp;
	else endptr->next=temp;
	temp->value=myval;  temp->next=NULL;
	endptr=temp;
	qhasitem.post();
}


// This will block until there is something in the queue
void genericQ::get(void **myval)
{
	if(deadyet) return;
	if(myval==NULL) _throw("NULL argument in genericQ::get()");
	qhasitem.wait();
	if(!deadyet)
	{
		rrcs::safelock l(qmutex);
		if(deadyet) return;
		if(startptr==NULL) _throw("Nothing in the queue");
		*myval=startptr->value;
		qstruct *temp=startptr->next;
		delete startptr;  startptr=temp;
	}
}


int genericQ::items(void)
{
	int retval=0;
	retval=qhasitem.getvalue();
	return retval;
}

