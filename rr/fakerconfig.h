/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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
#include <math.h>
#include "rr.h"
#include "rrutil.h"
#include <stdio.h>
#include <X11/X.h>
#include <X11/keysym.h>

#define getconfigstr(envvar, string) {  \
	getconfig("RR"envvar, string);  \
	getconfig("VGL_"envvar, string);}

#define getconfigint(envvar, val, min, max) { \
	getconfig("RR"envvar, val, min, max);  \
	getconfig("VGL_"envvar, val, min, max);}

#define getconfigbool(envvar, val) {  \
	getconfig("RR"envvar, val);  \
	getconfig("VGL_"envvar, val);}

#define getconfigdouble(envvar, val, min, max) {  \
	getconfig("RR"envvar, val, min, max);  \
	getconfig("VGL_"envvar, val, min, max);}

#define getconfiggamma(envvar) {  \
	getgamma("RR"envvar);  \
	getgamma("VGL_"envvar);}

#define DEFLOQUAL 90
#define DEFHIQUAL 95
#define DEFLOSUBSAMP RR_411
#define DEFHISUBSAMP RR_444

class FakerConfig
{
	public:

		FakerConfig(void) : currentqual(hiqual), currentsubsamp(hisubsamp)
		{
			// Defaults
			gllib=NULL;
			x11lib=NULL;
			client=NULL;
			localdpystring=(char *)":0";
			loqual=DEFLOQUAL;
			losubsamp=DEFLOSUBSAMP;
			hiqual=DEFHIQUAL;
			hisubsamp=DEFHISUBSAMP;
			currentqual=hiqual;
			currentsubsamp=hisubsamp;
			compress=RRCOMP_DEFAULT;
			spoil=true;
			ssl=false;
			port=-1;
			glp=false;
			usewindow=false;
			sync=false;
			np=min(numprocs(), MAXPROCS);  if(np>1) np--;
			autotest=false;
			oldgcf=0.0;
			#ifdef SUNOGL
			gcf=2.22;
			usesungamma=true;
			#else
			gcf=1.0;
			usesungamma=false;
			#endif
			transpixel=-1;
			tilesize=RR_DEFAULTTILESIZE;
			trace=false;
			readback=true;
			verbose=false;
			gui=true;
			guikey=XK_F9;
			guimod=ShiftMask|ControlMask;
			fps=-1.0;
			vendor=NULL;
			xtthreadinit=true;
			sunray=false;
			interframe=true;
			log=NULL;
			reloadenv();
		}

		void reloadenv(void)
		{
			// Fetch values from environment
			getconfigstr("GLLIB", gllib);
			getconfigstr("X11LIB", x11lib);
			getconfigstr("CLIENT", client);
			getconfigstr("DISPLAY", localdpystring);
			for(int i=0; i<(int)strlen(localdpystring); i++)
				if(localdpystring[i]!=' ' && localdpystring[i]!='\t')
				{
					localdpystring=&localdpystring[i];  break;
				}
			if(strlen(localdpystring)>0)
			{
				int i;
				for(i=0; i<(int)strlen(localdpystring); i++)
					if(localdpystring[i]==' ' || localdpystring[i]=='\t')
						{localdpystring[i]='\0';  break;}
			}
			#ifdef USEGLP
			if(localdpystring[0]=='/' || !strnicmp(localdpystring, "GLP", 3))
				glp=true;
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
			getconfigint("COMPRESS", compress, RRCOMP_NONE, RRCOMP_JPEG);
			if(compress==RRCOMP_DEFAULT)
			{
				char *temps=NULL;
				getconfigstr("COMPRESS", temps);
				if(temps && !stricmp(temps, "raw")) compress=RRCOMP_NONE;
				else if(temps && !stricmp(temps, "none")) compress=RRCOMP_NONE;
				else if(temps && !stricmp(temps, "jpeg")) compress=RRCOMP_JPEG;
			}
			getconfig("RRUSESSL", ssl);
			getconfig("VGL_SSL", ssl);
			getconfigint("PORT", port, 0, 65535);
			if(port==-1) port=ssl?RR_DEFAULTSSLPORT:RR_DEFAULTPORT;
			getconfigbool("WINDOW", usewindow);
			if(glp) usewindow=false;
			getconfigbool("SYNC", sync);
			getconfigint("NPROCS", np, 0, 1024);
			getconfigbool("AUTOTEST", autotest);
			getconfiggamma("GAMMA");
			getconfigint("TRANSPIXEL", transpixel, 0, 255);
			getconfigint("TILESIZE", tilesize, 8, 1024);
			getconfigbool("TRACE", trace);
			getconfigbool("READBACK", readback);
			getconfigbool("VERBOSE", verbose);
			char *guikeyseq=NULL;
			getconfigstr("GUI", guikeyseq);
			if(guikeyseq && strlen(guikeyseq)>0)
			{
				if(!stricmp(guikeyseq, "none")) gui=false;
				else
				{
					unsigned int mod=0, key=0;
					for(unsigned int i=0; i<strlen(guikeyseq); i++)
						guikeyseq[i]=tolower(guikeyseq[i]);
					if(strstr(guikeyseq, "ctrl")) mod|=ControlMask;
					if(strstr(guikeyseq, "alt")) mod|=Mod1Mask;
					if(strstr(guikeyseq, "shift")) mod|=ShiftMask;
					if(strstr(guikeyseq, "f10")) key=XK_F10;
					else if(strstr(guikeyseq, "f11")) key=XK_F11;
					else if(strstr(guikeyseq, "f12")) key=XK_F12;
					else if(strstr(guikeyseq, "f1")) key=XK_F1;
					else if(strstr(guikeyseq, "f2")) key=XK_F2;
					else if(strstr(guikeyseq, "f3")) key=XK_F3;
					else if(strstr(guikeyseq, "f4")) key=XK_F4;
					else if(strstr(guikeyseq, "f5")) key=XK_F5;
					else if(strstr(guikeyseq, "f6")) key=XK_F6;
					else if(strstr(guikeyseq, "f7")) key=XK_F7;
					else if(strstr(guikeyseq, "f8")) key=XK_F8;
					else if(strstr(guikeyseq, "f9")) key=XK_F9;
					if(key) guikey=key;  guimod=mod;
					gui=true;
				}
			}
			getconfigbool("GUI_XTTHREADINIT", xtthreadinit);
			getconfigdouble("FPS", fps, 0.0, 1000000.0);
			getconfigstr("XVENDOR", vendor);
			getconfigbool("INTERFRAME", interframe);
			getconfigstr("LOG", log);
			sanitycheck();
		}

