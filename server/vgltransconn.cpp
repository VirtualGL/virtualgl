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

#include "vgltransconn.h"
#include "Timer.h"
#include "fakerconfig.h"
#include "vglutil.h"
#include "Log.h"
#ifdef _WIN32
#include <io.h>
#define _POSIX_
#endif
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#define open _open
#define write _write
#define close _close
#define S_IREAD 0400
#define S_IWRITE 0200
#endif


void vgltransconn::sendheader(rrframeheader h, bool eof=false)
{
	if(_dosend)
	{
		if(_v.major==0 && _v.minor==0)
		{
			// Fake up an old (protocol v1.0) EOF packet and see if the client sends
			// back a CTS signal.  If so, it needs protocol 1.0
			rrframeheader_v1 h1;  char reply=0;
			cvthdr_v1(h, h1);
			h1.flags=RR_EOF;
			endianize_v1(h1);
			if(_sd)
			{
				send((char *)&h1, sizeof_rrframeheader_v1);
				recv(&reply, 1);
				if(reply==1) {_v.major=1;  _v.minor=0;}
				else if(reply=='V')
				{
					rrversion v;
					_v.id[0]=reply;
					recv((char *)&_v.id[1], sizeof_rrversion-1);
					if(strncmp(_v.id, "VGL", 3) || _v.major<1)
						_throw("Error reading client version");
					v=_v;
					v.major=RR_MAJOR_VERSION;  v.minor=RR_MINOR_VERSION;
					send((char *)&v, sizeof_rrversion);
				}
				if(fconfig.verbose)
					vglout.println("[VGL] Client version: %d.%d", _v.major, _v.minor);
			}
		}
		if((_v.major<2 || (_v.major==2 && _v.minor<1)) && h.compress!=RRCOMP_JPEG)
			_throw("This compression mode requires VirtualGL Client v2.1 or later");
	}
	if(eof) h.flags=RR_EOF;
	if(_dosend)
	{
		if(_v.major==1 && _v.minor==0)
		{
			rrframeheader_v1 h1;
			if(h.dpynum>255) _throw("Display number out of range for v1.0 client");
			cvthdr_v1(h, h1);
			endianize_v1(h1);
			if(_sd)
			{
				send((char *)&h1, sizeof_rrframeheader_v1);
				if(eof)
				{
					char cts=0;
					recv(&cts, 1);
					if(cts<1 || cts>2) _throw("CTS Error");
				}
			}
		}
		else
		{
			endianize(h);
			send((char *)&h, sizeof_rrframeheader);
		}
	}
}


vgltransconn::vgltransconn(void) : _np(fconfig.np),
	_dosend(false), _sd(NULL), _t(NULL), _deadyet(false), _dpynum(0)
{
	memset(&_v, 0, sizeof(rrversion));
	_prof_total.setName("Total(mov)");
}


