// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009-2011, 2014, 2017-2021 D. R. Commander
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

#include "VGLTrans.h"
#include "Timer.h"
#include "fakerconfig.h"
#include "vglutil.h"
#include "Log.h"
#include <fcntl.h>
#include <sys/stat.h>

using namespace util;
using namespace common;
using namespace server;


#define ENDIANIZE(h) \
{ \
	if(!LittleEndian()) \
	{ \
		h.size = BYTESWAP(h.size); \
		h.winid = BYTESWAP(h.winid); \
		h.framew = BYTESWAP16(h.framew); \
		h.frameh = BYTESWAP16(h.frameh); \
		h.width = BYTESWAP16(h.width); \
		h.height = BYTESWAP16(h.height); \
		h.x = BYTESWAP16(h.x); \
		h.y = BYTESWAP16(h.y); \
		h.dpynum = BYTESWAP16(h.dpynum); \
	} \
}

#define ENDIANIZE_V1(h) \
{ \
	if(!LittleEndian()) \
	{ \
		h.size = BYTESWAP(h.size); \
		h.winid = BYTESWAP(h.winid); \
		h.framew = BYTESWAP16(h.framew); \
		h.frameh = BYTESWAP16(h.frameh); \
		h.width = BYTESWAP16(h.width); \
		h.height = BYTESWAP16(h.height); \
		h.x = BYTESWAP16(h.x); \
		h.y = BYTESWAP16(h.y); \
	} \
}

#define CONVERT_HEADER(h, h1) \
{ \
	h1.size = h.size; \
	h1.winid = h.winid; \
	h1.framew = h.framew; \
	h1.frameh = h.frameh; \
	h1.width = h.width; \
	h1.height = h.height; \
	h1.x = h.x; \
	h1.y = h.y; \
	h1.qual = h.qual; \
	h1.subsamp = h.subsamp; \
	h1.flags = h.flags; \
	h1.dpynum = (unsigned char)h.dpynum; \
}


void VGLTrans::sendHeader(rrframeheader h, bool eof)
{
	if(version.major == 0 && version.minor == 0)
	{
		// Fake up an old (protocol v1.0) EOF packet and see if the client sends
		// back a CTS signal.  If so, it needs protocol 1.0
		rrframeheader_v1 h1;  char reply = 0;
		CONVERT_HEADER(h, h1);
		h1.flags = RR_EOF;
		ENDIANIZE_V1(h1);
		if(socket)
		{
			send((char *)&h1, sizeof_rrframeheader_v1);
			recv(&reply, 1);
			if(reply == 1)
			{
				version.major = 1;  version.minor = 0;
			}
			else if(reply == 'V')
			{
				rrversion v;
				version.id[0] = reply;
				recv(&version.id[1], sizeof_rrversion - 1);
				if(strncmp(version.id, "VGL", 3) || version.major < 1)
					THROW("Error reading client version");
				v = version;
				v.major = RR_MAJOR_VERSION;  v.minor = RR_MINOR_VERSION;
				send((char *)&v, sizeof_rrversion);
			}
			if(fconfig.verbose)
				vglout.println("[VGL] Client version: %d.%d", version.major,
					version.minor);
		}
	}
	if((version.major < 2 || (version.major == 2 && version.minor < 1))
		&& h.compress != RRCOMP_JPEG)
		THROW("This compression mode requires VirtualGL Client v2.1 or later");
	if(eof) h.flags = RR_EOF;
	if(version.major == 1 && version.minor == 0)
	{
		rrframeheader_v1 h1;
		if(h.dpynum > 255) THROW("Display number out of range for v1.0 client");
		CONVERT_HEADER(h, h1);
		ENDIANIZE_V1(h1);
		if(socket)
		{
			send((char *)&h1, sizeof_rrframeheader_v1);
			if(eof)
			{
				char cts = 0;
				recv(&cts, 1);
				if(cts < 1 || cts > 2) THROW("CTS Error");
			}
		}
	}
	else
	{
		ENDIANIZE(h);
		send((char *)&h, sizeof_rrframeheader);
	}
}


VGLTrans::VGLTrans(void) : nprocs(fconfig.np), socket(NULL), thread(NULL),
	deadYet(false), dpynum(0)
{
	memset(&version, 0, sizeof(rrversion));
	profTotal.setName("Total     ");
	#ifdef USEHELGRIND
	ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
	// NOTE: Without this line, helgrind reports a data race on the class
	// instance address in the destructor (false positive?)
	ANNOTATE_BENIGN_RACE_SIZED(this, sizeof(VGLTrans *), );
	#endif
}


