/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include "rrutil.h"
#include "rrthread.h"
#include "rrmutex.h"

rrevent event;
rrsem sem;

class testthread : public Runnable
{
	public:

		testthread(int myrank) : _myrank(myrank) {}

		void run(void)
		{
			try {
			switch(_myrank)
			{
				case 1:
					printf("\nGreetings from thread %d (ID %lu)\n", _myrank, _threadId);
					printf("Unlocking thread 2\n");
					fflush(stdout);
					sleep(2);
					event.signal();
					break;
				case 2:
					event.wait();
					printf("\nThread 2 unlocked\n");
					printf("Greetings from thread %d (ID %lu)\n", _myrank, _threadId);
					fflush(stdout);
					event.signal();
					sleep(2);
					printf("\n2: Releasing two threads.\n");
					fflush(stdout);
					sem.post();  sem.post();
					sleep(2);
					printf("\n2: Releasing a third thread.\n");
					fflush(stdout);
					sem.post();
					break;
				case 3:  case 4:  case 5:
					sem.wait();
					printf("Greetings from thread %d (ID %lu)\n", _myrank, _threadId);
					fflush(stdout);
					break;
			}
			}
			catch(rrerror &e)
			{
				printf("Error in %s (Thread %d):\n%s\n", e.getMethod(), _myrank, e.getMessage());
			}
			if(_myrank==5) _throw("Error test");
		}

	private:

		int _myrank;
};


int main(void)
{
	testthread *th[5];  Thread *t[5];  int i;

	try {

	printf("Number of CPU's in this system:  %d\n", numprocs());
	printf("Word size = %d-bit\n", (int)sizeof(long)*8);

	event.wait();

	for(i=0; i<5; i++)
	{
		th[i]=new testthread(i+1);
		t[i]=new Thread(th[i]);
		t[i]->start();
	}
	for(i=0; i<5; i++) t[i]->stop();
	for(i=0; i<5; i++) t[i]->checkerror();

	}
	catch(rrerror &e)
	{
		printf("Error in %s:\n%s\n", e.getMethod(), e.getMessage());
	}

	return 0;
}

