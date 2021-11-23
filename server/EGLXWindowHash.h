// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011, 2014-2015, 2018, 2021 D. R. Commander
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

#ifndef __EGLXWINDOWHASH_H__
#define __EGLXWINDOWHASH_H__

#include "Hash.h"
#include "EGLXVirtualWin.h"


#define HASH  Hash<EGLXDisplay *, EGLSurface, EGLXVirtualWin *>

// This maps an EGL display and surface to an EGLXVirtualWin instance

namespace faker
{
	class EGLXWindowHash : public HASH
	{
		public:

			static EGLXWindowHash *getInstance(void)
			{
				if(instance == NULL)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new EGLXWindowHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(EGLXDisplay *eglxdpy, EGLSurface surface,
				EGLXVirtualWin *eglxvw)
			{
				if(!eglxdpy || !surface || !eglxvw) THROW("Invalid argument");
				HASH::add(eglxdpy, surface, eglxvw);
			}

			EGLXVirtualWin *find(EGLXDisplay *eglxdpy, EGLSurface surface)
			{
				if(!eglxdpy || !surface) return NULL;
				return HASH::find(eglxdpy, surface);
			}

			EGLXVirtualWin *find(Display *dpy, Window win)
			{
				if(!dpy || !win) return NULL;

				HashEntry *entry = NULL;
				util::CriticalSection::SafeLock l(mutex);

				entry = start;
				while(entry != NULL)
				{
					if(entry->value->getX11Display() == dpy
						&& entry->value->getX11Drawable() == win)
						return entry->value;
					entry = entry->next;
				}
				return NULL;
			}

			// Find the EGLXVirtualWin instance corresponding to the real EGL
			// surface, not the dummy EGL surface that we passed back to the 3D
			// application.
			EGLXVirtualWin *findInternal(EGLXDisplay *eglxdpy, EGLSurface surface)
			{
				if(!eglxdpy || !surface) return NULL;

				HashEntry *entry = NULL;
				util::CriticalSection::SafeLock l(mutex);

				entry = start;
				while(entry != NULL)
				{
					if(entry->key1 == eglxdpy
						&& (EGLSurface)entry->value->getGLXDrawable() == surface)
						return entry->value;
					entry = entry->next;
				}
				return NULL;
			}

			void remove(EGLXDisplay *eglxdpy, EGLSurface surface)
			{
				if(eglxdpy && surface) HASH::remove(eglxdpy, surface);
			}

		private:

			~EGLXWindowHash(void)
			{
				EGLXWindowHash::kill();
			}

			void detach(HashEntry *entry)
			{
				EGLXVirtualWin *eglxvw = entry ? entry->value : NULL;
				delete eglxvw;
			}

			bool compare(EGLXDisplay *key1, EGLSurface key2, HashEntry *entry)
			{
				return false;
			}

			static EGLXWindowHash *instance;
			static util::CriticalSection instanceMutex;
	};
}

#undef HASH


#define eglxwinhash  (*(faker::EGLXWindowHash::getInstance()))

#endif  // __EGLXWINDOWHASH_H__
