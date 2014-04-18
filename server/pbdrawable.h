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

#ifndef __PBDRAWABLE_H__
#define __PBDRAWABLE_H__

#include "faker-sym.h"
#include "Mutex.h"
#include "X11Trans.h"
#include "fbx.h"

using namespace vglserver;


// A container class for the actual off-screen drawable

class glxdrawable
{
	public:

		glxdrawable(int, int, GLXFBConfig);
		glxdrawable(int, int, int, GLXFBConfig, const int *);
		~glxdrawable(void);
		GLXDrawable drawable(void) {return _drawable;}
		Pixmap pixmap(void)
		{
			if(!_ispixmap) _throw("Not a pixmap");
			return _pm;
		}
		int width(void) {return _w;}
		int height(void) {return _h;}
		int depth(void) {return _depth;}
		GLXFBConfig config(void) {return _config;}
		void clear(void);
		void swap(void);
		bool stereo(void) {return _stereo;}
		int format(void) {return _format;}
		XVisualInfo *visual(void);

	private:

		void setvisattribs(GLXFBConfig);
		bool _cleared, _stereo;
		GLXDrawable _drawable;
		int _w, _h, _depth;
		GLXFBConfig _config;
		int _format;
		Pixmap _pm;
		Window _win;
		bool _ispixmap;
};


class pbdrawable
{
	public:

		pbdrawable(Display *, Drawable);
		~pbdrawable(void);
		int init(int, int, GLXFBConfig);
		void setdirect(Bool);
		void clear(void);
		Display *get2ddpy(void);
		Drawable getx11drawable(void);
		GLXDrawable getglxdrawable(void);
		void copypixels(GLint, GLint, GLint, GLint, GLint, GLint, GLXDrawable);
		int width(void) {return _pb ? _pb->width() : -1;}
		int height(void) {return _pb ? _pb->height() : -1;}
		bool isinit(void) {return (_direct==True || _direct==False);}

	protected:

		void readpixels(GLint, GLint, GLint, GLint, GLint, GLenum, int, GLubyte *,
			GLint, bool);

		vglutil::CS _mutex;
		Display *_dpy;  Drawable _drawable;
		glxdrawable *_pb;  GLXFBConfig _config;
		GLXContext _ctx;
		Bool _direct;
		X11Trans *_x11trans;
		vglcommon::Profiler _prof_rb;
		char _autotestclr[80], _autotestrclr[80], _autotestframe[80];
		int _autotestframecount;
};


#endif // __PBDRAWABLE_H__
