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

void rrdisplayclient::run(void)
{
	rrframe *lastb=NULL, *b=NULL;
	rrprofiler prof_comp("Compress");  long bytes=0;

	int i;

	try {

	rrcompressor *c[MAXPROCS];  Thread *ct[MAXPROCS];
	for(i=0; i<np; i++)
		errifnot(c[i]=new rrcompressor(i, np, sd));
	if(np>1) for(i=1; i<np; i++)
	{
		errifnot(ct[i]=new Thread(c[i]));
		ct[i]->start();
	}

	while(!deadyet)
	{
		b=NULL;
		q.get((void **)&b);  if(deadyet) break;
		if(!b) _throw("Queue has been shut down");
		ready.unlock();
		if(b->h.flags==RR_RIGHT && !stereo) continue;
		prof_comp.startframe();
		if(np>1)
			for(i=1; i<np; i++) {
				ct[i]->checkerror();  c[i]->go(b, lastb);
			}
		c[0]->compresssend(b, lastb);
		bytes+=c[0]->bytes;
		if(np>1)
			for(i=1; i<np; i++) {
				c[i]->stop();  ct[i]->checkerror();  c[i]->send();
				bytes+=c[i]->bytes;
			}
		prof_comp.endframe(b->h.framew*b->h.frameh, 0,
			b->h.flags==RR_LEFT || b->h.flags==RR_RIGHT? 0.5 : 1);

		if(b->h.flags!=RR_LEFT)
		{
			rrframeheader h;
			memcpy(&h, &b->h, sizeof(rrframeheader));
			h.flags=RR_EOF;
			endianize(h);
			if(sd) sd->send((char *)&h, sizeof(rrframeheader));

			char cts=0;
			if(sd)
			{
				sd->recv(&cts, 1);
				if(cts<1 || cts>2) _throw("CTS error");
				if(stereo && (b->h.flags==RR_LEFT || b->h.flags==RR_RIGHT) && cts!=2)
				{
					rrout.println("[VGL] Disabling stereo because client doesn't support it");
					stereo=false;
				}
			}
		}
		prof_total.endframe(b->h.width*b->h.height, bytes,
			b->h.flags==RR_LEFT || b->h.flags==RR_RIGHT? 0.5 : 1);
		bytes=0;
		prof_total.startframe();

		lastb=b;
	}

	for(i=0; i<np; i++) c[i]->shutdown();
	if(np>1) for(i=1; i<np; i++)
	{
		ct[i]->stop();
		ct[i]->checkerror();
		delete ct[i];
	}
	for(i=0; i<np; i++) delete c[i];

	} catch(rrerror &e)
	{
		if(t) t->seterror(e);
		ready.unlock();
 		throw;
	}
}

rrframe *rrdisplayclient::getbitmap(int w, int h, int ps)
{
	rrframe *b=NULL;
	ready.lock();  if(deadyet) return NULL;
	if(t) t->checkerror();
	bmpmutex.lock();
	b=&bmp[bmpi];  bmpi=(bmpi+1)%NB;
	bmpmutex.unlock();
	rrframeheader hdr;
	hdr.height=hdr.frameh=h;
	hdr.width=hdr.framew=w;
	hdr.x=hdr.y=0;
	b->init(&hdr, ps);
	return b;
}

bool rrdisplayclient::frameready(void)
{
	if(t) t->checkerror();
	return(q.items()<=0);
}

void rrdisplayclient::sendframe(rrframe *b)
{
	if(t) t->checkerror();
	b->h.dpynum=dpynum;
	q.add((void *)b);
}

void rrdisplayclient::sendcompressedframe(rrframeheader &horig, unsigned char *bits)
{
	rrframeheader h=horig;
	endianize(h);
	if(sd) sd->send((char *)&h, sizeof(rrframeheader));
	if(sd) sd->send((char *)bits, horig.size);
	h=horig;
	h.flags=RR_EOF;
	h.dpynum=dpynum;
	endianize(h);
	if(sd) sd->send((char *)&h, sizeof(rrframeheader));
	char cts=0;
	if(sd) {sd->recv(&cts, 1);  if(cts!=1) _throw("CTS error");}
}

void rrcompressor::compresssend(rrframe *b, rrframe *lastb)
{
	int endline, startline;
	int STRIPH=b->strip_height;
	bool bu=false;  rrjpeg jpg;
	if(!b) return;
	if(b->flags&RRBMP_BOTTOMUP) bu=true;

	int nstrips=(b->h.height+STRIPH-1)/STRIPH;

	bytes=0;
	for(int strip=0; strip<nstrips; strip++)
	{
		if(strip%np!=myrank) continue;
		int i=strip*STRIPH;
		if(bu)
		{
			startline=b->h.height-i-STRIPH;
			if(startline<0) startline=0;
			endline=startline+min(b->h.height-i, STRIPH);
		}
		else
		{
			startline=i;
			endline=startline+min(b->h.height-i, STRIPH);
		}
		if(b->stripequals(lastb, startline, endline)) continue;
		rrframe *rrb=b->getstrip(startline, endline);
		rrjpeg *j=NULL;
		if(myrank>0) {errifnot(j=new rrjpeg());}
		else j=&jpg;
		*j=*rrb;
		bytes+=j->h.size;
		delete rrb;
		if(myrank==0)
		{
			unsigned int size=j->h.size;
			endianize(j->h);
			if(sd) sd->send((char *)&j->h, sizeof(rrframeheader));
			if(sd) sd->send((char *)j->bits, (int)size);
		}
		else
		{
			store(j);
		}
	}
}
