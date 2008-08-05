/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005-2008 Sun Microsystems, Inc.
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
#include "rrlog.h"
#include <stdio.h>

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
				if(strlen(temp)>0)
				{ 	 
					for(int i=0; i<(int)strlen(temp); i++) 	 
						if(temp[i]==' ' || temp[i]=='\t')
							{temp[i]='\0';  break;}
					set(temp); 	 
				}
			}
		}

		void set(char *s)
		{
			if(_s) free(_s);
			if(s && strlen(s)>0) {_s=strdup(s);  _set=true;}  else {_s=NULL;  _set=false;}
		}

	private:

		char *_s;
};

class ConfigGamma : public ConfigDouble
{
	public:

		ConfigGamma& operator= (double d) {set(d);  return *this;}

		double get(const char *envvar)
		{
			char *temp=NULL;
			if((temp=envget(envvar))!=NULL)
			{
				if(!strcmp(temp, "1"))
				{
					set(2.22);
				}
				else if(!strcmp(temp, "0"))
				{
					set(1.0);
				}
				else
				{
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
};

class FakerConfig
{
	public:

		FakerConfig(void)
		{
			// Defaults
			fps.setbounds(0.0, 1000000.0);
			gamma=1.0;
			spoil=true;
			stereo.setbounds(0, RR_STEREOOPT-1);
			stereo=RRSTEREO_REDCYAN;
			reloadenv();
		}

		void reloadenv(void)
		{
			char *env;

			// Fetch values from environment
			autotest.get("VGL_AUTOTEST");
			fps.get("VGL_FPS");
			gamma.get("VGL_GAMMA");
			log.get("VGL_LOG");
			serial.get("VGL_SERIAL");
			spoil.get("VGL_SPOIL");
			if((env=stereo.envget("VGL_STEREO"))!=NULL)
			{
				if(!strnicmp(env, "L", 1)) stereo=RRSTEREO_LEYE;
				else if(!strnicmp(env, "RC", 2)) stereo=RRSTEREO_REDCYAN;
				else if(!strnicmp(env, "R", 1)) stereo=RRSTEREO_REYE;
				else
				{
					char *t=NULL;  int itemp=strtol(env, &t, 10);
					if(t && t!=env) stereo=itemp;
				}
			}
			trace.get("VGL_TRACE");
			verbose.get("VGL_VERBOSE");
		}

		#define prconfint_s(i) rrout.println(#i"  =  %d  (set = %d)", (int)i, i.isset());
		#define prconfstr_s(s) rrout.println(#s"  =  \"%s\"  (set = %d)", (char *)s? (char *)s:"NULL", s.isset());
		#define prconfdbl_s(d) rrout.println(#d"  =  %f  (set = %d)", (double)d, d.isset());

		ConfigBool autotest;
		ConfigDouble fps;
		ConfigGamma gamma;
		ConfigString log;
		ConfigBool serial;
		ConfigBool spoil;
		ConfigInt stereo;
		ConfigBool trace;
		ConfigBool verbose;

		void print(void)
		{
			prconfint_s(autotest)
			prconfdbl_s(fps)
			prconfdbl_s(gamma)
			prconfstr_s(log)
			prconfint_s(serial)
			prconfint_s(spoil)
			prconfint_s(stereo)
			prconfint_s(trace)
			prconfint_s(verbose)
		}

};

#ifdef __FAKERCONFIG_STATICDEF__
FakerConfig fconfig;
#else
extern FakerConfig fconfig;
#endif

#endif
