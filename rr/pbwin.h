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
#include <pthread.h>
#include "rrblitter.h"
#include "rrprofiler.h"

// A container class for the actual Pbuffer

class pbuffer
{
	public:

		pbuffer(Display *, int, int, GLXFBConfig);
		GLXDrawable drawable(void) {return d;}
		~pbuffer(void);
		int width(void) {return w;}
		int height(void) {return h;}
		void clear(void);
		void swap(void);

	private:

		bool cleared;
		GLXPbuffer d;
		Display *dpy;
		int w, h;
};

class pbwin
{
	public:

		pbwin(Display *dpy, Window win, Display *);
		~pbwin(void);
		void clear(void);
		void cleanup(void);
		GLXDrawable getdrawable(void);
		GLXDrawable updatedrawable(void);
		void resize(int, int);
		void initfromwindow(GLXFBConfig config);
		Display *getwindpy(void);
		Display *getpbdpy(void);
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
		pthread_mutex_t mutex;
		Display *pbdpy, *windpy;  Window win;
		pbuffer *oldpb, *pb;  GLXFBConfig config;
		int neww, newh;
		rrblitter *blitter;
		rrprofiler prof_rb;
};

#endif
