// Copyright (C)2020-2021 D. R. Commander
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

#ifndef __EGLERROR_H__
#define __EGLERROR_H__

#include "Error.h"
#include <EGL/egl.h>
#include <X11/Xmd.h>
#include <GL/glxproto.h>


namespace backend
{
	class EGLError : public util::Error
	{
		public:

			EGLError(const char *method_, int line_) : eglError(_eglGetError())
			{
				init(method_, (char *)getEGLErrorString(), line_);
			}

			EGLError(const char *method_, int line_, EGLint eglError_) :
				eglError(eglError_)
			{
				init(method_, (char *)getEGLErrorString(), line_);
			}

			int getGLXError(void)
			{
				// NOTE: The error codes without GLX equivalents should never occur
				// except due to a bug in VirtualGL.
				switch(eglError)
				{
					case EGL_SUCCESS:  return Success;
					case EGL_BAD_ACCESS:  return BadAccess;
					case EGL_BAD_ALLOC:  return BadAlloc;
					case EGL_BAD_CONFIG:  return GLXBadFBConfig;
					case EGL_BAD_CONTEXT:  return GLXBadContext;
					case EGL_BAD_CURRENT_SURFACE:  return GLXBadCurrentDrawable;
					case EGL_BAD_MATCH:  return BadMatch;
					case EGL_BAD_NATIVE_PIXMAP:  return GLXBadPixmap;
					case EGL_BAD_NATIVE_WINDOW:  return GLXBadWindow;
					case EGL_BAD_PARAMETER:  return BadValue;
					case EGL_BAD_SURFACE:  return GLXBadDrawable;
					default:  return -1;
				}
			}

			bool isX11Error(void)
			{
				switch(eglError)
				{
					case EGL_SUCCESS:  return true;
					case EGL_BAD_ACCESS:  return true;
					case EGL_BAD_ALLOC:  return true;
					case EGL_BAD_MATCH:  return true;
					case EGL_BAD_PARAMETER:  return true;
					default:  return false;
				}
			}

		private:

			#define CASE(c) case c:  return #c

			const char *getEGLErrorString(void)
			{
				switch(eglError)
				{
					CASE(EGL_SUCCESS);
					CASE(EGL_NOT_INITIALIZED);
					CASE(EGL_BAD_ACCESS);
					CASE(EGL_BAD_ALLOC);
					CASE(EGL_BAD_ATTRIBUTE);
					CASE(EGL_BAD_CONFIG);
					CASE(EGL_BAD_CONTEXT);
					CASE(EGL_BAD_CURRENT_SURFACE);
					CASE(EGL_BAD_DISPLAY);
					CASE(EGL_BAD_MATCH);
					CASE(EGL_BAD_NATIVE_PIXMAP);
					CASE(EGL_BAD_NATIVE_WINDOW);
					CASE(EGL_BAD_PARAMETER);
					CASE(EGL_BAD_SURFACE);
					CASE(EGL_CONTEXT_LOST);
					default:  return "Unknown EGL error";
				}
			}

			EGLint eglError;
	};
}

#define THROW_EGL(f)  throw(backend::EGLError(f, __LINE__))

#endif  // __EGLERROR_H__
