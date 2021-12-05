// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2010-2011, 2014, 2019-2021 D. R. Commander
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

#ifndef __VGLTRANS_H__
#define __VGLTRANS_H__

#include "Socket.h"
#include "Thread.h"
#include "rr.h"
#include "Frame.h"
#include "GenericQ.h"
#include "Profiler.h"
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif


namespace server
{
	class VGLTrans : public util::Runnable
	{
		public:

			VGLTrans(void);

			virtual ~VGLTrans(void)
			{
				deadYet = true;  q.release();
				if(thread) { thread->stop();  delete thread;  thread = NULL; }
				delete socket;  socket = NULL;
			}

			common::Frame *getFrame(int, int, int, int, bool stereo);
			bool isReady(void);
			void synchronize(void);
			void sendFrame(common::Frame *);
			void run(void);
			void sendHeader(rrframeheader h, bool eof = false);
			void send(char *, int);
			void save(char *, int);
			void recv(char *, int);
			void connect(char *, unsigned short);

			int nprocs;

		private:

			util::Socket *socket;
			static const int NFRAMES = 4;
			util::CriticalSection mutex;
			common::Frame frames[NFRAMES];
			util::Event ready;
			util::GenericQ q;
			util::Thread *thread;  bool deadYet;
			common::Profiler profTotal;
			int dpynum;
			rrversion version;

		class Compressor : public util::Runnable
		{
			public:

				Compressor(int myRank_, VGLTrans *parent_) : bytes(0),
					storedFrames(0), cframes(NULL), frame(NULL), lastFrame(NULL),
					myRank(myRank_), deadYet(false), parent(parent_)
				{
					if(parent) nprocs = parent->nprocs;
					ready.wait();  complete.wait();
					char temps[20];
					snprintf(temps, 20, "Compress %d", myRank);
					profComp.setName(temps);
					#ifdef USEHELGRIND
					ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
					#endif
				}

				virtual ~Compressor(void)
				{
					shutdown();
					free(cframes);  cframes = NULL;
				}

				void run(void)
				{
					while(!deadYet)
					{
						try
						{
							ready.wait();  if(deadYet) break;
							compressSend(frame, lastFrame);
							complete.signal();
						}
						catch(...)
						{
							complete.signal();  throw;
						}
					}
				}

				void go(common::Frame *frame_, common::Frame *lastFrame_)
				{
					frame = frame_;  lastFrame = lastFrame_;
					ready.signal();
				}

				void stop(void)
				{
					complete.wait();
				}

				void shutdown(void) { deadYet = true;  ready.signal(); }
				void compressSend(common::Frame *frame, common::Frame *lastFrame);
				void send(void);

				long bytes;

			private:

				void store(common::CompressedFrame *cf)
				{
					storedFrames++;
					if(!(cframes = (common::CompressedFrame **)realloc(cframes,
						sizeof(common::CompressedFrame *) * storedFrames)))
						THROW("Memory allocation error");
					cframes[storedFrames - 1] = cf;
				}

				int storedFrames;  common::CompressedFrame **cframes;
				common::Frame *frame, *lastFrame;
				int myRank, nprocs;
				util::Event ready, complete;  bool deadYet;
				util::CriticalSection mutex;
				common::Profiler profComp;
				VGLTrans *parent;
		};
	};
}

#endif  // __VGLTRANS_H__
