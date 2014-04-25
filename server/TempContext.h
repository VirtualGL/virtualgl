/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014 D. R. Commander
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

// This is a container class which allows us to temporarily swap in a new
// drawable and then "pop the stack" after we're done with it.  It does nothing
// unless there is already a valid context established

#ifndef __TEMPCONTEXT_H__
#define __TEMPCONTEXT_H__

#include "faker-sym.h"


#define EXISTING_DRAWABLE (GLXDrawable)-1


namespace vglserver
{
	class TempContext
	{
		public:

			TempContext(Display *dpy, GLXDrawable draw, GLXDrawable read,
				GLXContext ctx=glXGetCurrentContext(), GLXFBConfig config=NULL,
				int renderType=0) : olddpy(_glXGetCurrentDisplay()),
				oldctx(glXGetCurrentContext()), newctx(NULL),
				oldread(_glXGetCurrentReadDrawable()),
				olddraw(_glXGetCurrentDrawable()), ctxChanged(false)
			{
				if(!dpy) return;
				if(!olddpy) olddpy=dpy;
				if(read==EXISTING_DRAWABLE) read=oldread;
				if(draw==EXISTING_DRAWABLE) draw=olddraw;
				if(draw && read && !ctx && config && renderType)
					newctx=ctx=_glXCreateNewContext(dpy, config, renderType, NULL, True);
				if(((read || draw) && ctx && dpy)
					&& (oldread!=read  || olddraw!=draw || oldctx!=ctx || olddpy!=dpy))
				{
					if(!_glXMakeContextCurrent(dpy, draw, read, ctx))
						_throw("Could not bind OpenGL context to window (window may have disappeared)");
					ctxChanged=true;
				}
			}

			void restore(void)
			{
				if(ctxChanged)
				{
					_glXMakeContextCurrent(olddpy, olddraw, oldread, oldctx);
					ctxChanged=false;
				}
				if(newctx)
				{
					_glXDestroyContext(olddpy, newctx);  newctx=NULL;
				}
			}

			~TempContext(void)
			{
				restore();
			}

		private:

			Display *olddpy;
			GLXContext oldctx, newctx;
			GLXDrawable oldread, olddraw;
			bool ctxChanged;
	};
}

#endif // __TEMPCONTEXT_H__
