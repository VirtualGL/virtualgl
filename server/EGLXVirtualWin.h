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

#ifndef __EGLXVIRTUALWIN_H__
#define __EGLXVIRTUALWIN_H__

#include <EGL/egl.h>
#include "VirtualWin.h"


namespace faker
{
	class EGLXVirtualWin : public VirtualWin
	{
		public:

			EGLXVirtualWin(Display *dpy, Window win, EGLDisplay edpy,
				EGLConfig config, const EGLint *pbAttribs);
			~EGLXVirtualWin(void);
			GLXDrawable updateGLXDrawable(void);
			EGLDisplay getEGLDisplay(void) { return edpy; }
			EGLSurface getDummySurface(void) { return dummyPB; }

		private:

			EGLint pbAttribs[MAX_ATTRIBS + 1];
			EGLSurface dummyPB;
	};
}

#endif  // __EGLXVIRTUALWIN_H__
