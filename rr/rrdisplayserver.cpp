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

void rrclient::receive(rrconn *c)
{
	rrcwin *w=NULL;
	rrjpeg *j;
	rrframeheader h;

	do
	{
		c->recv((char *)&h, sizeof(rrframeheader));
		errifnot(w=addwindow(h.dpynum, h.winid));

		try
		{
			j=w->getFrame();
		}
		catch (...) {if(w) delwindow(w);  throw;}

		j->init(&h);
		if(!h.eof)
		{
			c->recv((char *)j->bits, h.size);
		}

		try
		{
			w->drawFrame(j);
		}
		catch (...) {if(w) delwindow(w);  throw;}

		if(j->h.eof) break;
	}
	while(1);

	char cts=1;
	c->send(&cts, 1);
}


void rrclient::delwindow(rrcwin *w)
{
	int i, j;
	rrlock l(winmutex);
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
rrcwin *rrclient::addwindow(int dpynum, Window win)
{
	rrlock l(winmutex);
	int winid=windows;
	if(windows>0)
	{
		for(winid=0; winid<windows; winid++)
			if(rrw[winid] && rrw[winid]->match(dpynum, win)) return rrw[winid];
	}
	if(windows>=MAXWIN) _throw("No free window ID's");
	if(dpynum<0 || dpynum>255 || win==None) _throw("Invalid argument");
	rrw[winid]=new rrcwin(dpynum, win);
	if(!rrw[winid]) _throw("Could not create window instance");
	windows++;
	return rrw[winid];
}


void *rrdisplayserver::clientthread(void *param)
{
	rrltparam *tp=(rrltparam *)param;
	rrdisplayserver *rrl=(rrdisplayserver *)tp->rrl;
	rrconn *c=tp->c;
	int clientrank=tp->clientrank;
	delete tp;

	rrclient *rrc=new rrclient;

	try
	{
		if(!rrc) _throw("Could not create client instance");
		while(!rrl->deadyet) rrc->receive(c);
	}
	catch(RRError e)
	{
		if(!rrl->deadyet)
		{
			hpprintf("Client %d- %s (%d):\n%s\n", clientrank, e.file, e.line, e.message);
			if(rrc) {delete rrc;  rrc=NULL;}
			rrl->removeclient(c, false);
		}
	}
	if(rrc) delete rrc;
	return 0;
}
