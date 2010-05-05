/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2010 D. R. Commander
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

#ifndef __RRBLITTER_H
#define __RRBLITTER_H

#include "rrthread.h"
#include "rrframe.h"
#include "genericQ.h"
#include "rrprofiler.h"

class rrblitter : public Runnable
{
	public:

	rrblitter(void);

	virtual ~rrblitter(void)
	{
		_deadyet=true;  _q.release();
		if(_t) {_t->stop();  delete _t;  _t=NULL;}
		for(int i=0; i<NB; i++) {if(_bmp[i]) delete _bmp[i];  _bmp[i]=NULL;}
	}

	bool ready(void);
	void synchronize(void);
	void sendframe(rrfb *, bool sync=false);
	void run(void);
	rrfb *getbitmap(Display *, Window, int, int);

	private:

	static const int NB=3;
	rrcs _bmpmutex;  rrfb *_bmp[NB];
	rrevent _ready;
	genericQ _q;
	Thread *_t;  bool _deadyet;
	rrprofiler _prof_blit, _prof_total;
};

#endif
