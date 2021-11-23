// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2009-2015, 2017-2021 D. R. Commander
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

#ifndef __VIRTUALDRAWABLE_H__
#define __VIRTUALDRAWABLE_H__

#include "glxvisual.h"
#include "Mutex.h"
#include "X11Trans.h"
#include "fbx.h"
#include "Frame.h"


namespace faker
{
	class VirtualDrawable
	{
		public:

			VirtualDrawable(Display *dpy, Drawable x11Draw);
			~VirtualDrawable(void);
			int init(int width, int height, VGLFBConfig config);
			void setDirect(Bool direct);
			void clear(void);
			Display *getX11Display(void);
			Drawable getX11Drawable(void);
			GLXDrawable getGLXDrawable(void);
			void copyPixels(GLint srcX, GLint srcY, GLint width, GLint height,
				GLint destX, GLint destY, GLXDrawable draw, GLint readBuf = GL_FRONT,
				GLint drawBuf = GL_FRONT_AND_BACK);
			int getWidth(void) { return oglDraw ? oglDraw->getWidth() : -1; }
			int getHeight(void) { return oglDraw ? oglDraw->getHeight() : -1; }
			bool isInit(void) { return direct == True || direct == False; }
			void setEventMask(unsigned long mask) { eventMask = mask; }
			unsigned long getEventMask(void) { return eventMask; }

		protected:

			// A container class for the actual off-screen drawable
			class OGLDrawable
			{
				public:

					OGLDrawable(Display *dpy, int width, int height, VGLFBConfig config);
					OGLDrawable(int width, int height, int depth, VGLFBConfig config,
						const int *attribs);
					#ifdef EGLBACKEND
					OGLDrawable(EGLDisplay edpy, int width, int height, EGLConfig config,
						const EGLint *pbAttribs);
					#endif
					~OGLDrawable(void);
					GLXDrawable getGLXDrawable(void) { return glxDraw; }

					Pixmap getPixmap(void)
					{
						if(!isPixmap) THROW("Not a pixmap");
						return pm;
					}

					int getWidth(void) { return width; }
					int getHeight(void) { return height; }
					int getDepth(void) { return depth; }
					int getRGBSize(void) { return rgbSize; }
					VGLFBConfig getFBConfig(void) { return config; }
					void clear(void);
					void swap(void);
					bool isStereo(void) { return stereo; }
					GLenum getFormat(void) { return glFormat; }

				private:

					void setVisAttribs(void);

					bool cleared, stereo;
					GLXDrawable glxDraw;
					Display *dpy;
					#ifdef EGLBACKEND
					EGLDisplay edpy;
					#endif
					int width, height, depth, rgbSize;
					VGLFBConfig config;
					GLenum glFormat;
					Pixmap pm;
					Window win;
					bool isPixmap;
			};

			void initReadbackContext(void);
			bool checkRenderMode(void);
			void readPixels(GLint x, GLint y, GLint width, GLint pitch, GLint height,
				GLenum glFormat, PF *pf, GLubyte *bits, GLint readBuf, bool stereo);

			util::CriticalSection mutex;
			Display *dpy;  Drawable x11Draw;
			#ifdef EGLBACKEND
			EGLDisplay edpy;
			#endif
			OGLDrawable *oglDraw;  VGLFBConfig config;
			GLXContext ctx;
			Bool direct;
			server::X11Trans *x11Trans;
			common::Profiler profReadback;
			int autotestFrameCount;

			GLuint pbo;
			int numSync, numFrames, lastFormat;
			bool usePBO;
			bool alreadyPrinted, alreadyWarned, alreadyWarnedRenderMode;
			const char *ext;
			unsigned long eventMask;
	};
}

#endif  // __VIRTUALDRAWABLE_H__
