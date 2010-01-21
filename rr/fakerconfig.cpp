/* Copyright (C)2009 D. R. Commander
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

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "rrlog.h"
#include "fakerconfig.h"
#include <stdio.h>
#include <X11/keysym.h>
#if FCONFIG_USESHM==1
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#endif
#include "rrutil.h"
#include <X11/Xatom.h>
#ifdef USEXV
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#endif

#define DEFQUAL 95

static FakerConfig fcenv;
static bool fcenv_set=false;

#if FCONFIG_USESHM==1
static int fc_shmid=-1;
int fconfig_getshmid(void) {return fc_shmid;}
#endif
static FakerConfig *fc=NULL;
static rrcs fcmutex;

static void fconfig_init(void);

FakerConfig *fconfig_instance(void)
{
	if(fc==NULL)
	{
		rrcs::safelock l(fcmutex);
		if(fc==NULL) 
		{
			#if FCONFIG_USESHM==1

			void *addr=NULL;
			if((fc_shmid=shmget(IPC_PRIVATE, sizeof(FakerConfig), IPC_CREAT|0600))==-1)
				_throwunix();
			if((addr=shmat(fc_shmid, 0, 0))==(void *)-1) _throwunix();
			if(!addr) _throw("Could not attach to config structure in shared memory");
			#ifdef linux
			shmctl(fc_shmid, IPC_RMID, 0);
			#endif
			char *env=NULL;
			if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
				&& !strncmp(env, "1", 1))
				rrout.println("[VGL] Shared memory segment ID for vglconfig: %d",
					fc_shmid);
			fc=(FakerConfig *)addr;

			#else

			fc=new FakerConfig;
			if(!fc) _throw("Could not allocate config structure");

			#endif

			fconfig_init();
		}
	}
	return fc;
}

void fconfig_deleteinstance(void)
{
	if(fc!=NULL)
	{
		rrcs::safelock l(fcmutex);
		if(fc!=NULL)
		{
			#if FCONFIG_USESHM==1

			shmdt((char *)fc);
			if(fc_shmid!=-1)
			{
				int ret=shmctl(fc_shmid, IPC_RMID, 0);
				char *env=NULL;
				if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
					&& !strncmp(env, "1", 1) && ret!=-1)
					rrout.println("[VGL] Removed shared memory segment %d", fc_shmid);

			}

			#else

			delete fc;

			#endif

			fc=NULL;
		}
	}
}

static void fconfig_init(void)
{
	rrcs::safelock l(fcmutex);
	memset(&fconfig, 0, sizeof(FakerConfig));
	memset(&fcenv, 0, sizeof(FakerConfig));
	fconfig.compress=-1;
	strncpy(fconfig.config, "/opt/VirtualGL/bin/vglconfig", MAXSTR);
	#ifdef SUNOGL
	fconfig_setgamma(fconfig, 2.22);
	fconfig.gamma_usesun=1;
	#else
	fconfig_setgamma(fconfig, 1.0);
	fconfig.gamma_usesun=0;
	#endif
	fconfig.gui=1;
	fconfig.guikey=XK_F9;
	fconfig.guimod=ShiftMask|ControlMask;
	fconfig.interframe=1;
	strncpy(fconfig.localdpystring, ":0", MAXSTR);
	fconfig.np=1;
	fconfig.port=-1;
	fconfig.qual=DEFQUAL;
	fconfig.readback=1;
	fconfig.spoil=1;
	fconfig.stereo=RRSTEREO_QUADBUF;
	fconfig.subsamp=-1;
	fconfig.tilesize=RR_DEFAULTTILESIZE;
	fconfig.transpixel=-1;
	fconfig_reloadenv();
}

static void buildlut(FakerConfig &fc)
{
	if(fc.gamma!=0.0 && fc.gamma!=1.0 && fc.gamma!=-1.0)
	{
		for(int i=0; i<256; i++)
		{
			double g=fc.gamma>0.0? 1.0/fc.gamma : -fc.gamma;
			fc.gamma_lut[i]=(unsigned char)(255.*pow((double)i/255., g)+0.5);
		}
		for(int i=0; i<65536; i++)
		{
			double g=fc.gamma>0.0? 1.0/fc.gamma : -fc.gamma;
			fc.gamma_lut16[i]=(unsigned short)(255.*pow((double)(i/256)/255., g)+0.5)<<8;
			fc.gamma_lut16[i]|=(unsigned short)(255.*pow((double)(i%256)/255., g)+0.5);
		}
	}
}

#define fetchenv_str(envvar, s) { \
	if((env=getenv(envvar))!=NULL && strlen(env)>0 \
		&& (!fcenv_set || strncmp(env, fcenv.s, MAXSTR-1))) { \
		strncpy(fconfig.s, env, MAXSTR-1); \
		strncpy(fcenv.s, env, MAXSTR-1); \
	} \
}

#define fetchenv_bool(envvar, b) { \
	if((env=getenv(envvar))!=NULL && strlen(env)>0) { \
		if(!strncmp(env, "1", 1) && (!fcenv_set || fcenv.b!=1)) \
			fconfig.b=fcenv.b=1; \
		else if(!strncmp(env, "0", 1) && (!fcenv_set || fcenv.b!=0)) \
			fconfig.b=fcenv.b=0; \
	} \
}

#define fetchenv_int(envvar, i, min, max) { \
	if((env=getenv(envvar))!=NULL && strlen(env)>0) { \
		char *t=NULL;  int itemp=strtol(env, &t, 10); \
		if(t && t!=env && itemp>=min && itemp<=max \
			&& (!fcenv_set || itemp!=fcenv.i)) \
			fconfig.i=fcenv.i=itemp; \
	} \
}

#define fetchenv_dbl(envvar, d, min, max) { \
	char *temp=NULL; \
	if((temp=getenv(envvar))!=NULL && strlen(temp)>0) { \
		char *t=NULL;  double dtemp=strtod(temp, &t); \
		if(t && t!=temp && dtemp>=min && dtemp<=max \
			&& (!fcenv_set || dtemp!=fcenv.d)) \
			fconfig.d=fcenv.d=dtemp; \
	} \
}

static void removespaces(char *s)
{
	if(s && strlen(s)>0)
	{
		for(int i=0; i<(int)strlen(s); i++)
			if(s[i]==' ' || s[i]=='\t')
				{s[i]='\0';  break;}
	}
}

void fconfig_setgamma(FakerConfig &fc, double gamma)
{
	fc.gamma=gamma;
	buildlut(fc);
}

void fconfig_reloadenv(void)
{
	char *env;

	rrcs::safelock l(fcmutex);

	fetchenv_bool("VGL_AUTOTEST", autotest);
	fetchenv_str("VGL_CLIENT", client);
	if((env=getenv("VGL_SUBSAMP"))!=NULL && strlen(env)>0)
	{
		int subsamp=-1;
		if(!strnicmp(env, "G", 1)) subsamp=0;
		else
		{
			char *t=NULL;  int itemp=strtol(env, &t, 10);
			if(t && t!=env)
			{
				switch(itemp)
				{
					case 0:                              subsamp=0;  break;
					case 444: case 11: case 1:           subsamp=1;  break;
					case 422: case 21: case 2:           subsamp=2;  break;
					case 411: case 420: case 22: case 4: subsamp=4;  break;
					case 410: case 42: case 8:           subsamp=8;  break;
					case 44:  case 16:                   subsamp=16;  break;
				}
			}
		}
		if(subsamp>=0 && (!fcenv_set || fcenv.subsamp!=subsamp))
			fconfig.subsamp=fcenv.subsamp=subsamp;
	}
	fetchenv_str("VGL_TRANSPORT", transport);
	if((env=getenv("VGL_COMPRESS"))!=NULL && strlen(env)>0)
	{
		char *t=NULL;  int itemp=strtol(env, &t, 10);
		int compress=-1;
		if(t && t!=env && itemp>=0
			&& (itemp<RR_COMPRESSOPT || strlen(fconfig.transport)>0)) compress=itemp;
		else if(!strnicmp(env, "p", 1)) compress=RRCOMP_PROXY;
		else if(!strnicmp(env, "j", 1)) compress=RRCOMP_JPEG;
		else if(!strnicmp(env, "r", 1)) compress=RRCOMP_RGB;
		else if(!strnicmp(env, "x", 1)) compress=RRCOMP_XV;
		else if(!strnicmp(env, "y", 1)) compress=RRCOMP_YUV;
		if(compress>=0 && (!fcenv_set || fcenv.compress!=compress))
		{
			fconfig_setcompress(fconfig, compress);
			fcenv.compress=compress;
		}
	}
	fetchenv_str("VGL_CONFIG", config);
	if((env=getenv("VGL_DISPLAY"))!=NULL && strlen(env)>0)
	{
		#ifdef USEGLP
		if((env[0]=='/' || !strnicmp(env, "GLP", 3)))
		{
			fconfig.glp=true;  removespaces(env);
		}
		#endif
		if(!fcenv_set || strncmp(env, fcenv.localdpystring, MAXSTR-1))
		{
			strncpy(fconfig.localdpystring, env, MAXSTR-1);
			strncpy(fcenv.localdpystring, env, MAXSTR-1);
		}
	}
	fetchenv_dbl("VGL_FPS", fps, 0.0, 1000000.0);
	if((env=getenv("VGL_GAMMA"))!=NULL && strlen(env)>0)
	{
		if(!strcmp(env, "1"))
		{
			if(!fcenv_set || fcenv.gamma_usesun!=1 || fcenv.gamma!=2.22)
			{
				fconfig.gamma_usesun=fcenv.gamma_usesun=1;
				fcenv.gamma=2.22;
				fconfig_setgamma(fconfig, 2.22);
			}
		}
		else if(!strcmp(env, "0"))
		{
			if(!fcenv_set || fcenv.gamma_usesun!=0 || fcenv.gamma!=1.0)
			{
				fconfig.gamma_usesun=fcenv.gamma_usesun=0;
				fcenv.gamma=1.0;
				fconfig_setgamma(fconfig, 1.0);
			}
		}
		else
		{
			char *t=NULL;  double dtemp=strtod(env, &t);
			if(t && t!=env
				&& (!fcenv_set || fcenv.gamma_usesun!=0 || fcenv.gamma!=dtemp))
			{
				fconfig.gamma_usesun=fcenv.gamma_usesun=0;
				fcenv.gamma=dtemp;
				fconfig_setgamma(fconfig, dtemp);
			}
		}
	}
	fetchenv_str("VGL_GLLIB", gllib);
	fetchenv_str("VGL_GUI", guikeyseq);
	if(strlen(fconfig.guikeyseq)>0)
	{
		if(!stricmp(fconfig.guikeyseq, "none")) fconfig.gui=false;
		else
		{
			unsigned int mod=0, key=0;
			for(unsigned int i=0; i<strlen(fconfig.guikeyseq); i++)
				fconfig.guikeyseq[i]=tolower(fconfig.guikeyseq[i]);
			if(strstr(fconfig.guikeyseq, "ctrl")) mod|=ControlMask;
			if(strstr(fconfig.guikeyseq, "alt")) mod|=Mod1Mask;
			if(strstr(fconfig.guikeyseq, "shift")) mod|=ShiftMask;
			if(strstr(fconfig.guikeyseq, "f10")) key=XK_F10;
			else if(strstr(fconfig.guikeyseq, "f11")) key=XK_F11;
			else if(strstr(fconfig.guikeyseq, "f12")) key=XK_F12;
			else if(strstr(fconfig.guikeyseq, "f1")) key=XK_F1;
			else if(strstr(fconfig.guikeyseq, "f2")) key=XK_F2;
			else if(strstr(fconfig.guikeyseq, "f3")) key=XK_F3;
			else if(strstr(fconfig.guikeyseq, "f4")) key=XK_F4;
			else if(strstr(fconfig.guikeyseq, "f5")) key=XK_F5;
			else if(strstr(fconfig.guikeyseq, "f6")) key=XK_F6;
			else if(strstr(fconfig.guikeyseq, "f7")) key=XK_F7;
			else if(strstr(fconfig.guikeyseq, "f8")) key=XK_F8;
			else if(strstr(fconfig.guikeyseq, "f9")) key=XK_F9;
			if(key) fconfig.guikey=key;  fconfig.guimod=mod;
			fconfig.gui=true;
		}
	}
	fetchenv_bool("VGL_INTERFRAME", interframe);
	fetchenv_str("VGL_LOG", log);
	fetchenv_bool("VGL_LOGO", logo);
	if((env=getenv("VGL_MCOMPRESS"))!=NULL && strlen(env)>0)
	{
		char *t=NULL;  int itemp=strtol(env, &t, 10);
		int mcompress=-1;
		if(t && t!=env && itemp>=RRCOMP_JPEG && itemp<=RRCOMP_RGB) mcompress=itemp;
		else if(!strnicmp(env, "j", 1)) mcompress=RRCOMP_JPEG;
		else if(!strnicmp(env, "r", 1)) mcompress=RRCOMP_RGB;
		if(mcompress>=0 && (!fcenv_set || fcenv.mcompress!=mcompress))
			fconfig.mcompress=fcenv.mcompress=mcompress;
	}
	fetchenv_str("VGL_MOVIE", moviefile);
	fetchenv_int("VGL_MQUAL", mqual, 1, 100);
	if((env=getenv("VGL_MSUBSAMP"))!=NULL && strlen(env)>0)
	{
		int msubsamp=-1;
		if(!strnicmp(env, "G", 1)) msubsamp=0;
		else
		{
			char *t=NULL;  int itemp=strtol(env, &t, 10);
			if(t && t!=env)
			{
				switch(itemp)
				{
					case 0:                              msubsamp=0;  break;
					case 444: case 11: case 1:           msubsamp=1;  break;
					case 422: case 21: case 2:           msubsamp=2;  break;
					case 411: case 420: case 22: case 4: msubsamp=4;  break;
				}
			}
		}
		if(msubsamp>=0 && (!fcenv_set || fcenv.msubsamp!=msubsamp))
			fconfig.msubsamp=fcenv.msubsamp=msubsamp;
	}
	fetchenv_int("VGL_NPROCS", np, 1, min(numprocs(), MAXPROCS));
	fetchenv_int("VGL_PORT", port, 0, 65535);
	fetchenv_int("VGL_QUAL", qual, 1, 100);
	fetchenv_bool("VGL_READBACK", readback);
	fetchenv_bool("VGL_SPOIL", spoil);
	fetchenv_bool("VGL_SSL", ssl);
	{
		if((env=getenv("VGL_STEREO"))!=NULL && strlen(env)>0)
		{
			int stereo=-1;
			if(!strnicmp(env, "L", 1)) stereo=RRSTEREO_LEYE;
			else if(!strnicmp(env, "RC", 2)) stereo=RRSTEREO_REDCYAN;
			else if(!strnicmp(env, "R", 1)) stereo=RRSTEREO_REYE;
			else if(!strnicmp(env, "Q", 1)) stereo=RRSTEREO_QUADBUF;
			else
			{
				char *t=NULL;  int itemp=strtol(env, &t, 10);
				if(t && t!=env && itemp>=0 && itemp<RR_STEREOOPT) stereo=itemp;
			}
			if(stereo>=0 && (!fcenv_set || fcenv.stereo!=stereo))
				fconfig.stereo=fcenv.stereo=stereo;
		}
	}
	fetchenv_bool("VGL_SYNC", sync);
	fetchenv_int("VGL_TILESIZE", tilesize, 8, 1024);
	fetchenv_bool("VGL_TRACE", trace);
	fetchenv_int("VGL_TRANSPIXEL", transpixel, 0, 255);
	fetchenv_bool("VGL_TRAPX11", trapx11);
	fetchenv_bool("VGL_WINDOW", usewindow);
	fetchenv_str("VGL_XVENDOR", vendor);
	fetchenv_bool("VGL_VERBOSE", verbose);
	fetchenv_str("VGL_X11LIB", x11lib);

	if(strlen(fconfig.transport)>0)
	{
		if(fconfig.compress<0) fconfig.compress=0;
		if(fconfig.subsamp<0) fconfig.subsamp=1;
	}

	if(fconfig.glp) fconfig.usewindow=false;
	fcenv_set=true;
}

void fconfig_setdefaultsfromdpy(Display *dpy)
{
	rrcs::safelock l(fcmutex);

	if(fconfig.compress<0)
	{
		bool usesunray=false;
		Atom atom=None;
		if((atom=XInternAtom(dpy, "_SUN_SUNRAY_SESSION", True))!=None)
			usesunray=true;
		const char *dstr=DisplayString(dpy);
		if((strlen(dstr) && dstr[0]==':') || (strlen(dstr)>5
			&& !strnicmp(dstr, "unix", 4)))
		{
			if(usesunray) fconfig_setcompress(fconfig, RRCOMP_XV);
			else fconfig_setcompress(fconfig, RRCOMP_PROXY);
		}
		else
		{
			if(usesunray) fconfig_setcompress(fconfig, RRCOMP_YUV);
			else fconfig_setcompress(fconfig, RRCOMP_JPEG);
		}
	}

	if(fconfig.port<0)
	{
		fconfig.port=fconfig.ssl? RR_DEFAULTSSLPORT:RR_DEFAULTPORT;
		Atom atom=None;  unsigned long n=0, bytesleft=0;
		int actualformat=0;  Atom actualtype=None;
		unsigned short *prop=NULL;
		if((atom=XInternAtom(dpy,
			fconfig.ssl? "_VGLCLIENT_SSLPORT":"_VGLCLIENT_PORT", True))!=None)
		{
			if(XGetWindowProperty(dpy, RootWindow(dpy, DefaultScreen(dpy)), atom,
				0, 1, False, XA_INTEGER, &actualtype, &actualformat, &n,
				&bytesleft, (unsigned char **)&prop)==Success && n>=1
				&& actualformat==16 && actualtype==XA_INTEGER && prop)
				fconfig.port=*prop;
			if(prop) XFree(prop);
		}
	}

	#ifdef USEXV
	int k, port, nformats;
	unsigned int i, j, dummy1, dummy2, dummy3, dummy4, dummy5, nadaptors=0;
	XvAdaptorInfo *ai=NULL;
	XvImageFormatValues *ifv=NULL;

	if(XvQueryExtension(dpy, &dummy1, &dummy2, &dummy3, &dummy4, &dummy5)
		==Success && XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &nadaptors,
		&ai)==Success && nadaptors>=1 && ai)
	{
		port=-1;
		for(i=0; i<nadaptors; i++)
		{
			for(j=ai[i].base_id; j<ai[i].base_id+ai[i].num_ports; j++)
			{
				nformats=0;
				ifv=XvListImageFormats(dpy, j, &nformats);
				if(ifv && nformats>0)
				{
					for(k=0; k<nformats; k++)
					{
						if(ifv[k].id==0x30323449)
						{
							XFree(ifv);  port=j;
							goto found;
						}
					}
				}
				XFree(ifv);
			}
		}
		found:
		XvFreeAdaptorInfo(ai);  ai=NULL;
		if(port!=-1) fconfig.transvalid[RRTRANS_XV]=1;
	}
	#endif
}

void fconfig_setcompress(FakerConfig &fc, int i)
{
	if(i<0 || (i>=RR_COMPRESSOPT && strlen(fc.transport)==0)) return;
	rrcs::safelock l(fcmutex);

	bool is=(fc.compress>=0);
	fc.compress=i;
	if(strlen(fc.transport)>0) return;
	if(!is) fc.transvalid[_Trans[fc.compress]]=fc.transvalid[RRTRANS_X11]=1;
	if(fc.subsamp<0) fc.subsamp=_Defsubsamp[fc.compress];
	if(strlen(fc.transport)==0 && _Minsubsamp[fc.compress]>=0
		&& _Maxsubsamp[fc.compress]>=0)
	{
		if(fc.subsamp<_Minsubsamp[fc.compress]
			|| fc.subsamp>_Maxsubsamp[fc.compress])
			fc.subsamp=_Defsubsamp[fc.compress];
	}
}

#define prconfint(i) rrout.println(#i"  =  %d", (int)fc.i)
#define prconfstr(s) rrout.println(#s"  =  \"%s\"", strlen(fc.s)>0? fc.s:"{Empty}")
#define prconfdbl(d) rrout.println(#d"  =  %f", fc.d)

void fconfig_print(FakerConfig &fc)
{
	prconfstr(client);
	prconfint(compress);
	prconfstr(config);
	prconfdbl(fps);
	prconfdbl(flushdelay);
	prconfdbl(gamma);
	prconfint(gamma_usesun);
	prconfstr(gllib);
	prconfint(glp);
	prconfint(gui);
	prconfint(guikey);
	prconfstr(guikeyseq);
	prconfint(guimod);
	prconfint(interframe);
	prconfstr(localdpystring);
	prconfstr(log);
	prconfint(logo);
	prconfint(mcompress);
	prconfstr(moviefile);
	prconfint(mqual);
	prconfint(msubsamp);
	prconfint(np);
	prconfint(port);
	prconfint(qual);
	prconfint(readback);
	prconfint(spoil);
	prconfint(ssl);
	prconfint(stereo);
	prconfint(subsamp);
	prconfint(sync);
	prconfint(tilesize);
	prconfint(trace);
	prconfint(transpixel);
	prconfint(transvalid[RRTRANS_X11]);
	prconfint(transvalid[RRTRANS_VGL]);
	prconfint(transvalid[RRTRANS_XV]);
	prconfint(trapx11);
	prconfint(usewindow);
	prconfstr(vendor);
	prconfint(verbose);
	prconfstr(x11lib);
}
