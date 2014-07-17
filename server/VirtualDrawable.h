/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2009-2014 D. R. Commander
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

#ifndef __VIRTUALDRAWABLE_H__
#define __VIRTUALDRAWABLE_H__

#include "faker-sym.h"
#include "Mutex.h"
#include "X11Trans.h"
#include "fbx.h"


namespace vglserver
{
	class VirtualDrawable
	{
		public:

			VirtualDrawable(Display *dpy, Drawable x11Draw);
			~VirtualDrawable(void);
			int init(int width, int height, GLXFBConfig config);
			void setDirect(Bool direct);
			void clear(void);
			Display *getX11Display(void);
			Drawable getX11Drawable(void);
			GLXDrawable getGLXDrawable(void);
			void copyPixels(GLint srcX, GLint srcY, GLint width, GLint height,
				GLint destX, GLint destY, GLXDrawable draw);
			int getWidth(void) { return oglDraw ? oglDraw->getWidth() : -1; }
			int getHeight(void) { return oglDraw ? oglDraw->getHeight() : -1; }
			bool isInit(void) { return (direct==True || direct==False); }

		protected:

			// A container class for the actual off-screen drawable
			class OGLDrawable
			{
				public:

					OGLDrawable(int width, int height, GLXFBConfig config);
					OGLDrawable(int width, int height, int depth, GLXFBConfig config,
						const int *attribs);
					~OGLDrawable(void);
					GLXDrawable getGLXDrawable(void) { return glxDraw; }

					Pixmap getPixmap(void)
					{
						if(!isPixmap) _throw("Not a pixmap");
						return pm;
					}

					int getWidth(void) { return width; }
					int getHeight(void) { return height; }
					int getDepth(void) { return depth; }
					GLXFBConfig getConfig(void) { return config; }
					void clear(void);
					void swap(void);
					bool isStereo(void) { return stereo; }
					GLenum getFormat(void) { return format; }
					XVisualInfo *getVisual(void);

				private:

					void setVisAttribs(void);

					bool cleared, stereo;
					GLXDrawable glxDraw;
					int width, height, depth;
					GLXFBConfig config;
					GLenum format;
					Pixmap pm;
					Window win;
					bool isPixmap;
			};

			void readPixels(GLint x, GLint y, GLint width, GLint pitch, GLint height,
				GLenum format, int pixelSize, GLubyte *bits, GLint buf, bool stereo);

			vglutil::CriticalSection mutex;
			Display *dpy;  Drawable x11Draw;
			OGLDrawable *oglDraw;  GLXFBConfig config;
			GLXContext ctx;
			Bool direct;
			X11Trans *x11Trans;
			vglcommon::Profiler profReadback;
			char autotestColor[80], autotestRColor[80], autotestFrame[80];
			int autotestFrameCount;
	};
}

#endif // __VIRTUALDRAWABLE_H__
