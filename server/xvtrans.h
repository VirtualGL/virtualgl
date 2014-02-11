/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009-2011, 2014 D. R. Commander
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

#ifndef __XVTRANS_H__
#define __XVTRANS_H__

#include "Thread.h"
#include "Frame.h"
#include "GenericQ.h"
#include "Profiler.h"

using namespace vglcommon;


class xvtrans : public Runnable
{
	public:

		xvtrans(void);

		virtual ~xvtrans(void)
		{
			_deadyet=true;  _q.release();
			if(_t) {_t->stop();  delete _t;  _t=NULL;}
			for(int i=0; i<NFRAMES; i++)
			{
				if(_frame[i]) delete _frame[i];  _frame[i]=NULL;
			}
		}

		bool ready(void);
		void synchronize(void);
		void sendframe(XVFrame *, bool sync=false);
		void run(void);
		XVFrame *getframe(Display *, Window, int, int);

	private:

		static const int NFRAMES=3;
		CS _mutex;  XVFrame *_frame[NFRAMES];
		Event _ready;
		GenericQ _q;
		Thread *_t;  bool _deadyet;
		Profiler _prof_xv, _prof_total;
};


#endif // __XVTRANS_H__
