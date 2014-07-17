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

#include "Frame.h"
#include "Thread.h"
#include "GenericQ.h"


enum {RR_DRAWAUTO=-1, RR_DRAWX11=0, RR_DRAWOGL};


namespace vglclient
{
	class ClientWin : public vglutil::Runnable
	{
		public:

			ClientWin(int dpynum, Window window, int drawMethod, bool stereo);
			virtual ~ClientWin(void);
			vglcommon::Frame *getFrame(bool useXV);
			void drawFrame(vglcommon::Frame *f);
			int match(int dpynum, Window window);
			bool isStereo(void) { return stereo; }

		private:

			void initGL(void);
			void initX11(void);

			int drawMethod, reqDrawMethod;
			static const int NFRAMES=2;
			vglcommon::Frame *fb;
			vglcommon::CompressedFrame cframes[NFRAMES];  int cfindex;
			#ifdef USEXV
			vglcommon::XVFrame *xvframes[NFRAMES];
			#endif
			vglutil::GenericQ q;
			bool deadYet;
			int dpynum;  Window window;
			void run(void);
			vglutil::Thread *thread;
			vglutil::CriticalSection cfmutex;
			bool stereo;
			vglutil::CriticalSection mutex;
	};
}

#endif // __CLIENTWIN_H__
