// Copyright (C)2020 D. R. Commander
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

#ifndef __EGLRBOCONTEXT_H__
#define __EGLRBOCONTEXT_H__

#include "faker-sym.h"
#include "EGLError.h"
#include "Mutex.h"


class EGLRBOContext
{
	public:

		EGLRBOContext(EGLConfig config_) : config(config_), ctx(0), refCount(0)
		{
		}

		void createContext(void)
		{
			vglutil::CriticalSection::SafeLock l(mutex);

			if(!ctx)
			{
				if(!_eglBindAPI(EGL_OPENGL_API))
					THROW("Could not enable OpenGL API");
				ctx = _eglCreateContext(EDPY, config, NULL, NULL);
				if(!ctx) THROW_EGL("eglCreateContext()");
			}
			refCount++;
		}

		EGLContext getContext(void) { return ctx; }

		vglutil::CriticalSection &getMutex(void) { return mutex; }

		void destroyContext(void)
		{
			vglutil::CriticalSection::SafeLock l(mutex);

			if(ctx)
			{
				refCount--;
				if(refCount <= 0)
				{
					if(!_eglBindAPI(EGL_OPENGL_API))
						THROW("Could not enable OpenGL API");
					if(!_eglDestroyContext(EDPY, ctx))
						THROW_EGL("eglDestroyContext()");
					ctx = 0;
					refCount = 0;
				}
			}
		}

	private:

		EGLConfig config;
		// The EGL back end emulates Pbuffers using RBOs, but RBOs are specific to
		// a particular OpenGL context, whereas Pbuffers are not.  Thus, the EGL
		// back end creates, as needed, a dedicated OpenGL context (the "RBO
		// context") for every GLXFBConfig and shares that context with any new
		// unshared OpenGL contexts that the 3D application asks to create or that
		// the faker creates behind the scenes.  The RBO context for a particular
		// GLXFBConfig is used behind the scenes when creating, destroying, or
		// swapping a Pbuffer (either an application-requested Pbuffer or a Pbuffer
		// that the faker uses to emulate an OpenGL window), operations that do not
		// require an OpenGL context to be current.  Sharing the RBO context with
		// application-requested or faker-requested contexts allows those contexts
		// to be used for rendering into the RBOs, even though those contexts were
		// not current when the RBOs were created.
		EGLContext ctx;
		// The reference count for the RBO context is incremented when the RBO
		// context is shared with a new unshared OpenGL context in
		// VGLCreateContext() or when a new Pbuffer is created in
		// VGLCreatePbuffer().  The ref count is decremented when a context with
		// which it was shared is destroyed in VGLDestroyContext() or a Pbuffer is
		// destroyed in VGLDestroyPbuffer().  If the ref count reaches 0, then the
		// RBO context is destroyed.  Any remaining undestroyed RBO contexts are
		// destroyed in the body of XCloseDisplay().
		int refCount;
		// Mutex for the RBO context.  This guards any operations that alter the
		// context handle or ref count above, or any operations that occur while
		// the RBO context is current (an OpenGL context can only be current in one
		// thread at a time.)
		vglutil::CriticalSection mutex;
};

#endif
