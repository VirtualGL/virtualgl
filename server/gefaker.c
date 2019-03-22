/* Copyright (C)2007 Sun Microsystems, Inc.
 * Copyright (C)2019 D. R. Commander
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

#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


static pthread_mutex_t globalmutex = PTHREAD_MUTEX_INITIALIZER;

typedef char *(*_getenvType)(const char *);
static _getenvType __getenv = NULL;


static void __loadsymbol(void)
{
	const char *err = NULL;
	pthread_mutex_lock(&globalmutex);
	if(__getenv) { pthread_mutex_unlock(&globalmutex);  return; }
	dlerror();  /* Clear error state */
	__getenv = (_getenvType)dlsym(RTLD_NEXT, "getenv");
	err = dlerror();
	if(err) fprintf(stderr, "[gefaker] %s\n", err);
	else if(!__getenv) fprintf(stderr, "[gefaker] Could not load symbol.\n");
	pthread_mutex_unlock(&globalmutex);
}


char *getenv(const char *name)
{
	char *env = NULL, *tmp;  int verbose = 0;
	FILE *file = stderr;

	__loadsymbol();
	if(!__getenv) return NULL;

	if((env = __getenv("VGL_VERBOSE")) != NULL && strlen(env) > 0
		&& !strncmp(env, "1", 1)) verbose = 1;
	if((env = __getenv("VGL_LOG")) != NULL && strlen(env) > 0
		&& !strcasecmp(env, "stdout")) file = stdout;

	tmp = (char *)name;
	if(tmp && (!strcmp(name, "LD_PRELOAD")
	#ifdef sun
		|| !strcmp(name, "LD_PRELOAD_32") || !strcmp(name, "LD_PRELOAD_64")
	#endif
	))
	{
		if(verbose)
			fprintf(file, "[VGL] NOTICE: Fooling application into thinking that LD_PRELOAD is unset\n");
		return NULL;
	}
	else return __getenv(name);
}
