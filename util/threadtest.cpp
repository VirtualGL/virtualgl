/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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
#include "vglutil.h"
#include "Thread.h"
#include "Mutex.h"

using namespace vglutil;


Event event;
Semaphore sem;


class TestThread : public Runnable
{
	public:

		TestThread(int myRank_) : myRank(myRank_) {}

		void run(void)
		{
			try
			{
				switch(myRank)
				{
					case 1:
						printf("\nGreetings from thread %d (ID %lu)\n", myRank, threadID);
						printf("Unlocking thread 2\n");
						fflush(stdout);
						sleep(2);
						event.signal();
						break;
					case 2:
						event.wait();
						printf("\nThread 2 unlocked\n");
						printf("Greetings from thread %d (ID %lu)\n", myRank, threadID);
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
						printf("Greetings from thread %d (ID %lu)\n", myRank, threadID);
						fflush(stdout);
						break;
				}
			}
			catch(Error &e)
			{
				printf("Error in %s (Thread %d):\n%s\n", e.getMethod(), myRank,
					e.getMessage());
			}
			if(myRank==5) _throw("Error test");
		}

	private:

		int myRank;
};


int main(void)
{
	TestThread *testThread[5];  Thread *thread[5];  int i;

	try
	{
		printf("Number of CPU's in this system:  %d\n", numprocs());
		printf("Word size = %d-bit\n", (int)sizeof(long*)*8);

		event.wait();

		for(i=0; i<5; i++)
		{
			testThread[i]=new TestThread(i+1);
			thread[i]=new Thread(testThread[i]);
			thread[i]->start();
		}
		for(i=0; i<5; i++) thread[i]->stop();
		for(i=0; i<5; i++) thread[i]->checkError();
	}
	catch(Error &e)
	{
		printf("Error in %s:\n%s\n", e.getMethod(), e.getMessage());
	}

	return 0;
}
