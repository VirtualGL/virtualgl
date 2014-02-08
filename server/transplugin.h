/* Copyright (C)2009-2011, 2014 D. R. Commander
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

#define RRTRANS_NOPROTOTYPES
#include "rrtransport.h"
#include "Mutex.h"

using namespace vglutil;


typedef void* (*_RRTransInitType)(Display *, Window, FakerConfig *);
typedef int (*_RRTransConnectType)(void *, char *, int);
typedef RRFrame* (*_RRTransGetFrameType)(void *, int, int, int, int);
typedef int (*_RRTransReadyType)(void *);
typedef int (*_RRTransSynchronizeType)(void *);
typedef int (*_RRTransSendFrameType)(void *, RRFrame *, int);
typedef int (*_RRTransDestroyType)(void *);
typedef const char* (*_RRTransGetErrorType)(void);


class transplugin
{
	public:

		transplugin(Display *, Window, char *);
		~transplugin(void);
		void connect(char *, int);
		void destroy(void);
		int ready(void);
		void synchronize(void);
		RRFrame *getframe(int, int, int, bool);
		void sendframe(RRFrame *, bool);

	private:

		_RRTransInitType _RRTransInit;
		_RRTransConnectType _RRTransConnect;
		_RRTransGetFrameType _RRTransGetFrame;
		_RRTransReadyType _RRTransReady;
		_RRTransSynchronizeType _RRTransSynchronize;
		_RRTransSendFrameType _RRTransSendFrame;
		_RRTransDestroyType _RRTransDestroy;
		_RRTransGetErrorType _RRTransGetError;
		CS mutex;
		void *dllhnd, *handle;
};
