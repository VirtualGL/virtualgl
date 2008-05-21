/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <string.h>
#include <math.h>
#include "rrutil.h"
#include "rrtimer.h"
#include "rrthread.h"
#include "rrmutex.h"
#define __FAKERCONFIG_STATICDEF__
#include "fakerconfig.h"
#define __FAKERHASH_STATICDEF__
#include "faker-winhash.h"
#include "faker-ctxhash.h"
#include "faker-vishash.h"
#include "faker-cfghash.h"
#include "faker-rcfghash.h"
#include "faker-pmhash.h"
#include "faker-glxdhash.h"
#include "faker-sym.h"
#include "glxvisual.h"
#define __VGLCONFIGSTART_STATICDEF__
#include "vglconfigstart.h"
#include <sys/types.h>
#include <unistd.h>
#ifdef __DEBUG__
#include "x11err.h"
#endif

#ifdef SUNOGL
extern "C" {
static GLvoid r_glIndexd(OglContextPtr, GLdouble);
static GLvoid r_glIndexf(OglContextPtr, GLfloat);
static GLvoid r_glIndexi(OglContextPtr, GLint);
static GLvoid r_glIndexs(OglContextPtr, GLshort);
static GLvoid r_glIndexub(OglContextPtr, GLubyte);
static GLvoid r_glIndexdv(OglContextPtr, const GLdouble *);
static GLvoid r_glIndexfv(OglContextPtr, const GLfloat *);
static GLvoid r_glIndexiv(OglContextPtr, const GLint *);
static GLvoid r_glIndexsv(OglContextPtr, const GLshort *);
static GLvoid r_glIndexubv(OglContextPtr, const GLubyte *);
}
#endif

// Globals
Display *_localdpy=NULL;
#ifdef USEGLP
GLPDevice _localdev=-1;
#define _localdisplayiscurrent() ((fconfig.glp && glPGetCurrentDevice()==_localdev) || (!fconfig.glp && GetCurrentDisplay()==_localdpy))
#else
#define _localdisplayiscurrent() (GetCurrentDisplay()==_localdpy)
#endif
#define _isremote(dpy) (fconfig.glp || (_localdpy && dpy!=_localdpy))
#define _isfront(drawbuf) (drawbuf==GL_FRONT || drawbuf==GL_FRONT_AND_BACK \
	|| drawbuf==GL_FRONT_LEFT || drawbuf==GL_FRONT_RIGHT || drawbuf==GL_LEFT \
	|| drawbuf==GL_RIGHT)
#define _isright(drawbuf) (drawbuf==GL_RIGHT || drawbuf==GL_FRONT_RIGHT \
	|| drawbuf==GL_BACK_RIGHT)

static inline int _drawingtofront(void)
{
	GLint drawbuf=GL_BACK;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return _isfront(drawbuf);
}

static inline int _drawingtoright(void)
{
	GLint drawbuf=GL_LEFT;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return _isright(drawbuf);
}

static rrcs globalmutex;
static int __shutdown=0;

static inline int isdead(void)
{
	int retval=0;
	globalmutex.lock(false);
	retval=__shutdown;
	globalmutex.unlock(false);
	return retval;
}

static void __vgl_cleanup(void)
{
	if(pmhash::isalloc()) pmh.killhash();
	if(vishash::isalloc()) vish.killhash();
	if(cfghash::isalloc()) cfgh.killhash();
	if(rcfghash::isalloc()) rcfgh.killhash();
	if(ctxhash::isalloc()) ctxh.killhash();
	if(glxdhash::isalloc()) glxdh.killhash();
	if(winhash::isalloc()) winh.killhash();
	__vgl_unloadsymbols();
}

void __vgl_safeexit(int retcode)
{
	int shutdown;
	globalmutex.lock(false);
	shutdown=__shutdown;
	if(!__shutdown)
	{
		__shutdown=1;
		__vgl_cleanup();
		FakerConfig::deleteinstance();
	}
	globalmutex.unlock(false);
	if(!shutdown) exit(retcode);
	else pthread_exit(0);
}

class _globalcleanup
{
	public:
		~_globalcleanup()
		{
			globalmutex.lock(false);
			FakerConfig::deleteinstance();
			__shutdown=1;
			globalmutex.unlock(false);
		}
};
_globalcleanup gdt;

#define _die(f,m) {if(!isdead()) rrout.print("[VGL] ERROR: in %s--\n[VGL]    %s\n", f, m);  __vgl_safeexit(1);}

#define TRY() try {
#define CATCH() } catch(rrerror &e) {_die(e.getMethod(), e.getMessage());}

#define prargd(a) rrout.print("%s=0x%.8lx(%s) ", #a, (unsigned long)a, a?DisplayString(a):"NULL")
#define prargs(a) rrout.print("%s=%s ", #a, a?a:"NULL")
#define prargx(a) rrout.print("%s=0x%.8lx ", #a, (unsigned long)a)
#define prargi(a) rrout.print("%s=%d ", #a, a)
#define prargv(a) rrout.print("%s=0x%.8lx(0x%.2lx) ", #a, (unsigned long)a, a?a->visualid:0)
#define prargc(a) rrout.print("%s=0x%.8lx(0x%.2x) ", #a, (unsigned long)a, a?_FBCID(a):0)
#define prargal11(a) if(a) {  \
	rrout.print("attrib_list=[");  \
	for(int __an=0; attrib_list[__an]!=None; __an++) {  \
		rrout.print("0x%.4x", attrib_list[__an]);  \
		if(attrib_list[__an]!=GLX_USE_GL && attrib_list[__an]!=GLX_DOUBLEBUFFER  \
			&& attrib_list[__an]!=GLX_STEREO && attrib_list[__an]!=GLX_RGBA)  \
			rrout.print("=0x%.4x", attrib_list[++__an]);  \
		rrout.print(" ");  \
	}  rrout.print("] ");}
#define prargal13(a) if(a) {  \
	rrout.print("attrib_list=[");  \
	for(int __an=0; attrib_list[__an]!=None; __an+=2) {  \
		rrout.print("0x%.4x=0x%.4x ", attrib_list[__an], attrib_list[__an+1]);  \
	}  rrout.print("] ");}

