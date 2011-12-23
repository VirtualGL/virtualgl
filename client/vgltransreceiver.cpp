/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009-2011 D. R. Commander
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

#include "vgltransreceiver.h"
#include "rrutil.h"


extern Display *maindpy;


static unsigned short DisplayNumber(Display *dpy)
{
	int dpynum=0;  char *ptr=NULL;
	if((ptr=strchr(DisplayString(dpy), ':'))!=NULL)
	{
		if(strlen(ptr)>1) dpynum=atoi(ptr+1);
		if(dpynum<0 || dpynum>65535) dpynum=0;
	}
	return (unsigned short)dpynum;
}


#define endianize(h) { \
	if(!littleendian()) {  \
		h.size=byteswap(h.size);  \
		h.winid=byteswap(h.winid);  \
		h.framew=byteswap16(h.framew);  \
		h.frameh=byteswap16(h.frameh);  \
		h.width=byteswap16(h.width);  \
		h.height=byteswap16(h.height);  \
		h.x=byteswap16(h.x);  \
		h.y=byteswap16(h.y);  \
		h.dpynum=byteswap16(h.dpynum);}}

#define endianize_v1(h) { \
	if(!littleendian()) {  \
		h.size=byteswap(h.size);  \
		h.winid=byteswap(h.winid);  \
		h.framew=byteswap16(h.framew);  \
		h.frameh=byteswap16(h.frameh);  \
		h.width=byteswap16(h.width);  \
		h.height=byteswap16(h.height);  \
		h.x=byteswap16(h.x);  \
		h.y=byteswap16(h.y);}}

#define cvthdr_v1(h1, h) {  \
	h.size=h1.size;  \
	h.winid=h1.winid;  \
	h.framew=h1.framew;  \
	h.frameh=h1.frameh;  \
	h.width=h1.width;  \
	h.height=h1.height;  \
	h.x=h1.x;  \
	h.y=h1.y;  \
	h.qual=h1.qual;  \
	h.subsamp=h1.subsamp;  \
	h.flags=h1.flags;  \
	h.dpynum=(unsigned short)h1.dpynum;}


vgltransreceiver::vgltransreceiver(bool dossl, int drawmethod) :
	_drawmethod(drawmethod), _listensd(NULL), _t(NULL), _deadyet(false),
	_dossl(dossl)
{
	char *env=NULL;
	if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
		&& !strncmp(env, "1", 1)) fbx_printwarnings(rrout.getfile());
	errifnot(_t=new Thread(this));
}


vgltransreceiver::~vgltransreceiver(void)
{
	_deadyet=true;
	_listensdmutex.lock();
	if(_listensd) _listensd->close();
	_listensdmutex.unlock();
	if(_t) {_t->stop();  _t=NULL;}
}


void vgltransreceiver::listen(unsigned short port)
{
	try
	{
		errifnot(_listensd=new rrsocket(_dossl));
		_port=_listensd->listen(port);
	}
	catch(...)
	{
		if(_listensd) {delete _listensd;  _listensd=NULL;}
		throw;
	}
	_t->start();
}


void vgltransreceiver::run(void)
{
	rrsocket *sd=NULL;  vgltransserver *s=NULL;

	while(!_deadyet)
	{
		try
		{
			s=NULL;  sd=NULL;
			sd=_listensd->accept();  if(_deadyet) break;
			rrout.println("++ %sConnection from %s.", _dossl?"SSL ":"",
				sd->remotename());
			s=new vgltransserver(sd, _drawmethod);
			continue;
		}
		catch(rrerror &e)
		{
			if(!_deadyet)
			{
				rrout.println("%s-- %s", e.getMethod(), e.getMessage());
				if(s) delete s;  if(sd) delete sd;
				continue;
			}
		}
	}
	rrout.println("Listener exiting ...");
	_listensdmutex.lock();
	if(_listensd) {delete _listensd;  _listensd=NULL;}
	_listensdmutex.unlock();
}


