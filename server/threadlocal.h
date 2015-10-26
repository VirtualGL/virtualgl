/* Copyright (C) 2015 Open Text SA and/or Open Text ULC (in Canada).
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

// Simple pthread-based alternative to __thread for TLS on Linux x86*.
// Linux can't use __thread storage because it invokes __tls_get_addr which
// blocks on a mutex which may be locked during another shared object's _init().

#ifndef __THREADLOCAL_H__
#define __THREADLOCAL_H__

#ifdef _WIN32

#define VGL_THREAD_LOCAL(name, type, initvalue) \
__thread type name=initvalue; \
type get##name(void) { return name;} \
void set##name(type v) { name = v; }

#else

#define VGL_THREAD_LOCAL(name, type, initvalue) \
static pthread_key_t get##name##Key(void) \
{ \
	static pthread_key_t key; \
	static bool inited=false; \
	if (!inited) \
	{ \
		if(pthread_key_create(&key, NULL)) \
		{ \
			vglout.println("[VGL] Error: pthread_key_create() for " #name "failed.\n"); \
			vglfaker::safeExit(1); \
		} \
		pthread_setspecific(key, (const void *)initvalue); \
		inited=true; \
	} \
	return key; \
} \
type get##name(void) { return (type) pthread_getspecific(get##name##Key()); } \
void set##name(type v) { pthread_setspecific(get##name##Key(), (const void *)v); }

#endif

#endif // __THREADLOCAL_H__