#define opentrace(f)  \
	double __vgltracetime=0.;  \
	if(fconfig.trace) {  \
		rrout.print("[VGL] %s (", #f);  \

#define starttrace()  \
		__vgltracetime=rrtime();  \
	}

#define stoptrace()  \
	if(fconfig.trace) {  \
		__vgltracetime=rrtime()-__vgltracetime;

#define closetrace()  \
		rrout.PRINT(") %f ms\n", __vgltracetime*1000.);  \
	}

#include "faker-glx.cpp"

#if 0
// Used during debug so we can get a stack trace from an X11 protocol error
#ifdef __DEBUG__
int xhandler(Display *dpy, XErrorEvent *xe)
{
	rrout.PRINT("[VGL] ERROR: X11 error--\n[VGL]    %s\n", x11error(xe->error_code));
	return 0;
}
#endif
#endif

void __vgl_fakerinit(void)
{
	static int init=0;

	rrcs::safelock l(globalmutex);
	if(init) return;
	init=1;

	fconfig.reloadenv();
	if(fconfig.log) rrout.logto(fconfig.log);

	#ifdef __DEBUG__
	if(getenv("VGL_DEBUG"))
	{
		rrout.print("[VGL] Attach debugger to process %d ...\n", getpid());
		fgetc(stdin);
	}
	#if 0
	XSetErrorHandler(xhandler);
	#endif
	#endif

	__vgl_loadsymbols();
	#ifdef USEGLP
	if(fconfig.glp)
	{
		if(_localdev<0)
		{
			char **devices=NULL;  int ndevices=0;  char *device=NULL;
			if((devices=glPGetDeviceNames(&ndevices))==NULL || ndevices<1)
				_throw("No GLP devices are registered");
			device=fconfig.localdpystring;
			if(!strnicmp(device, "GLP", 3)) device=NULL;
			if(fconfig.verbose) rrout.println("[VGL] Opening GLP device %s",
				device?device:"(default)");
			if((_localdev=glPOpenDevice(device))<0)
			{
				rrout.print("[VGL] ERROR: Could not open GLP device %s.\n", (char *)fconfig.localdpystring);
				__vgl_safeexit(1);
			}
		}
	}
	else
	#endif
	if(!_localdpy)
	{
		if(fconfig.verbose) rrout.println("[VGL] Opening local display %s",
			fconfig.localdpystring?(char *)fconfig.localdpystring:"(default)");
		if((_localdpy=_XOpenDisplay(fconfig.localdpystring))==NULL)
		{
			rrout.print("[VGL] ERROR: Could not open display %s.\n", (char *)fconfig.localdpystring);
			__vgl_safeexit(1);
		}
	}
}

////////////////
// X11 functions
////////////////

extern "C" {

Bool XQueryExtension(Display *dpy, _Xconst char *name, int *major_opcode,
	int *first_event, int *first_error)
{
	Bool retval=True;

	// Prevent recursion
	if(!_isremote(dpy))
		return _XQueryExtension(dpy, name, major_opcode, first_event, first_error);
	////////////////////

		opentrace(XQueryExtension);  prargd(dpy);  prargs(name);  starttrace();

	retval=_XQueryExtension(dpy, name, major_opcode, first_event, first_error);
	if(!strcmp(name, "GLX")) retval=True;

		stoptrace();  if(major_opcode) prargi(*major_opcode);
		if(first_event) prargi(*first_event);
		if(first_error) prargi(*first_error);  closetrace();

 	return retval;
}

char **XListExtensions(Display *dpy, int *next)
{
	char **list=NULL, *liststr=NULL;  int n, i;
	int hasglx=0, listlen=0;

	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _XListExtensions(dpy, next);
	////////////////////

		opentrace(XListExtensions);  prargd(dpy);  starttrace();

	list=_XListExtensions(dpy, &n);
	if(list && n>0)
	{
		for(i=0; i<n; i++)
		{
			if(list[i])
			{
				listlen+=strlen(list[i])+1;
				if(!strcmp(list[i], "GLX")) hasglx=1;
			}
		}
	}
	if(!hasglx)
	{
		char **newlist=NULL;  int listndx=0;
		listlen+=4;  // "GLX" + terminating NULL
		errifnot(newlist=(char **)malloc(sizeof(char *)*(n+1)))
		errifnot(liststr=(char *)malloc(listlen+1))
		memset(liststr, 0, listlen+1);
		liststr=&liststr[1];  // For compatibility with X.org implementation
		if(list && n>0)
		{
			for(i=0; i<n; i++)
			{
				newlist[i]=&liststr[listndx];
				if(list[i])
				{
					strncpy(newlist[i], list[i], strlen(list[i]));
					listndx+=strlen(list[i]);
					liststr[listndx]='\0';  listndx++;
				}
			}
			XFreeExtensionList(list);
		}
		newlist[n]=&liststr[listndx];
		strncpy(newlist[n], "GLX", 3);  newlist[n][3]='\0';
		list=newlist;  n++;
	}

		stoptrace();  prargi(n);  closetrace();

	CATCH();

	if(next) *next=n;
 	return list;
}

char *XServerVendor(Display *dpy)
{
	if(fconfig.vendor) return fconfig.vendor;
	else return _XServerVendor(dpy);
}

Display *XOpenDisplay(_Xconst char* name)
{
	Display *dpy=NULL;
	TRY();

		opentrace(XOpenDisplay);  prargs(name);  starttrace();

	__vgl_fakerinit();
	if(!(dpy=_XOpenDisplay(name))) return NULL;

		stoptrace();  prargd(dpy);  closetrace();

	CATCH();
	return dpy;
}

Window XCreateWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	int depth, unsigned int c_class, Visual *visual, unsigned long valuemask,
	XSetWindowAttributes *attributes)
{
	Window win=0;
	TRY();

		opentrace(XCreateWindow);  prargd(dpy);  prargx(parent);  prargi(x);
		prargi(y);  prargi(width);  prargi(height);  prargi(depth);
		prargi(c_class);  prargv(visual);  starttrace();

	if(!(win=_XCreateWindow(dpy, parent, x, y, width, height, border_width,
		depth, c_class, visual, valuemask, attributes))) return 0;
	if(_isremote(dpy)) winh.add(dpy, win);

		stoptrace();  prargx(win);  closetrace();

	CATCH();
	return win;
}

Window XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	unsigned long border, unsigned long background)
{
	Window win=0;
	TRY();

		opentrace(XCreateSimpleWindow);  prargd(dpy);  prargx(parent);  prargi(x);
		prargi(y);  prargi(width);  prargi(height);  starttrace();

	if(!(win=_XCreateSimpleWindow(dpy, parent, x, y, width, height, border_width,
		border, background))) return 0;
	if(_isremote(dpy)) winh.add(dpy, win);

		stoptrace();  prargx(win);  closetrace();

	CATCH();
	return win;
}

int XDestroyWindow(Display *dpy, Window win)
{
	int retval=0;
	TRY();

		opentrace(XDestroyWindow);  prargd(dpy);  prargx(win);  starttrace();

	winh.remove(dpy, win);
	retval=_XDestroyWindow(dpy, win);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}

Status XGetGeometry(Display *display, Drawable drawable, Window *root, int *x,
	int *y, unsigned int *width, unsigned int *height,
	unsigned int *border_width, unsigned int *depth)
{
	Status ret=0;

		opentrace(XGetGeometry);  prargx(display);  prargx(drawable);
		starttrace();

	ret=_XGetGeometry(display, drawable, root, x, y, width, height, border_width,
		depth);
	pbwin *pbw=NULL;
	if(winh.findpb(display, drawable, pbw) && width && height
		&& *width>0 && *height>0)
		pbw->resize(*width, *height);

		stoptrace();  if(root) prargx(*root);  if(x) prargi(*x);  if(y) prargi(*y);
		if(width) prargi(*width);  if(height) prargi(*height);
		if(border_width) prargi(*border_width);  if(depth) prargi(*depth);
		closetrace();

	return ret;
}

