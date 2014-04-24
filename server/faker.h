/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011, 2013-2014 D. R. Commander
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

#ifndef __FAKER_H__
#define __FAKER_H__

#include <X11/Xlib.h>
#include "Error.h"
#include "Log.h"
#include "Mutex.h"
#include "glx.h"


extern vglutil::CS globalmutex;
extern Display *_localdpy;
extern void __vgl_safeexit(int);
extern int __shutdown;


#define isRemote(dpy) (_localdpy && dpy!=_localdpy)

#define isFront(drawbuf) (drawbuf==GL_FRONT || drawbuf==GL_FRONT_AND_BACK \
	|| drawbuf==GL_FRONT_LEFT || drawbuf==GL_FRONT_RIGHT || drawbuf==GL_LEFT \
	|| drawbuf==GL_RIGHT)

#define isRight(drawbuf) (drawbuf==GL_RIGHT || drawbuf==GL_FRONT_RIGHT \
	|| drawbuf==GL_BACK_RIGHT)


static inline int drawingToFront(void)
{
	GLint drawbuf=GL_BACK;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return isFront(drawbuf);
}


static inline int drawingToRight(void)
{
	GLint drawbuf=GL_LEFT;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return isRight(drawbuf);
}


static inline int isDead(void)
{
	int retval=0;
	globalmutex.lock(false);
	retval=__shutdown;
	globalmutex.unlock(false);
	return retval;
}


#define DIE(f,m) {  \
	if(!isDead())  \
		vglout.print("[VGL] ERROR: in %s--\n[VGL]    %s\n", f, m);  \
			__vgl_safeexit(1);}


#define TRY() try {
#define CATCH() }  \
	catch(vglutil::Error &e) { DIE(e.getMethod(), e.getMessage()); }


// Tracing stuff

static int __vgltracelevel=0;

#define prargd(a) vglout.print("%s=0x%.8lx(%s) ", #a, (unsigned long)a,  \
	a? DisplayString(a):"NULL")
#define prargs(a) vglout.print("%s=%s ", #a, a?a:"NULL")
#define prargx(a) vglout.print("%s=0x%.8lx ", #a, (unsigned long)a)
#define prargi(a) vglout.print("%s=%d ", #a, a)
#define prargf(a) vglout.print("%s=%f ", #a, (double)a)
#define prargv(a) vglout.print("%s=0x%.8lx(0x%.2lx) ", #a, (unsigned long)a,  \
	a? a->visualid:0)
#define prargc(a) vglout.print("%s=0x%.8lx(0x%.2x) ", #a, (unsigned long)a,  \
	a? _FBCID(a):0)
#define prargal11(a) if(a) {  \
	vglout.print(#a"=[");  \
	for(int __an=0; a[__an]!=None; __an++) {  \
		vglout.print("0x%.4x", a[__an]);  \
		if(a[__an]!=GLX_USE_GL && a[__an]!=GLX_DOUBLEBUFFER  \
			&& a[__an]!=GLX_STEREO && a[__an]!=GLX_RGBA)  \
			vglout.print("=0x%.4x", a[++__an]);  \
		vglout.print(" ");  \
	}  vglout.print("] ");}
#define prargal13(a) if(a) {  \
	vglout.print(#a"=[");  \
	for(int __an=0; a[__an]!=None; __an+=2) {  \
		vglout.print("0x%.4x=0x%.4x ", a[__an], a[__an+1]);  \
	}  vglout.print("] ");}

#define opentrace(f)  \
	double __vgltracetime=0.;  \
	if(fconfig.trace) {  \
		if(__vgltracelevel>0) {  \
			vglout.print("\n[VGL] ");  \
			for(int __i=0; __i<__vgltracelevel; __i++) vglout.print("  ");  \
		}  \
		else vglout.print("[VGL] ");  \
		__vgltracelevel++;  \
		vglout.print("%s (", #f);  \

#define starttrace()  \
		__vgltracetime=getTime();  \
	}

#define stoptrace()  \
	if(fconfig.trace) {  \
		__vgltracetime=getTime()-__vgltracetime;

#define closetrace()  \
		vglout.PRINT(") %f ms\n", __vgltracetime*1000.);  \
		__vgltracelevel--;  \
		if(__vgltracelevel>0) {  \
			vglout.print("[VGL] ");  \
			if(__vgltracelevel>1)  \
				for(int __i=0; __i<__vgltracelevel-1; __i++) vglout.print("  ");  \
    }  \
	}

#endif // __FAKER_H__
