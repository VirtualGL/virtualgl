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

rrdisplayserver::rrdisplayserver(unsigned short port, bool dossl, int _drawmethod) :
	drawmethod(_drawmethod), listensd(NULL), t(NULL), deadyet(false)
{
	errifnot(listensd=new rrsocket(dossl));
	listensd->listen(port==0?RR_DEFAULTPORT:port);
	errifnot(t=new Thread(this));
	t->start();
}

rrdisplayserver::~rrdisplayserver(void)
{
	deadyet=true;
	if(listensd) listensd->close();
	if(t) {t->stop();  t=NULL;}
}

void rrdisplayserver::run(void)
{
	rrsocket *sd;  rrserver *s;

	while(!deadyet)
	{
		try
		{
			s=NULL;  sd=NULL;
			sd=listensd->accept();  if(deadyet) break;
			rrout.println("++ Connection from %s.", sd->remotename());
			s=new rrserver(sd, drawmethod);
			continue;
		}
		catch(rrerror &e)
		{
			if(!deadyet)
			{
				rrout.println("%s:\n%s", e.getMethod(), e.getMessage());
				if(s) delete s;  if(sd) delete sd;
				continue;
			}
		}
	}
	rrout.println("Listener exiting ...");
	if(listensd) {delete listensd;  listensd=NULL;}
}

void rrserver::run(void)
{
	rrcwin *w=NULL;
	rrjpeg *j;
	rrframeheader h;

	try
	{
		while(1)
		{
			do
			{
				sd->recv((char *)&h, sizeof(rrframeheader));
				endianize(h);
				errifnot(w=addwindow(h.dpynum, h.winid));

				try {j=w->getFrame();}
				catch (...) {if(w) delwindow(w);  throw;}

				j->init(&h);
				if(!h.eof) {sd->recv((char *)j->bits, h.size);}

				try {w->drawFrame(j);}
				catch (...) {if(w) delwindow(w);  throw;}
			} while(!(j && j->h.eof));

			char cts=1;
			sd->send(&cts, 1);
		}
	}
	catch(rrerror &e)
	{
		rrout.println("Server error in %s\n%s", e.getMethod(), e.getMessage());
	}
	if(t) delete t;
	delete this;
}

void rrserver::delwindow(rrcwin *w)
{
	int i, j;
	rrcs::safelock l(winmutex);
	if(windows>0)
		for(i=0; i<windows; i++)
			if(rrw[i]==w)
			{
				delete w;  rrw[i]=NULL;
				if(i<windows-1) for(j=i; j<windows-1; j++) rrw[j]=rrw[j+1];
				rrw[windows-1]=NULL;  windows--;  break;
			}
}

// Register a new window with this server
rrcwin *rrserver::addwindow(int dpynum, Window win)
{
	rrcs::safelock l(winmutex);
	int winid=windows;
	if(windows>0)
	{
		for(winid=0; winid<windows; winid++)
			if(rrw[winid] && rrw[winid]->match(dpynum, win)) return rrw[winid];
	}
	if(windows>=MAXWIN) _throw("No free window ID's");
	if(dpynum<0 || dpynum>255 || win==None) _throw("Invalid argument");
	rrw[winid]=new rrcwin(dpynum, win, drawmethod);

	if(!rrw[winid]) _throw("Could not create window instance");
	windows++;
	return rrw[winid];
}
