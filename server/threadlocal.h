// Copyright (C)2015 Open Text SA and/or Open Text ULC (in Canada)
// Copyright (C)2021 D. R. Commander
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

// On Linux, using __thread within position-independent code (all of VirtualGL
// is compiled as PIC) invokes __tls_get_addr(), which is guarded by a mutex.
// This can lead to deadlocks in certain applications.  Therefore, this macro
// creates a simple pthreads-based alternative to __thread for the purposes of
// implementing TLS.

#ifndef __THREADLOCAL_H__
#define __THREADLOCAL_H__

#define VGL_THREAD_LOCAL(name, type, initValue) \
static pthread_key_t get##name##Key(void) \
{ \
	static pthread_key_t key; \
	static bool init = false; \
	if(!init) \
	{ \
		if(pthread_key_create(&key, NULL)) \
		{ \
			vglout.println("[VGL] ERROR: pthread_key_create() for " #name " failed.\n"); \
			faker::safeExit(1); \
		} \
		pthread_setspecific(key, (const void *)initValue); \
		init = true; \
	} \
	return key; \
} \
\
type get##name(void) \
{ \
	return (type)pthread_getspecific(get##name##Key()); \
} \
\
void set##name(type value) \
{ \
	pthread_setspecific(get##name##Key(), (const void *)value); \
}

#endif  // __THREADLOCAL_H__