void vgltransserver::run(void)
{
	clientwin *w=NULL;
	rrframe *f=NULL;
	rrframeheader h;  rrframeheader_v1 h1;  bool haveheader=false;
	rrversion v;

	try
	{
		recv((char *)&h1, sizeof_rrframeheader_v1);
		endianize(h1);
		if(h1.framew!=0 && h1.frameh!=0 && h1.width!=0 && h1.height!=0
			&& h1.winid!=0 && h1.size!=0 && h1.flags!=RR_EOF)
			{v.major=1;  v.minor=0;  haveheader=true;}
		else
		{
			strncpy(v.id, "VGL", 3);
			v.major=RR_MAJOR_VERSION;  v.minor=RR_MINOR_VERSION;
			send((char *)&v, sizeof_rrversion);
			recv((char *)&v, sizeof_rrversion);
			if(strncmp(v.id, "VGL", 3) || v.major<1)
				_throw("Error reading server version");
		}

		char *env=NULL;
		if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
			&& !strncmp(env, "1", 1))
			rrout.println("Server version: %d.%d", v.major, v.minor);
		rrout.flush();

		while(1)
		{
			do
			{
				if(v.major==1 && v.minor==0)
				{
					if(!haveheader)
					{
						recv((char *)&h1, sizeof_rrframeheader_v1);
						endianize_v1(h1);
					}
					haveheader=false;
					cvthdr_v1(h1, h);
				}
				else
				{
					recv((char *)&h, sizeof_rrframeheader);
					endianize(h);
				}
				bool stereo=(h.flags==RR_LEFT || h.flags==RR_RIGHT);
				unsigned short dpynum=(v.major<2 || (v.major==2 && v.minor<1))?
					h.dpynum : DisplayNumber(maindpy);
				errifnot(w=addwindow(dpynum, h.winid, stereo));

				if(!stereo || h.flags==RR_LEFT || !f)
				{
					try {f=w->getframe(h.compress==RRCOMP_YUV);}
					catch (...) {if(w) delwindow(w);  throw;}
				}
				#ifdef USEXV
				if(h.compress==RRCOMP_YUV)
				{
					((rrxvframe *)f)->init(h);
					if(h.size!=((rrxvframe *)f)->_h.size && h.flags!=RR_EOF)
						_throw("YUV image size mismatch");
				}
				else
				#endif
				((rrcompframe *)f)->init(h, h.flags);
				if(h.flags!=RR_EOF)
					recv((char *)(h.flags==RR_RIGHT? f->_rbits:f->_bits), h.size);

				if(!stereo || h.flags!=RR_LEFT)
				{
					try {w->drawframe(f);}
					catch (...) {if(w) delwindow(w);  throw;}
				}

			} while(!(f && f->_h.flags==RR_EOF));

			if(v.major==1 && v.minor==0)
			{
				char cts=1;
				send(&cts, 1);
			}
		}
	}
	catch(rrerror &e)
	{
		rrout.println("%s-- %s", e.getMethod(), e.getMessage());
	}
	if(_t) {_t->detach();  delete _t;}
	delete this;
}


void vgltransserver::delwindow(clientwin *w)
{
	int i, j;
	rrcs::safelock l(_winmutex);
	if(_nwin>0)
		for(i=0; i<_nwin; i++)
			if(_win[i]==w)
			{
				delete w;  _win[i]=NULL;
				if(i<_nwin-1) for(j=i; j<_nwin-1; j++) _win[j]=_win[j+1];
				_win[_nwin-1]=NULL;  _nwin--;  break;
			}
}


// Register a new window with this server
clientwin *vgltransserver::addwindow(int dpynum, Window win, bool stereo)
{
	rrcs::safelock l(_winmutex);
	int winid=_nwin;
	if(_nwin>0)
	{
		for(winid=0; winid<_nwin; winid++)
			if(_win[winid] && _win[winid]->match(dpynum, win)) return _win[winid];
	}
	if(_nwin>=MAXWIN) _throw("No free window ID's");
	if(dpynum<0 || dpynum>65535 || win==None) _throw("Invalid argument");
	_win[winid]=new clientwin(dpynum, win, _drawmethod, stereo);

	if(!_win[winid]) _throw("Could not create window instance");
	_nwin++;
	return _win[winid];
}


void vgltransserver::send(char *buf, int len)
{
	try
	{
		if(_sd) _sd->send(buf, len);
	}
	catch(...)
	{
		rrout.println("Error sending data to server.  Server may have disconnected.");
		rrout.println("   (this is normal if the application exited.)");
		throw;
	}
}


void vgltransserver::recv(char *buf, int len)
{
	try
	{
		if(_sd) _sd->recv(buf, len);
	}
	catch(...)
	{
		rrout.println("Error receiving data from server.  Server may have disconnected.");
		rrout.println("   (this is normal if the application exited.)");
		throw;
	}
}
