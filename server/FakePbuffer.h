// Copyright (C)2019-2022 D. R. Commander
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

// This class emulates multi-buffered Pbuffers using RBOs, since EGL doesn't
// support multi-buffered Pbuffers.

#ifndef __FAKEPBUFFER_H__
#define __FAKEPBUFFER_H__

#include "glxvisual.h"


namespace backend
{
	class FakePbuffer
	{
		public:

			FakePbuffer(Display *dpy, VGLFBConfig config, const int *glxAttribs);
			~FakePbuffer(void);

			void createBuffer(bool useRBOContext, bool ignoreReadDrawBufs = false,
				bool ignoreDrawFBO = false, bool ignoreReadFBO = false);
			Display *getDisplay(void) { return dpy; }
			GLXDrawable getID(void) { return id; }
			VGLFBConfig getFBConfig(void) { return config; }
			GLuint getFBO(void) { return fbo; }
			int getWidth(void) { return width; }
			int getHeight(void) { return height; }
			void setDrawBuffer(GLenum mode, bool deferred);
			void setDrawBuffers(GLsizei n, const GLenum *bufs, bool deferred);
			void setReadBuffer(GLenum readBuf, bool deferred);
			void swap(void);

		private:

			void destroy(bool errorCheck);

			Display *dpy;
			VGLFBConfig config;
			GLXDrawable id;
			// 0 = front left, 1 = back left, 2 = front right, 3 = back right
			GLuint fbo, rboc[4], rbod;
			int width, height;
			static util::CriticalSection idMutex;
			static GLXDrawable nextID;
	};
}

#endif  // __FAKEPBUFFER_H__
