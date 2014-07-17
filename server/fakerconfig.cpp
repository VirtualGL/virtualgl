/* Copyright (C)2009-2014 D. R. Commander
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
#include "Error.h"
#include "Log.h"
#include "Mutex.h"
#include "fakerconfig.h"
#include <stdio.h>
#include <X11/keysym.h>
#if FCONFIG_USESHM==1
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#endif
#include "vglutil.h"
#include <X11/Xatom.h>
#ifdef USEXV
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#endif

using namespace vglutil;


#define DEFQUAL 95

static FakerConfig fconfig_env;
static bool fconfig_envset=false;

#if FCONFIG_USESHM==1
static int fconfig_shmid=-1;
int fconfig_getshmid(void) { return fconfig_shmid; }
#endif
static FakerConfig *fc=NULL;


/* This is a hack necessary to defer the initialization of the recursive mutex
   so MainWin will not interfere with it */

class DeferredCS : CriticalSection
{
	public:
		DeferredCS() : isInit(false) {}

		DeferredCS *init(void)
		{
			if(!isInit)
			{
				isInit=true;
				pthread_mutexattr_t ma;
				pthread_mutexattr_init(&ma);
				pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
				pthread_mutex_init(&mutex, &ma);
				pthread_mutexattr_destroy(&ma);
			}
			return this;
		}

	private:

		bool isInit;
};

static DeferredCS fconfig_mutex;
#define fcmutex ((CriticalSection &)(*fconfig_mutex.init()))


static void fconfig_init(void);


FakerConfig *fconfig_instance(void)
{
	if(fc==NULL)
	{
		CriticalSection::SafeLock l(fcmutex);
		if(fc==NULL)
		{
			#if FCONFIG_USESHM==1

			void *addr=NULL;
			if((fconfig_shmid=shmget(IPC_PRIVATE, sizeof(FakerConfig),
				IPC_CREAT|0600))==-1)
				_throwunix();
			if((addr=shmat(fconfig_shmid, 0, 0))==(void *)-1) _throwunix();
			if(!addr)
				_throw("Could not attach to config structure in shared memory");
			#ifdef linux
			shmctl(fconfig_shmid, IPC_RMID, 0);
			#endif
			char *env=NULL;
			if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
				&& !strncmp(env, "1", 1))
				vglout.println("[VGL] Shared memory segment ID for vglconfig: %d",
					fconfig_shmid);
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
		CriticalSection::SafeLock l(fcmutex, false);
		if(fc!=NULL)
		{
			#if FCONFIG_USESHM==1

			shmdt((char *)fc);
			if(fconfig_shmid!=-1)
			{
				int ret=shmctl(fconfig_shmid, IPC_RMID, 0);
				char *env=NULL;
				if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
					&& !strncmp(env, "1", 1) && ret!=-1)
					vglout.println("[VGL] Removed shared memory segment %d",
						fconfig_shmid);
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
	CriticalSection::SafeLock l(fcmutex);
	memset(&fconfig, 0, sizeof(FakerConfig));
	memset(&fconfig_env, 0, sizeof(FakerConfig));
	fconfig.compress=-1;
	strncpy(fconfig.config, VGLCONFIG_PATH, MAXSTR);
	fconfig.forcealpha=0;
	fconfig_setgamma(fconfig, 1.0);
	fconfig.glflushtrigger=1;
	fconfig.gui=1;
	fconfig.guikey=XK_F9;
	fconfig.guimod=ShiftMask|ControlMask;
	fconfig.interframe=1;
	strncpy(fconfig.localdpystring, ":0", MAXSTR);
	fconfig.np=1;
	fconfig.port=-1;
	fconfig.probeglx=1;
	fconfig.qual=DEFQUAL;
	fconfig.readback=RRREAD_PBO;
	fconfig.refreshrate=60.0;
	fconfig.samples=-1;
	fconfig.spoil=1;
	fconfig.spoillast=1;
	fconfig.stereo=RRSTEREO_QUADBUF;
	fconfig.subsamp=-1;
	fconfig.tilesize=RR_DEFAULTTILESIZE;
	fconfig.transpixel=-1;
	fconfig_reloadenv();
}


static void fconfig_buildlut(FakerConfig &fc)
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
			fc.gamma_lut16[i]=
				(unsigned short)(255.*pow((double)(i/256)/255., g)+0.5)<<8;
			fc.gamma_lut16[i]|=
				(unsigned short)(255.*pow((double)(i%256)/255., g)+0.5);
		}
	}
}


