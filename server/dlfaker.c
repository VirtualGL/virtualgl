/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2012, 2015, 2017-2021 D. R. Commander
 * Copyright (C)2015 Open Text SA and/or Open Text ULC (in Canada)
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
#include <pthread.h>
#include "vendor.h"


extern void *_vgl_dlopen(const char *, int);


static pthread_key_t getDLFakerLevelKey(void)
{
	static pthread_key_t key;
	static int init = 0;

	if(!init)
	{
		if(pthread_key_create(&key, NULL))
		{
			fprintf(stderr, "[VGL] ERROR: pthread_key_create() for DLFakerLevel failed.\n");
			return (pthread_key_t)-1;
		}
		pthread_setspecific(key, (const void *)0);
		init = 1;
	}
	return key;
}

static int getDLFakerLevel(void)
{
	pthread_key_t dlFakerLevelKey = getDLFakerLevelKey();

	if(dlFakerLevelKey == (pthread_key_t)-1)
		return -1;
	else return (int)(size_t)pthread_getspecific(dlFakerLevelKey);
}

static void setDLFakerLevel(int value)
{
	pthread_key_t dlFakerLevelKey = getDLFakerLevelKey();

	if(dlFakerLevelKey != (pthread_key_t)-1)
		pthread_setspecific(dlFakerLevelKey, (const void *)(size_t)value);
}


/* If an application uses dlopen()/dlsym() to load functions from libGL,
   libOpenCL, or libX11, this bypasses the LD_PRELOAD mechanism.  Thus,
   VirtualGL has to intercept dlopen() and return a handle to itself rather
   than a handle to libGL, libOpenCL, or libX11.

   NOTE: If the application tries to use dlopen() to obtain a handle to libdl,
   we similarly replace the handle with a handle to libdlfaker.  This works
   around an interaction issue between 180.xx of the nVidia drivers and WINE.
*/

void *dlopen(const char *filename, int flag)
{
	char *env = NULL, *env2 = NULL;  const char *envname = "FAKERLIB32";
	int verbose = 0, trace = 0;
	#ifdef FAKEOPENCL
	int fakeOpenCL = 0;
	#endif
	void *retval = NULL;
	FILE *file = stderr;

	/* Prevent segfault if an application interposes getenv() and calls dlopen()
	   within the body of its interposed getenv() function (I'm looking at you,
	   Cadence Virtuoso)
	*/
	if(getDLFakerLevel() > 0)
		return _vgl_dlopen(filename, flag);

	setDLFakerLevel(getDLFakerLevel() + 1);

	if(sizeof(long) == 8) envname = "FAKERLIB";
	if((env2 = getenv("VGL_VERBOSE")) != NULL && strlen(env2) > 0
		&& !strncmp(env2, "1", 1)) verbose = 1;
	if((env2 = getenv("VGL_TRACE")) != NULL && strlen(env2) > 0
		&& !strncmp(env2, "1", 1)) trace = 1;
	#ifdef FAKEOPENCL
	if((env2 = getenv("VGL_FAKEOPENCL")) != NULL && strlen(env2) > 0
		&& !strncmp(env2, "1", 1)) fakeOpenCL = 1;
	#endif
	if((env2 = getenv("VGL_LOG")) != NULL && strlen(env2) > 0
		&& !strcasecmp(env2, "stdout")) file = stdout;

	if(trace)
	{
		fprintf(file, "[VGL] dlopen (filename=%s flag=%d",
			filename ? filename : "NULL", flag);
	}

	#ifdef RTLD_DEEPBIND
	flag &= (~RTLD_DEEPBIND);
	#endif

	if((env = getenv(envname)) == NULL || strlen(env) < 1)
	{
		#ifdef FAKEOPENCL
		if(fakeOpenCL) env = "lib"VGL_FAKER_NAME"-opencl.so";
		else
		#endif
		env = "lib"VGL_FAKER_NAME".so";
	}
	if(filename
		&& (!strncmp(filename, "libGL.", 6) || strstr(filename, "/libGL.")
			|| !strncmp(filename, "libGLX.", 7) || strstr(filename, "/libGLX.")
			|| !strncmp(filename, "libOpenGL.", 10)
			|| strstr(filename, "/libOpenGL.")
			#ifdef FAKEOPENCL
			|| (!strncmp(filename, "libOpenCL.", 10) && fakeOpenCL)
				|| (strstr(filename, "/libOpenCL.") && fakeOpenCL)
			#endif
			#ifdef EGLBACKEND
			|| !strncmp(filename, "libEGL.", 7) || strstr(filename, "/libEGL.")
			#endif
			|| !strncmp(filename, "libX11.", 7) || strstr(filename, "/libX11.")
			|| (flag & RTLD_LAZY
					&& (!strncmp(filename, "libopengl.", 10)
							|| strstr(filename, "/libopengl.")))))
	{
		if(verbose)
			fprintf(file,
				"[VGL] NOTICE: Replacing dlopen(\"%s\") with dlopen(\"%s\")\n",
				filename ? filename : "NULL", env ? env : "NULL");
		retval = _vgl_dlopen(env, flag);
	}
	else if(filename && (!strncmp(filename, "libdl.", 6)
		|| strstr(filename, "/libdl.")))
	{
		if(verbose)
			fprintf(file, "[VGL] NOTICE: Replacing dlopen(\"%s\") with dlopen(\"lib" VGL_DLFAKER_NAME ".so\")\n",
				filename ? filename : "NULL");
		retval = _vgl_dlopen("lib" VGL_DLFAKER_NAME ".so", flag);
	}
	else retval = _vgl_dlopen(filename, flag);

	if(!retval && filename && !strncmp(filename, "VBoxOGL", 7))
	{
		char temps[256];
		snprintf(temps, 255, "/usr/lib/virtualbox/%s", filename);
		if(verbose)
		{
			fprintf(file, "[VGL] NOTICE: dlopen(\"%s\") failed.\n", filename);
			fprintf(file, "[VGL]    Trying dlopen(\"%s\")\n", temps);
		}
		retval = _vgl_dlopen(temps, flag);
	}

	if(trace)
	{
		fprintf(file, " retval=0x%.8lx)\n", (long)retval);
	}

	setDLFakerLevel(getDLFakerLevel() - 1);

	return retval;
}
