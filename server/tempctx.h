/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011 D. R. Commander
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

#ifndef __TEMPCTX_H
#define __TEMPCTX_H

#define EXISTING_DRAWABLE (GLXDrawable)-1

#include "faker-sym.h"

class tempctx
{
	public:

		tempctx(Display *dpy, GLXDrawable draw, GLXDrawable read,
			GLXContext ctx=glXGetCurrentContext(), GLXFBConfig config=NULL,
			int rendtype=0) : _olddpy(_glXGetCurrentDisplay()),
			_oldctx(glXGetCurrentContext()), _newctx(NULL),
			_oldread(_glXGetCurrentReadDrawable()),
			_olddraw(_glXGetCurrentDrawable()), _ctxchanged(false)
		{
			if(!dpy) return;
			if(!_olddpy) _olddpy=dpy;
			if(read==EXISTING_DRAWABLE) read=_oldread;
			if(draw==EXISTING_DRAWABLE) draw=_olddraw;
			if(draw && read && !ctx && config && rendtype)
				_newctx=ctx=_glXCreateNewContext(dpy, config, rendtype, NULL, True);
			if(((read || draw) && ctx && dpy)
				&& (_oldread!=read  || _olddraw!=draw || _oldctx!=ctx || _olddpy!=dpy))
			{
				if(!_glXMakeContextCurrent(dpy, draw, read, ctx))
					_throw("Could not bind OpenGL context to window (window may have disappeared)");
				_ctxchanged=true;
			}
		}

		void restore(void)
		{
			if(_ctxchanged)
			{
				_glXMakeContextCurrent(_olddpy, _olddraw, _oldread, _oldctx);
				_ctxchanged=false;
			}
			if(_newctx)
			{
				_glXDestroyContext(_olddpy, _newctx);  _newctx=NULL;
			}
		}

		~tempctx(void)
		{
			restore();
		}

	private:

		Display *_olddpy;
		GLXContext _oldctx, _newctx;
		GLXDrawable _oldread, _olddraw;
		bool _ctxchanged;
};

#endif // __TEMPCTX_H
