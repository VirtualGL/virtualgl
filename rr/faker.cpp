/* Copyright (C)2004 Landmark Graphics
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <string.h>
#include "rrutil.h"
#include "rrtimer.h"
#include "rrthread.h"
#include "rrmutex.h"
#include "fakerconfig.h"

FakerConfig fconfig;

#include "faker-dpyhash.h"
#include "faker-winhash.h"
#include "faker-ctxhash.h"
#include "faker-vishash.h"
#include "faker-pmhash.h"
#include "faker-sym.h"
#include <sys/types.h>
#include <unistd.h>
#ifdef __DEBUG__
#include "x11err.h"
#endif

// Did I mention that debugging real-time systems is hell?
void _fprintf (FILE *f, const char *format, ...)
{
	static rrcs mutex;  static rrtimer timer;
	rrcs::safelock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	fprintf(f, "T0x%.8lx %.6f C0x%.8lx D0x%.8lx R0x%.8lx - ", (unsigned long)rrthread_id(), timer.time(),
		(unsigned long)glXGetCurrentContext(), glXGetCurrentDrawable(), glXGetCurrentReadDrawable());
	vfprintf(f, format, arglist);
	fflush(f);
	va_end(arglist);
}

// Globals
Display *_localdpy=NULL;
static rrcs globalmutex;
winhash *_winh=NULL;  dpyhash *_dpyh=NULL;  ctxhash ctxh;  vishash vish;  pmhash pmh;
#define dpyh (*(_dpyh?_dpyh:(_dpyh=new dpyhash())))
#define winh (*(_winh?_winh:(_winh=new winhash())))

static int __shutdown=0;

int isdead(void)
{
	int retval=0;
	rrcs::safelock l(globalmutex);
	retval=__shutdown;
	return retval;
}

void safeexit(int retcode)
{
	int shutdown;
	globalmutex.lock(false);
	shutdown=__shutdown;
	if(!__shutdown)
	{
		__shutdown=1;
		pmh.killhash();
		vish.killhash();
		ctxh.killhash();
		if(_dpyh) _dpyh->killhash();
		if(_winh) _winh->killhash();
	}
	globalmutex.unlock(false);
	if(!shutdown) exit(retcode);
	else pthread_exit(0);
}

#define _die(f,m) {if(!isdead()) fprintf(stderr, "%s--\n%s\n", f, m);  safeexit(1);}

#define TRY() try {
#define CATCH() } catch(rrerror &e) {_die(e.getMethod(), e.getMessage());}

#include "faker-glx.cpp"

#if 0
// Used during debug so we can get a stack trace from an X11 protocol error
#ifdef __DEBUG__
int xhandler(Display *dpy, XErrorEvent *xe)
{
	fprintf(stderr, "X11 Error: %s\n", x11error(xe->error_code));  fflush(stderr);
	return 0;
}
#endif
#endif

void fakerinit(void)
{
	static int init=0;

	rrcs::safelock l(globalmutex);
	if(init) return;
	init=1;

	if(!_dpyh) errifnot(_dpyh=new dpyhash());
	if(!_winh) errifnot(_winh=new winhash())
	#ifdef __DEBUG__
	if(getenv("RRDEBUGPAUSE"))
	{
		printf("Attach debugger to process %d ...\n", getpid());
		sleep(30);
	}
	#if 0
	XSetErrorHandler(xhandler);
	#endif
	#endif

	loadsymbols();
	if(!_localdpy)
	{
		if((_localdpy=_XOpenDisplay(fconfig.localdpystring))==NULL)
		{
			fprintf(stderr, "Could not open display %s.\n", fconfig.localdpystring);
			safeexit(1);
		}
	}
}

////////////////
// X11 functions
////////////////

extern "C" {

Display *XOpenDisplay(_Xconst char* name)
{
	Display *dpy=NULL;
	TRY();
	fakerinit();
	if(!(dpy=_XOpenDisplay(name))) return NULL;
	dpyh.add(dpy);
	CATCH();
	return dpy;
}

int XCloseDisplay(Display *dpy)
{
	return _XCloseDisplay(dpy);
}

Window XCreateWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	int depth, unsigned int c_class, Visual *visual, unsigned long valuemask,
	XSetWindowAttributes *attributes)
{
	Window win=0;
	TRY();
	if(!(win=_XCreateWindow(dpy, parent, x, y, width, height, border_width,
		depth, c_class, visual, valuemask, attributes))) return 0;
	winh.add(dpy, win);
	CATCH();
	return win;
}

Window XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	unsigned long border, unsigned long background)
{
	Window win=0;
	TRY();
	if(!(win=_XCreateSimpleWindow(dpy, parent, x, y, width, height, border_width,
		border, background))) return 0;
	winh.add(dpy, win);
	CATCH();
	return win;
}

int XDestroyWindow(Display *dpy, Window win)
{
	TRY();
	winh.remove(dpy, win);
	CATCH();
	return _XDestroyWindow(dpy, win);
}

#if 0
Window FindTopLevelWindow(Display *dpy, Window win)
{
	Window w=win, root, parent, *children;  unsigned int nchildren;
	for(;;)
	{
		XQueryTree(dpy, w, &root, &parent, &children, &nchildren);
		if(parent==root) break;
		else w=parent;
	}
	return w;
}

// Walk the window tree, notifying any GL windows belonging to the same TLW
void SetQualRecursive(Display *dpy, Window start, int qual, int subsamp, bool readback)
{
	Window root, parent, *children;  unsigned int nchildren;
	unsigned int i;  pbwin *pbw=NULL;
	pbw=winh.findpb(dpy, start);
	if(pbw)
	{
		_fprintf(stdout, "SQR: Setting qual\n");
		pbw->setqual(qual, subsamp, readback);
	}
	XQueryTree(dpy, start, &root, &parent, &children, &nchildren);
	if(nchildren==0) return;
	for(i=0; i<nchildren; i++) SetQualRecursive(dpy, children[i], qual, subsamp, readback);
}
#endif

void _HandleEvent(Display *dpy, XEvent *xe)
{
	pbwin *pbw=NULL;
	if(xe && xe->type==ConfigureNotify)
	{
		pbw=winh.findpb(dpy, xe->xconfigure.window);
		if(pbw) pbw->resize(xe->xconfigure.width, xe->xconfigure.height);
	}
	#if 0
	else if(xe && xe->type==ButtonPress)
	{
		fconfig.setloqual();
//		Window start=FindTopLevelWindow(dpy, xe->xbutton.window);
//		SetQualRecursive(dpy, start, fconfig.loqual, fconfig.losubsamp, false);
	}
	else if(xe && xe->type==ButtonRelease)
	{
		fconfig.sethiqual();
//		Window start=FindTopLevelWindow(dpy, xe->xbutton.window);
//		SetQualRecursive(dpy, start, fconfig.hiqual, fconfig.hisubsamp, false);
	}
	#endif
}

int XNextEvent(Display *dpy, XEvent *xe)
{
	int retval=0;
	TRY();
	retval=_XNextEvent(dpy, xe);
	_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}

int XWindowEvent(Display *dpy, Window win, long event_mask, XEvent *xe)
{
	int retval=0;
	TRY();
	retval=_XWindowEvent(dpy, win, event_mask, xe);
	_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}

Bool XCheckWindowEvent(Display *dpy, Window win, long event_mask, XEvent *xe)
{
	Bool retval=0;
	TRY();
	if((retval=_XCheckWindowEvent(dpy, win, event_mask, xe))==True)
		_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}

int XMaskEvent(Display *dpy, long event_mask, XEvent *xe)
{
	int retval=0;
	TRY();
	retval=_XMaskEvent(dpy, event_mask, xe);
	_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}

Bool XCheckMaskEvent(Display *dpy, long event_mask, XEvent *xe)
{
	Bool retval=0;
	TRY();
	if((retval=_XCheckMaskEvent(dpy, event_mask, xe))==True)
		_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}

Bool XCheckTypedEvent(Display *dpy, int event_type, XEvent *xe)
{
	Bool retval=0;
	TRY();
	if((retval=_XCheckTypedEvent(dpy, event_type, xe))==True)
		_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}

Bool XCheckTypedWindowEvent(Display *dpy, Window win, int event_type, XEvent *xe)
{
	Bool retval=0;
	TRY();
	if((retval=_XCheckTypedWindowEvent(dpy, win, event_type, xe))==True)
		_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}

int XConfigureWindow(Display *dpy, Window win, unsigned int value_mask, XWindowChanges *values)
{
	TRY();
	pbwin *pbw=NULL;
	pbw=winh.findpb(dpy, win);
	if(pbw && values)
		pbw->resize(value_mask&CWWidth?values->width:0, value_mask&CWHeight?values->height:0);
	CATCH();
	return _XConfigureWindow(dpy, win, value_mask, values);
}

int XResizeWindow(Display *dpy, Window win, unsigned int width, unsigned int height)
{
	TRY();
	pbwin *pbw=NULL;
	pbw=winh.findpb(dpy, win);
	if(pbw) pbw->resize(width, height);
	CATCH();
	return _XResizeWindow(dpy, win, width, height);
}

int XMoveResizeWindow(Display *dpy, Window win, int x, int y, unsigned int width, unsigned int height)
{
	TRY();
	pbwin *pbw=NULL;
	pbw=winh.findpb(dpy, win);
	if(pbw) pbw->resize(width, height);
	CATCH();
	return _XMoveResizeWindow(dpy, win, x, y, width, height);
}

// We have to trap any attempts to copy from/to a GLXPixmap (ugh)
// It should work as long as a valid GLX context is current
// in the calling thread
int XCopyArea(Display *dpy, Drawable src, Drawable dst, GC gc, int src_x, int src_y,
	unsigned int w, unsigned int h, int dest_x, int dest_y)
{
	TRY();
	pbuffer *pb;
	GLXDrawable read=src, draw=dst;  bool srcpm=false, dstpm=false;
	if((pb=pmh.find(dpy, src))!=0) {read=pb->drawable();  srcpm=true;}
	if((pb=pmh.find(dpy, dst))!=0) {draw=pb->drawable();  dstpm=true;}
	if(!srcpm && !dstpm) return _XCopyArea(dpy, src, dst, gc, src_x, src_y, w, h, dest_x, dest_y);

	GLXDrawable oldread=glXGetCurrentReadDrawable();
	GLXDrawable olddraw=glXGetCurrentDrawable();
	GLXContext ctx=glXGetCurrentContext();
	Display *olddpy=glXGetCurrentDisplay();
	if(!ctx || !olddpy) return 0;  // Does ... not ... compute

	// Intentionally call the faked function so it will map a PB if src or dst is a window
	glXMakeContextCurrent(dpy, draw, read, ctx);

	unsigned int srch, dsth, dstw;
	_glXQueryDrawable(glXGetCurrentDisplay(), glXGetCurrentDrawable(), GLX_WIDTH, &dstw);
	_glXQueryDrawable(glXGetCurrentDisplay(), glXGetCurrentDrawable(), GLX_HEIGHT, &dsth);
	_glXQueryDrawable(glXGetCurrentDisplay(), glXGetCurrentReadDrawable(), GLX_HEIGHT, &srch);

	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPushAttrib(GL_VIEWPORT_BIT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	_glViewport(0, 0, dstw, dsth);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, dstw, 0, dsth, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	for(unsigned int i=0; i<h; i++)
	{
		glRasterPos2i(dest_x, dsth-dest_y-i-1);
		glCopyPixels(src_x, srch-src_y-i-1, w, 1, GL_COLOR);
	}
	glFlush();  // call faked function here, so it will perform a readback

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
	glPopClientAttrib();

	_glXMakeContextCurrent(olddpy, olddraw, oldread, ctx);

	CATCH();
	return 0;
}

/////////////////////////////
// GLX 1.0 Context management
/////////////////////////////

// This calls glXChooseVisual() on the rendering server to obtain an appropriate
// visual for rendering, then it matches the visual across to the 2D client
// using X11 functions and hashes the two together for future reference.
// It returns the 2D visual so that it can be used in subsequent X11 calls.
XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
	XVisualInfo *v=NULL, *_localv=NULL, vtemp;  int n=0;
	TRY();

	// Prevent recursion
	if(dpy==_localdpy) return _glXChooseVisual(dpy, screen, attrib_list);
	////////////////////

	if((_localv=_glXChooseVisual(_localdpy, DefaultScreen(_localdpy), attrib_list))==NULL)
		return NULL;
	if(!XMatchVisualInfo(dpy, screen, _localv->depth, _localv->c_class, &vtemp)) {XFree(_localv);  return NULL;}
	if(!(v=XGetVisualInfo(dpy, VisualIDMask, &vtemp, &n))) {XFree(_localv);  return NULL;}
	vish.add(dpy, v, _localdpy, _localv);
	XFree(_localv);

	CATCH();
	return v;
}

XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
	XVisualInfo *v=NULL, *_localv=NULL, vtemp;  int n=0;
	TRY();

	// Prevent recursion
	if(dpy==_localdpy) return _glXGetVisualFromFBConfig(dpy, config);
	////////////////////

	if((_localv=_glXGetVisualFromFBConfig(_localdpy, config))==NULL)
		return NULL;
	if(!XMatchVisualInfo(dpy, DefaultScreen(dpy), _localv->depth, _localv->c_class, &vtemp)) {XFree(_localv);  return NULL;}
	if(!(v=XGetVisualInfo(dpy, VisualIDMask, &vtemp, &n))) {XFree(_localv);  return NULL;}
	vish.add(dpy, v, _localdpy, _localv);
	XFree(_localv);
	CATCH();
	return v;
}

XVisualInfo *glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config)
{
	return glXGetVisualFromFBConfig(dpy, config);
}

GLXFBConfig remotevis2localpbconfig(Display *dpy, XVisualInfo *vis)
{
	GLXFBConfig c=0;
	if(!(c=vish.getpbconfig(dpy, vis)))
	{
		// Visual was not obtained from glXChooseVisual(), so all we can do is
		// match it to a visual on the rendering server with like depth and class
		// and hope for the best
		XVisualInfo vtemp, *v;  int n;
		if(!XMatchVisualInfo(_localdpy, DefaultScreen(_localdpy), vis->depth, vis->c_class, &vtemp)) return NULL;
		if(!(v=XGetVisualInfo(_localdpy, VisualIDMask, &vtemp, &n)) || !n) return NULL;
		glxvisual *glxv=new glxvisual(_localdpy, v);
		if(!glxv) {XFree(v);  return NULL;}
		c=glxv->getpbconfig();
		delete glxv;
		XFree(v);
	}
	return c;
}

GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis, GLXContext share_list, Bool direct)
{
	GLXContext ctx=0;  GLXFBConfig c;
	TRY();

	// Prevent recursion
	if(dpy==_localdpy) return _glXCreateContext(dpy, vis, share_list, direct);
	////////////////////

	if(!(c=remotevis2localpbconfig(dpy, vis)))
		_throw("Could not obtain Pbuffer visual");
	int render_type=GLX_RGBA_BIT;
	_glXGetFBConfigAttrib(_localdpy, c, GLX_RENDER_TYPE, &render_type);
	if(!(ctx=_glXCreateNewContext(_localdpy, c,
		render_type==GLX_COLOR_INDEX_BIT?GLX_COLOR_INDEX_TYPE:GLX_RGBA_TYPE, share_list, True)))
		return NULL;
	ctxh.add(ctx, c);
	CATCH();
	return ctx;
}

Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
	Bool retval=0;
	TRY();
	pbwin *pbw;  GLXFBConfig config=0;

	// Prevent recursion
	if(dpy==_localdpy) return _glXMakeCurrent(dpy, drawable, ctx);
	////////////////////

	GLXDrawable curdraw=glXGetCurrentDrawable();
	if(glXGetCurrentContext() && glXGetCurrentDisplay()==_localdpy
	&& curdraw && (pbw=winh.findpb(_localdpy, curdraw))!=NULL)
	{
		pbwin *newpbw;
		if(drawable==0 || (newpbw=winh.findpb(dpy, drawable))==NULL
		|| newpbw->getdrawable()!=curdraw)
		{
			GLint drawbuf=GL_BACK;
			glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
			if(drawbuf==GL_FRONT || drawbuf==GL_FRONT_AND_BACK) pbw->readback(true);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to a Pbuffer
	if(dpy && drawable && ctx)
	{
		errifnot(config=ctxh.findconfig(ctx));
		pbw=winh.setpb(dpy, drawable, config);
		if(pbw) drawable=pbw->updatedrawable();
	}
	retval=_glXMakeContextCurrent(_localdpy, drawable, drawable, ctx);
	if((pbw=winh.findpb(_localdpy, drawable))!=NULL) {pbw->clear();  pbw->cleanup();}
	pbuffer *pb;
	if((pb=pmh.find(dpy, drawable))!=NULL) pb->clear();
	CATCH();
	return retval;
}

void glXDestroyContext(Display* dpy, GLXContext ctx)
{
	TRY();
	ctxh.remove(ctx);
	_glXDestroyContext(_localdpy, ctx);
	CATCH();
}

/////////////////////////////
// GLX 1.3 Context management
/////////////////////////////

GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct)
{
	GLXContext ctx=0;  GLXFBConfig c;
	TRY();

	// Prevent recursion
	if(dpy==_localdpy) return _glXCreateNewContext(dpy, config, render_type, share_list, direct);
	////////////////////

	glxvisual *glxv;
	if(!(glxv=new glxvisual(_localdpy, config))) return NULL;
	if(!(c=glxv->getpbconfig())) {delete glxv;  return NULL;}
	delete glxv;
	if(!(ctx=_glXCreateNewContext(_localdpy, c, render_type, share_list, True)))
		return NULL;
	ctxh.add(ctx, c);
	CATCH();
	return ctx;
}

Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
	Bool retval=0;
	pbwin *pbw;  GLXFBConfig config=0;
	TRY();

	// Prevent recursion
	if(dpy==_localdpy) return _glXMakeContextCurrent(dpy, draw, read, ctx);
	////////////////////

	GLXDrawable curdraw=glXGetCurrentDrawable();
	if(glXGetCurrentContext() && glXGetCurrentDisplay()==_localdpy
	&& curdraw && (pbw=winh.findpb(_localdpy, curdraw))!=NULL)
	{
		pbwin *newpbw;
		if(draw==0 || (newpbw=winh.findpb(dpy, draw))==NULL
		|| newpbw->getdrawable()!=curdraw)
		{
			GLint drawbuf=GL_BACK;
			glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
			if(drawbuf==GL_FRONT || drawbuf==GL_FRONT_AND_BACK) pbw->readback(true);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to a Pbuffer
	pbwin *drawpbw, *readpbw;
	if(dpy && (draw || read) && ctx)
	{
		errifnot(config=ctxh.findconfig(ctx));
		drawpbw=winh.setpb(dpy, draw, config);
		readpbw=winh.setpb(dpy, read, config);
		if(drawpbw) draw=drawpbw->updatedrawable();
		if(readpbw) read=readpbw->updatedrawable();
	}
	retval=_glXMakeContextCurrent(_localdpy, draw, read, ctx);
	if((drawpbw=winh.findpb(_localdpy, draw))!=NULL) {drawpbw->clear();  drawpbw->cleanup();}
	if((readpbw=winh.findpb(_localdpy, read))!=NULL) readpbw->cleanup();
	pbuffer *pb;
	if((pb=pmh.find(dpy, draw))!=NULL) pb->clear();
	CATCH();
	return retval;
}

///////////////////////////////////
// SGIX_fbconfig Context management
///////////////////////////////////

// On Linux, GLXFBConfigSGIX is typedef'd to GLXFBConfig
GLXContext glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct)
{
	return glXCreateNewContext(dpy, config, render_type, share_list, direct);
}

/////////////////////////
// Other GL/GLX functions
/////////////////////////

// Here, we fake out the client into thinking it's getting a window drawable,
// but really it's getting a Pbuffer drawable
GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config, Window win, const int *attrib_list)
{
	TRY();
	pbwin *pbw;  GLXFBConfig c=0;
	XSync(dpy, False);

	// Prevent recursion
	if(dpy==_localdpy) return _glXCreateWindow(dpy, config, win, attrib_list);
	////////////////////

	glxvisual *glxv;
	errifnot(glxv=new glxvisual(_localdpy, config));
	errifnot(c=glxv->getpbconfig());
	errifnot(pbw=winh.setpb(dpy, win, c));
	delete glxv;
	CATCH();
	return win;  // Make the client store the original window handle, which we use
               // to find the Pbuffer in the hash
}

void glXDestroyWindow(Display *dpy, GLXWindow win)
{
	TRY();
	// Prevent recursion
	if(dpy==_localdpy) {_glXDestroyWindow(dpy, win);  return;}
	////////////////////

	winh.remove(dpy, win);
	CATCH();
}

// Pixmap rendering, another shameless hack.  What we're really returning is a
// Pbuffer handle

GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vi, Pixmap pm)
{
	TRY();
	GLXFBConfig c=remotevis2localpbconfig(dpy, vi);

	// Prevent recursion
	if(dpy==_localdpy) return _glXCreateGLXPixmap(dpy, vi, pm);
	////////////////////

	Window root;  int x, y;  unsigned int w, h, bw, d;
	XGetGeometry(dpy, pm, &root, &x, &y, &w, &h, &bw, &d);
	pbuffer *pb=new pbuffer(_localdpy, w, h, c);
	if(pb)
	{
		pmh.add(dpy, pm, pb);
		return pb->drawable();
	}
	CATCH();
	return 0;
}

void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
	TRY();
	// Prevent recursion
	if(dpy==_localdpy) {_glXDestroyGLXPixmap(dpy, pix);  return;}
	////////////////////

	pmh.remove(dpy, pix);
	CATCH();
}

GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pm, const int *attribs)
{
	TRY();
	GLXFBConfig c=0;

	// Prevent recursion
	if(dpy==_localdpy) return _glXCreatePixmap(dpy, config, pm, attribs);
	////////////////////

	glxvisual *glxv;
	errifnot(glxv=new glxvisual(_localdpy, config));
	errifnot(c=glxv->getpbconfig());
	delete glxv;
	Window root;  int x, y;  unsigned int w, h, bw, d;
	XGetGeometry(dpy, pm, &root, &x, &y, &w, &h, &bw, &d);
	pbuffer *pb=new pbuffer(_localdpy, w, h, c);
	if(pb)
	{
		pmh.add(dpy, pm, pb);
		return pb->drawable();
	}
	CATCH();
	return 0;
}

GLXPixmap glXCreateGLXPixmapWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap)
{
	return glXCreatePixmap(dpy, config, pixmap, NULL);
}

void glXDestroyPixmap(Display *dpy, GLXPixmap pix)
{
	TRY();
	// Prevent recursion
	if(dpy==_localdpy) {_glXDestroyPixmap(dpy, pix);  return;}
	////////////////////

	pmh.remove(dpy, pix);
	CATCH();
}

// This will fail if the server and client don't have a reasonably
// similar set of fonts
void glXUseXFont(Font font, int first, int count, int list_base)
{
	TRY();
	XFontStruct *fs=NULL;
	Display *dpy;
	errifnot(dpy=_XOpenDisplay(0));
	errifnot(fs=XQueryFont(dpy, font));
	char *fontname=NULL;
	for(int i=0; i<fs->n_properties; i++)
	{
		char *atom;
		errifnot(atom=XGetAtomName(dpy, fs->properties[i].name));
		if(!strcmp(atom, "FONT"))
			errifnot(fontname=XGetAtomName(dpy, fs->properties[i].card32));
	}
	errifnot(font=XLoadFont(_localdpy, fontname));
	_glXUseXFont(font, first, count, list_base);
	XUnloadFont(_localdpy, font);
	XFreeFontInfo(NULL, fs, 1);
	_XCloseDisplay(dpy);
	CATCH();
}

void glXSwapBuffers(Display* dpy, GLXDrawable drawable)
{
	TRY();
	pbwin *pbw=NULL;
	if(dpy!=_localdpy && (pbw=winh.findpb(dpy, drawable))!=NULL)
	{
		pbw->readback(false);
		pbw->swapbuffers();
	}
	else _glXSwapBuffers(_localdpy, drawable);
	CATCH();
}

void _doGLreadback(bool force)
{
	pbwin *pbw;
	Display *dpy;  GLXDrawable drawable;
	dpy=glXGetCurrentDisplay();
	drawable=glXGetCurrentDrawable();
	if(!dpy || !drawable) return;
	if((pbw=winh.findpb(dpy, drawable))!=NULL)
		pbw->readback(GL_FRONT, force);
}

void glFlush(void)
{
	TRY();
	_glFlush();
	_doGLreadback(false);
	CATCH();
}

void glFinish(void)
{
	TRY();
	_glFinish();
	_doGLreadback(false);
	CATCH();
}

// Sometimes XNextEvent() is called from a thread other than the
// rendering thread, so we wait until glViewport() is called and
// take that opportunity to resize the Pbuffer
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRY();
	Display *dpy=glXGetCurrentDisplay();
	GLXContext ctx=glXGetCurrentContext();
	GLXDrawable draw=glXGetCurrentDrawable();
	GLXDrawable read=glXGetCurrentReadDrawable();
	if(dpy && (draw || read) && ctx)
	{
		GLXDrawable newread=read, newdraw=draw;
		pbwin *drawpbw=winh.findpb(dpy, draw);
		pbwin *readpbw=winh.findpb(dpy, read);
		if(drawpbw) newdraw=drawpbw->updatedrawable();
		if(readpbw) newread=readpbw->updatedrawable();
		if(newread!=read || newdraw!=draw)
		{
			_glXMakeContextCurrent(dpy, newdraw, newread, ctx);
			if(drawpbw) {drawpbw->clear();  drawpbw->cleanup();}
			if(readpbw) readpbw->cleanup();
		}
	}
	_glViewport(x, y, width, height);
	CATCH();
}

} // extern "C"
