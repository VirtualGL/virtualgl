// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2010-2011, 2014, 2018-2019, 2021 D. R. Commander
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

#ifndef __VGLTRANSRECEIVER_H__
#define __VGLTRANSRECEIVER_H__

#include "Socket.h"
#include "ClientWin.h"
#include "Log.h"


#define MAXWIN  1024


namespace client
{
	class VGLTransReceiver : public util::Runnable
	{
		public:

			VGLTransReceiver(bool ipv6, int drawmethod);
			void listen(unsigned short port);
			unsigned short getPort(void) { return port; }
			virtual ~VGLTransReceiver(void);

		private:

			void run(void);

			int drawMethod;
			util::Socket *listenSocket;
			util::CriticalSection listenMutex;
			util::Thread *thread;
			bool deadYet;
			bool ipv6;
			unsigned short port;

		class Listener : public util::Runnable
		{
			public:

				Listener(util::Socket *socket_, int drawMethod_) :
					drawMethod(drawMethod_), nwin(0), socket(socket_), thread(NULL),
					remoteName(NULL)
				{
					memset(windows, 0, sizeof(ClientWin *) * MAXWIN);
					if(socket) remoteName = socket->remoteName();
					thread = new util::Thread(this);
					thread->start();
				}

				virtual ~Listener(void)
				{
					int i;

					winMutex.lock(false);
					for(i = 0; i < nwin; i++)
					{
						delete windows[i];  windows[i] = NULL;
					}
					nwin = 0;
					winMutex.unlock(false);
					if(!remoteName) vglout.PRINTLN("-- Disconnecting\n");
					else vglout.PRINTLN("-- Disconnecting %s", remoteName);
					delete socket;  socket = NULL;
				}

				void send(char *buf, int len);
				void recv(char *buf, int len);

			private:

				void run(void);

				int drawMethod;
				ClientWin *windows[MAXWIN];
				int nwin;
				ClientWin *addWindow(int dpynum, Window win, bool stereo = false);
				void deleteWindow(ClientWin *win);
				util::CriticalSection winMutex;
				util::Socket *socket;
				util::Thread *thread;
				const char *remoteName;
		};
	};
}

#endif  // __VGLTRANSRECEIVER_H__
