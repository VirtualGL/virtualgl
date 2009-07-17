/* Copyright (C)2009 D. R. Commander
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
#include "rrmutex.h"

typedef void* (*_RRTransInitType)(FakerConfig *);
typedef int (*_RRTransConnectType)(void *, char *, int);
typedef RRFrame* (*_RRTransGetFrameType)(void *, int, int, int, int);
typedef int (*_RRTransFrameReadyType)(void *);
typedef int (*_RRTransSendFrameType)(void *, RRFrame *);
typedef int (*_RRTransDestroyType)(void *);
typedef const char* (*_RRTransGetErrorType)(void);

class rrplugin
{
	public:

		rrplugin(char *);
		~rrplugin(void);
		void connect(char *, int);
		void destroy(void);
		int frameready(void);
		RRFrame *getframe(int, int, bool, bool);
		void sendframe(RRFrame *);

	private:

		_RRTransInitType _RRTransInit;
		_RRTransConnectType _RRTransConnect;
		_RRTransGetFrameType _RRTransGetFrame;
		_RRTransFrameReadyType _RRTransFrameReady;
		_RRTransSendFrameType _RRTransSendFrame;
		_RRTransDestroyType _RRTransDestroy;
		_RRTransGetErrorType _RRTransGetError;
		rrcs mutex;
		void *dllhnd, *handle;
};
