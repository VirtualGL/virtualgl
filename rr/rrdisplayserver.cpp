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

#ifdef _WIN32
#include <io.h>
#define access(a,b) _access(a,b)
#define F_OK 0
#endif
#include "rrdisplayserver.h"

#define CERTF "rrcert.pem"

#ifdef _WIN32
	#define DIRSEP "\\"
	#define ALTDIR "."
#else
	#include <pwd.h>
	#define DIRSEP "/"
	#define ALTDIR "/etc"
#endif

char *gethomedir(void)
{
	#ifdef _WIN32
	return getenv("USERPROFILE");
	#else
	char *homedir=NULL;  struct passwd *pw;
	if((homedir=getenv("HOME"))!=NULL) return homedir;
	if((pw=getpwuid(getuid()))==NULL) return NULL;
	return pw->pw_dir;
	#endif
}

rrdisplayserver::rrdisplayserver(unsigned short port, bool dossl) :
	listensd(NULL), t(NULL), deadyet(false)
{
	char temppath[1024];  temppath[0]=0;

	if(dossl)
	{
		strncpy(temppath, gethomedir(), 1024);  temppath[1024]=0;
		strncat(temppath, DIRSEP CERTF, 1024-strlen(temppath));
		if(access(temppath, F_OK)!=0)
		{
			#ifdef _WIN32
			if(GetModuleFileName(NULL, temppath, 1024))
			{
				char *ptr;
				if((ptr=strrchr(temppath, '\\'))!=NULL) *ptr='\0';
			}
			else
			#endif
			strncpy(temppath, ALTDIR, 1024);
			strncat(temppath, DIRSEP CERTF, 1024-strlen(temppath));
		}
		rrout.println("Using certificate file %s\n", temppath);
	}

	errifnot(listensd=new rrsocket(dossl));
	listensd->listen(port==0?RR_DEFAULTPORT:port, temppath, temppath);
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
			s=new rrserver(sd);
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
	rrw[winid]=new rrcwin(dpynum, win);

	if(!rrw[winid]) _throw("Could not create window instance");
	windows++;
	return rrw[winid];
}
