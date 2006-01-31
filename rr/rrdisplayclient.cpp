/* Copyright (C)2004 Landmark Graphics
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

extern FakerConfig fconfig;

static rrversion _v={{0, 0, 0}, 0, 0};

static void sendheader(rrsocket *_sd, rrframeheader h, bool eof=false)
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
			_sd->send((char *)&h1, sizeof(rrframeheader_v1));
			_sd->recv(&reply, 1);
			if(reply==1) {_v.major=1;  _v.minor=0;}
			else if(reply=='V')
			{
				rrversion v;
				_v.id[0]=reply;
				_sd->recv((char *)&_v.id[1], sizeof(rrversion)-1);
				if(strncmp(_v.id, "VGL", 3) || _v.major<1)
					_throw("Error reading client version");
				v=_v;
				v.major=RR_MAJOR_VERSION;  v.minor=RR_MINOR_VERSION;
				_sd->send((char *)&v, sizeof(rrversion));
			}
			rrout.println("Client version: %d.%d", _v.major, _v.minor);
		}
	}
	if(eof) h.flags=RR_EOF;  else h.flags=0;
	if(_v.major==1 && _v.minor==0)
	{
		rrframeheader_v1 h1;
		cvthdr_v1(h, h1);
		endianize_v1(h1);
		if(_sd)
		{
			_sd->send((char *)&h1, sizeof(rrframeheader_v1));
			if(eof)
			{
				char cts=0;
				_sd->recv(&cts, 1);
				if(cts<1 || cts>2) _throw("CTS Error");
			}
		}
	}
	else
	{
		endianize(h);
		if(_sd) _sd->send((char *)&h, sizeof(rrframeheader));
	}
}


rrdisplayclient::rrdisplayclient(char *displayname) : _sd(NULL), _bmpi(0),
	_t(NULL), _deadyet(false), _np(fconfig.np), _dpynum(0), _stereo(true)
{
	rrout.println("[VGL] Using %d / %d CPU's for compression", fconfig.np, numprocs());
	char *servername=NULL;
	try
	{
		if(displayname)
		{
			char *ptr=NULL;  servername=strdup(displayname);
			if((ptr=strchr(servername, ':'))!=NULL)
			{
				if(strlen(ptr)>1) _dpynum=atoi(ptr+1);
				if(_dpynum<0 || _dpynum>255) _dpynum=0;
				*ptr='\0';
			}
			if(!strlen(servername)) {free(servername);  servername=strdup("localhost");}
		}
		if(servername)
		{
			errifnot(_sd=new rrsocket(fconfig.ssl));
			_sd->connect(servername, fconfig.port);
		}
		_prof_total.setname("Total   ");
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
	rrprofiler prof_comp("Compress");  long bytes=0;

	int i;

	try {

	rrcompressor *c[MAXPROCS];  Thread *ct[MAXPROCS];
	for(i=0; i<_np; i++)
		errifnot(c[i]=new rrcompressor(i, _np, _sd));
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
		if(b->_h.flags==RR_RIGHT && !_stereo) continue;
		prof_comp.startframe();
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
		prof_comp.endframe(b->_h.framew*b->_h.frameh, 0,
			b->_h.flags==RR_LEFT || b->_h.flags==RR_RIGHT? 0.5 : 1);

		if(b->_h.flags!=RR_LEFT) sendheader(_sd, b->_h, true);

		_prof_total.endframe(b->_h.width*b->_h.height, bytes,
			b->_h.flags==RR_LEFT || b->_h.flags==RR_RIGHT? 0.5 : 1);
		bytes=0;
		_prof_total.startframe();

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

rrframe *rrdisplayclient::getbitmap(int w, int h, int ps)
{
	rrframe *b=NULL;
	_ready.wait();  if(_deadyet) return NULL;
	if(_t) _t->checkerror();
	_bmpmutex.lock();
	b=&_bmp[_bmpi];  _bmpi=(_bmpi+1)%NB;
	_bmpmutex.unlock();
	rrframeheader hdr;
	memset(&hdr, 0, sizeof(rrframeheader));
	hdr.height=hdr.frameh=h;
	hdr.width=hdr.framew=w;
	hdr.x=hdr.y=0;
	b->init(&hdr, ps);
	return b;
}

bool rrdisplayclient::frameready(void)
{
	if(_t) _t->checkerror();
	return(_q.items()<=0);
}

void rrdisplayclient::sendframe(rrframe *b)
{
	if(_t) _t->checkerror();
	b->_h.dpynum=_dpynum;
	_q.add((void *)b);
}

void rrdisplayclient::sendcompressedframe(rrframeheader &h, unsigned char *bits)
{
	h.dpynum=_dpynum;
	sendheader(_sd, h);
	if(_sd) _sd->send((char *)bits, h.size);
	sendheader(_sd, h, true);
}

void rrcompressor::compresssend(rrframe *b, rrframe *lastb)
{
	bool bu=false;  rrjpeg jpg;
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
			if(n%_np!=_myrank) continue;
			if(b->_h.width-j<(3*tilesizex/2)) {w=b->_h.width-j;  j+=tilesizex;}
			if(b->tileequals(lastb, x, y, w, h)) continue;
			rrframe *rrb=b->gettile(x, y, w, h);
			rrjpeg *jptr=NULL;
			if(_myrank>0) {errifnot(jptr=new rrjpeg());}
			else jptr=&jpg;
			*jptr=*rrb;
			_bytes+=jptr->_h.size;
			delete rrb;
			if(_myrank==0)
			{
				sendheader(_sd, jptr->_h);
				if(_sd) _sd->send((char *)jptr->_bits, jptr->_h.size);
			}
			else
			{	
				store(jptr);
			}
		}
	}
}

void rrcompressor::send(void)
{
	for(int i=0; i<_storedframes; i++)
	{
		rrjpeg *j=_frame[i];
		errifnot(j);
		sendheader(_sd, j->_h);
		if(_sd) _sd->send((char *)j->_bits, j->_h.size);
		delete j;		
	}
	_storedframes=0;
}
