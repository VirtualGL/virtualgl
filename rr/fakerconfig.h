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

#ifndef __FAKER_CONFIG_H
#define __FAKER_CONFIG_H

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "rr.h"
#include "rrutil.h"
#include <stdio.h>

#define getconfigstr(envvar, string) {  \
	getconfig("RR"envvar, string);  \
	getconfig("VGL_"envvar, string);}

#define getconfigint(envvar, val, min, max) { \
	getconfig("RR"envvar, val, min, max);  \
	getconfig("VGL_"envvar, val, min, max);}

#define getconfigbool(envvar, val) {  \
	getconfig("RR"envvar, val);  \
	getconfig("VGL_"envvar, val);}

// Compression types
enum {RRCOMP_NONE=0, RRCOMP_MJPEG};

class FakerConfig
{
	public:

		FakerConfig(void)
		{
			// Defaults
			gllib=NULL;
			x11lib=NULL;
			client=NULL;
			localdpystring=(char *)":0";
			loqual=90;
			losubsamp=RR_411;
			hiqual=95;
			hisubsamp=RR_444;
			currentqual=hiqual;
			currentsubsamp=hisubsamp;
			compress=RRCOMP_MJPEG;
			spoil=true;
			ssl=false;
			port=-1;
			glp=false;
			usewindow=false;
			sync=false;

			// Fetch values from environment
			getconfigstr("GLLIB", gllib);
			getconfigstr("X11LIB", x11lib);
			getconfigstr("CLIENT", client);
			getconfigstr("DISPLAY", localdpystring);
			#ifdef USEGLP
			char *temps=strdup(localdpystring), *ptr=NULL;
			if((ptr=strtok(temps, " \t"))!=NULL)
			{
				if(ptr[0]=='/' || !stricmp(ptr, "GLP"))
				{
					glp=true;
					strcpy(localdpystring, ptr);
				}
			}
			if(temps) free(temps);
			#endif
			getconfigint("LOQUAL", loqual, 1, 100);
			getconfigint("LOSUBSAMP", losubsamp, 411, 444);
			switch(losubsamp)
			{
				case 411:  losubsamp=RR_411;  break;
				case 422:  losubsamp=RR_422;  break;
				case 444:  losubsamp=RR_444;  break;
			}
			getconfigint("QUAL", hiqual, 1, 100);
			getconfigint("SUBSAMP", hisubsamp, 411, 444);
			switch(hisubsamp)
			{
				case 411:  hisubsamp=RR_411;  break;
				case 422:  hisubsamp=RR_422;  break;
				case 444:  hisubsamp=RR_444;  break;
			}
			sethiqual();
			bool temp=false;
			getconfigbool("NOSPOIL", temp);
			if(temp) spoil=false;
			temp=false;
			getconfigbool("NOCOMPRESS", temp);
			if(temp) compress=RRCOMP_NONE;
			getconfigbool("SPOIL", spoil);
			getconfigint("COMPRESS", compress, RRCOMP_NONE, RRCOMP_MJPEG);
			getconfig("RRUSESSL", ssl);
			getconfig("VGL_SSL", ssl);
			getconfigint("PORT", port, 0, 65535);
			if(port==-1) port=ssl?RR_DEFAULTPORT+1:RR_DEFAULTPORT;
			getconfigbool("WINDOW", usewindow);
			if(glp) usewindow=false;
			getconfigbool("SYNC", sync);
		}

		void setloqual(void)
		{
			currentqual=loqual;  currentsubsamp=losubsamp;
		}

		void sethiqual(void)
		{
			currentqual=hiqual;  currentsubsamp=hisubsamp;
		}

		char *gllib;
		char *x11lib;
		char *client;
		char *localdpystring;

		int loqual;
		int losubsamp;
		int hiqual;
		int hisubsamp;
		int currentqual;
		int currentsubsamp;
		int compress;
		int port;

		bool spoil;
		bool ssl;
		bool glp;
		bool usewindow;
		bool sync;

	private:

		void getconfig(const char *envvar, char* &string)
		{
			char *temp=NULL;
			if((temp=getenv(envvar))!=NULL && strlen(temp)>0)
				string=temp;
		}

		void getconfig(const char *envvar, int &val, int min, int max)
		{
			char *temp=NULL;
			if((temp=getenv(envvar))!=NULL && strlen(temp)>0)
			{
				int itemp;
				if((itemp=atoi(temp))>=min && itemp<=max) val=itemp;
			}
		}

		void getconfig(const char *envvar, bool &val)
		{
			char *temp=NULL;
			if((temp=getenv(envvar))!=NULL && strlen(temp)>0)
			{
				if(!strncmp(temp, "1", 1)) val=true;
				else if(!strncmp(temp, "0", 1)) val=false;
			}
		}

};

#endif