#define fetchenv_str(envvar, s) {  \
	if((env=getenv(envvar))!=NULL && strlen(env)>0  \
		&& (!fconfig_envset || strncmp(env, fconfig_env.s, MAXSTR-1))) {  \
		strncpy(fconfig.s, env, MAXSTR-1);  \
		strncpy(fconfig_env.s, env, MAXSTR-1);  \
	}  \
}

#define fetchenv_bool(envvar, b) {  \
	if((env=getenv(envvar))!=NULL && strlen(env)>0) {  \
		if(!strncmp(env, "1", 1) && (!fconfig_envset || fconfig_env.b!=1))  \
			fconfig.b=fconfig_env.b=1;  \
		else if(!strncmp(env, "0", 1) && (!fconfig_envset || fconfig_env.b!=0))  \
			fconfig.b=fconfig_env.b=0;  \
	}  \
}

#define fetchenv_int(envvar, i, min, max) {  \
	if((env=getenv(envvar))!=NULL && strlen(env)>0) {  \
		char *t=NULL;  int itemp=strtol(env, &t, 10);  \
		if(t && t!=env && itemp>=min && itemp<=max  \
			&& (!fconfig_envset || itemp!=fconfig_env.i))  \
			fconfig.i=fconfig_env.i=itemp;  \
	}  \
}

#define fetchenv_dbl(envvar, d, min, max) {  \
	char *temp=NULL;  \
	if((temp=getenv(envvar))!=NULL && strlen(temp)>0) {  \
		char *t=NULL;  double dtemp=strtod(temp, &t);  \
		if(t && t!=temp && dtemp>=min && dtemp<=max  \
			&& (!fconfig_envset || dtemp!=fconfig_env.d))  \
			fconfig.d=fconfig_env.d=dtemp;  \
	}  \
}


void fconfig_setgamma(FakerConfig &fc, double gamma)
{
	fc.gamma=gamma;
	fconfig_buildlut(fc);
}


