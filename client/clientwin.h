/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011, 2014 D. R. Commander
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

#ifndef __CLIENTWIN_H__
#define __CLIENTWIN_H__

#ifdef _WIN32
#include <windows.h>
#endif
#include "Frame.h"
#include "Thread.h"
#include "genericQ.h"

using namespace vglcommon;


enum {RR_DRAWAUTO=-1, RR_DRAWX11=0, RR_DRAWOGL};


class clientwin : public Runnable
{
	public:

		clientwin(int, Window, int, bool);
		virtual ~clientwin(void);
		Frame *getframe(bool);
		void drawframe(Frame *);
		int match(int, Window);
		bool stereoenabled(void) {return _stereo;}

	private:

		int _drawmethod, _reqdrawmethod;
		static const int NFRAMES=2;
		Frame *_fb;  CompressedFrame _cf[NFRAMES];  int _cfi;
		#ifdef USEXV
		XVFrame *_xvf[NFRAMES];
		#endif
		void initgl(void);
		void initx11(void);
		void setdrawmethod(void);
		GenericQ _q;
		void showprofile(rrframeheader *, int);
		bool _deadyet;
		int _dpynum;  Window _window;
		void run(void);
		Thread *_t;
		CS _cfmutex;
		bool _stereo;
		CS _mutex;
};


#endif // __CLIENTWIN_H__