void VGLTrans::run(void)
{
	Frame *lastf = NULL, *f = NULL;
	long bytes = 0;
	Timer timer, sleepTimer;  double err = 0.;  bool first = true;
	int i;

	try
	{
		VGLTrans::Compressor *comp[MAXPROCS];  Thread *cthread[MAXPROCS];
		if(fconfig.verbose)
			vglout.println("[VGL] Using %d compression threads on %d CPU cores",
				nprocs, NumProcs());
		for(i = 0; i < nprocs; i++)
			comp[i] = new VGLTrans::Compressor(i, this);
		if(nprocs > 1) for(i = 1; i < nprocs; i++)
		{
			cthread[i] = new Thread(comp[i]);
			cthread[i]->start();
		}

		while(!deadYet)
		{
			int np;
			void *ftemp = NULL;

			q.get(&ftemp);  f = (Frame *)ftemp;  if(deadYet) break;
			if(!f) THROW("Queue has been shut down");
			ready.signal();
			np = nprocs;  if(f->hdr.compress == RRCOMP_YUV) np = 1;
			if(np > 1)
			{
				for(i = 1; i < np; i++)
				{
					cthread[i]->checkError();  comp[i]->go(f, lastf);
				}
			}
			comp[0]->compressSend(f, lastf);
			bytes += comp[0]->bytes;
			if(np > 1)
			{
				for(i = 1; i < np; i++)
				{
					comp[i]->stop();  cthread[i]->checkError();  comp[i]->send();
					bytes += comp[i]->bytes;
				}
			}
			sendHeader(f->hdr, true);

			profTotal.endFrame(f->hdr.width * f->hdr.height, bytes, 1);
			bytes = 0;
			profTotal.startFrame();

			if(fconfig.flushdelay > 0.)
			{
				long usec = (long)(fconfig.flushdelay * 1000000.);
				if(usec > 0) usleep(usec);
			}
			if(fconfig.fps > 0.)
			{
				double elapsed = timer.elapsed();
				if(first) first = false;
				else
				{
					if(elapsed < 1. / fconfig.fps)
					{
						sleepTimer.start();
						long usec = (long)((1. / fconfig.fps - elapsed - err) * 1000000.);
						if(usec > 0) usleep(usec);
						double sleepTime = sleepTimer.elapsed();
						err = sleepTime - (1. / fconfig.fps - elapsed - err);
						if(err < 0.) err = 0.;
					}
				}
				timer.start();
			}

			if(lastf) lastf->signalComplete();
			lastf = f;
		}

		for(i = 0; i < nprocs; i++) comp[i]->shutdown();
		if(nprocs > 1) for(i = 1; i < nprocs; i++)
		{
			cthread[i]->stop();
			cthread[i]->checkError();
			delete cthread[i];
		}
		for(i = 0; i < nprocs; i++) delete comp[i];

	}
	catch(std::exception &e)
	{
		if(thread) thread->setError(e);
		ready.signal();
		throw;
	}
}


Frame *VGLTrans::getFrame(int width, int height, int pixelFormat, int flags,
	bool stereo)
{
	Frame *f = NULL;

	if(deadYet) return NULL;
	if(thread) thread->checkError();
	{
		CriticalSection::SafeLock l(mutex);

		int index = -1;
		for(int i = 0; i < NFRAMES; i++)
			if(frames[i].isComplete()) index = i;
		if(index < 0) THROW("No free buffers in pool");
		f = &frames[index];  f->waitUntilComplete();
	}

	rrframeheader hdr;
	memset(&hdr, 0, sizeof(rrframeheader));
	hdr.x = hdr.y = 0;
	hdr.width = hdr.framew = width;
	hdr.height = hdr.frameh = height;
	f->init(hdr, pixelFormat, flags, stereo);
	return f;
}


bool VGLTrans::isReady(void)
{
	if(thread) thread->checkError();
	return q.items() <= 0;
}


void VGLTrans::synchronize(void)
{
	ready.wait();
}


static void _VGLTrans_spoilfct(void *f)
{
	if(f) ((Frame *)f)->signalComplete();
}


void VGLTrans::sendFrame(Frame *f)
{
	if(thread) thread->checkError();
	f->hdr.dpynum = dpynum;
	q.spoil((void *)f, _VGLTrans_spoilfct);
}


