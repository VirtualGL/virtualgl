/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#ifndef __FAKER_CONFIG_H
#define __FAKER_CONFIG_H

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "rr.h"
#include "rrlog.h"
#include "rrsunray.h"
#include <stdio.h>
#include <X11/X.h>
#include <X11/keysym.h>
#ifndef _WIN32
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#endif
#include "rrutil.h"

#define DEFQUAL 95

#define MAXSTR 256

class Config
{
	public:

		Config() {_set=false;  _env=NULL;}
		~Config() {if(_env) {free(_env);  _env=NULL;}}
		bool isset(void) {return _set;}

		char *envget(const char *envvar)
		{
			char *temp=NULL;
			if((temp=getenv(envvar))!=NULL && strlen(temp)>0 && newenv(temp))
				return temp;
			else return NULL;
		}

	protected:

		bool newenv(char *env)
		{
			if(_env)
			{
				if(strlen(env)==strlen(_env) && !strncmp(env, _env, strlen(env)))
					return false;
				free(_env);  _env=NULL;
			}
			_env=strdup(env);
			return true;
		}

		bool _set;
		char *_env;
};

class ConfigBool : public Config
{
	public:

		ConfigBool() {_b=false;}
		ConfigBool& operator= (bool b) {set(b);  return *this;}
		bool operator= (ConfigBool &cb) {return cb._b;}
		operator bool() const {return _b;}

		void get(const char *envvar)
		{
			char *temp=NULL;
			if((temp=envget(envvar))!=NULL)
			{
				if(!strncmp(temp, "1", 1)) set(true);
				else if(!strncmp(temp, "0", 1)) set(false);
			}
		}

		void set(bool b)
		{
			_b=b;  _set=true;
		}

	private:

		bool _b;
};


class ConfigDouble : public Config
{
	public:

		ConfigDouble() {_d=0.;  _usebounds=false;  _min=_max=0.;}
		ConfigDouble& operator= (double d) {set(d);  return *this;}
		double operator= (ConfigDouble &cd) {return cd._d;}
		operator double() const {return _d;}

		void setbounds(double min, double max)
		{
			_min=min;  _max=max;  _usebounds=true;
		}

		void get(const char *envvar)
		{
			char *temp=NULL;
			if((temp=envget(envvar))!=NULL)
			{
				char *t=NULL;  double dtemp=strtod(temp, &t);
				if(t && t!=temp) set(dtemp);
			}
		}

		void set(double d)
		{
			if((d>=_min && d<=_max) || !_usebounds)
			{
				_d=d;  _set=true;
			}
		}

	protected:

		double _d, _min, _max;
		bool _usebounds;
};

class ConfigInt : public Config
{
	public:

		ConfigInt() {_i=0;  _usebounds=false;  _min=_max=0;  _pow2=false;}
		ConfigInt& operator= (int i) {set(i);  return *this;}
		int operator= (ConfigInt &ci) {return ci._i;}
		operator int() const {return _i;}

		void setbounds(int min, int max)
		{
			_min=min;  _max=max;  _usebounds=true;
		}

		void setpow2(void) {_pow2=true;}

		void get(const char *envvar)
		{
			char *temp=NULL;
			if((temp=envget(envvar))!=NULL)
			{
				char *t=NULL;  int itemp=strtol(temp, &t, 10);
				if(t && t!=temp) set(itemp);
			}
		}

		void set(int i)
		{
			if((!_usebounds || (i>=_min && i<=_max))
				&& (!_pow2 || isPow2(i)))
			{
				_i=i;  _set=true;
			}
		}

	protected:

		int _i, _min, _max;
		bool _usebounds, _pow2;
};

class ConfigString : public Config
{
	public:

		ConfigString() {_s=NULL;}
		ConfigString& operator= (char *s) {set(s);  return *this;}
		char* operator= (ConfigString &cs) {return cs._s;}
		~ConfigString() {if(_s) free(_s);}
		operator char*() const {return _s;}

