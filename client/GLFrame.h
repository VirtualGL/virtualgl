/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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

#ifndef __GLFRAME_H
#define __GLFRAME_H

// Frame drawn using OpenGL

#include <GL/glx.h>
#include "Frame.h"


namespace vglcommon
{
	class GLFrame : public Frame
	{
		public:

			GLFrame(char *dpystring, Window win);
			GLFrame(Display *dpy, Window win);
			~GLFrame(void);
			void init(rrframeheader &h, bool stereo);
			GLFrame &operator= (CompressedFrame &cf);
			void redraw(void);
			void drawTile(int x, int y, int width, int height);
			void sync(void);

		private:

			void init(void);
			int glError(void);

			Display *dpy;  Window win;
			GLXContext ctx;
			tjhandle tjhnd;
			bool newdpy;
	};
}

#endif // __GLFRAME_H