static void _HandleEvent(Display *dpy, XEvent *xe)
{
	pbwin *pbw=NULL;
	if(xe && xe->type==ConfigureNotify)
	{
		if(winh.findpb(dpy, xe->xconfigure.window, pbw))
		{
				opentrace(_HandleEvent);  prargi(xe->xconfigure.width);
				prargi(xe->xconfigure.height);  starttrace();

			pbw->resize(xe->xconfigure.width, xe->xconfigure.height);

				stoptrace();  closetrace();
		}
	}
	else if(xe && xe->type==KeyPress)
	{
		unsigned int state2, state=(xe->xkey.state)&(~(LockMask));
		state2=fconfig.guimod;  if(state2&Mod1Mask) {state2&=(~(Mod1Mask));  state2|=Mod2Mask;}
		if(fconfig.gui && XKeycodeToKeysym(dpy, xe->xkey.keycode, 0)==fconfig.guikey
			&& (state==fconfig.guimod || state==state2)
			&& FakerConfig::_Shmid!=-1)
			vglpopup(dpy, FakerConfig::_Shmid);
	}
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
	int retval=0;
	TRY();

		opentrace(XConfigureWindow);  prargd(dpy);  prargx(win);
		if(values && (value_mask&CWWidth)) {prargi(values->width);}
		if(values && (value_mask&CWHeight)) {prargi(values->height);}  starttrace();

	pbwin *pbw=NULL;
	if(winh.findpb(dpy, win, pbw) && values)
		pbw->resize(value_mask&CWWidth?values->width:0, value_mask&CWHeight?values->height:0);
	retval=_XConfigureWindow(dpy, win, value_mask, values);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}

int XResizeWindow(Display *dpy, Window win, unsigned int width, unsigned int height)
{
	int retval=0;
	TRY();

		opentrace(XResizeWindow);  prargd(dpy);  prargx(win);  prargi(width);
		prargi(height);  starttrace();

	pbwin *pbw=NULL;
	if(winh.findpb(dpy, win, pbw)) pbw->resize(width, height);
	retval=_XResizeWindow(dpy, win, width, height);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}

int XMoveResizeWindow(Display *dpy, Window win, int x, int y, unsigned int width, unsigned int height)
{
	int retval=0;
	TRY();

		opentrace(XMoveResizeWindow);  prargd(dpy);  prargx(win);  prargi(x);
		prargi(y);  prargi(width);  prargi(height);  starttrace();

	pbwin *pbw=NULL;
	if(winh.findpb(dpy, win, pbw)) pbw->resize(width, height);
	retval=_XMoveResizeWindow(dpy, win, x, y, width, height);

		stoptrace();  closetrace();

	CATCH();
	return retval;
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

	if(!srcpm && !dstpm)
	{
		int retval=_XCopyArea(dpy, src, dst, gc, src_x, src_y, w, h, dest_x, dest_y);
		return retval;
	}

		opentrace(XCopyArea);  prargd(dpy);  prargx(src);  prargx(dst);  prargx(gc);
		prargi(src_x);  prargi(src_y);  prargi(w);  prargi(h);  prargi(dest_x);
		prargi(dest_y);  prargx(read);  prargx(draw);  starttrace();

	GLXDrawable oldread=GetCurrentReadDrawable();
	GLXDrawable olddraw=GetCurrentDrawable();
	GLXContext ctx=GetCurrentContext();
	Display *olddpy=NULL;
	if(!ctx || (!fconfig.glp && !(olddpy=GetCurrentDisplay())))
	{
		stoptrace();  closetrace();
		return 0;  // Does ... not ... compute
	}

	// Intentionally call the faked function so it will map a PB if src or dst is a window
	glXMakeContextCurrent(dpy, draw, read, ctx);

	unsigned int srch, dsth, dstw;
	#ifdef USEGLP
	if(fconfig.glp)
	{
		glPQueryBuffer(glPGetCurrentBuffer(), GLP_WIDTH, &dstw);
		glPQueryBuffer(glPGetCurrentBuffer(), GLP_HEIGHT, &dsth);
		glPQueryBuffer(glPGetCurrentReadBuffer(), GLP_HEIGHT, &srch);
	}
	else
	#endif
	{
		_glXQueryDrawable(_glXGetCurrentDisplay(), _glXGetCurrentDrawable(), GLX_WIDTH, &dstw);
		_glXQueryDrawable(_glXGetCurrentDisplay(), _glXGetCurrentDrawable(), GLX_HEIGHT, &dsth);
		_glXQueryDrawable(_glXGetCurrentDisplay(), _glXGetCurrentReadDrawable(), GLX_HEIGHT, &srch);
	}

	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPushAttrib(GL_VIEWPORT_BIT|GL_COLOR_BUFFER_BIT|GL_PIXEL_MODE_BIT);
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

	_glDrawBuffer(GL_FRONT_AND_BACK);
	glReadBuffer(GL_FRONT);
	for(unsigned int i=0; i<h; i++)
	{
		glRasterPos2i(dest_x, dsth-dest_y-i-1);
		glCopyPixels(src_x, srch-src_y-i-1, w, 1, GL_COLOR);
	}
	glFinish();  // call faked function here, so it will perform a readback

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	_glPopAttrib();
	glPopClientAttrib();

	#ifdef USEGLP
	if(fconfig.glp) glPMakeContextCurrent(olddraw, oldread, ctx);
	else
	#endif
	_glXMakeContextCurrent(olddpy, olddraw, oldread, ctx);

		stoptrace();  closetrace();

	CATCH();
	return 0;
}

int XFree(void *data)
{
	int ret=0;
	TRY();
	ret=_XFree(data);
	if(data && !isdead()) vish.remove(NULL, (XVisualInfo *)data);
	CATCH();
	return ret;
}

/////////////////////////////
// GLX 1.0 Context management
/////////////////////////////

XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
	XVisualInfo *v=NULL;
	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _glXChooseVisual(dpy, screen, attrib_list);
	////////////////////

		opentrace(glXChooseVisual);  prargd(dpy);  prargi(screen);
		prargal11(attrib_list);  starttrace();


	if(attrib_list)
	{
		bool overlayreq=false;
		for(int i=0; attrib_list[i]!=None && i<=254; i++)
		{
			if(attrib_list[i]==GLX_DOUBLEBUFFER || attrib_list[i]==GLX_RGBA
				|| attrib_list[i]==GLX_STEREO || attrib_list[i]==GLX_USE_GL)
				continue;
			else if(attrib_list[i]==GLX_LEVEL && attrib_list[i+1]==1)
			{
				overlayreq=true;  i++;
			}
			else i++;
		}
		if(overlayreq)
		{
			v=_glXChooseVisual(dpy, screen, attrib_list);
			stoptrace();  prargv(v);  closetrace();
			return v;
		}
	}


	GLXFBConfig *configs=NULL, c=0;  int n=0;
	if(!dpy || !attrib_list) return NULL;
	int depth=24, c_class=TrueColor, level=0, stereo=0, trans=0;
	if(!(configs=__vglConfigsFromVisAttribs(attrib_list, depth, c_class,
		level, stereo, trans, n)) || n<1) return NULL;
	c=configs[0];
	XFree(configs);
	VisualID vid=__vglMatchVisual(dpy, screen, depth, c_class, level, stereo, trans);
	if(!vid) return NULL;
	v=__vglVisualFromVisualID(dpy, vid);
	if(!v) return NULL;
	vish.add(dpy, v, c);

		stoptrace();  prargv(v);  prargc(c);  closetrace();

	CATCH();
	return v;
}

XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
	XVisualInfo *v=NULL;
	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _glXGetVisualFromFBConfig(dpy, config);
	////////////////////

		opentrace(glXGetVisualFromFBConfig);  prargd(dpy);  prargc(config);
		starttrace();

	if(rcfgh.isoverlay(dpy, config))
	{
		v=_glXGetVisualFromFBConfig(dpy, config);
		stoptrace();  prargv(v);  closetrace();
		return v;
	}

	VisualID vid=0;
	if(!dpy || !config) return NULL;
	vid=_MatchVisual(dpy, config);
	if(!vid) return NULL;
	v=__vglVisualFromVisualID(dpy, vid);
	if(!v) return NULL;
	vish.add(dpy, v, config);

		stoptrace();  prargv(v);  closetrace();

	CATCH();
	return v;
}

XVisualInfo *glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config)
{
	return glXGetVisualFromFBConfig(dpy, config);
}

GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis, GLXContext share_list, Bool direct)
{
	GLXContext ctx=0;
	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _glXCreateContext(dpy, vis, share_list, direct);
	////////////////////

		opentrace(glXCreateContext);  prargd(dpy);  prargv(vis);
		prargi(direct);  starttrace();

	if(vis)
	{
		int level=__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_LEVEL);
		int trans=(__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_TRANSPARENT_TYPE)==GLX_TRANSPARENT_INDEX);
		if(level && trans)
		{
			ctx=_glXCreateContext(dpy, vis, share_list, direct);
			if(ctx) ctxh.add(ctx, (GLXFBConfig)-1);
			stoptrace();  prargx(ctx);  closetrace();
			return ctx;
		}
	}

	GLXFBConfig c;
	if(!(c=_MatchConfig(dpy, vis))) _throw("Could not obtain Pbuffer-capable RGB visual on the server");
	int render_type=__vglServerVisualAttrib(c, GLX_RENDER_TYPE);
	#ifdef USEGLP
	if(fconfig.glp)
	{
		if(!(ctx=glPCreateNewContext(c,
			render_type==GLP_COLOR_INDEX_BIT?GLP_COLOR_INDEX_TYPE:GLP_RGBA_TYPE, share_list)))
		return NULL;
	}
	else
	#endif
	{
		if(!(ctx=_glXCreateNewContext(_localdpy, c,
			render_type==GLX_COLOR_INDEX_BIT?GLX_COLOR_INDEX_TYPE:GLX_RGBA_TYPE, share_list, True)))
			return NULL;
		if(!glXIsDirect(_localdpy, ctx))
		{
			rrout.println("[VGL] WARNING: The OpenGL rendering context obtained on X display");
			rrout.println("[VGL]    %s is indirect, which may cause performance to suffer.", DisplayString(_localdpy));
			rrout.println("[VGL]    If %s is a local X display, then the framebuffer device", DisplayString(_localdpy));
			rrout.println("[VGL]    permissions may be set incorrectly.");
		}
	}
	ctxh.add(ctx, c);

		stoptrace();  prargc(c);  prargx(ctx);  closetrace();

	CATCH();
	return ctx;
}

Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
	Bool retval=0;
	TRY();
	pbwin *pbw;  GLXFBConfig config=0;

	// Prevent recursion
	if(!_isremote(dpy)) return _glXMakeCurrent(dpy, drawable, ctx);
	////////////////////

		opentrace(glXMakeCurrent);  prargd(dpy);  prargx(drawable);  prargx(ctx);
		starttrace();

	if(ctx) config=ctxh.findconfig(ctx);
	if(config==(GLXFBConfig)-1)
	{
		retval=_glXMakeCurrent(dpy, drawable, ctx);
		winh.setoverlay(dpy, drawable);
		stoptrace();  closetrace();
		return retval;
	}

	// Equivalent of a glFlush()
	GLXDrawable curdraw=GetCurrentDrawable();
	if(GetCurrentContext() && _localdisplayiscurrent()
	&& curdraw && winh.findpb(curdraw, pbw))
	{
		pbwin *newpbw;
		if(drawable==0 || !winh.findpb(dpy, drawable, newpbw)
		|| newpbw->getdrawable()!=curdraw)
		{
			if(_drawingtofront() || pbw->_dirty) pbw->readback(GL_FRONT, false, false);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to a Pbuffer
	if(dpy && drawable && ctx)
	{
		if(!config)
		{
			rrout.PRINTLN("[VGL] WARNING: glXMakeCurrent() called with a previously-destroyed context.");
			return False;
		}
		pbw=winh.setpb(dpy, drawable, config);
		if(pbw)
			drawable=pbw->updatedrawable();
		else if(!glxdh.getcurrentdpy(drawable))
		{
			// Apparently it isn't a Pbuffer or a Pixmap, so it must be a window
			// that was created in another application.  This code is necessary
			// to make CRUT (Chromium Utility Toolkit) applications work.
			if(_isremote(dpy))
			{
				winh.add(dpy, drawable);
				pbw=winh.setpb(dpy, drawable, config);
				if(pbw)
					drawable=pbw->updatedrawable();
			}
		}
	}

	#ifdef USEGLP
	if(fconfig.glp)
		retval=glPMakeContextCurrent(drawable, drawable, ctx);
	else
	#endif
		retval=_glXMakeContextCurrent(_localdpy, drawable, drawable, ctx);
	if(winh.findpb(drawable, pbw)) {pbw->clear();  pbw->cleanup();}
	pbuffer *pb;
	if((pb=pmh.find(dpy, drawable))!=NULL) pb->clear();
	#ifdef SUNOGL
	sunOglCurPrimTablePtr->oglIndexd=r_glIndexd;
	sunOglCurPrimTablePtr->oglIndexf=r_glIndexf;
	sunOglCurPrimTablePtr->oglIndexi=r_glIndexi;
	sunOglCurPrimTablePtr->oglIndexs=r_glIndexs;
	sunOglCurPrimTablePtr->oglIndexub=r_glIndexub;
	sunOglCurPrimTablePtr->oglIndexdv=r_glIndexdv;
	sunOglCurPrimTablePtr->oglIndexfv=r_glIndexfv;
	sunOglCurPrimTablePtr->oglIndexiv=r_glIndexiv;
	sunOglCurPrimTablePtr->oglIndexsv=r_glIndexsv;
	sunOglCurPrimTablePtr->oglIndexubv=r_glIndexubv;
	#endif

		stoptrace();  prargc(config);  prargx(drawable);  closetrace();

	CATCH();
	return retval;
}

void glXDestroyContext(Display* dpy, GLXContext ctx)
{
	TRY();

		opentrace(glXDestroyContext);  prargd(dpy);  prargx(ctx);  starttrace();

	if(ctxh.isoverlay(ctx))
	{
		_glXDestroyContext(dpy, ctx);
		stoptrace();  closetrace();
		return;
	}

	ctxh.remove(ctx);
	#ifdef USEGLP
	if(fconfig.glp)	glPDestroyContext(ctx);
	else
	#endif
	_glXDestroyContext(_localdpy, ctx);

		stoptrace();  closetrace();

	CATCH();
}

/////////////////////////////
// GLX 1.3 Context management
/////////////////////////////

GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct)
{
	GLXContext ctx=0;
	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _glXCreateNewContext(dpy, config, render_type, share_list, direct);
	////////////////////

		opentrace(glXCreateNewContext);  prargd(dpy);  prargc(config);
		prargi(render_type);  prargi(direct);  starttrace();

	if(rcfgh.isoverlay(dpy, config)) // Overlay config
	{
		ctx=_glXCreateNewContext(dpy, config, render_type, share_list, direct);
		if(ctx) ctxh.add(ctx, (GLXFBConfig)-1);
		stoptrace();  prargx(ctx);  closetrace();
		return ctx;
	}

	if(__vglServerVisualAttrib(config, GLX_RENDER_TYPE)==GLX_COLOR_INDEX_BIT)
		render_type=GLX_COLOR_INDEX_TYPE;
	else
		render_type=GLX_RGBA_TYPE;

	#ifdef USEGLP
	if(fconfig.glp)
	{
		if(!(ctx=glPCreateNewContext(config, render_type, share_list)))
			return NULL;
	}
	else
	#endif
	{
		if(!(ctx=_glXCreateNewContext(_localdpy, config, render_type, share_list, True)))
			return NULL;
		if(!glXIsDirect(_localdpy, ctx))
		{
			rrout.println("[VGL] WARNING: The OpenGL rendering context obtained on X display");
			rrout.println("[VGL]    %s is indirect, which may cause performance to suffer.", DisplayString(_localdpy));
			rrout.println("[VGL]    If %s is a local X display, then the framebuffer device", DisplayString(_localdpy));
			rrout.println("[VGL]    permissions may be set incorrectly.");
		}
	}
	ctxh.add(ctx, config);

		stoptrace();  prargx(ctx);  closetrace();

	CATCH();
	return ctx;
}

Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
	Bool retval=0;
	pbwin *pbw;  GLXFBConfig config=0;
	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _glXMakeContextCurrent(dpy, draw, read, ctx);
	////////////////////

		opentrace(glXMakeContextCurrent);  prargd(dpy);  prargx(draw);
		prargx(read);  prargx(ctx);  starttrace();

	if(ctx) config=ctxh.findconfig(ctx);
	if(config==(GLXFBConfig)-1)
	{
		retval=_glXMakeContextCurrent(dpy, draw, read, ctx);
		winh.setoverlay(dpy, draw);
		winh.setoverlay(dpy, read);
		stoptrace();  closetrace();
		return retval;
	}

	// Equivalent of a glFlush()
	GLXDrawable curdraw=GetCurrentDrawable();
	if(GetCurrentContext() && _localdisplayiscurrent()
	&& curdraw && winh.findpb(curdraw, pbw))
	{
		pbwin *newpbw;
		if(draw==0 || !winh.findpb(dpy, draw, newpbw)
			|| newpbw->getdrawable()!=curdraw)
		{
			if(_drawingtofront() || pbw->_dirty) pbw->readback(GL_FRONT, false, false);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to a Pbuffer
	pbwin *drawpbw, *readpbw;
	if(dpy && (draw || read) && ctx)
	{
		if(!config)
		{
			rrout.PRINTLN("[VGL] WARNING: glXMakeContextCurrent() called with a previously-destroyed context");
			return False;
		}
		drawpbw=winh.setpb(dpy, draw, config);
		readpbw=winh.setpb(dpy, read, config);
		if(drawpbw)
			draw=drawpbw->updatedrawable();
		if(readpbw) read=readpbw->updatedrawable();
	}
	#ifdef USEGLP
	if(fconfig.glp)
		retval=glPMakeContextCurrent(draw, read, ctx);
	else
	#endif
		retval=_glXMakeContextCurrent(_localdpy, draw, read, ctx);
	if(winh.findpb(draw, drawpbw)) {drawpbw->clear();  drawpbw->cleanup();}
	if(winh.findpb(read, readpbw)) readpbw->cleanup();
	pbuffer *pb;
	if((pb=pmh.find(dpy, draw))!=NULL) pb->clear();
	#ifdef SUNOGL
	sunOglCurPrimTablePtr->oglIndexd=r_glIndexd;
	sunOglCurPrimTablePtr->oglIndexf=r_glIndexf;
	sunOglCurPrimTablePtr->oglIndexi=r_glIndexi;
	sunOglCurPrimTablePtr->oglIndexs=r_glIndexs;
	sunOglCurPrimTablePtr->oglIndexub=r_glIndexub;
	sunOglCurPrimTablePtr->oglIndexdv=r_glIndexdv;
	sunOglCurPrimTablePtr->oglIndexfv=r_glIndexfv;
	sunOglCurPrimTablePtr->oglIndexiv=r_glIndexiv;
	sunOglCurPrimTablePtr->oglIndexsv=r_glIndexsv;
	sunOglCurPrimTablePtr->oglIndexubv=r_glIndexubv;
	#endif

		stoptrace();  prargc(config);  prargx(draw);  prargx(read);  closetrace();

	CATCH();
	return retval;
}

Bool glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
	return glXMakeContextCurrent(dpy, draw, read, ctx);
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
	// Prevent recursion
	if(!_isremote(dpy)) return _glXCreateWindow(dpy, config, win, attrib_list);
	////////////////////

	TRY();

		opentrace(glXCreateWindow);  prargd(dpy);  prargc(config);  prargx(win);
		starttrace();

	pbwin *pbw=NULL;
	if(rcfgh.isoverlay(dpy, config))
	{
		GLXWindow glxw=_glXCreateWindow(dpy, config, win, attrib_list);
		winh.setoverlay(dpy, glxw);
	}
	else
	{
		XSync(dpy, False);
		errifnot(pbw=winh.setpb(dpy, win, config));
	}

		stoptrace();  if(pbw) {prargx(pbw->getdrawable());}  closetrace();

	CATCH();
	return win;  // Make the client store the original window handle, which we use
               // to find the Pbuffer in the hash
}