void fconfig_reloadenv(void)
{
	char *env;

	CriticalSection::SafeLock l(fcmutex);

	fetchenv_bool("VGL_ALLOWINDIRECT", allowindirect);
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
		if(subsamp>=0 && (!fconfig_envset || fconfig_env.subsamp!=subsamp))
			fconfig.subsamp=fconfig_env.subsamp=subsamp;
	}
	fetchenv_str("VGL_TRANSPORT", transport);
	if((env=getenv("VGL_COMPRESS"))!=NULL && strlen(env)>0)
	{
		char *t=NULL;  int itemp=strtol(env, &t, 10);
		int compress=-1;
		if(t && t!=env && itemp>=0
			&& (itemp<RR_COMPRESSOPT || strlen(fconfig.transport)>0))
			compress=itemp;
		else if(!strnicmp(env, "p", 1)) compress=RRCOMP_PROXY;
		else if(!strnicmp(env, "j", 1)) compress=RRCOMP_JPEG;
		else if(!strnicmp(env, "r", 1)) compress=RRCOMP_RGB;
		else if(!strnicmp(env, "x", 1)) compress=RRCOMP_XV;
		else if(!strnicmp(env, "y", 1)) compress=RRCOMP_YUV;
		if(compress>=0 && (!fconfig_envset || fconfig_env.compress!=compress))
		{
			fconfig_setcompress(fconfig, compress);
			fconfig_env.compress=compress;
		}
	}
	fetchenv_str("VGL_CONFIG", config);
	fetchenv_str("VGL_DEFAULTFBCONFIG", defaultfbconfig);
	if((env=getenv("VGL_DISPLAY"))!=NULL && strlen(env)>0)
	{
		if(!fconfig_envset || strncmp(env, fconfig_env.localdpystring, MAXSTR-1))
		{
			strncpy(fconfig.localdpystring, env, MAXSTR-1);
			strncpy(fconfig_env.localdpystring, env, MAXSTR-1);
		}
	}
	if((env=getenv("VGL_DRAWABLE"))!=NULL && strlen(env)>0)
	{
		int drawable=-1;
		if(!strnicmp(env, "PB", 2)) drawable=RRDRAWABLE_PBUFFER;
		else if(!strnicmp(env, "PI", 2)) drawable=RRDRAWABLE_PIXMAP;
		else
		{
			char *t=NULL;  int itemp=strtol(env, &t, 10);
			if(t && t!=env && itemp>=0 && itemp<RR_DRAWABLEOPT) drawable=itemp;
		}
		if(drawable>=0 && (!fconfig_envset || fconfig_env.drawable!=drawable))
			fconfig.drawable=fconfig_env.drawable=drawable;
	}
	fetchenv_bool("VGL_FORCEALPHA", forcealpha);
	fetchenv_dbl("VGL_FPS", fps, 0.0, 1000000.0);
	if((env=getenv("VGL_GAMMA"))!=NULL && strlen(env)>0)
	{
		if(!strcmp(env, "1"))
		{
			if(!fconfig_envset || fconfig_env.gamma!=2.22)
			{
				fconfig_env.gamma=2.22;
				fconfig_setgamma(fconfig, 2.22);
			}
		}
		else if(!strcmp(env, "0"))
		{
			if(!fconfig_envset || fconfig_env.gamma!=1.0)
			{
				fconfig_env.gamma=1.0;
				fconfig_setgamma(fconfig, 1.0);
			}
		}
		else
		{
			char *t=NULL;  double dtemp=strtod(env, &t);
			if(t && t!=env
				&& (!fconfig_envset || fconfig_env.gamma!=dtemp))
			{
				fconfig_env.gamma=dtemp;
				fconfig_setgamma(fconfig, dtemp);
			}
		}
	}
	fetchenv_bool("VGL_GLFLUSHTRIGGER", glflushtrigger);
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
	fetchenv_int("VGL_NPROCS", np, 1, min(numprocs(), MAXPROCS));
	fetchenv_int("VGL_PORT", port, 0, 65535);
	fetchenv_bool("VGL_PROBEGLX", probeglx);
	fetchenv_int("VGL_QUAL", qual, 1, 100);
	if((env=getenv("VGL_READBACK"))!=NULL && strlen(env)>0)
	{
		int readback=-1;
		if(!strnicmp(env, "N", 1)) readback=RRREAD_NONE;
		else if(!strnicmp(env, "P", 1)) readback=RRREAD_PBO;
		else if(!strnicmp(env, "S", 1)) readback=RRREAD_SYNC;
		else
		{
			char *t=NULL;  int itemp=strtol(env, &t, 10);
			if(t && t!=env && itemp>=0 && itemp<RR_READBACKOPT) readback=itemp;
		}
		if(readback>=0 && (!fconfig_envset || fconfig_env.readback!=readback))
			fconfig.readback=fconfig_env.readback=readback;
	}
	fetchenv_dbl("VGL_REFRESHRATE", refreshrate, 0.0, 1000000.0);
	fetchenv_int("VGL_SAMPLES", samples, 0, 64);
	fetchenv_bool("VGL_SPOIL", spoil);
	fetchenv_bool("VGL_SPOILLAST", spoillast);
	fetchenv_bool("VGL_SSL", ssl);
	{
		if((env=getenv("VGL_STEREO"))!=NULL && strlen(env)>0)
		{
			int stereo=-1;
			if(!strnicmp(env, "L", 1)) stereo=RRSTEREO_LEYE;
			else if(!strnicmp(env, "RC", 2)) stereo=RRSTEREO_REDCYAN;
			else if(!strnicmp(env, "R", 1)) stereo=RRSTEREO_REYE;
			else if(!strnicmp(env, "Q", 1)) stereo=RRSTEREO_QUADBUF;
			else if(!strnicmp(env, "GM", 2)) stereo=RRSTEREO_GREENMAGENTA;
			else if(!strnicmp(env, "BY", 2)) stereo=RRSTEREO_BLUEYELLOW;
			else if(!strnicmp(env, "I", 2)) stereo=RRSTEREO_INTERLEAVED;
			else if(!strnicmp(env, "TB", 2)) stereo=RRSTEREO_TOPBOTTOM;
			else if(!strnicmp(env, "SS", 2)) stereo=RRSTEREO_SIDEBYSIDE;
			else
			{
				char *t=NULL;  int itemp=strtol(env, &t, 10);
				if(t && t!=env && itemp>=0 && itemp<RR_STEREOOPT) stereo=itemp;
			}
			if(stereo>=0 && (!fconfig_envset || fconfig_env.stereo!=stereo))
				fconfig.stereo=fconfig_env.stereo=stereo;
		}
	}
	fetchenv_bool("VGL_SYNC", sync);
	fetchenv_int("VGL_TILESIZE", tilesize, 8, 1024);
	fetchenv_bool("VGL_TRACE", trace);
	fetchenv_int("VGL_TRANSPIXEL", transpixel, 0, 255);
	fetchenv_bool("VGL_TRAPX11", trapx11);
	fetchenv_str("VGL_XVENDOR", vendor);
	fetchenv_bool("VGL_VERBOSE", verbose);
	fetchenv_bool("VGL_WM", wm);
	fetchenv_str("VGL_X11LIB", x11lib);

	if(strlen(fconfig.transport)>0)
	{
		if(fconfig.compress<0) fconfig.compress=0;
		if(fconfig.subsamp<0) fconfig.subsamp=1;
	}

	fconfig_envset=true;
}