void vgltransconn::run(void)
{
	Frame *lastf=NULL, *f=NULL;
	long bytes=0;
	Timer t, sleept;  double err=0.;  bool first=true;
	int i;

	try
	{
		vgltranscompressor *c[MAXPROCS];  Thread *ct[MAXPROCS];
		if(fconfig.verbose)
			vglout.println("[VGL] Using %d / %d CPU's for compression",
				_np, numprocs());
		for(i=0; i<_np; i++)
			errifnot(c[i]=new vgltranscompressor(i, this));
		if(_np>1) for(i=1; i<_np; i++)
		{
			errifnot(ct[i]=new Thread(c[i]));
			ct[i]->start();
		}

		while(!_deadyet)
		{
			int np;
			void *ftemp=NULL;
			_q.get(&ftemp);  f=(Frame *)ftemp;  if(_deadyet) break;
			if(!f) _throw("Queue has been shut down");
			_ready.signal();
			np=_np;  if(f->hdr.compress==RRCOMP_YUV) np=1;
			if(np>1)
				for(i=1; i<np; i++) {
					ct[i]->checkError();  c[i]->go(f, lastf);
				}
			c[0]->compresssend(f, lastf);
			bytes+=c[0]->_bytes;
			if(np>1)
				for(i=1; i<np; i++) {
					c[i]->stop();  ct[i]->checkError();  c[i]->send();
					bytes+=c[i]->_bytes;
				}

			sendheader(f->hdr, true);

			_prof_total.endFrame(f->hdr.width*f->hdr.height, bytes, 1);
			bytes=0;
			_prof_total.startFrame();

			if(fconfig.flushdelay>0.)
			{
				long usec=(long)(fconfig.flushdelay*1000000.);
				if(usec>0) usleep(usec);
			}
			if(fconfig.fps>0.)
			{
				double elapsed=t.elapsed();
				if(first) first=false;
				else
				{
					if(elapsed<1./fconfig.fps)
					{
						sleept.start();
						long usec=(long)((1./fconfig.fps-elapsed-err)*1000000.);
						if(usec>0) usleep(usec);
						double sleeptime=sleept.elapsed();
						err=sleeptime-(1./fconfig.fps-elapsed-err);  if(err<0.) err=0.;
					}
				}
				t.start();
			}

			if(lastf) lastf->signalComplete();
			lastf=f;
		}

		for(i=0; i<_np; i++) c[i]->shutdown();
		if(_np>1) for(i=1; i<_np; i++)
		{
			ct[i]->stop();
			ct[i]->checkError();
			delete ct[i];
		}
		for(i=0; i<_np; i++) delete c[i];

	}
	catch(Error &e)
	{
		if(_t) _t->setError(e);
		_ready.signal();
 		throw;
	}
}


Frame *vgltransconn::getframe(int w, int h, int ps, int flags,
	bool stereo)
{
	Frame *f=NULL;
	if(_deadyet) return NULL;
	if(_t) _t->checkError();
	{
	CS::SafeLock l(_mutex);
	int framei=-1;
	for(int i=0; i<NFRAMES; i++) if(_frame[i].isComplete()) framei=i;
	if(framei<0) _throw("No free buffers in pool");
	f=&_frame[framei];  f->waitUntilComplete();
	}
	rrframeheader hdr;
	memset(&hdr, 0, sizeof(rrframeheader));
	hdr.height=hdr.frameh=h;
	hdr.width=hdr.framew=w;
	hdr.x=hdr.y=0;
	f->init(hdr, ps, flags, stereo);
	return f;
}


bool vgltransconn::ready(void)
{
	if(_t) _t->checkError();
	return(_q.items()<=0);
}


void vgltransconn::synchronize(void)
{
	_ready.wait();
}


static void __vgltransconn_spoilfct(void *f)
{
	if(f) ((Frame *)f)->signalComplete();
}


void vgltransconn::sendframe(Frame *f)
{
	if(_t) _t->checkError();
	f->hdr.dpynum=_dpynum;
	_q.spoil((void *)f, __vgltransconn_spoilfct);
}