void glXDestroyWindow(Display *dpy, GLXWindow win)
{
	TRY();
	// Prevent recursion
	if(!_isremote(dpy)) {_glXDestroyWindow(dpy, win);  return;}
	////////////////////

		opentrace(glXDestroyWindow);  prargd(dpy);  prargx(win);  starttrace();

	if(winh.isoverlay(dpy, win)) _glXDestroyWindow(dpy, win);  
	winh.remove(dpy, win);

		stoptrace();  closetrace();

	CATCH();
}

// Pixmap rendering, another shameless hack.  What we're really returning is a
// Pbuffer handle

GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vi, Pixmap pm)
{
	GLXPixmap drawable=0;
	TRY();
	GLXFBConfig c;

	// Prevent recursion
	if(!_isremote(dpy)) return _glXCreateGLXPixmap(dpy, vi, pm);
	////////////////////

		opentrace(glXCreateGLXPixmap);  prargd(dpy);  prargv(vi);  prargx(pm);
		starttrace();

	if(vi)
	{
		int level=__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vi->visualid,
			GLX_LEVEL);
		int trans=(__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vi->visualid,
			GLX_TRANSPARENT_TYPE)==GLX_TRANSPARENT_INDEX);
		if(level && trans)
		{
			drawable=_glXCreateGLXPixmap(dpy, vi, pm);
			stoptrace();  prargx(drawable);  closetrace();
			return drawable;
		}
	}

	Window root;  int x, y;  unsigned int w, h, bw, d;
	XGetGeometry(dpy, pm, &root, &x, &y, &w, &h, &bw, &d);
	if(!(c=_MatchConfig(dpy, vi))) _throw("Could not obtain Pbuffer-capable RGB visual on the server");
	pbuffer *pb=new pbuffer(w, h, c);
	if(pb)
	{
		pmh.add(dpy, pm, pb);
		glxdh.add(pb->drawable(), dpy);
		drawable=pb->drawable();
	}

		stoptrace();  prargi(x);  prargi(y);  prargi(w);  prargi(h);
		prargi(d);  prargc(c);  prargx(drawable);  closetrace();

	CATCH();
	return drawable;
}

