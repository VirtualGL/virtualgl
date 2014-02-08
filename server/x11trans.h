/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2010-2011, 2014 D. R. Commander
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

#ifndef __X11TRANS_H__
#define __X11TRANS_H__

#include "Thread.h"
#include "rrframe.h"
#include "GenericQ.h"
#include "rrprofiler.h"

using namespace vglutil;


class x11trans : public Runnable
{
	public:

		x11trans(void);

		virtual ~x11trans(void)
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
		void sendframe(rrfb *, bool sync=false);
		void run(void);
		rrfb *getframe(Display *, Window, int, int);

	private:

		static const int NFRAMES=3;
		CS _mutex;  rrfb *_frame[NFRAMES];
		Event _ready;
		GenericQ _q;
		Thread *_t;  bool _deadyet;
		rrprofiler _prof_blit, _prof_total;
};


#endif // __X11TRANS_H__
