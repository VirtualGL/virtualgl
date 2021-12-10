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

#ifndef __TRANSPLUGIN_H__
#define __TRANSPLUGIN_H__

#define RRTRANS_NOPROTOTYPES
#include "rrtransport.h"
#include "Mutex.h"


typedef void *(*_RRTransInitType)(Display *, Window, FakerConfig *);
typedef int (*_RRTransConnectType)(void *, char *, int);
typedef RRFrame *(*_RRTransGetFrameType)(void *, int, int, int, int);
typedef int (*_RRTransReadyType)(void *);
typedef int (*_RRTransSynchronizeType)(void *);
typedef int (*_RRTransSendFrameType)(void *, RRFrame *, int);
typedef int (*_RRTransDestroyType)(void *);
typedef const char *(*_RRTransGetErrorType)(void);


namespace server
{
	class TransPlugin
	{
		public:

			TransPlugin(Display *dpy, Window win, char *name);
			~TransPlugin(void);
			void connect(char *receiverName, int port);
			void destroy(void);
			int ready(void);
			void synchronize(void);
			RRFrame *getFrame(int width, int height, int format, bool stereo);
			void sendFrame(RRFrame *frame, bool sync);

		private:

			_RRTransInitType _RRTransInit;
			_RRTransConnectType _RRTransConnect;
			_RRTransGetFrameType _RRTransGetFrame;
			_RRTransReadyType _RRTransReady;
			_RRTransSynchronizeType _RRTransSynchronize;
			_RRTransSendFrameType _RRTransSendFrame;
			_RRTransDestroyType _RRTransDestroy;
			_RRTransGetErrorType _RRTransGetError;
			util::CriticalSection mutex;
			void *dllhnd, *handle;
	};
}

#endif  // __TRANSPLUGIN_H__