void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
	TRY();
	// Prevent recursion
	if(!_isremote(dpy)) {_glXDestroyGLXPixmap(dpy, pix);  return;}
	////////////////////

		opentrace(glXDestroyGLXPixmap);  prargd(dpy);  prargx(pix);  starttrace();

	glxdh.remove(pix);
	pmh.remove(dpy, pix);

		stoptrace();  closetrace();

	CATCH();
}

GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pm, const int *attribs)
{
	GLXPixmap drawable=0;
	TRY();

	// Prevent recursion && handle overlay configs
	if(!_isremote(dpy) || rcfgh.isoverlay(dpy, config))
		return _glXCreatePixmap(dpy, config, pm, attribs);
	////////////////////

		opentrace(glXCreatePixmap);  prargd(dpy);  prargc(config);  prargx(pm);
		starttrace();

	Window root;  int x, y;  unsigned int w, h, bw, d;
	XGetGeometry(dpy, pm, &root, &x, &y, &w, &h, &bw, &d);
	pbuffer *pb=new pbuffer(w, h, config);
	if(pb)
	{
		pmh.add(dpy, pm, pb);
		glxdh.add(pb->drawable(), dpy);
		drawable=pb->drawable();
	}

		stoptrace();  prargi(x);  prargi(y);  prargi(w);  prargi(h);
		prargi(d);  prargx(drawable);  closetrace();

	CATCH();
	return drawable;
}

GLXPixmap glXCreateGLXPixmapWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap)
{
	return glXCreatePixmap(dpy, config, pixmap, NULL);
}

void glXDestroyPixmap(Display *dpy, GLXPixmap pix)
{
	TRY();
	// Prevent recursion
	if(!_isremote(dpy)) {_glXDestroyPixmap(dpy, pix);  return;}
	////////////////////

		opentrace(glXDestroyPixmap);  prargd(dpy);  prargx(pix);  starttrace();

	glxdh.remove(pix);
	pmh.remove(dpy, pix);

		stoptrace();  closetrace();

	CATCH();
}

#include "xfonts.c"

// We use a tweaked out version of the Mesa glXUseXFont()
// implementation.
void glXUseXFont(Font font, int first, int count, int list_base)
{
	TRY();

		opentrace(glXUseXFont);  prargx(font);  prargi(first);  prargi(count);
		prargi(list_base);  starttrace();

	if(ctxh.overlaycurrent()) _glXUseXFont(font, first, count, list_base);
	else Fake_glXUseXFont(font, first, count, list_base);

		stoptrace();  closetrace();

	return;
	CATCH();
}

void glXSwapBuffers(Display* dpy, GLXDrawable drawable)
{
	TRY();

		opentrace(glXSwapBuffers);  prargd(dpy);  prargx(drawable);  starttrace();

	if(winh.isoverlay(dpy, drawable))
	{
		_glXSwapBuffers(dpy, drawable);
		stoptrace();  closetrace();  
		return;
	}

	pbwin *pbw=NULL;
	if(_isremote(dpy) && winh.findpb(dpy, drawable, pbw))
	{
		pbw->readback(GL_BACK, false, fconfig.sync);
		pbw->swapbuffers();
	}
	else
	{
		if(!fconfig.glp) _glXSwapBuffers(_localdpy, drawable);
		#ifdef USEGLP
		else if(__glPSwapBuffers) _glPSwapBuffers(drawable);
		#endif
	}

		stoptrace();  if(_isremote(dpy) && pbw) {prargx(pbw->getdrawable());}
		closetrace();  

	CATCH();
}

static void _doGLreadback(bool spoillast, bool sync)
{
	pbwin *pbw;
	GLXDrawable drawable;
	if(ctxh.overlaycurrent()) return;
	drawable=GetCurrentDrawable();
	if(!drawable) return;
	if(winh.findpb(drawable, pbw))
	{
		if(_drawingtofront() || pbw->_dirty)
		{
				opentrace(_doGLreadback);  prargx(pbw->getdrawable());  prargi(sync);
				prargi(spoillast);  starttrace();

			pbw->readback(GL_FRONT, spoillast, sync);

				stoptrace();  closetrace();
		}
	}
}

void glFlush(void)
{
	TRY();

		if(fconfig.trace) rrout.print("[VGL] glFlush()\n");

	_glFlush();
	_doGLreadback(true, false);
	CATCH();
}

void glFinish(void)
{
	TRY();

		if(fconfig.trace) rrout.print("[VGL] glFinish()\n");

	_glFinish();
	_doGLreadback(false, fconfig.sync);
	CATCH();
}

void glXWaitGL(void)
{
	TRY();

		if(fconfig.trace) rrout.print("[VGL] glXWaitGL()\n");

	if(ctxh.overlaycurrent()) {_glXWaitGL();  return;}

	_glFinish();  // glXWaitGL() on some systems calls glFinish(), so we do this to avoid 2 readbacks
	_doGLreadback(false, fconfig.sync);
	CATCH();
}

// If the application switches the draw buffer before calling glFlush(), we
// set a lazy readback trigger
void glDrawBuffer(GLenum mode)
{
	TRY();

	if(ctxh.overlaycurrent()) {_glDrawBuffer(mode);  return;}

		opentrace(glDrawBuffer);  prargx(mode);  starttrace();

	pbwin *pbw=NULL;  int before=-1, after=-1, rbefore=-1, rafter=-1;
	GLXDrawable drawable=GetCurrentDrawable();
	if(drawable && winh.findpb(drawable, pbw))
	{
		before=_drawingtofront();
		rbefore=_drawingtoright();
		_glDrawBuffer(mode);
		after=_drawingtofront();
		rafter=_drawingtoright();
		if(before && !after) pbw->_dirty=true;
		if(rbefore && !rafter && pbw->stereo()) pbw->_rdirty=true;
	}
	else _glDrawBuffer(mode);

		stoptrace();  if(drawable && pbw) {prargi(pbw->_dirty);
		prargi(pbw->_rdirty);  prargx(pbw->getdrawable());}  closetrace();

	CATCH();
}

// glPopAttrib() can change the draw buffer state as well :/
void glPopAttrib(void)
{
	TRY();

	if(ctxh.overlaycurrent()) {_glPopAttrib();  return;}

		opentrace(glPopAttrib);  starttrace();

	pbwin *pbw=NULL;  int before=-1, after=-1, rbefore=-1, rafter=-1;
	GLXDrawable drawable=GetCurrentDrawable();
	if(drawable && winh.findpb(drawable, pbw))
	{
		before=_drawingtofront();
		rbefore=_drawingtoright();
		_glPopAttrib();
		after=_drawingtofront();
		rafter=_drawingtoright();
		if(before && !after) pbw->_dirty=true;
		if(rbefore && !rafter && pbw->stereo()) pbw->_rdirty=true;
	}
	else _glPopAttrib();

		stoptrace();  if(drawable && pbw) {prargi(pbw->_dirty);
		prargi(pbw->_rdirty);  prargx(pbw->getdrawable());}  closetrace();

	CATCH();
}

