/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include "rrdisplayclient.h"
#include "rrtimer.h"
#include "fakerconfig.h"
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
#include <X11/Xatom.h>

extern FakerConfig fconfig;

void rrdisplayclient::sendheader(rrframeheader h, bool eof=false)
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
					rrout.println("[VGL] Client version: %d.%d", _v.major, _v.minor);
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
	if(_domovie) save((char *)&h, sizeof_rrframeheader);
}


rrdisplayclient::rrdisplayclient(Display *dpy, char *displayname,
	bool domovie) : _np(fconfig.np), _domovie(domovie), _sd(NULL), _t(NULL),
	_deadyet(false), _dpynum(0)
{
	memset(&_v, 0, sizeof(rrversion));
	if(fconfig.verbose)
		rrout.println("[VGL] Using %d / %d CPU's for compression",
			(int)fconfig.np, numprocs());
	char *servername=NULL;
	try
	{
		if(displayname)
		{
			unsigned short port=fconfig.ssl? RR_DEFAULTSSLPORT:RR_DEFAULTPORT;
			char *ptr=NULL;  servername=strdup(displayname);
			if((ptr=strchr(servername, ':'))!=NULL)
			{
				if(strlen(ptr)>1) _dpynum=atoi(ptr+1);
				if(_dpynum<0 || _dpynum>65535) _dpynum=0;
				*ptr='\0';
			}
			if(!strlen(servername) || !strcmp(servername, "unix"))
				{free(servername);  servername=strdup("localhost");}
			if(fconfig.port.isset()) port=fconfig.port;
			else
			{
				Atom atom=None;  unsigned long n=0, bytesleft=0;
				int actualformat=0;  Atom actualtype=None;
				unsigned short *prop=NULL;
				if((atom=XInternAtom(dpy,
					fconfig.ssl? "_VGLCLIENT_SSLPORT":"_VGLCLIENT_PORT", True))!=None)
				{
					if(XGetWindowProperty(dpy, RootWindow(dpy, DefaultScreen(dpy)), atom,
						0, 1, False, XA_INTEGER, &actualtype, &actualformat, &n,
						&bytesleft, (unsigned char **)&prop)==Success && n>=1
						&& actualformat==16 && actualtype==XA_INTEGER && prop)
						port=*prop;
					if(prop) XFree(prop);
				}
			}
			connect(servername, port);
			_dosend=true;
		}
		else _dosend=false;
		_prof_total.setname(_dosend? "Total     ":"Total(mov)");
		errifnot(_t=new Thread(this));
		_t->start();
	}
	catch(...)
	{
		if(servername) free(servername);  throw;
	}
	if(servername) free(servername);
}

void rrdisplayclient::run(void)
{
	rrframe *lastb=NULL, *b=NULL;
	long bytes=0;
	rrtimer t, sleept;  double err=0.;  bool first=true;
	int i;

	try {

	rrcompressor *c[MAXPROCS];  Thread *ct[MAXPROCS];
	for(i=0; i<_np; i++)
		errifnot(c[i]=new rrcompressor(i, this));
	if(_np>1) for(i=1; i<_np; i++)
	{
		errifnot(ct[i]=new Thread(c[i]));
		ct[i]->start();
	}

	while(!_deadyet)
	{
		b=NULL;
		_q.get((void **)&b);  if(_deadyet) break;
		if(!b) _throw("Queue has been shut down");
		_ready.signal();
		if(_np>1)
			for(i=1; i<_np; i++) {
				ct[i]->checkerror();  c[i]->go(b, lastb);
			}
		c[0]->compresssend(b, lastb);
		bytes+=c[0]->_bytes;
		if(_np>1)
			for(i=1; i<_np; i++) {
				c[i]->stop();  ct[i]->checkerror();  c[i]->send();
				bytes+=c[i]->_bytes;
			}

		sendheader(b->_h, true);

		_prof_total.endframe(b->_h.width*b->_h.height, bytes, 1);
		bytes=0;
		_prof_total.startframe();

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

		if(lastb) lastb->complete();
		lastb=b;
	}

	for(i=0; i<_np; i++) c[i]->shutdown();
	if(_np>1) for(i=1; i<_np; i++)
	{
		ct[i]->stop();
		ct[i]->checkerror();
		delete ct[i];
	}
	for(i=0; i<_np; i++) delete c[i];

	} catch(rrerror &e)
	{
		if(_t) _t->seterror(e);
		_ready.signal();
 		throw;
	}
}

rrframe *rrdisplayclient::getbitmap(int w, int h, int ps, int flags,
	bool stereo, bool spoil)
{
	rrframe *b=NULL;
	if(!spoil) _ready.wait();  if(_deadyet) return NULL;
	if(_t) _t->checkerror();
	{
	rrcs::safelock l(_bmpmutex);
	int bmpi=-1;
	for(int i=0; i<NB; i++) if(_bmp[i].iscomplete()) bmpi=i;
	if(bmpi<0) _throw("No free buffers in pool");
	b=&_bmp[bmpi];  b->waituntilcomplete();
	}
	rrframeheader hdr;
	memset(&hdr, 0, sizeof(rrframeheader));
	hdr.height=hdr.frameh=h;
	hdr.width=hdr.framew=w;
	hdr.x=hdr.y=0;
	b->init(hdr, ps, flags, stereo);
	return b;
}

