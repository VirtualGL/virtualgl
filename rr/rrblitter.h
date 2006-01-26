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
#include "rrprofiler.h"

class rrblitter : public Runnable
{
	public:

	rrblitter(void) : _bmpi(0), _t(NULL), _deadyet(false)
	{
		for(int i=0; i<NB; i++) _bmp[i]=NULL;
		errifnot(_t=new Thread(this));
		_t->start();
		_prof_blit.setname("Blit");
		_lastb=NULL;
	}

	virtual ~rrblitter(void)
	{
		_deadyet=true;  _q.release();
		if(_t) {_t->stop();  delete _t;  _t=NULL;}
		for(int i=0; i<NB; i++) {if(_bmp[i]) delete _bmp[i];  _bmp[i]=NULL;}
	}

	bool frameready(void);
	void sendframe(rrfb *, bool sync=false);
	void run(void);
	rrfb *getbitmap(Display *, Window, int, int);

	private:

	static const int NB=2;
	rrcs _bmpmutex;  rrfb *_bmp[NB];  int _bmpi;
	rrevent _ready;
	genericQ _q;
	void blitdiff(rrfb *, rrfb *);
	Thread *_t;  bool _deadyet;
	rrprofiler _prof_blit;
	rrfb *_lastb;
};

#endif
