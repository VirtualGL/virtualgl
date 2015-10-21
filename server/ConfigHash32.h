/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2014-2015 D. R. Commander
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

#ifndef __CONFIGHASH32_H__
#define __CONFIGHASH32_H__

#include "glx.h"
#include "glxvisual.h"
#include "Hash.h"


extern Display *_dpy3D;


#define HASH Hash<int, void *, int>

namespace vglserver
{
	class ConfigHash32 : public HASH
	{
		public:

			static ConfigHash32 *getInstance(void)
			{
				if(instance==NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance==NULL) instance=new ConfigHash32;
				}
				return instance;
			}

			static bool isAlloc(void) { return (instance!=NULL); }

			void add(GLXFBConfig config, GLXFBConfig config32)
			{
				if(!config || !config32) _throw("Invalid argument");
				HASH::add(_FBCID(config), NULL, _FBCID(config32));
			}

			GLXFBConfig getConfig32(GLXFBConfig config)
			{
				if(!config) _throw("Invalid argument");
				int attrib_list[]={ GLX_FBCONFIG_ID, 0, 0, 0 };
				attrib_list[1]=HASH::find(_FBCID(config), NULL);
				int n=0;
				GLXFBConfig *configs=_glXChooseFBConfig(_dpy3D, DefaultScreen(_dpy3D),
					attrib_list, &n);
				if(configs && n>0)
				{
					GLXFBConfig config=configs[0];
					XFree(configs);
					return config;
				}
				return 0;
			}

			void remove(GLXFBConfig config)
			{
				if(!config) _throw("Invalid argument");
				HASH::remove(_FBCID(config), NULL);
			}

		private:

			~ConfigHash32(void)
			{
				HASH::kill();
			}

			bool compare(int key1, void *key2, HashEntry *entry)
			{
				return false;
			}

			void detach(HashEntry *entry)
			{
			}

			static ConfigHash32 *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#undef HASH


#define cfghash32 (*(ConfigHash32::getInstance()))

#endif // __CONFIGHASH32_H__