// Sometimes XNextEvent() is called from a thread other than the
// rendering thread, so we wait until glViewport() is called and
// take that opportunity to resize the Pbuffer
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRY();

	if(ctxh.overlaycurrent()) {_glViewport(x, y, width, height);  return;}

		opentrace(glViewport);  prargi(x);  prargi(y);  prargi(width);
		prargi(height);  starttrace();

	GLXContext ctx=GetCurrentContext();
	GLXDrawable draw=GetCurrentDrawable();
	GLXDrawable read=GetCurrentReadDrawable();
	Display *dpy=NULL;
	if(!fconfig.glp) dpy=GetCurrentDisplay();
	GLXDrawable newread=0, newdraw=0;
	if((dpy || fconfig.glp) && (draw || read) && ctx)
	{
		newread=read, newdraw=draw;
		pbwin *drawpbw=NULL, *readpbw=NULL;
		winh.findpb(draw, drawpbw);
		winh.findpb(read, readpbw);
		if(drawpbw) drawpbw->checkresize();
		if(readpbw && readpbw!=drawpbw) readpbw->checkresize();
		if(drawpbw) newdraw=drawpbw->updatedrawable();
		if(readpbw) newread=readpbw->updatedrawable();
		if(newread!=read || newdraw!=draw)
		{
			#ifdef USEGLP
			if(fconfig.glp) glPMakeContextCurrent(newdraw, newread, ctx);
			else
			#endif
			_glXMakeContextCurrent(dpy, newdraw, newread, ctx);
			if(drawpbw) {drawpbw->clear();  drawpbw->cleanup();}
			if(readpbw) readpbw->cleanup();
		}
	}
	_glViewport(x, y, width, height);

		stoptrace();  if(draw!=newdraw) {prargx(draw);  prargx(newdraw);}
		if(read!=newread) {prargx(read);  prargx(newread);}  closetrace();

	CATCH();
}

// The following nastiness is necessary to make color index rendering work,
// since most platforms don't support color index Pbuffers

#ifdef SUNOGL

static GLvoid r_glIndexd(OglContextPtr ctx, GLdouble c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexd() called on overlay context.");
	else glColor3d(c/255., 0.0, 0.0);
	CATCH();
}

static GLvoid r_glIndexf(OglContextPtr ctx, GLfloat c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexf() called on overlay context.");
	else glColor3f(c/255., 0., 0.);
	CATCH();
}

static GLvoid r_glIndexi(OglContextPtr ctx, GLint c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexi() called on overlay context.");
	else glColor3f((GLfloat)c/255., 0, 0);
	CATCH();
}

static GLvoid r_glIndexs(OglContextPtr ctx, GLshort c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexs() called on overlay context.");
	else glColor3f((GLfloat)c/255., 0, 0);
	CATCH();
}

static GLvoid r_glIndexub(OglContextPtr ctx, GLubyte c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexub() called on overlay context.");
	else glColor3f((GLfloat)c/255., 0, 0);
	CATCH();
}

static GLvoid r_glIndexdv(OglContextPtr ctx, const GLdouble *c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexdv() called on overlay context.");
	else
	{
		GLdouble v[3]={c? (*c)/255.:0., 0., 0.};
		glColor3dv(c? v:NULL);  return;
	}
	CATCH();
}

static GLvoid r_glIndexfv(OglContextPtr ctx, const GLfloat *c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexfv() called on overlay context.");
	else
	{
		GLfloat v[3]={c? (*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
	CATCH();
}

static GLvoid r_glIndexiv(OglContextPtr ctx, const GLint *c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexiv() called on overlay context.");
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
	CATCH();
}

static GLvoid r_glIndexsv(OglContextPtr ctx, const GLshort *c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexsv() called on overlay context.");
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
	CATCH();
}

static GLvoid r_glIndexubv(OglContextPtr ctx, const GLubyte *c)
{
	TRY();
	if(ctxh.overlaycurrent())
		_throw("r_glIndexubv() called on overlay context.");
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
	CATCH();
}

void glBegin(GLenum mode)
{
	_glBegin(mode);
	if(!ctxh.overlaycurrent())
	{
		sunOglCurPrimTablePtr->oglIndexd=r_glIndexd;
		sunOglCurPrimTablePtr->oglIndexf=r_glIndexf;
		sunOglCurPrimTablePtr->oglIndexi=r_glIndexi;
		sunOglCurPrimTablePtr->oglIndexs=r_glIndexs;
		sunOglCurPrimTablePtr->oglIndexub=r_glIndexub;
		sunOglCurPrimTablePtr->oglIndexdv=r_glIndexdv;
		sunOglCurPrimTablePtr->oglIndexfv=r_glIndexfv;
		sunOglCurPrimTablePtr->oglIndexiv=r_glIndexiv;
		sunOglCurPrimTablePtr->oglIndexsv=r_glIndexsv;
		sunOglCurPrimTablePtr->oglIndexubv=r_glIndexubv;
	}
}

#else

void glIndexd(GLdouble c)
{
	if(ctxh.overlaycurrent()) _glIndexd(c);
	else glColor3d(c/255., 0.0, 0.0);
}

void glIndexf(GLfloat c)
{
	if(ctxh.overlaycurrent()) _glIndexf(c);
	else glColor3f(c/255., 0., 0.);
}

void glIndexi(GLint c)
{
	if(ctxh.overlaycurrent()) _glIndexi(c);
	else glColor3f((GLfloat)c/255., 0, 0);
}

void glIndexs(GLshort c)
{
	if(ctxh.overlaycurrent()) _glIndexs(c);
	else glColor3f((GLfloat)c/255., 0, 0);
}

void glIndexub(GLubyte c)
{
	if(ctxh.overlaycurrent()) _glIndexub(c);
	else glColor3f((GLfloat)c/255., 0, 0);
}

void glIndexdv(const GLdouble *c)
{
	if(ctxh.overlaycurrent()) _glIndexdv(c);
	else
	{
		GLdouble v[3]={c? (*c)/255.:0., 0., 0.};
		glColor3dv(c? v:NULL);
	}
}

void glIndexfv(const GLfloat *c)
{
	if(ctxh.overlaycurrent()) _glIndexfv(c);
	else
	{
		GLfloat v[3]={c? (*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
}

void glIndexiv(const GLint *c)
{
	if(ctxh.overlaycurrent()) _glIndexiv(c);
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
}

void glIndexsv(const GLshort *c)
{
	if(ctxh.overlaycurrent()) _glIndexsv(c);
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
}

void glIndexubv(const GLubyte *c)
{
	if(ctxh.overlaycurrent()) _glIndexubv(c);
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
}

#endif

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	GLfloat mat[]={1., 1., 1., 1.};
	if(pname==GL_COLOR_INDEXES && params)
	{
		mat[0]=params[0]/255.;
		_glMaterialfv(face, GL_AMBIENT, mat);
		mat[0]=params[1]/255.;
		_glMaterialfv(face, GL_DIFFUSE, mat);
		mat[0]=params[2]/255.;
		_glMaterialfv(face, GL_SPECULAR, mat);
	}
	else _glMaterialfv(face, pname, params);
}

void glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
	GLfloat mat[]={1., 1., 1., 1.};
	if(pname==GL_COLOR_INDEXES && params)
	{
		mat[0]=(GLfloat)params[0]/255.;
		_glMaterialfv(face, GL_AMBIENT, mat);
		mat[0]=(GLfloat)params[1]/255.;
		_glMaterialfv(face, GL_DIFFUSE, mat);
		mat[0]=(GLfloat)params[2]/255.;
		_glMaterialfv(face, GL_SPECULAR, mat);
	}
	else _glMaterialiv(face, pname, params);
}

#define __round(f) ((f)>=0?(long)((f)+0.5):(long)((f)-0.5))

void glGetDoublev(GLenum pname, GLdouble *params)
{
	if(!ctxh.overlaycurrent())
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_COLOR, c);
			if(params) *params=(GLdouble)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, c);
			if(params) *params=(GLdouble)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			_glGetDoublev(GL_RED_SCALE, params);
			if(params) *params=(GLdouble)__round(log(*params)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			_glGetDoublev(GL_RED_BIAS, params);
			if(params) *params=(GLdouble)__round((*params)*255.);
			return;
		}
	}
	_glGetDoublev(pname, params);
}

void glGetFloatv(GLenum pname, GLfloat *params)
{
	if(!ctxh.overlaycurrent())
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_COLOR, c);
			if(params) *params=(GLfloat)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, c);
			if(params) *params=(GLfloat)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			GLdouble d;
			_glGetDoublev(GL_RED_SCALE, &d);
			if(params) *params=(GLfloat)__round(log(d)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			GLdouble d;
			_glGetDoublev(GL_RED_BIAS, &d);
			if(params) *params=(GLfloat)__round(d*255.);
			return;
		}
	}
	_glGetFloatv(pname, params);
}

