/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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

#ifndef __XCBCONNHASH_H__
#define __XCBCONNHASH_H__

#ifdef FAKEXCB

#include <X11/Xlib.h>
extern "C" {
#include <xcb/xcb.h>
}
#include "Hash.h"


#define HASH Hash<xcb_connection_t *, void *, Display *>

namespace vglserver
{
	class XCBConnHash : public HASH
	{
		public:

			static XCBConnHash *getInstance(void)
			{
				if(instance==NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance==NULL) instance=new XCBConnHash;
				}
				return instance;
			}

			static bool isAlloc(void) { return (instance!=NULL); }

			void add(xcb_connection_t *conn, Display *dpy)
			{
				if(!conn || !dpy) _throw("Invalid argument");
				HASH::add(conn, NULL, dpy);
			}

			void remove(xcb_connection_t *conn)
			{
				if(!conn) return;
				HASH::remove(conn, NULL);
			}

			Display *getX11Display(xcb_connection_t *conn)
			{
				if(!conn) _throw("Invalid_argument");
				return HASH::find(conn, NULL);
			}

		private:

			~XCBConnHash(void)
			{
				HASH::kill();
			}

			bool compare(xcb_connection_t *key1, void *key2, HashEntry *entry)
			{
				return(key1==entry->key1);
			}

			void detach(HashEntry *entry)
			{
			}

			static XCBConnHash *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define xcbconnhash (*(XCBConnHash::getInstance()))

#endif // FAKEXCB

#endif // __XCBCONNHASH_H__
