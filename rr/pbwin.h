/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#ifndef __PBWIN_H__
#define __PBWIN_H__

#include "faker-sym.h"
#include "rrmutex.h"
#include "rrblitter.h"
#include "fbx.h"
#ifdef USEGLP
#include <GL/glp.h>
#endif

// A container class for the actual Pbuffer

class pbuffer
{
	public:

		pbuffer(int, int, GLXFBConfig);
		GLXDrawable drawable(void) {return d;}
		~pbuffer(void);
		int width(void) {return _w;}
		int height(void) {return _h;}
		void clear(void);
		void swap(void);

	private:

		bool cleared;
		GLXPbuffer d;
		int _w, _h;
};

class pbwin
{
	public:

		pbwin(Display *dpy, Window win);
		~pbwin(void);
		void clear(void);
		void cleanup(void);
		GLXDrawable getdrawable(void);
		GLXDrawable updatedrawable(void);
		void resize(int, int);
		void initfromwindow(GLXFBConfig config);
		Display *getwindpy(void);
		Window getwin(void);
		void readback(bool);
		void readback(GLint, bool);
		void swapbuffers(void);

	private:

		int init(int, int, GLXFBConfig);
		void blit(GLint);
		void readpixels(GLint, GLint, GLint, GLint, GLint, GLenum, GLubyte *,
			GLint, bool);

		bool force;
		rrcs mutex;
		Display *windpy;  Window win;
		pbuffer *oldpb, *pb;  GLXFBConfig config;
		int neww, newh;
		rrblitter *blitter;
		rrprofiler prof_rb;
};

#endif
