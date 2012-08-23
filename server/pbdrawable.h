/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2009-2012 D. R. Commander
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

#ifndef __PBDRAWABLE_H__
#define __PBDRAWABLE_H__

#include "faker-sym.h"
#include "rrmutex.h"
#include "x11trans.h"
#include "fbx.h"


// A container class for the actual Pbuffer

class pbuffer
{
	public:

		pbuffer(int, int, GLXFBConfig);
		GLXDrawable drawable(void) {return _drawable;}
		~pbuffer(void);
		int width(void) {return _w;}
		int height(void) {return _h;}
		GLXFBConfig config(void) {return _config;}
		void clear(void);
		void swap(void);
		bool stereo(void) {return _stereo;}
		int format(void) {return _format;}

	private:

		bool _cleared, _stereo;
		GLXPbuffer _drawable;
		int _w, _h;
		GLXFBConfig _config;
		int _format;
};


class pbdrawable
{
	public:

		pbdrawable(Display *, Drawable);
		~pbdrawable(void);
		int init(int, int, GLXFBConfig);
		void clear(void);
		Display *get2ddpy(void);
		Drawable getx11drawable(void);
		GLXDrawable getglxdrawable(void);
		void copypixels(GLint, GLint, GLint, GLint, GLint, GLint, GLXDrawable);

	protected:

		void readpixels(GLint, GLint, GLint, GLint, GLint, GLenum, int, GLubyte *,
			GLint, bool, bool stereo);

		rrcs _mutex;
		Display *_dpy;  Drawable _drawable;
		pbuffer *_pb;  GLXFBConfig _config;
		GLXContext _ctx;
		x11trans *_x11trans;
		rrprofiler _prof_rb;
		char _autotestclr[80], _autotestrclr[80], _autotestframe[80];
		int _autotestframecount;
};


#endif // __PBDRAWABLE_H__
