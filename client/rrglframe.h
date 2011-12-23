/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#ifndef __RRGLFRAME_H
#define __RRGLFRAME_H

// Frame drawn using OpenGL

#ifdef XDK
 #include "xdk-sym.h"
#else
 #include <GL/glx.h>
#endif
#include "rrframe.h"


class rrglframe : public rrframe
{
	public:

		rrglframe(char *, Window);
		rrglframe(Display *, Window);
		void init(Display *, Window);
		~rrglframe(void);
		void init(rrframeheader &, bool);
		rrglframe &operator= (rrcompframe &);
		void redraw(void);
		void drawtile(int, int, int, int);
		void sync(void);

	private:

		int glerror(void);

		#ifdef XDK
		static int _Instancecount;
		static
		#endif
		Display *_dpy;  Window _win;
		GLXContext _ctx;
		tjhandle _tjhnd;
		bool _newdpy;
};


#endif // __RRGLFRAME_H
