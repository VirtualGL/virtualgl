/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

// This is a container class which allows us to temporarily swap in a new
// drawable and then "pop the stack" after we're done with it.  It does nothing
// unless there is already a valid context established

#ifndef __TEMPCTX_H
#define __TEMPCTX_H

#define EXISTING_DRAWABLE (GLXDrawable)-1

#ifdef INFAKER
 #include "faker-sym.h"
 #ifdef USEGLP
 #define glXGetCurrentContext GetCurrentContext
 #endif
 #define glXGetCurrentDisplay GetCurrentDisplay
 #define glXGetCurrentDrawable GetCurrentDrawable
 #define glXGetCurrentReadDrawable GetCurrentReadDrawable
 #define glXMakeCurrent _glXMakeCurrent
 #define glXMakeContextCurrent _glXMakeContextCurrent
#elif defined(XDK)
 #include "xdk-sym.h"
 #define glXGetCurrentContext _glXGetCurrentContext
 #define glXGetCurrentDisplayEXT _glXGetCurrentDisplayEXT
 #define glXGetCurrentDrawable _glXGetCurrentDrawable
 #define glXMakeCurrent _glXMakeCurrent
#else
 #include <GL/glx.h>
#endif

#ifdef USEGLP
 #include <GL/glp.h>
 #include "glpweak.h"
 #include "fakerconfig.h"
 extern FakerConfig fconfig;
#endif

class tempctx
{
	public:

		tempctx(Display *dpy, GLXDrawable draw, GLXDrawable read,
			GLXContext ctx=glXGetCurrentContext(), bool glx11=false) :
			_dpy(glXGetCurrentDisplay()), _ctx(glXGetCurrentContext()),
			#ifndef GLX11
			_read(glXGetCurrentReadDrawable()),
			#else
			_read(EXISTING_DRAWABLE),
			#endif
			_draw(glXGetCurrentDrawable()),
			_mc(false), _glx11(glx11)
		{
			#ifdef USEGLP
			if(!fconfig.glp && !dpy) return;
			#else
			if(!dpy) return;
			#endif
			if(read==EXISTING_DRAWABLE) read=_read;
			if(draw==EXISTING_DRAWABLE) draw=_draw;
			if(_read!=read || _draw!=draw || _ctx!=ctx
			#ifdef USEGLP
			|| (!fconfig.glp && _dpy!=dpy)
			#else
			|| (_dpy!=dpy)
			#endif
			)
			{
				Bool ret=True;
				#ifdef USEGLP
				if(fconfig.glp) ret=glPMakeContextCurrent(draw, read, ctx);
				else
				#endif
				{
					if(glx11) ret=glXMakeCurrent(dpy, draw, ctx);
					#ifndef GLX11
					else ret=glXMakeContextCurrent(dpy, draw, read, ctx);
					#endif
				}
				if(!ret) _throw("Could not bind OpenGL context to window (window may have disappeared)");
				_mc=true;
			}
		}

		void restore(void)
		{
			if(_mc && _ctx && (_draw || _read))
			{
				#ifdef USEGLP
				if(fconfig.glp) glPMakeContextCurrent(_draw, _read, _ctx);
				else
				#endif
				if(_dpy)
				{
					if(_glx11) glXMakeCurrent(_dpy, _draw, _ctx);
					#ifndef GLX11
					else glXMakeContextCurrent(_dpy, _draw, _read, _ctx);
					#endif
				}
			}
		}

		~tempctx(void)
		{
			restore();
		}

	private:

		Display *_dpy;
		GLXContext _ctx;
		GLXDrawable _read, _draw;
		bool _mc, _glx11;
};

#if defined(INFAKER)||defined(XDK)
 #undef glXGetCurrentContext
 #undef glXGetCurrentDisplay
 #undef glXGetCurrentDrawable
 #undef glXGetCurrentReadDrawable
 #undef glXMakeCurrent
 #undef glXMakeContextCurrent
#endif

#endif // __TEMPCTX_H
