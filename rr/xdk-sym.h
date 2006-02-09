/* Copyright (C)2005 Sun Microsystems, Inc.
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

#ifndef __XDK_SYM_H__
#define __XDK_SYM_H__

#ifdef XDK

#define NOREDIRECT
#include <GL/glx.h>
#include "rrerror.h"

#ifdef __LOCALSYM__
#define symdef(f) _##f##Type __##f=NULL
#else
#define symdef(f) extern _##f##Type __##f
#endif

#define checksym(s) {if(!__##s) _throw(#s" symbol not loaded");}

#define funcdef0(rettype, f, ret) \
	typedef rettype (*_##f##Type)(void); \
	symdef(f); \
	static inline rettype _##f(void) {checksym(f);  ret __##f();}

#define funcdef1(rettype, f, at1, a1, ret) \
	typedef rettype (*_##f##Type)(at1); \
	symdef(f); \
	static inline rettype _##f(at1 a1) {checksym(f);  ret __##f(a1);}

#define funcdef2(rettype, f, at1, a1, at2, a2, ret) \
	typedef rettype (*_##f##Type)(at1, at2); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2) {checksym(f);  ret __##f(a1, a2);}

#define funcdef3(rettype, f, at1, a1, at2, a2, at3, a3, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3) {checksym(f);  ret __##f(a1, a2, a3);}

#define funcdef4(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4) {checksym(f);  ret __##f(a1, a2, a3, a4);}

#define funcdef5(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) {checksym(f);  ret __##f(a1, a2, a3, a4, a5);}

#ifdef __cplusplus
extern "C" {
#endif

funcdef4(GLXContext, glXCreateContext, Display *, dpy, XVisualInfo *, vis,
	GLXContext, share_list, Bool, direct, return);

funcdef2(void, glXDestroyContext, Display *, dpy, GLXContext, ctx,);

funcdef0(GLXContext, glXGetCurrentContext, return);

funcdef0(Display*, glXGetCurrentDisplayEXT, return);

funcdef0(GLXDrawable, glXGetCurrentDrawable, return);

funcdef3(Bool, glXMakeCurrent, Display *, dpy, GLXDrawable, drawable,
	GLXContext, ctx, return);

funcdef2(void, glXSwapBuffers, Display *, dpy, GLXDrawable, drawable,);

funcdef1(void, glDrawBuffer, GLenum, drawbuf,);

funcdef5(void, glDrawPixels, GLsizei, width, GLsizei, height, GLenum, format,
	GLenum, type, const GLvoid*, pixels,);

funcdef0(void, glFinish,);

funcdef4(void, glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei, height,);

funcdef0(GLenum, glGetError, return);

funcdef2(void, glGetIntegerv, GLenum, pname, GLint *, params,);

funcdef2(void, glPixelStorei, GLenum, pname, GLint, param,);

funcdef2(void, glRasterPos2f, GLfloat, x, GLfloat, y,);

#ifdef __cplusplus
}
#endif

void __vgl_loadsymbols(void);
void __vgl_unloadsymbols(void);

#endif // XDK

#endif
