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

#ifndef __RBOCONTEXT_H__
#define __RBOCONTEXT_H__

#include "faker-sym.h"
#include "EGLError.h"
#include "Mutex.h"


namespace backend
{

class RBOContext
{
	public:

		RBOContext() : ctx(0)
		{
		}

		void createContext(void)
		{
			util::CriticalSection::SafeLock l(mutex);

			if(!ctx)
			{
				if(!_eglBindAPI(EGL_OPENGL_API))
					THROW("Could not enable OpenGL API");
				ctx = _eglCreateContext(EDPY, (EGLConfig)0, NULL, NULL);
				if(!ctx) THROW_EGL("eglCreateContext()");
			}
		}

		EGLContext getContext(void) { return ctx; }

		util::CriticalSection &getMutex(void) { return mutex; }

		~RBOContext()
		{
			util::CriticalSection::SafeLock l(mutex);

			if(ctx)
			{
				if(_eglBindAPI(EGL_OPENGL_API))
					_eglDestroyContext(EDPY, ctx);
				ctx = 0;
			}
		}

	private:

		// The EGL back end emulates Pbuffers using RBOs, but RBOs are specific to
		// a particular OpenGL context, whereas Pbuffers are not.  Thus, the EGL
		// back end creates, as needed, a dedicated OpenGL context (the "RBO
		// context") and shares that context with any new unshared OpenGL contexts
		// that the 3D application asks to create or that the faker creates behind
		// the scenes.  The RBO context is used behind the scenes when creating,
		// destroying, or swapping a Pbuffer (either an application-requested
		// Pbuffer or a Pbuffer that the faker uses to emulate an OpenGL window),
		// operations that do not require an OpenGL context to be current.  Sharing
		// the RBO context with application-requested or faker-requested contexts
		// allows those contexts to be used for rendering into the RBOs, even
		// though those contexts were not current when the RBOs were created.
		EGLContext ctx;
		// Mutex for the RBO context.  This guards any operations that alter the
		// context handle above, or any operations that occur while the RBO context
		// is current (an OpenGL context can only be current in one thread at a
		// time.)
		util::CriticalSection mutex;
};


INLINE RBOContext &getRBOContext(Display *dpy)
{
	XEDataObject obj = { dpy };
	XExtData *extData;

	if(!fconfig.egl)
		THROW("backend::getRBOContext() called while using the GLX back end (this should never happen)");
	int minExtensionNumber =
		XFindOnExtensionList(XEHeadOfExtensionList(obj), 0) ? 0 : 1;
	extData = XFindOnExtensionList(XEHeadOfExtensionList(obj),
		minExtensionNumber + 4);
	ERRIFNOT(extData);
	ERRIFNOT(extData->private_data);

	return *(backend::RBOContext *)extData->private_data;
}

}  // namespace

#endif
