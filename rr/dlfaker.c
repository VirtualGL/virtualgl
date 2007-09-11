/* Copyright (C)2006 Sun Microsystems, Inc.
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

#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t globalmutex=PTHREAD_MUTEX_INITIALIZER;

typedef	void* (*_dlopenType)(const char *, int);
static _dlopenType __dlopen=NULL;

static void __loadsymbol(void)
{
	const char *err=NULL;
	pthread_mutex_lock(&globalmutex);
	if(__dlopen) {pthread_mutex_unlock(&globalmutex);  return;}
	dlerror();  // Clear error state
	__dlopen=(_dlopenType)dlsym(RTLD_NEXT, "dlopen");
	err=dlerror();
	if(err) fprintf(stderr, "[dlfaker] %s\n", err);
	else if(!__dlopen) fprintf(stderr, "[dlfaker] Could not load symbol.\n");
	pthread_mutex_unlock(&globalmutex);
}

void *dlopen(const char *filename, int flag)
{
	char *env=NULL;  const char *envname="FAKERLIB32";  int verbose=0;
	char *env2=NULL;
	if(sizeof(long)==8) envname="FAKERLIB";
	if((env2=getenv("VGL_VERBOSE"))!=NULL && strlen(env2)>0
		&& !strncmp(env2, "1", 1)) verbose=1;
	if((env=getenv(envname))==NULL || strlen(env)<1)
		env="librrfaker.so";
	__loadsymbol();
	if(!__dlopen) return NULL;
	if(filename && strstr(filename, "libGL"))
	{
		if(verbose)
			fprintf(stderr, "[VGL] NOTICE: Replacing dlopen(\"%s\") with dlopen(\"%s\")\n",
				filename, env);
		return __dlopen(env, flag);
	}
	else return __dlopen(filename, flag);
}
