/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005 Sun Microsystems, Inc.
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
#include "rrdisplayclient.h"
#include "fbx.h"
#ifdef USEGLP
#include <GL/glp.h>
#include "glpweak.h"
#endif

// A container class for the actual Pbuffer

class pbuffer
{
	public:

		pbuffer(int, int, GLXFBConfig);
		GLXDrawable drawable(void) {return _drawable;}
		~pbuffer(void);
		int width(void) {return _w;}
		int height(void) {return _h;}
		void clear(void);
		void swap(void);
		bool stereo(void) {return _stereo;}

	private:

		bool _cleared, _stereo;
		GLXPbuffer _drawable;
		int _w, _h;
};

class pbwin
{
	public:

		pbwin(Display *, Window);
		~pbwin(void);
		void clear(void);
		void cleanup(void);
		GLXDrawable getdrawable(void);
		GLXDrawable updatedrawable(void);
		void resize(int, int);
		void initfromwindow(GLXFBConfig);
		Display *getwindpy(void);
		Window getwin(void);
		void readback(GLint, bool, bool);
		void swapbuffers(void);
		bool stereo(void);
		bool _dirty, _rdirty;

	private:

		int init(int, int, GLXFBConfig);
		void blit(GLint);
		void readpixels(GLint, GLint, GLint, GLint, GLint, GLenum, int, GLubyte *,
			GLint, bool stereo=false);
		void makeanaglyph(rrframe *, int);
		void senddirect(rrdisplayclient *, GLint, bool, bool, int, int, int, int,
			bool);
		void sendraw(GLint, bool, bool, bool, int);
		void sendsr(GLint, bool, bool, int);

		bool _force;
		rrcs _mutex;
		Display *_windpy;  Window _win;
		pbuffer *_oldpb, *_pb;  GLXFBConfig _config;
		int _neww, _newh;
		rrblitter *_blitter;
		rrdisplayclient *_rrdpy, *_rrmoviedpy;
		rrprofiler _prof_rb, _prof_gamma, _prof_anaglyph;
		bool _syncdpy;
		char _autotestclr[80], _autotestrclr[80], _autotestframe[80];
		int _autotestframecount;
		void *_sunrayhandle;
		int _usesunray;
		bool _truecolor;
		bool _gammacorrectedvisual;
		bool _stereovisual;
		rrframe _r, _g, _b;
};

#endif