		void get(const char *envvar)
		{
			char *temp=NULL;
			if((temp=envget(envvar))!=NULL)
			{
				for(int i=0; i<(int)strlen(temp); i++)
					if(temp[i]!=' ' && temp[i]!='\t')
					{
						temp=&temp[i];  break;
					}
				if(strlen(temp)>0) set(temp);
			}
		}

		void set(char *s)
		{
			if(_s) free(_s);
			if(s && strlen(s)>0) {_s=strdup(s);  _set=true;}  else {_s=NULL;  _set=false;}
		}

		void removespaces(void)
		{
			if(_s && strlen(_s)>0)
			{
				for(int i=0; i<(int)strlen(_s); i++)
					if(_s[i]==' ' || _s[i]=='\t')
						{_s[i]='\0';  break;}
			}
		}

	private:

		char *_s;
};

class ConfigGamma : public ConfigDouble
{
	public:

		ConfigGamma& operator= (double d) {set(d);  return *this;}

		void usesun(bool b) {_usesungamma=b;}
		bool usesun(void) {return _usesungamma;}

		double get(const char *envvar)
		{
			char *temp=NULL;
			if((temp=envget(envvar))!=NULL)
			{
				if(!strcmp(temp, "1"))
				{
					usesun(true);  set(2.22);
				}
				else if(!strcmp(temp, "0"))
				{
					usesun(false);  set(1.0);
				}
				else
				{
					usesun(false);
					char *t=NULL;  double dtemp=strtod(temp, &t);
					if(t && t!=temp) set(dtemp);
				}
			}
			return _d;
		}

		unsigned char _lut[256];
		unsigned short _lut16[65536];

	private:

		void set(double d)
		{
			if(d!=_d) {_d=d;  buildlut();}
			_set=true;
		}

		void buildlut(void)
		{
			if(_d!=0.0 && _d!=1.0 && _d!=-1.0)
			{
				for(int i=0; i<256; i++)
				{
					double g=_d>0.0? 1.0/_d : -_d;
					_lut[i]=(unsigned char)(255.*pow((double)i/255., g)+0.5);
				}
				for(int i=0; i<65536; i++)
				{
					double g=_d>0.0? 1.0/_d : -_d;
					_lut16[i]=(unsigned short)(255.*pow((double)(i/256)/255., g)+0.5)<<8;
					_lut16[i]|=(unsigned short)(255.*pow((double)(i%256)/255., g)+0.5);
				}
			}
		}

		bool _usesungamma;
};

class FakerConfig
{
	#ifndef _WIN32
	private:
	#else
	public:
	#endif

		FakerConfig(void)
		{
			// Defaults
			client=NULL;
			_compress.setbounds(0, RR_COMPRESSOPT-1);
			config=(char *)"/opt/VirtualGL/bin/vglconfig";
			config64=(char *)"/opt/VirtualGL/bin/vglconfig64";
			fps.setbounds(0.0, 1000000.0);
			flushdelay.setbounds(0.0, 1000000.0);
			#ifdef SUNOGL
			gamma=2.22;
			gamma.usesun(true);
			#else
			gamma=1.0;
			gamma.usesun(false);
			#endif
			gllib=NULL;
			glp=false;
			gui=true;
			guikey=XK_F9;
			guimod=ShiftMask|ControlMask;
			interframe=true;
			localdpystring=(char *)":0";
			mcompress.setbounds(RRCOMP_JPEG, RRCOMP_RGB);
			mcompress=RRCOMP_JPEG;
			mqual.setbounds(1, 100);
			mqual=DEFQUAL;
			msubsamp.setbounds(0, 4);
			msubsamp.setpow2();
			msubsamp=1;
			np.setbounds(1, min(numprocs(), MAXPROCS));
			np=1;
			port.setbounds(0, 65535);
			qual.setbounds(1, 100);
			qual=DEFQUAL;
			readback=true;
			spoil=true;
			ssl=false;
			stereo.setbounds(0, RR_STEREOOPT-1);
			stereo=RRSTEREO_QUADBUF;
			subsamp.setbounds(0, 16);
			subsamp.setpow2();
			x11lib=NULL;
			tilesize.setbounds(8, 1024);
			tilesize=RR_DEFAULTTILESIZE;
			transpixel.setbounds(0, 255);
			for(int i=0; i<RR_TRANSPORTOPT; i++) transvalid[i]=false;
			vendor=NULL;
			zoomx.setbounds(1, 2);
			zoomy.setbounds(1, 2);
			reloadenv();
		}

