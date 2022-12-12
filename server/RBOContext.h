// Copyright (C)2020-2022 D. R. Commander
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

		static const int REFCOUNT_CONTEXT = 1 << 0;
		static const int REFCOUNT_DRAWABLE = 1 << 1;

		RBOContext() : ctx(0), contextRefCount(0), drawableRefCount(0)
		{
		}

		void createContext(int refCounts)
		{
			util::CriticalSection::SafeLock l(mutex);

			if(!ctx)
			{
				if(!_eglBindAPI(EGL_OPENGL_API))
					THROW("Could not enable OpenGL API");
				ctx = _eglCreateContext(EDPY, (EGLConfig)0, NULL, NULL);
				if(!ctx) THROW_EGL("eglCreateContext()");
			}

			if(refCounts & REFCOUNT_CONTEXT) contextRefCount++;
			if(refCounts & REFCOUNT_DRAWABLE) drawableRefCount++;
		}

		void destroyContext(int refCounts, bool force = false)
		{
			util::CriticalSection::SafeLock l(mutex);

			if(refCounts & REFCOUNT_CONTEXT)
			{
				contextRefCount--;
				if(contextRefCount < 0) contextRefCount = 0;
			}
			if(refCounts & REFCOUNT_DRAWABLE)
			{
				drawableRefCount--;
				if(drawableRefCount < 0) drawableRefCount = 0;
			}

			if(ctx && (force || (contextRefCount == 0 && drawableRefCount == 0)))
			{
				if(_eglBindAPI(EGL_OPENGL_API))
					_eglDestroyContext(EDPY, ctx);
				ctx = 0;
				contextRefCount = drawableRefCount = 0;
			}
		}

		EGLContext getContext(void) { return ctx; }

		util::CriticalSection &getMutex(void) { return mutex; }

		~RBOContext()
		{
			util::CriticalSection::SafeLock l(mutex);

			destroyContext(REFCOUNT_CONTEXT | REFCOUNT_DRAWABLE, true);
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
		// The context reference count is incremented in the body of
		// glXCreate*Context*() and decremented in the body of glXDestroyContext().
		// The drawable reference count is incremented when the EGL back end
		// creates a Pbuffer (either an application-requested Pbuffer or a Pbuffer
		// that the faker uses to emulate an OpenGL window) and decremented when
		// the EGL back end destroys a Pbuffer.  When both reference counts reach
		// 0, the RBO context is destroyed.  That prevents per-context objects,
		// such as textures, from surviving longer than the GLX drawable and
		// context in which they were created.  (Note, however, that this behavior
		// is technically incorrect, as OpenGL objects are supposed to survive only
		// as long as the context in which they were created.  It's the best we can
		// do given the limitations of emulating Pbuffers using RBOs.)
		int contextRefCount, drawableRefCount;
		// Mutex for the RBO context.  This guards any operations that alter the
		// context handle or reference counts above, or any operations that occur
		// while the RBO context is current (an OpenGL context can only be current
		// in one thread at a time.)
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
