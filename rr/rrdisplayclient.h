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
		lastb=NULL;
		connect(servername, port, dossl);
	}

	~rrdisplayclient(void)
	{
		deadyet=1;
		q.release();
		pthread_mutex_unlock(&ready);
		rraconn::disconnect();
	}

	rrbmp *getbitmap(int, int, int);
	bool frameready(void);
	void sendframe(rrbmp *);

	private:

	rrbmp *lastb;
	pthread_mutex_t ready, complete;
	genericQ q;
	void compresssend(rrbmp *, rrbmp *);
	void dispatch(void);
	void allocbmp(rrbmp *, int, int, int);
};

#endif