void glGetIntegerv(GLenum pname, GLint *params)
{
	if(!ctxh.overlaycurrent())
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_COLOR, c);
			if(params) *params=(GLint)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, c);
			if(params) *params=(GLint)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			double d;
			_glGetDoublev(GL_RED_SCALE, &d);
			if(params) *params=(GLint)__round(log(d)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			double d;
			_glGetDoublev(GL_RED_BIAS, &d);
			if(params) *params=(GLint)__round(d*255.);
			return;
		}
	}
	_glGetIntegerv(pname, params);
}

void glPixelTransferf(GLenum pname, GLfloat param)
{
	if(!ctxh.overlaycurrent())
	{
		if(pname==GL_INDEX_SHIFT)
		{
			_glPixelTransferf(GL_RED_SCALE, pow(2., (double)param));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			_glPixelTransferf(GL_RED_BIAS, param/255.);
			return;
		}
	}
	_glPixelTransferf(pname, param);
}

void glPixelTransferi(GLenum pname, GLint param)
{
	if(!ctxh.overlaycurrent())
	{
		if(pname==GL_INDEX_SHIFT)
		{
			_glPixelTransferf(GL_RED_SCALE, pow(2., (double)param));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			_glPixelTransferf(GL_RED_BIAS, (GLfloat)param/255.);
			return;
		}
	}
	_glPixelTransferi(pname, param);
}

#define _rpixelconvert(ctype, gltype, size)  \
	if(type==gltype) {  \
		unsigned char *p=(unsigned char *)pixels;  \
		unsigned char *b=buf;  \
		int w=(rowlen>0? rowlen:width)*size;  \
		if(size<align) w=(w+align-1)&(~(align-1));  \
		for(int i=0; i<height; i++, p+=w, b+=width) {  \
			ctype *p2=(ctype *)p;  \
			for(int j=0; j<width; j++) p2[j]=(ctype)b[j];  \
		}}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
	GLenum format, GLenum type, GLvoid *pixels)
{
	TRY();
	if(format==GL_COLOR_INDEX && !ctxh.overlaycurrent() && type!=GL_BITMAP)
	{
		format=GL_RED;
		if(type==GL_BYTE || type==GL_UNSIGNED_BYTE) type=GL_UNSIGNED_BYTE;
		else
		{
			int rowlen=-1, align=-1;  GLubyte *buf=NULL;
			_glGetIntegerv(GL_PACK_ALIGNMENT, &align);
			_glGetIntegerv(GL_PACK_ROW_LENGTH, &rowlen);
			newcheck(buf=new unsigned char[width*height])
			if(type==GL_SHORT) type=GL_UNSIGNED_SHORT;
			if(type==GL_INT) type=GL_UNSIGNED_INT;
			glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 1);
			_glReadPixels(x, y, width, height, format, GL_UNSIGNED_BYTE, buf);
			glPopClientAttrib();
			_rpixelconvert(unsigned short, GL_UNSIGNED_SHORT, 2)
			_rpixelconvert(unsigned int, GL_UNSIGNED_INT, 4)
			_rpixelconvert(float, GL_FLOAT, 4)
			delete [] buf;
			return;
		}
	}
	_glReadPixels(x, y, width, height, format, type, pixels);
	CATCH();
}

#define _dpixelconvert(ctype, gltype, size)  \
	if(type==gltype) {  \
		unsigned char *p=(unsigned char *)pixels;  \
		unsigned char *b=buf;  \
		int w=(rowlen>0? rowlen:width)*size;  \
		if(size<align) w=(w+align-1)&(~(align-1));  \
		for(int i=0; i<height; i++, p+=w, b+=width) {  \
			ctype *p2=(ctype *)p;  \
			for(int j=0; j<width; j++) b[j]=(unsigned char)p2[j];  \
		}}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
	const GLvoid *pixels)
{
	TRY();
	if(format==GL_COLOR_INDEX && !ctxh.overlaycurrent() && type!=GL_BITMAP)
	{
		format=GL_RED;
		if(type==GL_BYTE || type==GL_UNSIGNED_BYTE) type=GL_UNSIGNED_BYTE;
		else
		{
			int rowlen=-1, align=-1;  GLubyte *buf=NULL;
			_glGetIntegerv(GL_PACK_ALIGNMENT, &align);
			_glGetIntegerv(GL_PACK_ROW_LENGTH, &rowlen);
			newcheck(buf=new unsigned char[width*height])
			if(type==GL_SHORT) type=GL_UNSIGNED_SHORT;
			if(type==GL_INT) type=GL_UNSIGNED_INT;
			_dpixelconvert(unsigned short, GL_UNSIGNED_SHORT, 2)
			_dpixelconvert(unsigned int, GL_UNSIGNED_INT, 4)
			_dpixelconvert(float, GL_FLOAT, 4)
			glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 1);
			_glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, buf);
			glPopClientAttrib();
			delete [] buf;
			return;
		}
	}
	_glDrawPixels(width, height, format, type, pixels);
	CATCH();
}

void glClearIndex(GLfloat c)
{
	if(ctxh.overlaycurrent()) _glClearIndex(c);
	else glClearColor(c/255., 0., 0., 0.);
}

} // extern "C"
