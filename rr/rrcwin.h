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

#ifndef __RRCWIN_H
#define __RRCWIN_H

#include <X11/X.h>
#ifndef _WIN32
#define USEGL 1
#endif
#include "rrframe.h"
#include "rrthread.h"
#include "genericQ.h"

enum {RR_DRAWAUTO=0, RR_DRAWX11, RR_DRAWOGL};

class rrcwin : public Runnable
{
	public:

	rrcwin(int, Window, int, bool);
	virtual ~rrcwin(void);
	rrjpeg *getFrame(void);
	void drawFrame(rrjpeg *);
	void redrawFrame(rrjpeg *);
	int match(int, Window);
	bool stereoenabled(void) {return _stereo;}

	private:

	int _drawmethod;
	static const int NB=2;
	rrfb *_b;  rrjpeg _jpg[NB];  int _jpgi;
	#ifdef USEGL
	rrglframe *_glf;
	void initgl(void);
	#endif
	genericQ _q;
	void showprofile(rrframeheader *, int);
	bool _deadyet;
	int _dpynum;  Window _window;
	void run(void);
	Thread *_t;
	rrcs _jpgmutex;
	bool _stereo;
};

#endif
