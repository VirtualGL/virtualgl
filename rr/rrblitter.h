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

#include "rrthread.h"
#include "rrframe.h"
#include "genericQ.h"

class rrblitter : public Runnable
{
	public:

	rrblitter(void) : bmpi(0), t(NULL), deadyet(false)
	{
		for(int i=0; i<NB; i++) bmp[i]=NULL;
		errifnot(t=new Thread(this));
		t->start();
	}

	virtual ~rrblitter(void)
	{
		deadyet=true;  q.release();
		if(t) {t->stop();  delete t;  t=NULL;}
		for(int i=0; i<NB; i++) {if(bmp[i]) delete bmp[i];  bmp[i]=NULL;}
	}

	bool frameready(void);
	void sendframe(rrfb *);
	void run(void);
	rrfb *getbitmap(Display *, Window, int, int);

	private:

	static const int NB=3;
	rrcs bmpmutex;  rrfb *bmp[NB];  int bmpi;
	rrmutex ready;
	genericQ q;
	void blitdiff(rrfb *, rrfb *);
	Thread *t;  bool deadyet;
};

#endif
