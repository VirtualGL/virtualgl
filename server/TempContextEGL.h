// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011, 2014-2015, 2018-2021 D. R. Commander
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

// This is exactly like TempContext, except that it uses raw EGL surfaces and
// context handles rather than GLX drawables and context handles.

#ifndef __TEMPCONTEXTEGL_H__
#define __TEMPCONTEXTEGL_H__

#include "faker-sym.h"
#include "EGLError.h"


namespace backend
{
	class TempContextEGL
	{
		public:

			TempContextEGL(EGLContext ctx) : oldctx(_eglGetCurrentContext()),
				ctxChanged(false)
			{
				if(!ctx) THROW("Invalid argument");
				if(oldctx != ctx)
				{
					if(!_eglBindAPI(EGL_OPENGL_API))
						THROW("Could not enable OpenGL API");
					if(!_eglMakeCurrent(EDPY, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx))
						THROW_EGL("eglMakeCurrent()");
					ctxChanged = true;
				}
			}

			~TempContextEGL(void)
			{
				if(ctxChanged)
				{
					_eglBindAPI(EGL_OPENGL_API);
					_eglMakeCurrent(EDPY, EGL_NO_SURFACE, EGL_NO_SURFACE, oldctx);
					ctxChanged = false;
				}
			}

		private:

			EGLContext oldctx;
			bool ctxChanged;
	};
}

#endif  // __TEMPCONTEXTEGL_H__
