// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009-2011, 2014, 2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#ifndef __XVTRANS_H__
#define __XVTRANS_H__

#include "Thread.h"
#include "Frame.h"
#include "GenericQ.h"
#include "Profiler.h"


namespace server
{
	class XVTrans : public util::Runnable
	{
		public:

			XVTrans(void);

			virtual ~XVTrans(void)
			{
				deadYet = true;
				q.release();
				if(thread) { thread->stop();  delete thread;  thread = NULL; }
				for(int i = 0; i < NFRAMES; i++)
				{
					delete frames[i];  frames[i] = NULL;
				}
			}

			bool isReady(void);
			void synchronize(void);
			void sendFrame(common::XVFrame *f, bool sync = false);
			void run(void);
			common::XVFrame *getFrame(Display *dpy, Window win, int w, int h);

		private:

			static const int NFRAMES = 3;
			util::CriticalSection mutex;
			common::XVFrame *frames[NFRAMES];
			util::Event ready;
			util::GenericQ q;
			util::Thread *thread;
			bool deadYet;
			common::Profiler profXV, profTotal;
	};
}

#endif  // __XVTRANS_H__
