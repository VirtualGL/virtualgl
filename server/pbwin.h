/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2009-2011 D. R. Commander
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

#ifndef __PBWIN_H__
#define __PBWIN_H__

#include "pbdrawable.h"
#include "vgltransconn.h"
#ifdef USEXV
#include "xvtrans.h"
#endif
#include "transplugin.h"


class pbwin : public pbdrawable
{
	public:

		pbwin(Display *, Window);
		~pbwin(void);
		void clear(void);
		void cleanup(void);
		GLXDrawable getglxdrawable(void);
		GLXDrawable updatedrawable(void);
		void checkconfig(GLXFBConfig);
		void resize(int, int);
		void checkresize(void);
		void initfromwindow(GLXFBConfig);
		void readback(GLint, bool, bool);
		void swapbuffers(void);
		bool stereo(void);
		bool _dirty, _rdirty;
		void wmdelete(void);

	private:

		int init(int, int, GLXFBConfig);
		void readpixels(GLint, GLint, GLint, GLint, GLint, GLenum, int, GLubyte *,
			GLint, bool, bool stereo);
		void makeanaglyph(rrframe *, int);
		void sendvgl(vgltransconn *, GLint, bool, bool, int, int, int, int);
		void sendx11(GLint, bool, bool, bool, int);
		void sendplugin(GLint, bool, bool, bool, int);
		#ifdef USEXV
		void sendxv(GLint, bool, bool, bool, int);
		#endif

		Display *_eventdpy;
		pbuffer *_oldpb;
		int _neww, _newh;
		x11trans *_x11trans;
		#ifdef USEXV
		xvtrans *_xvtrans;
		#endif
		vgltransconn *_vglconn;
		rrprofiler _prof_gamma, _prof_anaglyph;
		bool _syncdpy;
		transplugin *_plugin;
		bool _truecolor;
		bool _gammacorrectedvisuals;
		bool _stereovisual;
		rrframe _r, _g, _b, _f;
		bool _wmdelete;
		bool _newconfig;
};


#endif // __PBWIN_H__
