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

// This gets called by the parent class
void rrdisplayclient::dispatch(void)
{
	try {

	rrbmp *b=NULL;
	q.get((void **)&b);  if(deadyet) return;
	if(!b) _throw("Queue has been shut down");
	pthread_mutex_unlock(&ready);
	compresssend(b, lastb);
	prof_total.endframe(b->h.bmpw*b->h.bmph, 0, 1);
	prof_total.startframe();
	lastb=b;

	} catch(...) {pthread_mutex_unlock(&ready);  throw;}
}


void rrdisplayclient::allocbmp(rrbmp *bmp, int w, int h, int pixelsize)
{
	if(!bmp || w<1 || h<1 || pixelsize<3 || pixelsize>4)
		_throw("Invalid argument to rrdisplayclient::allocbmp()");
	if(bmp->h.bmpw==w && bmp->h.bmph==h && bmp->pixelsize==pixelsize
		&& bmp->bits) return;
	if(bmp->bits) delete [] bmp->bits;
	errifnot(bmp->bits=new unsigned char[w*h*pixelsize+1]);
	bmp->h.bmpw=w;  bmp->h.bmph=h;
	bmp->pixelsize=pixelsize;
}


rrbmp *rrdisplayclient::getbitmap(int w, int h, int pixelsize)
{
	rrbmp *b=NULL;
	tryunix(pthread_mutex_lock(&ready));  if(deadyet) return NULL;
	checkerror();
	tryunix(pthread_mutex_lock(&bmpmutex));
	b=&bmp[bmpi];  bmpi=(bmpi+1)%NB;
	tryunix(pthread_mutex_unlock(&bmpmutex));
	allocbmp(b, w, h, pixelsize);
	return b;
}


bool rrdisplayclient::frameready(void)
{
	checkerror();
	return(q.items()<=0);
}


void rrdisplayclient::sendframe(rrbmp *bmp)
{
	checkerror();
	q.add((void *)bmp);
}


void rrdisplayclient::compresssend(rrbmp *b, rrbmp *lastb)
{
	int endline, i;  int startline;
	rrjpeg j;
	bool bu=false;
	if(b->flags&RRBMP_BOTTOMUP) bu=true;
	int pitch=b->h.bmpw*b->pixelsize;

	for(i=0; i<b->h.bmph; i+=b->strip_height)
	{
		unsigned char eof=0;
		int bufstartline;
		if(bu)
		{
			startline=b->h.bmph-i-b->strip_height;
			if(startline<0) startline=0;
			endline=startline+min(b->h.bmph-i, b->strip_height);
			if(b->h.bmph-i<2*b->strip_height) {startline=0;  i+=b->strip_height;  eof=1;}
			bufstartline=b->h.bmph-endline;
		}
		else
		{
			startline=i;
			endline=startline+min(b->h.bmph-i, b->strip_height);
			if(b->h.bmph-i<2*b->strip_height) {endline=b->h.bmph;  i+=b->strip_height;  eof=1;}
			bufstartline=startline;
		}

		if(lastb && b->h.bmpw==lastb->h.bmpw && b->h.bmph==lastb->h.bmph
		&& b->h.winw==lastb->h.winw && b->h.winh==lastb->h.winh
		&& b->h.qual==lastb->h.qual && b->h.subsamp==lastb->h.subsamp
		&& b->pixelsize==lastb->pixelsize && b->h.winid==lastb->h.winid
		&& b->h.dpynum==lastb->h.dpynum && b->bits
		&& lastb->bits && !memcmp(&b->bits[pitch*bufstartline],
			&lastb->bits[pitch*bufstartline],
			pitch*(endline-startline)))
			continue;
		rrbmp rrb;
		memcpy(&rrb.h, &b->h, sizeof(rrframeheader));
		rrb.h.bmph=endline-startline;
		rrb.h.bmpy+=startline;
		rrb.pixelsize=b->pixelsize;
		rrb.flags=b->flags;
		rrb.bits=&b->bits[pitch*bufstartline];
		prof_comp.startframe();
		j=rrb;
		prof_comp.endframe(j.h.bmpw*j.h.bmph, j.h.size, 0);
		j.h.eof=0;
		send((char *)&j.h, sizeof(rrframeheader));
		send((char *)j.bits, (int)j.h.size);
	}

	rrframeheader h;
	memcpy(&h, &b->h, sizeof(rrframeheader));
	h.eof=1;
	send((char *)&h, sizeof(rrframeheader));

	char cts=0;
	recv(&cts, 1);  if(cts!=1) _throw("CTS error");
}
