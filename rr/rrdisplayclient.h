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

#ifndef __RRDISPLAYCLIENT_H
#define __RRDISPLAYCLIENT_H

#include "rrconn.h"
#include "rr.h"
#include "rrframe.h"
#include "genericQ.h"

class rrdisplayclient : rraconn
{

	public:

	rrdisplayclient(char *servername, unsigned short port, bool dossl)
		: rraconn()
	{
		tryunix(pthread_mutex_init(&ready, NULL));
		tryunix(pthread_mutex_init(&bmpmutex, NULL));
		lastb=NULL;  bmpi=0;
		for(int i=0; i<NB; i++) memset(&bmp[i], 0, sizeof(rrbmp));
		connect(servername, port, dossl);
	}

	~rrdisplayclient(void)
	{
		deadyet=1;
		q.release();
		pthread_mutex_unlock(&ready);
		rraconn::disconnect();
		for(int i=0; i<NB; i++) if(bmp[i].bits) delete [] bmp[i].bits;
	}

	rrbmp *getbitmap(int, int, int);
	bool frameready(void);
	void sendframe(rrbmp *);

	private:

	static const int NB=3;
	rrbmp bmp[NB];  int bmpi;
	rrbmp *lastb;
	pthread_mutex_t ready, bmpmutex;
	genericQ q;
	void compresssend(rrbmp *, rrbmp *);
	void dispatch(void);
	void allocbmp(rrbmp *, int, int, int);
};

#endif
