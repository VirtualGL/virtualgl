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
#include "rrframe.h"
#include "rrthread.h"
#include "genericQ.h"

class rrcwin : public Runnable
{
	public:

	rrcwin(int, Window);
	virtual ~rrcwin(void);
	rrjpeg *getFrame(void);
	void drawFrame(rrjpeg *);
	void redrawFrame(rrjpeg *);
	int match(int, Window);

	private:

	static const int NB=2;
	rrfb *b;  rrjpeg jpg[NB];  int jpgi;
	genericQ q;
	void showprofile(rrframeheader *, int);
	bool deadyet;
	int dpynum;  Window window;
	void run(void);
	Thread *t;
	rrcs jpgmutex;
};

#endif
