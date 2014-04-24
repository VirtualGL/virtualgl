/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014 D. R. Commander
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

#ifndef __WINDOWHASH_H__
#define __WINDOWHASH_H__

#include "VirtualWin.h"
#include "Hash.h"


#define _Hash Hash<char *, Window, VirtualWin *>

// This maps a window ID to an off-screen drawable instance

namespace vglserver
{
	class WindowHash : public _Hash
	{
		public:

			static WindowHash *instance(void)
			{
				if(instancePtr==NULL)
				{
					vglutil::CS::SafeLock l(instanceMutex);
					if(instancePtr==NULL) instancePtr=new WindowHash;
				}
				return instancePtr;
			}

			static bool isAlloc(void) { return (instancePtr!=NULL); }

			void add(Display *dpy, Window win)
			{
				if(!dpy || !win) return;
				char *dpystring=strdup(DisplayString(dpy));
				if(!_Hash::add(dpystring, win, NULL))
					free(dpystring);
			}

			VirtualWin *find(Display *dpy, Window win)
			{
				if(!dpy || !win) return NULL;
				return _Hash::find(DisplayString(dpy), win);
			}

			bool find(Display *dpy, GLXDrawable glxd, VirtualWin* &vwin)
			{
				VirtualWin *vw;
				if(!dpy || !glxd) return false;
				vw=_Hash::find(DisplayString(dpy), glxd);
				if(vw==NULL || vw==(VirtualWin *)-1) return false;
				else { vwin=vw;  return true; }
			}

			bool isOverlay(Display *dpy, GLXDrawable glxd)
			{
				VirtualWin *vw;
				if(!dpy || !glxd) return false;
				vw=_Hash::find(DisplayString(dpy), glxd);
				if(vw==(VirtualWin *)-1) return true;
				return false;
			}

			bool find(GLXDrawable glxd, VirtualWin* &vwin)
			{
				VirtualWin *vw;
				if(!glxd) return false;
				vw=_Hash::find(NULL, glxd);
				if(vw==NULL || vw==(VirtualWin *)-1) return false;
				else { vwin=vw;  return true; }
			}

			VirtualWin *initVW(Display *dpy, Window win, GLXFBConfig config)
			{
				if(!dpy || !win || !config) _throw("Invalid argument");
				HashEntry *ptr=NULL;
				vglutil::CS::SafeLock l(mutex);
				if((ptr=_Hash::findEntry(DisplayString(dpy), win))!=NULL)
				{
					if(!ptr->value)
					{
						errifnot(ptr->value=new VirtualWin(dpy, win));
						VirtualWin *vw=ptr->value;
						vw->initFromWindow(config);
					}
					else
					{
						VirtualWin *vw=ptr->value;
						vw->checkConfig(config);
					}
					return ptr->value;
				}
				return NULL;
			}

			void setOverlay(Display *dpy, Window win)
			{
				if(!dpy || !win) return;
				HashEntry *ptr=NULL;
				vglutil::CS::SafeLock l(mutex);
				if((ptr=_Hash::findEntry(DisplayString(dpy), win))!=NULL)
				{
					if(!ptr->value) ptr->value=(VirtualWin *)-1;
				}
			}

			void remove(Display *dpy, GLXDrawable glxd)
			{
				if(!dpy || !glxd) return;
				_Hash::remove(DisplayString(dpy), glxd);
			}

			void remove(Display *dpy)
			{
				if(!dpy) return;
				HashEntry *ptr=NULL, *next=NULL;
				vglutil::CS::SafeLock l(mutex);
				ptr=start;
				while(ptr!=NULL)
				{
					VirtualWin *vw=ptr->value;
					next=ptr->next;
					if(vw && vw!=(VirtualWin *)-1 && dpy==vw->getX11Display())
						_Hash::killEntry(ptr);
					ptr=next;
				}
			}

		private:

			~WindowHash(void)
			{
				WindowHash::kill();
			}

			void detach(HashEntry *entry)
			{
				VirtualWin *vw=entry->value;
				if(entry && entry->key1) free(entry->key1);
				if(entry && vw && vw!=(VirtualWin *)-1) delete vw;
			}

			bool compare(char *key1, Window key2, HashEntry *entry)
			{
				VirtualWin *vw=entry->value;
				return (
					// Match 2D X Server display string and Window ID stored in
					// VirtualDrawable instance
					(vw && vw!=(VirtualWin *)-1 && key1
						&& !strcasecmp(DisplayString(vw->getX11Display()), key1)
						&& key2==vw->getX11Drawable())
					||
					// If key1 is NULL, match off-screen drawable ID instead of X Window
					// ID
					(vw && vw!=(VirtualWin *)-1 && key1==NULL
						&& key2==vw->getGLXDrawable())
					||
					// Direct match
					(key1 && !strcasecmp(key1, entry->key1) && key2==entry->key2)
				);
			}

			static WindowHash *instancePtr;
			static vglutil::CS instanceMutex;
	};
}

#undef _Hash


#define winhash (*(WindowHash::instance()))

#endif // __WINDOWHASH_H__