void fconfig_setdefaultsfromdpy(Display *dpy)
{
	CriticalSection::SafeLock l(fcmutex);

	if(fconfig.compress<0)
	{
		bool useSunRay=false;
		Atom atom=None;
		if((atom=XInternAtom(dpy, "_SUN_SUNRAY_SESSION", True))!=None)
			useSunRay=true;
		const char *dstr=DisplayString(dpy);
		if((strlen(dstr) && dstr[0]==':') || (strlen(dstr)>5
			&& !strnicmp(dstr, "unix", 4)))
		{
			if(useSunRay) fconfig_setcompress(fconfig, RRCOMP_XV);
			else fconfig_setcompress(fconfig, RRCOMP_PROXY);
		}
		else
		{
			if(useSunRay) fconfig_setcompress(fconfig, RRCOMP_YUV);
			else fconfig_setcompress(fconfig, RRCOMP_JPEG);
		}
	}

	if(fconfig.port<0)
	{
		fconfig.port=fconfig.ssl? RR_DEFAULTSSLPORT:RR_DEFAULTPORT;
		Atom atom=None;  unsigned long n=0, bytesLeft=0;
		int actualFormat=0;  Atom actualType=None;
		unsigned char *prop=NULL;
		if((atom=XInternAtom(dpy,
			fconfig.ssl? "_VGLCLIENT_SSLPORT":"_VGLCLIENT_PORT", True))!=None)
		{
			if(XGetWindowProperty(dpy, RootWindow(dpy, DefaultScreen(dpy)), atom,
				0, 1, False, XA_INTEGER, &actualType, &actualFormat, &n,
				&bytesLeft, &prop)==Success && n>=1 && actualFormat==16
				&& actualType==XA_INTEGER && prop)
				fconfig.port=*(unsigned short *)prop;
			if(prop) XFree(prop);
		}
	}

	#ifdef USEXV

	int k, port, nformats, dummy1, dummy2, dummy3;
	unsigned int i, j, nadaptors=0;
	XvAdaptorInfo *ai=NULL;
	XvImageFormatValues *ifv=NULL;

	if(XQueryExtension(dpy, "XVideo", &dummy1, &dummy2, &dummy3)
		&& XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &nadaptors,
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
	CriticalSection::SafeLock l(fcmutex);

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


#define prconfint(i) vglout.println(#i"  =  %d", (int)fc.i)
#define prconfstr(s)  \
	vglout.println(#s"  =  \"%s\"", strlen(fc.s)>0? fc.s:"{Empty}")
#define prconfdbl(d) vglout.println(#d"  =  %f", fc.d)

void fconfig_print(FakerConfig &fc)
{
	prconfint(allowindirect);
	prconfstr(client);
	prconfint(compress);
	prconfstr(config);
	prconfstr(defaultfbconfig);
	prconfint(drawable);
	prconfdbl(fps);
	prconfdbl(flushdelay);
	prconfint(forcealpha);
	prconfdbl(gamma);
	prconfint(glflushtrigger);
	prconfstr(gllib);
	prconfint(gui);
	prconfint(guikey);
	prconfstr(guikeyseq);
	prconfint(guimod);
	prconfint(interframe);
	prconfstr(localdpystring);
	prconfstr(log);
	prconfint(logo);
	prconfint(np);
	prconfint(port);
	prconfint(qual);
	prconfint(readback);
	prconfint(samples);
	prconfint(spoil);
	prconfint(spoillast);
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
	prconfstr(vendor);
	prconfint(verbose);
	prconfint(wm);
	prconfstr(x11lib);
}
