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

#include "rrdisplayserver.h"

#define endianize(h) { \
	if(!littleendian()) {  \
		h.size=byteswap(h.size);  \
		h.winid=byteswap(h.winid);  \
		h.framew=byteswap16(h.framew);  \
		h.frameh=byteswap16(h.frameh);  \
		h.width=byteswap16(h.width);  \
		h.height=byteswap16(h.height);  \
		h.x=byteswap16(h.x);  \
		h.y=byteswap16(h.y);}}

rrdisplayserver::rrdisplayserver(unsigned short port, bool dossl, int drawmethod) :
	_drawmethod(drawmethod), _listensd(NULL), _t(NULL), _deadyet(false)
{
	errifnot(_listensd=new rrsocket(dossl));
	_listensd->listen(port==0?RR_DEFAULTPORT:port);
	errifnot(_t=new Thread(this));
	_t->start();
}

rrdisplayserver::~rrdisplayserver(void)
{
	_deadyet=true;
	if(_listensd) _listensd->close();
	if(_t) {_t->stop();  _t=NULL;}
}

void rrdisplayserver::run(void)
{
	rrsocket *sd=NULL;  rrserver *s=NULL;

	while(!_deadyet)
	{
		try
		{
			s=NULL;  sd=NULL;
			sd=_listensd->accept();  if(_deadyet) break;
			rrout.println("++ Connection from %s.", sd->remotename());
			s=new rrserver(sd, _drawmethod);
			continue;
		}
		catch(rrerror &e)
		{
			if(!_deadyet)
			{
				rrout.println("%s:\n%s", e.getMethod(), e.getMessage());
				if(s) delete s;  if(sd) delete sd;
				continue;
			}
		}
	}
	rrout.println("Listener exiting ...");
	if(_listensd) {delete _listensd;  _listensd=NULL;}
}

void rrserver::run(void)
{
	rrcwin *w=NULL;
	rrjpeg *j;
	rrframeheader h;
	bool ctsreq=false;

	try
	{
		while(1)
		{
			do
			{
				_sd->recv((char *)&h, sizeof(rrframeheader));
				endianize(h);
				if(h.flags==(RR_EOF|RR_NOCTS)) {h.flags=RR_EOF;  ctsreq=false;}
				else if(h.flags==RR_EOF) ctsreq=true;
				errifnot(w=addwindow(h.dpynum, h.winid, h.flags==RR_LEFT || h.flags==RR_RIGHT));

				try {j=w->getFrame();}
				catch (...) {if(w) delwindow(w);  throw;}

				j->init(&h);
				if(h.flags!=RR_EOF) {_sd->recv((char *)j->_bits, h.size);}

				try {w->drawFrame(j);}
				catch (...) {if(w) delwindow(w);  throw;}
			} while(!(j && j->_h.flags==RR_EOF));

			if(ctsreq)
			{
				char cts=1;
				_sd->send(&cts, 1);
			}
		}
	}
	catch(rrerror &e)
	{
		rrout.println("Server error in %s\n%s", e.getMethod(), e.getMessage());
	}
	if(_t) {_t->detach();  delete _t;}
	delete this;
}

void rrserver::delwindow(rrcwin *w)
{
	int i, j;
	rrcs::safelock l(_winmutex);
	if(_windows>0)
		for(i=0; i<_windows; i++)
			if(_rrw[i]==w)
			{
				delete w;  _rrw[i]=NULL;
				if(i<_windows-1) for(j=i; j<_windows-1; j++) _rrw[j]=_rrw[j+1];
				_rrw[_windows-1]=NULL;  _windows--;  break;
			}
}

// Register a new window with this server
rrcwin *rrserver::addwindow(int dpynum, Window win, bool stereo)
{
	rrcs::safelock l(_winmutex);
	int winid=_windows;
	if(_windows>0)
	{
		for(winid=0; winid<_windows; winid++)
			if(_rrw[winid] && _rrw[winid]->match(dpynum, win)) return _rrw[winid];
	}
	if(_windows>=MAXWIN) _throw("No free window ID's");
	if(dpynum<0 || dpynum>255 || win==None) _throw("Invalid argument");
	_rrw[winid]=new rrcwin(dpynum, win, _drawmethod, stereo);

	if(!_rrw[winid]) _throw("Could not create window instance");
	_windows++;
	return _rrw[winid];
}
