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

#ifndef __RRBLITTER_H
#define __RRBLITTER_H

#include "pthread.h"
#include "rrframe.h"
#include "rrerror.h"
#include "genericQ.h"
#include "rrprofiler.h"

class rrblitter
{
	public:

	rrblitter(void) : bmpi(0), thnd(0), deadyet(false)
	{
		for(int i=0; i<NB; i++) bmp[i]=NULL;
		pthread_mutex_init(&ready, NULL);
		pthread_mutex_init(&bmpmutex, NULL);
		prof_blit.setname("Blit");
		tryunix(pthread_create(&thnd, NULL, run, this));
	}

	virtual ~rrblitter(void)
	{
		deadyet=true;  q.release();
		if(thnd) {pthread_join(thnd, NULL);  thnd=0;}
		for(int i=0; i<NB; i++) {if(bmp[i]) delete bmp[i];  bmp[i]=NULL;}
		pthread_mutex_unlock(&ready);  pthread_mutex_destroy(&ready);
		pthread_mutex_unlock(&bmpmutex);  pthread_mutex_destroy(&bmpmutex);
	}

	bool frameready(void);
	void sendframe(rrbitmap *);
	static void *run(void *);
	rrbitmap *getbitmap(Display *, Window, int, int);

	private:

	void checkerror(void);
	rrerror lasterror;
	static const int NB=2;
	pthread_mutex_t bmpmutex;  rrbitmap *bmp[NB];  int bmpi;
	pthread_mutex_t ready;
	genericQ q;
	void blitdiff(rrbitmap *, rrbitmap *);
	pthread_t thnd;  bool deadyet;
	rrprofiler prof_blit;
};

#endif
