/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009 D. R. Commander
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

#ifndef __RRCWIN_H
#define __RRCWIN_H

#ifdef _WIN32
#include <windows.h>
#endif
#include "rrframe.h"
#undef USEGLP
#include "rrthread.h"
#include "genericQ.h"

enum {RR_DRAWAUTO=-1, RR_DRAWX11=0, RR_DRAWOGL};

class rrcwin : public Runnable
{
	public:

	rrcwin(int, Window, int, bool);
	virtual ~rrcwin(void);
	rrframe *getFrame(bool);
	void drawFrame(rrframe *);
	int match(int, Window);
	bool stereoenabled(void) {return _stereo;}

	private:

	int _drawmethod, _reqdrawmethod;
	static const int NB=2;
	rrframe *_b;  rrcompframe _cf[NB];  int _cfi;
	#ifdef USEXV
	rrxvframe *_xvf[NB];
	#endif
	void initgl(void);
	void initx11(void);
	void setdrawmethod(void);
	genericQ _q;
	void showprofile(rrframeheader *, int);
	bool _deadyet;
	int _dpynum;  Window _window;
	void run(void);
	Thread *_t;
	rrcs _cfmutex;
	bool _stereo;
	rrcs _mutex;
};

#endif