void vgltranscompressor::compresssend(Frame *f, Frame *lastf)
{
	bool bu=false;  CompressedFrame cf;
	if(!f) return;
	if(f->flags&FRAME_BOTTOMUP) bu=true;
	int tilesizex=fconfig.tilesize? fconfig.tilesize:f->hdr.width;
	int tilesizey=fconfig.tilesize? fconfig.tilesize:f->hdr.height;
	int i, j, n=0;

	if(f->hdr.compress==RRCOMP_YUV)
	{
		_prof_comp.startFrame();
		cf=*f;
		_prof_comp.endFrame(f->hdr.framew*f->hdr.frameh, 0, 1);
		_parent->sendheader(cf.hdr);
		if(_parent->_dosend) _parent->send((char *)cf.bits, cf.hdr.size);
		return;
	}

	_bytes=0;
	for(i=0; i<f->hdr.height; i+=tilesizey)
	{
		int h=tilesizey, y=i;
		if(f->hdr.height-i<(3*tilesizey/2)) {h=f->hdr.height-i;  i+=tilesizey;}
		for(j=0; j<f->hdr.width; j+=tilesizex, n++)
		{
			int w=tilesizex, x=j;
			if(f->hdr.width-j<(3*tilesizex/2)) {w=f->hdr.width-j;  j+=tilesizex;}
			if(n%_np!=_myrank) continue;
			if(fconfig.interframe)
			{
				if(f->tileEquals(lastf, x, y, w, h)) continue;
			}
			Frame *tile=f->getTile(x, y, w, h);
			CompressedFrame *c=NULL;
			if(_myrank>0) {errifnot(c=new CompressedFrame());}
			else c=&cf;
			_prof_comp.startFrame();
			*c=*tile;
			double frames=(double)(tile->hdr.width*tile->hdr.height)
				/(double)(tile->hdr.framew*tile->hdr.frameh);
			_prof_comp.endFrame(tile->hdr.width*tile->hdr.height, 0, frames);
			_bytes+=c->hdr.size;  if(c->stereo) _bytes+=c->rhdr.size;
			delete tile;
			if(_myrank==0)
			{
				_parent->sendheader(c->hdr);
				if(_parent->_dosend) _parent->send((char *)c->bits, c->hdr.size);
				if(c->stereo && c->rbits)
				{
					_parent->sendheader(c->rhdr);
					if(_parent->_dosend) _parent->send((char *)c->rbits, c->rhdr.size);
				}
			}
			else
			{	
				store(c);
			}
		}
	}
}


void vgltransconn::send(char *buf, int len)
{
	try
	{
		if(_sd) _sd->send(buf, len);
	}
	catch(...)
	{
		vglout.println("[VGL] ERROR: Could not send data to client.  Client may have disconnected.");
		throw;
	}
}


void vgltransconn::recv(char *buf, int len)
{
	try
	{
		if(_sd) _sd->recv(buf, len);
	}
	catch(...)
	{
		vglout.println("[VGL] ERROR: Could not receive data from client.  Client may have disconnected.");
		throw;
	}
}


void vgltransconn::connect(char *displayname, unsigned short port)
{
	char *servername=NULL;
	try
	{
		if(!displayname || strlen(displayname)<=0)
			_throw("Invalid receiver name");
		char *ptr=NULL;  servername=strdup(displayname);
		if((ptr=strchr(servername, ':'))!=NULL)
		{
			if(strlen(ptr)>1) _dpynum=atoi(ptr+1);
			if(_dpynum<0 || _dpynum>65535) _dpynum=0;
			*ptr='\0';
		}
		if(!strlen(servername) || !strcmp(servername, "unix"))
			{free(servername);  servername=strdup("localhost");}
		errifnot(_sd=new Socket((bool)fconfig.ssl));
		try
		{
			_sd->connect(servername, port);
		}
		catch(...)
		{
			vglout.println("[VGL] ERROR: Could not connect to VGL client.  Make sure that vglclient is");
			vglout.println("[VGL]    running and that either the DISPLAY or VGL_CLIENT environment");
			vglout.println("[VGL]    variable points to the machine on which vglclient is running.");
			throw;
		}
		_dosend=true;
		_prof_total.setName("Total     ");
		errifnot(_t=new Thread(this));
		_t->start();
	}
	catch(...)
	{
		if(servername) free(servername);  throw;
	}
	if(servername) free(servername);
}


void vgltranscompressor::send(void)
{
	for(int i=0; i<_storedframes; i++)
	{
		CompressedFrame *c=_frame[i];
		errifnot(c);
		_parent->sendheader(c->hdr);
		if(_parent->_dosend) _parent->send((char *)c->bits, c->hdr.size);
		if(c->stereo && c->rbits)
		{
			_parent->sendheader(c->rhdr);
			if(_parent->_dosend) _parent->send((char *)c->rbits, c->rhdr.size);
		}
		delete c;		
	}
	_storedframes=0;
}
