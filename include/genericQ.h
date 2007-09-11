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

#ifndef __GENERICQ_H__
#define __GENERICQ_H__

#include "rrmutex.h"


enum {GENQ_SUCCESS=0, GENQ_FAILURE};


typedef struct _qstruct
{
	void *value;	struct _qstruct *next;
} qstruct;


typedef void (*qspoilfct)(void *);


class genericQ
{
	public:
		genericQ(void);
		~genericQ(void);
		void add(void *);
		void spoil(void *, qspoilfct);
		void get(void **, bool nonblocking=false);
		void release(void);
		int items(void);
	private:
		qstruct *startptr, *endptr;
		rrsem qhasitem;
		rrcs qmutex;
		int deadyet;
};

#endif