		void sanitycheck(void)
		{
			np=min(np, min(numprocs(), MAXPROCS));
			if(np==0)
			{
				np=min(numprocs(), MAXPROCS);	 if(np>1) np--;
			}
			if(hiqual<1 || hiqual>100) hiqual=DEFHIQUAL;
			if(loqual<1 || loqual>100) loqual=DEFLOQUAL;
			if(hisubsamp<0 || hisubsamp>=RR_SUBSAMPOPT) hisubsamp=DEFHISUBSAMP;
			if(losubsamp<0 || losubsamp>=RR_SUBSAMPOPT) losubsamp=DEFLOSUBSAMP;
			if(port==0) port=ssl?RR_DEFAULTSSLPORT:RR_DEFAULTPORT;
			if(compress<0 || compress>=RR_COMPRESSOPT) compress=RRCOMP_DEFAULT;
			if(transpixel<0 || transpixel>255) transpixel=-1;
			if(tilesize<8 || tilesize>1024) tilesize=RR_DEFAULTTILESIZE;
			buildlut();
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
		char *log;
		char *vendor;

		int loqual;
		int losubsamp;
		int hiqual;
		int hisubsamp;
		int &currentqual;
		int &currentsubsamp;
		int compress;
		int port;
		int transpixel;
		int np;
		int tilesize;

		bool spoil;
		bool ssl;
		bool glp;
		bool usewindow;
		bool sync;
		bool autotest;
		bool trace;
		bool readback;
		bool verbose;
		bool gui;
		bool xtthreadinit;
		bool sunray;
		bool interframe;

		unsigned int guikey;
		unsigned int guimod;

		double fps;

		bool usesungamma;
		double oldgcf, gcf;
		unsigned char lut[256];
		unsigned short lut16[65536];

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
				char *t=NULL;  int itemp=strtol(temp, &t, 10);
				if(t && t!=temp && itemp>=min && itemp<=max) val=itemp;
			}
		}

		void getconfig(const char *envvar, double &val, double min, double max)
		{
			char *temp=NULL;
			if((temp=getenv(envvar))!=NULL && strlen(temp)>0)
			{
				char *t=NULL;  double dtemp=strtod(temp, &t);
				if(t && t!=temp && dtemp>min && dtemp<max) val=dtemp;
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

		void getgamma(const char *envvar)
		{
			char *temp=NULL;
			if((temp=getenv(envvar))!=NULL && strlen(temp)>0)
			{
				if(!strcmp(temp, "1"))
				{
					usesungamma=true;  gcf=2.22;
				}
				else if(!strcmp(temp, "0"))
				{
					usesungamma=false;  gcf=1.0;
				}
				else
				{
					usesungamma=false;
					char *t=NULL;  double dtemp=strtod(temp, &t);
					if(t && t!=temp) gcf=dtemp;
				}
			}
		}

		void buildlut(void)
		{
			if(gcf!=oldgcf)
			{
				oldgcf=gcf;
				if(gcf!=0.0 && gcf!=1.0 && gcf!=-1.0)
				{
					for(int i=0; i<256; i++)
					{
						double g=gcf>0.0? 1.0/gcf : -gcf;
						lut[i]=(unsigned char)(255.*pow((double)i/255., g)+0.5);
					}
					for(int i=0; i<65536; i++)
					{
						double g=gcf>0.0? 1.0/gcf : -gcf;
						lut16[i]=(unsigned short)(255.*pow((double)(i/256)/255., g)+0.5)<<8;
						lut16[i]|=(unsigned short)(255.*pow((double)(i%256)/255., g)+0.5);
					}
				}
			}
		}

};

#endif