void VGLTrans::Compressor::compressSend(Frame *f, Frame *lastf)
{
	CompressedFrame cframe;

	if(!f) return;
	int tilesizex = fconfig.tilesize ? fconfig.tilesize : f->hdr.width;
	int tilesizey = fconfig.tilesize ? fconfig.tilesize : f->hdr.height;
	int i, j, n = 0;

	if(f->hdr.compress == RRCOMP_YUV)
	{
		profComp.startFrame();
		cframe = *f;
		profComp.endFrame(f->hdr.framew * f->hdr.frameh, 0, 1);
		parent->sendHeader(cframe.hdr);
		parent->send((char *)cframe.bits, cframe.hdr.size);
		return;
	}

	bytes = 0;
	for(i = 0; i < f->hdr.height; i += tilesizey)
	{
		int height = tilesizey, y = i;

		if(f->hdr.height - i < (3 * tilesizey / 2))
		{
			height = f->hdr.height - i;  i += tilesizey;
		}
		for(j = 0; j < f->hdr.width; j += tilesizex, n++)
		{
			int width = tilesizex, x = j;

			if(f->hdr.width - j < (3 * tilesizex / 2))
			{
				width = f->hdr.width - j;  j += tilesizex;
			}
			if(n % nprocs != myRank) continue;
			if(fconfig.interframe)
			{
				if(f->tileEquals(lastf, x, y, width, height)) continue;
			}
			Frame *tile = f->getTile(x, y, width, height);
			CompressedFrame *ctile = NULL;
			if(myRank > 0) { ctile = new CompressedFrame(); }
			else ctile = &cframe;
			profComp.startFrame();
			*ctile = *tile;
			double frames = (double)(tile->hdr.width * tile->hdr.height) /
				(double)(tile->hdr.framew * tile->hdr.frameh);
			profComp.endFrame(tile->hdr.width * tile->hdr.height, 0, frames);
			bytes += ctile->hdr.size;
			if(ctile->stereo) bytes += ctile->rhdr.size;
			delete tile;
			if(myRank == 0)
			{
				parent->sendHeader(ctile->hdr);
				parent->send((char *)ctile->bits, ctile->hdr.size);
				if(ctile->stereo && ctile->rbits)
				{
					parent->sendHeader(ctile->rhdr);
					parent->send((char *)ctile->rbits, ctile->rhdr.size);
				}
			}
			else
			{
				store(ctile);
			}
		}
	}
}


void VGLTrans::send(char *buf, int len)
{
	try
	{
		if(socket) socket->send(buf, len);
	}
	catch(...)
	{
		vglout.println("[VGL] ERROR: Could not send data to client.  Client may have disconnected.");
		throw;
	}
}


void VGLTrans::recv(char *buf, int len)
{
	try
	{
		if(socket) socket->recv(buf, len);
	}
	catch(...)
	{
		vglout.println("[VGL] ERROR: Could not receive data from client.  Client may have disconnected.");
		throw;
	}
}


void VGLTrans::connect(char *displayName, unsigned short port)
{
	char *serverName = NULL;

	try
	{
		if(!displayName || strlen(displayName) <= 0)
			THROW("Invalid receiver name");
		char *ptr = NULL;  serverName = strdup(displayName);
		if((ptr = strrchr(serverName, ':')) != NULL)
		{
			// For backward compatibility with v2.0.x and earlier of the VirtualGL
			// Client, we assume that anything after the final colon in the display
			// string should be interpreted as a display number, if there are no
			// other colons in the string or if the IP address is surrounded by
			// square brackets.  Otherwise, we assume that the colon is part of an
			// IPv6 address.
			if(strlen(ptr) > 1)
			{
				*ptr = '\0';
				if(!strchr(serverName, ':') || (serverName[0] == '['
					&& serverName[strlen(serverName) - 1] == ']'))
				{
					dpynum = atoi(ptr + 1);
					if(dpynum < 0 || dpynum > 65535) dpynum = 0;
				}
				else
				{
					free(serverName);  serverName = strdup(displayName);
				}
			}
		}
		if(serverName[0] == '[' && serverName[strlen(serverName) - 1] == ']'
			&& strlen(serverName) > 2)
		{
			serverName[strlen(serverName) - 1] = '\0';
			char *tmp = strdup(&serverName[1]);
			free(serverName);  serverName = tmp;
		}
		if(!strlen(serverName) || !strcmp(serverName, "unix"))
		{
			free(serverName);  serverName = strdup("localhost");
		}
		socket = new Socket(true);
		try
		{
			socket->connect(serverName, port);
		}
		catch(...)
		{
			vglout.println("[VGL] ERROR: Could not connect to VGL client.  Make sure that vglclient is");
			vglout.println("[VGL]    running and that either the DISPLAY or VGL_CLIENT environment");
			vglout.println("[VGL]    variable points to the machine on which vglclient is running.");
			throw;
		}
		thread = new Thread(this);
		thread->start();
	}
	catch(...)
	{
		free(serverName);
		throw;
	}
	free(serverName);
}


void VGLTrans::Compressor::send(void)
{
	for(int i = 0; i < storedFrames; i++)
	{
		CompressedFrame *cf = cframes[i];
		ERRIFNOT(cf);
		parent->sendHeader(cf->hdr);
		parent->send((char *)cf->bits, cf->hdr.size);
		if(cf->stereo && cf->rbits)
		{
			parent->sendHeader(cf->rhdr);
			parent->send((char *)cf->rbits, cf->rhdr.size);
		}
		delete cf;
	}
	storedFrames = 0;
}
