/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2012 D. R. Commander
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


extern void *_vgl_dlopen(const char *, int);


/* If an application uses dlopen()/dlsym() to load functions from libGL or
   libX11, this bypasses the LD_PRELOAD mechanism.  Thus, VirtualGL has to
   intercept dlopen() and return a handle to itself rather than a handle to
   libGL or libX11.

   NOTE: If the application tries to use dlopen() to obtain a handle to libdl,
   we similarly replace the handle with a handle to libdlfaker.  This works
   around an interaction issue between 180.xx of the nVidia drivers and WINE.
*/

void *dlopen(const char *filename, int flag)
{
	char *env=NULL, *env2=NULL;  const char *envname="FAKERLIB32";
	int verbose=0, trace=0;
	void *retval=NULL;

	if(sizeof(long)==8) envname="FAKERLIB";
	if((env2=getenv("VGL_VERBOSE"))!=NULL && strlen(env2)>0
		&& !strncmp(env2, "1", 1)) verbose=1;
	if((env2=getenv("VGL_TRACE"))!=NULL && strlen(env2)>0
		&& !strncmp(env2, "1", 1)) trace=1;

	if(trace)
	{
		fprintf(stderr, "[VGL] dlopen (filename=%s flag=%d",
			filename? filename:"NULL", flag);
	}

	if((env=getenv(envname))==NULL || strlen(env)<1)
		env="librrfaker.so";
	if(filename &&
		(!strncmp(filename, "libGL.", 6) || strstr(filename, "/libGL.")
			|| !strncmp(filename, "libX11.", 7) || strstr(filename, "/libX11.")
			|| (flag&RTLD_LAZY
					&& (!strncmp(filename, "libopengl.", 10)
							|| strstr(filename, "/libopengl.")))))
	{
		if(verbose)
			fprintf(stderr,
				"[VGL] NOTICE: Replacing dlopen(\"%s\") with dlopen(\"%s\")\n",
				filename? filename:"NULL", env? env:"NULL");
		retval=_vgl_dlopen(env, flag);
	}
	else if(filename && (!strncmp(filename, "libdl.", 6)
		|| strstr(filename, "/libdl.")))
	{
		if(verbose)
			fprintf(stderr, "[VGL] NOTICE: Replacing dlopen(\"%s\") with dlopen(\"libdlfaker.so\")\n",
				filename? filename:"NULL");
		retval=_vgl_dlopen("libdlfaker.so", flag);
	}
	else retval=_vgl_dlopen(filename, flag);

	if(!retval && filename && !strncmp(filename, "VBoxOGL", 7))
	{
		char temps[256];
		snprintf(temps, 255, "/usr/lib/virtualbox/%s", filename);
		if(verbose)
		{
			fprintf(stderr, "[VGL] NOTICE: dlopen(\"%s\") failed.\n", filename);
			fprintf(stderr, "[VGL]    Trying dlopen(\"%s\")\n", temps);
		}
		retval=_vgl_dlopen(temps, flag);
	}

	if(trace)
	{
		fprintf(stderr, " retval=0x%.8lx)\n", (long)retval);
	}

	return retval;
}
