/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005-2008 Sun Microsystems, Inc.
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

#include <windows.h>
#include <GL/gl.h>
#include <GL/wglext.h>
#include "rrmutex.h"
#include "rrblitter.h"
#include "fbx.h"

// A container class for the actual Pbuffer

class pbuffer
{
	public:

		pbuffer(HDC, int, int, int);
		HDC hdc(void) {return _hdc;}
		~pbuffer(void);
		int width(void) {return _w;}
		int height(void) {return _h;}
		void clear(void);
		void swap(void);
		bool stereo(void) {return _stereo;}

	private:

		bool _cleared, _stereo;
		HPBUFFERARB _drawable;
		int _w, _h;
		HDC _hdc;
};

class pbwin
{
	public:

		pbwin(HWND, HDC);
		~pbwin(void);
		void clear(void);
		void cleanup(void);
		HDC gethdc(void);
		HDC updatehdc(void);
		void resize(int, int);
		void checkresize(void);
		void initfromwindow(HDC);
		void readback(GLint, bool);
		void swapbuffers(void);
		bool stereo(void);
		bool _dirty, _rdirty;

	private:

		int init(HDC, int, int);
		void readpixels(GLint, GLint, GLint, GLint, GLint, GLenum, int, GLubyte *,
			GLint, bool stereo=false);
		void makeanaglyph(rrframe *, int);
		void sendw32(GLint, bool, bool, int);

		bool _force;
		rrcs _mutex;
		HWND _hwnd;
		pbuffer *_pb;
		rrblitter *_blitter;
		rrprofiler _prof_rb, _prof_gamma, _prof_anaglyph;
		char _autotestclr[80], _autotestrclr[80], _autotestframe[80];
		int _autotestframecount;
		rrframe _r, _g, _b;
		int _pixelformat;
};

#endif