	public:

		#ifndef _WIN32
		static FakerConfig *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				rrcs::safelock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new FakerConfig;
			}
			return _Instanceptr;
		}

		static void deleteinstance(void)
		{
			if(_Instanceptr!=NULL)
			{
				rrcs::safelock l(_Instancemutex);
				if(_Instanceptr!=NULL) {delete _Instanceptr;  _Instanceptr=NULL;}
			}
		}

		static int _Shmid;
		#endif

		void reloadenv(void)
		{
			char *env;

			// Fetch values from environment
			gllib.get("VGL_GLLIB");
			x11lib.get("VGL_X11LIB");
			client.get("VGL_CLIENT");
			if((env=_compress.envget("VGL_COMPRESS"))!=NULL)
			{
				char *t=NULL;  int itemp=strtol(env, &t, 10);
				if(t && t!=env && itemp>=0 && itemp<RR_COMPRESSOPT) compress(itemp);
				else if(!strnicmp(env, "p", 1)) compress(RRCOMP_PROXY);
				else if(!strnicmp(env, "j", 1)) compress(RRCOMP_JPEG);
				else if(!strnicmp(env, "r", 1)) compress(RRCOMP_RGB);
				else if(!stricmp(env, "sr")) compress(RRCOMP_SRDPCM);
				else if(!strnicmp(env, "srl", 3)) compress(RRCOMP_SRRGB);
				else if(!strnicmp(env, "srr", 3)) compress(RRCOMP_SRRGB);
				else if(!strnicmp(env, "srd", 3)) compress(RRCOMP_SRDPCM);
				else if(!strnicmp(env, "sry", 3)) compress(RRCOMP_SRYUV);
			}
			config.get("VGL_CONFIG");
			config64.get("VGL_CONFIG64");
			localdpystring.get("VGL_DISPLAY");
			#ifdef USEGLP
			if(localdpystring &&
				(localdpystring[0]=='/' || !strnicmp(localdpystring, "GLP", 3)))
			{
				glp=true;
				localdpystring.removespaces();
			}
			#endif
			qual.get("VGL_QUAL");
			if((env=subsamp.envget("VGL_SUBSAMP"))!=NULL)
			{
				if(!strnicmp(env, "G", 1)) subsamp=0;
				else if(!strnicmp(env, "L", 1)) compress(RRCOMP_SRRGB);
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
			}
			if((env=mcompress.envget("VGL_MCOMPRESS"))!=NULL)
			{
				char *t=NULL;  int itemp=strtol(env, &t, 10);
				if(t && t!=env && itemp>=0 && itemp<RR_COMPRESSOPT) mcompress=itemp;
				else if(!strnicmp(env, "j", 1)) mcompress=RRCOMP_JPEG;
				else if(!strnicmp(env, "r", 1)) mcompress=RRCOMP_RGB;
			}
			moviefile.get("VGL_MOVIE");
			mqual.get("VGL_MQUAL");
			if((env=msubsamp.envget("VGL_MSUBSAMP"))!=NULL)
			{
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
			}
			spoil.get("VGL_SPOIL");
			ssl.get("VGL_SSL");
			port.get("VGL_PORT");
			usewindow.get("VGL_WINDOW");
			if(glp) usewindow=false;
			sync.get("VGL_SYNC");
			np.get("VGL_NPROCS");
			autotest.get("VGL_AUTOTEST");
			gamma.get("VGL_GAMMA");
			transpixel.get("VGL_TRANSPIXEL");
			tilesize.get("VGL_TILESIZE");
			trace.get("VGL_TRACE");
			readback.get("VGL_READBACK");
			verbose.get("VGL_VERBOSE");
			guikeyseq.get("VGL_GUI");
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
			fps.get("VGL_FPS");
			vendor.get("VGL_XVENDOR");
			if((env=stereo.envget("VGL_STEREO"))!=NULL)
			{
				if(!strnicmp(env, "L", 1)) stereo=RRSTEREO_LEYE;
				else if(!strnicmp(env, "RC", 2)) stereo=RRSTEREO_REDCYAN;
				else if(!strnicmp(env, "R", 1)) stereo=RRSTEREO_REYE;
				else if(!strnicmp(env, "Q", 1)) stereo=RRSTEREO_QUADBUF;
				else
				{
					char *t=NULL;  int itemp=strtol(env, &t, 10);
					if(t && t!=env) stereo=itemp;
				}
			}
			interframe.get("VGL_INTERFRAME");
			log.get("VGL_LOG");
			zoomx.get("VGL_ZOOM_X");
			zoomy.get("VGL_ZOOM_Y");
			progressive.get("VGL_PROGRESSIVE");
		}

		void compress(Display *dpy)
		{
			if(!_compress.isset())
			{
				if(RRSunRayQueryDisplay(dpy)!=RRSUNRAY_NOT) compress(RRCOMP_SRDPCM);
				else
				{
					const char *dstr=DisplayString(dpy);
					if((strlen(dstr) && dstr[0]==':') || (strlen(dstr)>5
						&& !strnicmp(dstr, "unix", 4))) compress(RRCOMP_PROXY);
					else compress(RRCOMP_JPEG);
				}
			}
		}

		void compress(int i)
		{
			bool is=_compress.isset();
			_compress=i;
			if(!is) transvalid[_Trans[_compress]]=transvalid[RRTRANS_X11]=true;
			if(!subsamp.isset()) subsamp.set(_Defsubsamp[_compress]);
			if(_Minsubsamp[_compress]>=0 && _Maxsubsamp[_compress]>=0)
			{
				subsamp.setbounds(_Minsubsamp[_compress], _Maxsubsamp[_compress]);
				if(subsamp<_Minsubsamp[_compress] || subsamp>_Maxsubsamp[_compress])
					subsamp.set(_Defsubsamp[_compress]);
			}
		}

		int compress(void) {return _compress;}

		#define prconfint(i) rrout.println(#i"  =  %d", (int)i);
		#define prconfstr(s) rrout.println(#s"  =  \"%s\"", (char *)s? (char *)s:"NULL");
		#define prconfdbl(d) rrout.println(#d"  =  %f", (double)d);
		#define prconfint_s(i) rrout.println(#i"  =  %d  (set = %d)", (int)i, i.isset());
		#define prconfstr_s(s) rrout.println(#s"  =  \"%s\"  (set = %d)", (char *)s? (char *)s:"NULL", s.isset());
		#define prconfdbl_s(d) rrout.println(#d"  =  %f  (set = %d)", (double)d, d.isset());

		ConfigBool autotest;
		ConfigString client;
		ConfigString config;
		ConfigString config64;
		ConfigDouble fps;
		ConfigDouble flushdelay;
		ConfigGamma gamma;
		ConfigString gllib;
		bool glp;
		ConfigBool gui;
		unsigned int guikey;
		ConfigString guikeyseq;
		unsigned int guimod;
		ConfigBool interframe;
		ConfigString localdpystring;
		ConfigString log;
		ConfigInt mcompress;
		ConfigInt mqual;
		ConfigInt msubsamp;
		ConfigString moviefile;
		ConfigInt np;
		ConfigInt port;
		ConfigBool progressive;
		ConfigInt qual;
		ConfigBool readback;
		ConfigBool spoil;
		ConfigBool ssl;
		ConfigInt stereo;
		ConfigInt subsamp;
		ConfigBool sync;
		ConfigInt tilesize;
		ConfigBool trace;
		ConfigInt transpixel;
		bool transvalid[RR_TRANSPORTOPT];
		ConfigBool usewindow;
		ConfigString vendor;
		ConfigBool verbose;
		ConfigString x11lib;
		ConfigInt zoomx, zoomy;

		void print(void)
		{
			prconfint_s(autotest)
			prconfstr_s(client)
			prconfint(compress())
			prconfint(_compress.isset())
			prconfstr_s(config)
			prconfdbl_s(fps)
			prconfdbl_s(flushdelay)
			prconfdbl_s(gamma)
			prconfstr_s(gllib)
			prconfint(glp)
			prconfint_s(gui)
			prconfint(guikey)
			prconfstr_s(guikeyseq)
			prconfint(guimod)
			prconfint_s(interframe)
			prconfstr_s(localdpystring)
			prconfstr_s(log)
			prconfint_s(mcompress)
			prconfstr_s(moviefile)
			prconfint_s(mqual)
			prconfint_s(msubsamp)
			prconfint_s(np)
			prconfint_s(port)
			prconfint_s(progressive)
			prconfint_s(qual)
			prconfint_s(readback)
			prconfint_s(spoil)
			prconfint_s(ssl)
			prconfint_s(stereo)
			prconfint_s(subsamp)
			prconfint(gamma.usesun())
			prconfint_s(sync)
			prconfint_s(tilesize)
			prconfint_s(trace)
			prconfint_s(transpixel)
			prconfint(transvalid[RRTRANS_X11])
			prconfint(transvalid[RRTRANS_VGL])
			prconfint(transvalid[RRTRANS_SR])
			prconfint_s(usewindow)
			prconfstr_s(vendor)
			prconfint_s(verbose)
			prconfstr_s(x11lib)
			prconfint_s(zoomx)
			prconfint_s(zoomy)
		}

	private:

		#ifndef _WIN32
		void *operator new(size_t size)
		{
			void *addr=NULL;
			if(size!=sizeof(FakerConfig))
				_throw("Attempted to allocate config structure with incorrect size");
			if((_Shmid=shmget(IPC_PRIVATE, sizeof(FakerConfig), IPC_CREAT|0600))==-1)
				_throwunix();
			if((addr=shmat(_Shmid, 0, 0))==(void *)-1) _throwunix();
			if(!addr) _throw("Could not attach to config structure in shared memory");
			#ifdef linux
			shmctl(_Shmid, IPC_RMID, 0);
			#endif
			char *env=NULL;
			if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
				&& !strncmp(env, "1", 1))
				rrout.println("[VGL] Shared memory segment ID for vglconfig: %d",
					_Shmid);
			return addr;
		}

		void operator delete(void *addr)
		{
			if(addr) shmdt((char *)addr);
			if(_Shmid!=-1)
			{
				int ret=shmctl(_Shmid, IPC_RMID, 0);
				char *env=NULL;
				if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
					&& !strncmp(env, "1", 1) && ret!=-1)
					rrout.println("[VGL] Removed shared memory segment %d", _Shmid);

			}
		}

		static FakerConfig *_Instanceptr;
		static rrcs _Instancemutex;
		#endif

		ConfigInt _compress;
};

#ifndef _WIN32

#ifdef __FAKERCONFIG_STATICDEF__
int FakerConfig::_Shmid=-1;
FakerConfig *FakerConfig::_Instanceptr=NULL;
rrcs FakerConfig::_Instancemutex;
#endif

#define fconfig (*(FakerConfig::instance()))

#else

#ifdef __FAKERCONFIG_STATICDEF__
FakerConfig fconfig;
#else
extern FakerConfig fconfig;
#endif

#endif

#endif