bool rrdisplayclient::frameready(void)
{
	if(_t) _t->checkerror();
	return(_q.items()<=0);
}

static void __rrdisplayclient_spoilfct(void *b)
{
	if(b) ((rrframe *)b)->complete();
}

void rrdisplayclient::sendframe(rrframe *b)
{
	if(_t) _t->checkerror();
	b->_h.dpynum=_dpynum;
	_q.spoil((void *)b, __rrdisplayclient_spoilfct);
}

void rrcompressor::compresssend(rrframe *b, rrframe *lastb)
{
	bool bu=false;  rrcompframe cf;
	if(!b) return;
	if(b->_flags&RRBMP_BOTTOMUP) bu=true;
	int tilesizex=fconfig.tilesize? fconfig.tilesize:b->_h.height;
	int tilesizey=fconfig.tilesize? fconfig.tilesize:b->_h.width;
	int i, j, n=0;

	_bytes=0;
	for(i=0; i<b->_h.height; i+=tilesizey)
	{
		int h=tilesizey, y=i;
		if(b->_h.height-i<(3*tilesizey/2)) {h=b->_h.height-i;  i+=tilesizey;}
		for(j=0; j<b->_h.width; j+=tilesizex, n++)
		{
			int w=tilesizex, x=j;
			if(b->_h.width-j<(3*tilesizex/2)) {w=b->_h.width-j;  j+=tilesizex;}
			if(n%_np!=_myrank) continue;
			if(fconfig.interframe)
			{
				if(b->tileequals(lastb, x, y, w, h)) continue;
			}
			rrframe *rrb=b->gettile(x, y, w, h);
			rrcompframe *c=NULL;
			if(_myrank>0) {errifnot(c=new rrcompframe());}
			else c=&cf;
			_prof_comp.startframe();
			*c=*rrb;
			double frames=(double)(rrb->_h.width*rrb->_h.height)
				/(double)(rrb->_h.framew*rrb->_h.frameh);
			_prof_comp.endframe(rrb->_h.width*rrb->_h.height, 0, frames);
			_bytes+=c->_h.size;  if(c->_stereo) _bytes+=c->_rh.size;
			delete rrb;
			if(_myrank==0)
			{
				_parent->sendheader(c->_h);
				if(_parent->_dosend) _parent->send((char *)c->_bits, c->_h.size);
				if(_parent->_domovie) _parent->save((char *)c->_bits, c->_h.size);
				if(c->_stereo && c->_rbits)
				{
					_parent->sendheader(c->_rh);
					if(_parent->_dosend) _parent->send((char *)c->_rbits, c->_rh.size);
					if(_parent->_domovie) _parent->save((char *)c->_rbits, c->_rh.size);
				}
			}
			else
			{	
				store(c);
			}
		}
	}
}

void rrdisplayclient::send(char *buf, int len)
{
	try
	{
		if(_sd) _sd->send(buf, len);
	}
	catch(...)
	{
		rrout.println("[VGL] Error sending data to client.  Client may have disconnected.");
		throw;
	}
}

void rrdisplayclient::recv(char *buf, int len)
{
	try
	{
		if(_sd) _sd->recv(buf, len);
	}
	catch(...)
	{
		rrout.println("[VGL] Error receiving data from client.  Client may have disconnected.");
		throw;
	}
}

void rrdisplayclient::save(char *buf, int len)
{
	if(fconfig.moviefile)
	{
		int fd=open(fconfig.moviefile, O_CREAT|O_APPEND|O_RDWR, S_IREAD|S_IWRITE);
		if(fd==-1)
		{
			rrout.println("[VGL] Could not open %s (%s.)",
				(char *)fconfig.moviefile, strerror(errno));
			rrout.println("[VGL]    Disabling movie creation.");
			fconfig.moviefile=(char *)NULL;
		}
		else
		{
			if(write(fd, buf, len)==-1)
			{
				rrout.println("[VGL] Could not write to movie file (%s.)\n",
					strerror(errno));
				rrout.println("[VGL]    Disabling movie creation.");
				fconfig.moviefile=(char *)NULL;
			}
			close(fd);
		}
	}
}

void rrdisplayclient::connect(char *servername, unsigned short port)
{
	if(servername)
	{
		errifnot(_sd=new rrsocket(fconfig.ssl));
		try
		{
			_sd->connect(servername, port);
		}
		catch(...)
		{
			rrout.println("[VGL] Could not connect to VGL client.  Make sure the VGL client is running and");
			rrout.println("[VGL]    that either the DISPLAY or VGL_CLIENT environment variable points to");
			rrout.println("[VGL]    the machine on which it is running.");
			throw;
		}
	}
}

void rrcompressor::send(void)
{
	for(int i=0; i<_storedframes; i++)
	{
		rrcompframe *c=_frame[i];
		errifnot(c);
		_parent->sendheader(c->_h);
		if(_parent->_dosend) _parent->send((char *)c->_bits, c->_h.size);
		if(_parent->_domovie) _parent->save((char *)c->_bits, c->_h.size);
		if(c->_stereo && c->_rbits)
		{
			_parent->sendheader(c->_rh);
			if(_parent->_dosend) _parent->send((char *)c->_rbits, c->_rh.size);
			if(_parent->_domovie) _parent->save((char *)c->_rbits, c->_rh.size);
		}
		delete c;		
	}
	_storedframes=0;
}
