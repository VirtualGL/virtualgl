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
#include "rrcommon.h"
#include "rrerror.h"


genericQ::genericQ(void)
{
	startptr=NULL;  endptr=NULL;
	sem_init(&qhasitem, 0, 0);
	pthread_mutex_init(&qmutex, NULL);
	deadyet=0;
}


genericQ::~genericQ(void)
{
	deadyet=1;
	release();
	pthread_mutex_lock(&qmutex);
	if(startptr!=NULL)
	{
		qstruct *temp;
		do {temp=startptr->next;  delete startptr;  startptr=temp;} while(startptr!=NULL);
	}
	pthread_mutex_unlock(&qmutex);
	pthread_mutex_destroy(&qmutex);
	sem_destroy(&qhasitem);
}


void genericQ::release(void)
{
	deadyet=1;
	sem_post(&qhasitem);
}


void genericQ::add(void *myval)
{
	if(deadyet) return;
	if(myval==NULL) _throw("NULL argument in genericQ::add()");
	rrlock l(qmutex);
	if(deadyet) return;
	qstruct *temp=new qstruct;
	if(temp==NULL) _throw("Alloc error");
	if(startptr==NULL) startptr=temp;
	else endptr->next=temp;
	temp->value=myval;  temp->next=NULL;
	endptr=temp;
	sem_post(&qhasitem);
}


// This will block until there is something in the queue
void genericQ::get(void **myval)
{
	if(deadyet) return;
	if(myval==NULL) _throw("NULL argument in genericQ::get()");
	tryunix(sem_wait(&qhasitem));
	if(!deadyet)
	{
		rrlock l(qmutex);
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
	tryunix(sem_getvalue(&qhasitem, &retval));
	return retval;
}
