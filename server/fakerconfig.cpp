// Copyright (C)2009-2023 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

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
#if FCONFIG_USESHM == 1
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
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif

using namespace util;


#define DEFQUAL  95

static FakerConfig fconfig_env;
static bool fconfig_envset = false;

#if FCONFIG_USESHM == 1
static int fconfig_shmid = -1;
int fconfig_getshmid(void) { return fconfig_shmid; }
#endif
static FakerConfig *fconfig_instance = NULL;


// This is a hack necessary to defer the initialization of the recursive mutex
// so MainWin will not interfere with it

class DeferredCS : CriticalSection
{
	public:
		DeferredCS() : isInit(false) {}

		DeferredCS *init(void)
		{
			if(!isInit)
			{
				isInit = true;
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
#define fcmutex  ((CriticalSection &)(*fconfig_mutex.init()))


static void fconfig_init(void);


FakerConfig *fconfig_getinstance(void)
{
	if(fconfig_instance == NULL)
	{
		CriticalSection::SafeLock l(fcmutex);
		if(fconfig_instance == NULL)
		{
			#if FCONFIG_USESHM == 1

			void *addr = NULL;
			if((fconfig_shmid = shmget(IPC_PRIVATE, sizeof(FakerConfig),
				IPC_CREAT | 0600)) == -1)
				THROW_UNIX();
			if((addr = shmat(fconfig_shmid, 0, 0)) == (void *)-1) THROW_UNIX();
			if(!addr)
				THROW("Could not attach to config structure in shared memory");
			#ifndef sun
			shmctl(fconfig_shmid, IPC_RMID, 0);
			#endif
			char *env = NULL;
			if((env = getenv("VGL_VERBOSE")) != NULL && strlen(env) > 0
				&& !strncmp(env, "1", 1))
				vglout.println("[VGL] Shared memory segment ID for vglconfig: %d",
					fconfig_shmid);
			fconfig_instance = (FakerConfig *)addr;

			#else

			fconfig_instance = new FakerConfig;
			if(!fconfig_instance) THROW("Could not allocate config structure");

			#endif

			fconfig_init();
		}
	}
	return fconfig_instance;
}


void fconfig_deleteinstance(CriticalSection *mutex)
{
	if(fconfig_instance != NULL)
	{
		CriticalSection::SafeLock l(mutex ? *mutex : fcmutex, false);
		if(fconfig_instance != NULL)
		{
			#if FCONFIG_USESHM == 1

			shmdt((char *)fconfig_instance);
			if(fconfig_shmid != -1)
			{
				int ret = shmctl(fconfig_shmid, IPC_RMID, 0);
				char *env = NULL;
				if((env = getenv("VGL_VERBOSE")) != NULL && strlen(env) > 0
					&& !strncmp(env, "1", 1) && ret != -1)
					vglout.println("[VGL] Removed shared memory segment %d",
						fconfig_shmid);
			}

			#else

			delete fconfig_instance;

			#endif

			fconfig_instance = NULL;
		}
	}
}


static void fconfig_init(void)
{
	CriticalSection::SafeLock l(fcmutex);
	memset(&fconfig, 0, sizeof(FakerConfig));
	memset(&fconfig_env, 0, sizeof(FakerConfig));
	fconfig.compress = -1;
	strncpy(fconfig.config, VGLCONFIG_PATH, MAXSTR);
	#ifdef sun
	fconfig.dlsymloader = true;
	#endif
	#ifdef FAKEXCB
	fconfig.fakeXCB = 1;
	#endif
	fconfig.forcealpha = 0;
	fconfig_setgamma(fconfig, 1.0);
	fconfig.glflushtrigger = 1;
	fconfig.gui = 1;
	fconfig.guikey = XK_F9;
	fconfig.guimod = ShiftMask | ControlMask;
	fconfig.interframe = 1;
	strncpy(fconfig.localdpystring, ":0", MAXSTR);
	fconfig.np = 1;
	fconfig.port = -1;
	fconfig.probeglx = -1;
	fconfig.qual = DEFQUAL;
	fconfig.readback = RRREAD_PBO;
	fconfig.refreshrate = 60.0;
	fconfig.samples = -1;
	fconfig.spoil = 1;
	fconfig.spoillast = 1;
	fconfig.stereo = RRSTEREO_QUADBUF;
	fconfig.subsamp = -1;
	fconfig.tilesize = RR_DEFAULTTILESIZE;
	fconfig.transpixel = -1;
	fconfig_reloadenv();
	#ifdef USEHELGRIND
	ANNOTATE_BENIGN_RACE_SIZED(&fconfig.egl, sizeof(bool), );
	ANNOTATE_BENIGN_RACE_SIZED(&fconfig.flushdelay, sizeof(double), );
	#endif
}


static void fconfig_buildlut(FakerConfig &fc)
{
	if(fc.gamma != 0.0 && fc.gamma != 1.0 && fc.gamma != -1.0)
	{
		double g = fc.gamma > 0.0 ? 1.0 / fc.gamma : -fc.gamma;
		for(int i = 0; i < 256; i++)
			fc.gamma_lut[i] = (unsigned char)(255. * pow((double)i / 255., g) + 0.5);
		for(int i = 0; i < 1024; i++)
			fc.gamma_lut10[i] =
				(unsigned short)(1023. * pow((double)i / 1023., g) + 0.5);
		for(int i = 0; i < 65536; i++)
		{
			fc.gamma_lut16[i] =
				(unsigned short)(255. * pow((double)(i / 256) / 255., g) + 0.5) << 8;
			fc.gamma_lut16[i] |=
				(unsigned short)(255. * pow((double)(i % 256) / 255., g) + 0.5);
		}
	}
}


#define FETCHENV_STR(envvar, s) \
{ \
	if((env = getenv(envvar)) != NULL && strlen(env) > 0 \
		&& (!fconfig_envset || strncmp(env, fconfig_env.s, MAXSTR - 1))) \
	{ \
		strncpy(fconfig.s, env, MAXSTR - 1); \
		strncpy(fconfig_env.s, env, MAXSTR - 1); \
	} \
}

#define FETCHENV_BOOL(envvar, b) \
{ \
	if((env = getenv(envvar)) != NULL && strlen(env) > 0) \
	{ \
		if(!strncmp(env, "1", 1) && (!fconfig_envset || fconfig_env.b != 1)) \
			fconfig.b = fconfig_env.b = 1; \
		else if(!strncmp(env, "0", 1) && (!fconfig_envset || fconfig_env.b != 0)) \
			fconfig.b = fconfig_env.b = 0; \
	} \
}

#define FETCHENV_INT(envvar, i, min, max) \
{ \
	if((env = getenv(envvar)) != NULL && strlen(env) > 0) \
	{ \
		char *t = NULL;  int itemp = strtol(env, &t, 10); \
		if(t && t != env && itemp >= min && itemp <= max \
			&& (!fconfig_envset || itemp != fconfig_env.i)) \
			fconfig.i = fconfig_env.i = itemp; \
	} \
}

#define FETCHENV_DBL(envvar, d, min, max) \
{ \
	char *temp = NULL; \
	if((temp = getenv(envvar)) != NULL && strlen(temp) > 0) \
	{ \
		char *t = NULL;  double dtemp = strtod(temp, &t); \
		if(t && t != temp && dtemp >= min && dtemp <= max \
			&& (!fconfig_envset || dtemp != fconfig_env.d)) \
			fconfig.d = fconfig_env.d = dtemp; \
	} \
}


void fconfig_setgamma(FakerConfig &fc, double gamma)
{
	fc.gamma = gamma;
	fconfig_buildlut(fc);
}


void fconfig_reloadenv(void)
{
	char *env;

	CriticalSection::SafeLock l(fcmutex);

	FETCHENV_BOOL("VGL_ALLOWINDIRECT", allowindirect);
	FETCHENV_BOOL("VGL_AMDGPUHACK", amdgpuHack);
	FETCHENV_BOOL("VGL_AUTOTEST", autotest);
	FETCHENV_STR("VGL_CLIENT", client);
	if((env = getenv("VGL_SUBSAMP")) != NULL && strlen(env) > 0)
	{
		int subsamp = -1;
		if(!strnicmp(env, "G", 1)) subsamp = 0;
		else
		{
			char *t = NULL;  int itemp = strtol(env, &t, 10);
			if(t && t != env)
			{
				switch(itemp)
				{
					case 0:                              subsamp = 0;  break;
					case 444: case 11: case 1:           subsamp = 1;  break;
					case 422: case 21: case 2:           subsamp = 2;  break;
					case 411: case 420: case 22: case 4: subsamp = 4;  break;
					case 410: case 42: case 8:           subsamp = 8;  break;
					case 44:  case 16:                   subsamp = 16;  break;
				}
			}
		}
		if(subsamp >= 0 && (!fconfig_envset || fconfig_env.subsamp != subsamp))
			fconfig.subsamp = fconfig_env.subsamp = subsamp;
	}
	FETCHENV_STR("VGL_TRANSPORT", transport);
	if((env = getenv("VGL_COMPRESS")) != NULL && strlen(env) > 0)
	{
		char *t = NULL;  int itemp = strtol(env, &t, 10);
		int compress = -1;
		if(t && t != env && itemp >= 0
			&& (itemp < RR_COMPRESSOPT || strlen(fconfig.transport) > 0))
			compress = itemp;
		else if(!strnicmp(env, "p", 1)) compress = RRCOMP_PROXY;
		else if(!strnicmp(env, "j", 1)) compress = RRCOMP_JPEG;
		else if(!strnicmp(env, "r", 1)) compress = RRCOMP_RGB;
		else if(!strnicmp(env, "x", 1)) compress = RRCOMP_XV;
		else if(!strnicmp(env, "y", 1)) compress = RRCOMP_YUV;
		if(compress >= 0 && (!fconfig_envset || fconfig_env.compress != compress))
		{
			fconfig_setcompress(fconfig, compress);
			fconfig_env.compress = compress;
		}
	}
	FETCHENV_STR("VGL_CONFIG", config);
	FETCHENV_STR("VGL_DEFAULTFBCONFIG", defaultfbconfig);
	if((env = getenv("VGL_DISPLAY")) != NULL && strlen(env) > 0)
	{
		if(!fconfig_envset || strncmp(env, fconfig_env.localdpystring, MAXSTR - 1))
		{
			strncpy(fconfig.localdpystring, env, MAXSTR - 1);
			strncpy(fconfig_env.localdpystring, env, MAXSTR - 1);
		}
		if((env[0] == '/' || !strnicmp(env, "EGL", 3)))
			fconfig.egl = true;
	}
	FETCHENV_BOOL("VGL_DLSYM", dlsymloader);
	#ifdef EGLBACKEND
	FETCHENV_STR("VGL_EGLLIB", egllib);
	// This is a hack to allow piglit tests to pass with the EGL/X11 front end,
	// which doesn't (yet) support Pixmap surfaces.
	FETCHENV_BOOL("VGL_EGLXIGNOREPIXMAPBIT", eglxIgnorePixmapBit);
	#endif
	FETCHENV_STR("VGL_EXCLUDE", excludeddpys);
	FETCHENV_STR("VGL_EXITFUNCTION", exitfunction);
	#ifdef FAKEXCB
	FETCHENV_BOOL("VGL_FAKEXCB", fakeXCB);
	#endif
	FETCHENV_BOOL("VGL_FORCEALPHA", forcealpha);
	FETCHENV_DBL("VGL_FPS", fps, 0.0, 1000000.0);
	if((env = getenv("VGL_GAMMA")) != NULL && strlen(env) > 0)
	{
		if(!strcmp(env, "1"))
		{
			if(!fconfig_envset || fconfig_env.gamma != 2.22)
			{
				fconfig_env.gamma = 2.22;
				fconfig_setgamma(fconfig, 2.22);
			}
		}
		else if(!strcmp(env, "0"))
		{
			if(!fconfig_envset || fconfig_env.gamma != 1.0)
			{
				fconfig_env.gamma = 1.0;
				fconfig_setgamma(fconfig, 1.0);
			}
		}
		else
		{
			char *t = NULL;  double dtemp = strtod(env, &t);
			if(t && t != env
				&& (!fconfig_envset || fconfig_env.gamma != dtemp))
			{
				fconfig_env.gamma = dtemp;
				fconfig_setgamma(fconfig, dtemp);
			}
		}
	}
	FETCHENV_BOOL("VGL_GLFLUSHTRIGGER", glflushtrigger);
	FETCHENV_STR("VGL_GLLIB", gllib);
	FETCHENV_STR("VGL_GLXVENDOR", glxvendor);
	FETCHENV_STR("VGL_GUI", guikeyseq);
	if(strlen(fconfig.guikeyseq) > 0)
	{
		if(!stricmp(fconfig.guikeyseq, "none")) fconfig.gui = false;
		else
		{
			unsigned int mod = 0, key = 0;
			for(unsigned int i = 0; i < strlen(fconfig.guikeyseq); i++)
				fconfig.guikeyseq[i] = tolower(fconfig.guikeyseq[i]);
			if(strstr(fconfig.guikeyseq, "ctrl")) mod |= ControlMask;
			if(strstr(fconfig.guikeyseq, "alt")) mod |= Mod1Mask;
			if(strstr(fconfig.guikeyseq, "shift")) mod |= ShiftMask;
			if(strstr(fconfig.guikeyseq, "f10")) key = XK_F10;
			else if(strstr(fconfig.guikeyseq, "f11")) key = XK_F11;
			else if(strstr(fconfig.guikeyseq, "f12")) key = XK_F12;
			else if(strstr(fconfig.guikeyseq, "f1")) key = XK_F1;
			else if(strstr(fconfig.guikeyseq, "f2")) key = XK_F2;
			else if(strstr(fconfig.guikeyseq, "f3")) key = XK_F3;
			else if(strstr(fconfig.guikeyseq, "f4")) key = XK_F4;
			else if(strstr(fconfig.guikeyseq, "f5")) key = XK_F5;
			else if(strstr(fconfig.guikeyseq, "f6")) key = XK_F6;
			else if(strstr(fconfig.guikeyseq, "f7")) key = XK_F7;
			else if(strstr(fconfig.guikeyseq, "f8")) key = XK_F8;
			else if(strstr(fconfig.guikeyseq, "f9")) key = XK_F9;
			if(key) fconfig.guikey = key;
			fconfig.guimod = mod;
			fconfig.gui = true;
		}
	}
	FETCHENV_BOOL("VGL_INTERFRAME", interframe);
	FETCHENV_STR("VGL_LOG", log);
	FETCHENV_BOOL("VGL_LOGO", logo);
	FETCHENV_INT("VGL_NPROCS", np, 1, min(NumProcs(), MAXPROCS));
	#ifdef FAKEOPENCL
	FETCHENV_STR("VGL_OCLLIB", ocllib);
	#endif
	FETCHENV_INT("VGL_PORT", port, 0, 65535);
	FETCHENV_BOOL("VGL_PROBEGLX", probeglx);
	FETCHENV_INT("VGL_QUAL", qual, 1, 100);
	if((env = getenv("VGL_READBACK")) != NULL && strlen(env) > 0)
	{
		int readback = -1;
		if(!strnicmp(env, "N", 1)) readback = RRREAD_NONE;
		else if(!strnicmp(env, "P", 1)) readback = RRREAD_PBO;
		else if(!strnicmp(env, "S", 1)) readback = RRREAD_SYNC;
		else
		{
			char *t = NULL;  int itemp = strtol(env, &t, 10);
			if(t && t != env && itemp >= 0 && itemp < RR_READBACKOPT)
				readback = itemp;
		}
		if(readback >= 0 && (!fconfig_envset || fconfig_env.readback != readback))
			fconfig.readback = fconfig_env.readback = readback;
	}
	FETCHENV_DBL("VGL_REFRESHRATE", refreshrate, 0.0, 1000000.0);
	FETCHENV_INT("VGL_SAMPLES", samples, 0, 64);
	FETCHENV_BOOL("VGL_SPOIL", spoil);
	FETCHENV_BOOL("VGL_SPOILLAST", spoillast);
	{
		if((env = getenv("VGL_STEREO")) != NULL && strlen(env) > 0)
		{
			int stereo = -1;
			if(!strnicmp(env, "L", 1)) stereo = RRSTEREO_LEYE;
			else if(!strnicmp(env, "RC", 2)) stereo = RRSTEREO_REDCYAN;
			else if(!strnicmp(env, "R", 1)) stereo = RRSTEREO_REYE;
			else if(!strnicmp(env, "Q", 1)) stereo = RRSTEREO_QUADBUF;
			else if(!strnicmp(env, "GM", 2)) stereo = RRSTEREO_GREENMAGENTA;
			else if(!strnicmp(env, "BY", 2)) stereo = RRSTEREO_BLUEYELLOW;
			else if(!strnicmp(env, "I", 2)) stereo = RRSTEREO_INTERLEAVED;
			else if(!strnicmp(env, "TB", 2)) stereo = RRSTEREO_TOPBOTTOM;
			else if(!strnicmp(env, "SS", 2)) stereo = RRSTEREO_SIDEBYSIDE;
			else
			{
				char *t = NULL;  int itemp = strtol(env, &t, 10);
				if(t && t != env && itemp >= 0 && itemp < RR_STEREOOPT) stereo = itemp;
			}
			if(stereo >= 0 && (!fconfig_envset || fconfig_env.stereo != stereo))
				fconfig.stereo = fconfig_env.stereo = stereo;
		}
	}
	FETCHENV_BOOL("VGL_SYNC", sync);
	FETCHENV_INT("VGL_TILESIZE", tilesize, 8, 1024);
	FETCHENV_BOOL("VGL_TRACE", trace);
	FETCHENV_INT("VGL_TRANSPIXEL", transpixel, 0, 255);
	FETCHENV_BOOL("VGL_TRAPX11", trapx11);
	FETCHENV_STR("VGL_XVENDOR", vendor);
	FETCHENV_BOOL("VGL_VERBOSE", verbose);
	FETCHENV_BOOL("VGL_WM", wm);
	FETCHENV_STR("VGL_X11LIB", x11lib);
	#ifdef FAKEXCB
	FETCHENV_STR("VGL_XCBLIB", xcblib);
	FETCHENV_STR("VGL_XCBGLXLIB", xcbglxlib);
	FETCHENV_STR("VGL_XCBKEYSYMSLIB", xcbkeysymslib);
	FETCHENV_STR("VGL_XCBX11LIB", xcbkeysymslib);
	#endif

	if(strlen(fconfig.transport) > 0)
	{
		if(fconfig.compress < 0) fconfig.compress = 0;
		if(fconfig.subsamp < 0) fconfig.subsamp = 1;
	}

	fconfig_envset = true;
}


static void fconfig_setcompressfromdpy(Display *dpy, FakerConfig &fc)
{
	if(fc.compress < 0)
	{
		bool useSunRay = false;
		Atom atom = None;
		if((atom = XInternAtom(dpy, "_SUN_SUNRAY_SESSION", True)) != None)
			useSunRay = true;
		const char *dstr = DisplayString(dpy);
		if((strlen(dstr) && dstr[0] == ':') || (strlen(dstr) > 5
			&& !strnicmp(dstr, "unix", 4)))
		{
			if(useSunRay) fconfig_setcompress(fc, RRCOMP_XV);
			else fconfig_setcompress(fc, RRCOMP_PROXY);
		}
		else
		{
			if(useSunRay) fconfig_setcompress(fc, RRCOMP_YUV);
			else fconfig_setcompress(fc, RRCOMP_JPEG);
		}
	}
}


void fconfig_setdefaultsfromdpy(Display *dpy)
{
	CriticalSection::SafeLock l(fcmutex);

	fconfig_setcompressfromdpy(dpy, fconfig);

	if(fconfig.port < 0)
	{
		fconfig.port = RR_DEFAULTPORT;
		Atom atom = None;  unsigned long n = 0, bytesLeft = 0;
		int actualFormat = 0;  Atom actualType = None;
		unsigned char *prop = NULL;
		if((atom = XInternAtom(dpy, "_VGLCLIENT_PORT", True)) != None)
		{
			if(XGetWindowProperty(dpy, RootWindow(dpy, DefaultScreen(dpy)), atom, 0,
				1, False, XA_INTEGER, &actualType, &actualFormat, &n, &bytesLeft,
				&prop) == Success && n >= 1 && actualFormat == 16
				&& actualType == XA_INTEGER && prop)
				fconfig.port = *(unsigned short *)prop;
			if(prop) XFree(prop);
		}
	}

	#ifdef USEXV

	int k, port, nformats, dummy1, dummy2, dummy3;
	unsigned int i, j, nadaptors = 0;
	XvAdaptorInfo *ai = NULL;
	XvImageFormatValues *ifv = NULL;

	if(XQueryExtension(dpy, "XVideo", &dummy1, &dummy2, &dummy3)
		&& XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &nadaptors,
		&ai) == Success && nadaptors >= 1 && ai)
	{
		port = -1;
		for(i = 0; i < nadaptors; i++)
		{
			for(j = ai[i].base_id; j < ai[i].base_id + ai[i].num_ports; j++)
			{
				nformats = 0;
				ifv = XvListImageFormats(dpy, j, &nformats);
				if(ifv && nformats > 0)
				{
					for(k = 0; k < nformats; k++)
					{
						if(ifv[k].id == 0x30323449)
						{
							XFree(ifv);  port = j;
							goto found;
						}
					}
				}
				XFree(ifv);
			}
		}
		found:
		XvFreeAdaptorInfo(ai);  ai = NULL;
		if(port != -1) fconfig.transvalid[RRTRANS_XV] = 1;
	}

	#endif
}


void fconfig_setcompress(FakerConfig &fc, int i)
{
	if(i < 0 || (i >= RR_COMPRESSOPT && strlen(fc.transport) == 0)) return;
	CriticalSection::SafeLock l(fcmutex);

	bool is = (fc.compress >= 0);
	fc.compress = i;
	if(strlen(fc.transport) > 0) return;
	if(!is) fc.transvalid[_Trans[fc.compress]] = fc.transvalid[RRTRANS_X11] = 1;
	if(fc.subsamp < 0) fc.subsamp = _Defsubsamp[fc.compress];
	if(strlen(fc.transport) == 0 && _Minsubsamp[fc.compress] >= 0
		&& _Maxsubsamp[fc.compress] >= 0)
	{
		if(fc.subsamp < _Minsubsamp[fc.compress]
			|| fc.subsamp > _Maxsubsamp[fc.compress])
			fc.subsamp = _Defsubsamp[fc.compress];
	}
}


void fconfig_setprobeglxfromdpy(Display *dpy)
{
	CriticalSection::SafeLock l(fcmutex);

	if(fconfig.probeglx < 0)
	{
		FakerConfig fc = fconfig;

		fconfig_setcompressfromdpy(dpy, fc);

		if(strlen(fc.transport) != 0 || fc.transvalid[RRTRANS_VGL] == 1)
			fconfig.probeglx = 1;
		else
			fconfig.probeglx = 0;
	}
}


#define PRCONF_INT(i)  vglout.println(#i "  =  %d", fc.i)
#define PRCONF_STR(s) \
	vglout.println(#s "  =  \"%s\"", strlen(fc.s) > 0 ? fc.s : "{Empty}")
#define PRCONF_DBL(d)  vglout.println(#d "  =  %f", fc.d)

void fconfig_print(FakerConfig &fc)
{
	PRCONF_INT(allowindirect);
	PRCONF_INT(amdgpuHack);
	PRCONF_STR(client);
	PRCONF_INT(compress);
	PRCONF_STR(config);
	PRCONF_STR(defaultfbconfig);
	PRCONF_INT(dlsymloader);
	#ifdef EGLBACKEND
	PRCONF_INT(egl);
	PRCONF_STR(egllib);
	#endif
	PRCONF_STR(excludeddpys);
	PRCONF_DBL(fps);
	PRCONF_DBL(flushdelay);
	PRCONF_INT(forcealpha);
	PRCONF_DBL(gamma);
	PRCONF_INT(glflushtrigger);
	PRCONF_STR(gllib);
	PRCONF_STR(glxvendor);
	PRCONF_INT(gui);
	PRCONF_INT(guikey);
	PRCONF_STR(guikeyseq);
	PRCONF_INT(guimod);
	PRCONF_INT(interframe);
	PRCONF_STR(localdpystring);
	PRCONF_STR(log);
	PRCONF_INT(logo);
	PRCONF_INT(np);
	#ifdef FAKEOPENCL
	PRCONF_STR(ocllib);
	#endif
	PRCONF_INT(port);
	PRCONF_INT(qual);
	PRCONF_INT(readback);
	PRCONF_INT(samples);
	PRCONF_INT(spoil);
	PRCONF_INT(spoillast);
	PRCONF_INT(stereo);
	PRCONF_INT(subsamp);
	PRCONF_INT(sync);
	PRCONF_INT(tilesize);
	PRCONF_INT(trace);
	PRCONF_INT(transpixel);
	PRCONF_INT(transvalid[RRTRANS_X11]);
	PRCONF_INT(transvalid[RRTRANS_VGL]);
	PRCONF_INT(transvalid[RRTRANS_XV]);
	PRCONF_INT(trapx11);
	PRCONF_STR(vendor);
	PRCONF_INT(verbose);
	PRCONF_INT(wm);
	PRCONF_STR(x11lib);
	#ifdef FAKEXCB
	PRCONF_STR(xcblib);
	PRCONF_STR(xcbglxlib);
	PRCONF_STR(xcbkeysymslib);
	PRCONF_STR(xcbx11lib);
	#endif
}
