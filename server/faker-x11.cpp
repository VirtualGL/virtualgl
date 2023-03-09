// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011-2016, 2018-2023 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include "PixmapHash.h"
#include "VisualHash.h"
#include "WindowHash.h"
#include "faker.h"
#include "vglconfigLauncher.h"
#ifdef FAKEXCB
#include "XCBConnHash.h"
#endif
#ifdef EGLBACKEND
#include "EGLXWindowHash.h"
#endif
#include "keycodetokeysym.h"


// Interposed X11 functions


extern "C" {

// XCloseDisplay() implicitly closes all windows and subwindows that were
// attached to the display handle, so we have to make sure that the
// corresponding VirtualWin instances are shut down.

int XCloseDisplay(Display *dpy)
{
	// MainWin calls various X11 functions from the destructor of one of its
	// shared libraries, which is executed after the VirtualGL Faker has shut
	// down, so we cannot access fconfig or vglout or winh without causing
	// deadlocks or other issues.  At this point, all we can safely do is hand
	// off to libX11.
	if(faker::deadYet || faker::getFakerLevel() > 0)
		return _XCloseDisplay(dpy);

	int retval = 0;
	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XCloseDisplay);  PRARGD(dpy);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	#ifdef FAKEXCB
	if(fconfig.fakeXCB)
	{
		CHECKSYM_NONFATAL(XGetXCBConnection)
		if(!__XGetXCBConnection)
		{
			if(fconfig.verbose)
				vglout.print("[VGL] Disabling XCB interposer\n");
			fconfig.fakeXCB = 0;
		}
		else
		{
			xcb_connection_t *conn = _XGetXCBConnection(dpy);
			xcbconnhash.remove(conn);
		}
	}
	#endif

	winhash.remove(dpy);
	retval = _XCloseDisplay(dpy);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


// We have to override this function in order to handle GLX pixmap rendering

int XCopyArea(Display *dpy, Drawable src, Drawable dst, GC gc, int src_x,
	int src_y, unsigned int width, unsigned int height, int dest_x, int dest_y)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XCopyArea(dpy, src, dst, gc, src_x, src_y, width, height, dest_x,
			dest_y);

	DISABLE_FAKER();

	faker::VirtualDrawable *srcVW = NULL, *dstVW = NULL;
	bool srcWin = false, dstWin = false;
	bool copy2d = true, copy3d = false, triggerRB = false;
	GLXDrawable glxsrc = 0, glxdst = 0;

	if(src == 0 || dst == 0) return BadDrawable;

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XCopyArea);  PRARGD(dpy);  PRARGX(src);  PRARGX(dst);  PRARGX(gc);
	PRARGI(src_x);  PRARGI(src_y);  PRARGI(width);  PRARGI(height);
	PRARGI(dest_x);  PRARGI(dest_y);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(!(srcVW = (faker::VirtualDrawable *)pmhash.find(dpy, src)))
	{
		srcVW = (faker::VirtualDrawable *)winhash.find(dpy, src);
		if(srcVW) srcWin = true;
	}
	if(srcVW && !srcVW->isInit())
	{
		// If the 3D drawable hasn't been made current yet, then its contents will
		// be identical to the corresponding 2D drawable
		srcVW = NULL;
		srcWin = false;
	}
	if(!(dstVW = (faker::VirtualDrawable *)pmhash.find(dpy, dst)))
	{
		dstVW = (faker::VirtualDrawable *)winhash.find(dpy, dst);
		if(dstVW) dstWin = true;
	}
	if(dstVW && !dstVW->isInit())
	{
		dstVW = NULL;
		dstWin = false;
	}

	// GLX (3D) Pixmap --> non-GLX (2D) drawable
	// Sync pixels from the 3D pixmap (on the 3D X Server) to the corresponding
	// 2D pixmap (on the 2D X Server) and let the "real" XCopyArea() do the rest.
	if(srcVW && !srcWin && !dstVW)
		((faker::VirtualPixmap *)srcVW)->readback();

	// non-GLX (2D) drawable --> non-GLX (2D) drawable
	// Source and destination are not backed by a drawable on the 3D X Server, so
	// defer to the real XCopyArea() function.
	//
	// non-GLX (2D) drawable --> GLX (3D) drawable
	// We don't really handle this yet (and won't until we have to.)  Copy to the
	// 2D destination drawable only, without updating the corresponding 3D
	// drawable.
	//
	// GLX (3D) Window --> non-GLX (2D) drawable
	// We assume that glFinish() or another frame trigger function has been
	// called prior to XCopyArea(), in order to copy the rendered frame from the
	// off-screen drawable on the 3D X Server to the corresponding window on the
	// 2D X Server.  Thus, we defer to the real XCopyArea() function (but this
	// may not work properly without VGL_SYNC=1.)
	{}

	// GLX (3D) Window --> GLX (3D) drawable
	// GLX (3D) Pixmap --> GLX (3D) Pixmap
	// Sync both 2D and 3D pixels.
	if(srcVW && srcWin && dstVW) copy3d = true;
	if(srcVW && !srcWin && dstVW && !dstWin) copy3d = true;

	// GLX (3D) Pixmap --> GLX (3D) Window
	// Copy rendered frame to the window's corresponding off-screen drawable,
	// then trigger a readback to transport the frame from the off-screen
	// drawable to the window.
	if(srcVW && !srcWin && dstVW && dstWin)
	{
		copy2d = false;  copy3d = true;  triggerRB = true;
	}

	if(copy2d)
		_XCopyArea(dpy, src, dst, gc, src_x, src_y, width, height, dest_x, dest_y);

	if(copy3d)
	{
		glxsrc = srcVW->getGLXDrawable();
		glxdst = dstVW->getGLXDrawable();
		srcVW->copyPixels(src_x, src_y, width, height, dest_x, dest_y, glxdst);
		if(triggerRB)
			((faker::VirtualWin *)dstVW)->readback(GL_FRONT, false, fconfig.sync);
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(copy3d) PRARGX(glxsrc);  if(copy3d) PRARGX(glxdst);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return 0;
}


// When a window is created, add it to the hash.  A VirtualWin instance does
// not get created and hashed to the window until/unless the window is made
// current in OpenGL.

Window XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	unsigned long border, unsigned long background)
{
	Window win = 0;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XCreateSimpleWindow(dpy, parent, x, y, width, height, border_width,
			border, background);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XCreateSimpleWindow);  PRARGD(dpy);  PRARGX(parent);  PRARGI(x);
	PRARGI(y);  PRARGI(width);  PRARGI(height);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	win = _XCreateSimpleWindow(dpy, parent, x, y, width, height, border_width,
		border, background);
	if(win) winhash.add(dpy, win);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(win);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return win;
}


Window XCreateWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	int depth, unsigned int c_class, Visual *visual, unsigned long valuemask,
	XSetWindowAttributes *attributes)
{
	Window win = 0;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XCreateWindow(dpy, parent, x, y, width, height, border_width,
			depth, c_class, visual, valuemask, attributes);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XCreateWindow);  PRARGD(dpy);  PRARGX(parent);  PRARGI(x);
	PRARGI(y);  PRARGI(width);  PRARGI(height);  PRARGI(depth);  PRARGI(c_class);
	PRARGV(visual);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	win = _XCreateWindow(dpy, parent, x, y, width, height, border_width, depth,
		c_class, visual, valuemask, attributes);
	if(win) winhash.add(dpy, win);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(win);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return win;
}


// When a window is destroyed, we shut down the corresponding VirtualWin
// instance, but we also have to walk the window tree to ensure that VirtualWin
// instances attached to subwindows are also shut down.

static void DeleteWindow(Display *dpy, Window win, bool subOnly = false)
{
	Window root, parent, *children = NULL;  unsigned int n = 0;

	if(!subOnly) winhash.remove(dpy, win);
	if(XQueryTree(dpy, win, &root, &parent, &children, &n)
		&& children && n > 0)
	{
		for(unsigned int i = 0; i < n; i++) DeleteWindow(dpy, children[i]);
		_XFree(children);
	}
}


int XDestroySubwindows(Display *dpy, Window win)
{
	int retval = 0;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XDestroySubwindows(dpy, win);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XDestroySubwindows);  PRARGD(dpy);  PRARGX(win);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	if(dpy && win) DeleteWindow(dpy, win, true);
	retval = _XDestroySubwindows(dpy, win);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


int XDestroyWindow(Display *dpy, Window win)
{
	int retval = 0;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XDestroyWindow(dpy, win);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XDestroyWindow);  PRARGD(dpy);  PRARGX(win);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	if(dpy && win) DeleteWindow(dpy, win);
	retval = _XDestroyWindow(dpy, win);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


// If we're freeing a visual that is hashed to an FB config, then remove the
// corresponding hash entry.

int XFree(void *data)
{
	int ret = 0;
	TRY();
	ret = _XFree(data);
	if(data && !faker::deadYet) vishash.remove(NULL, (XVisualInfo *)data);
	CATCH();
	return ret;
}


// Chromium is mainly to blame for this one.  Since it is using separate
// processes to do 3D and X11 rendering, the 3D process will call
// XGetGeometry() repeatedly to obtain the window size, and since the 3D
// process has no X event loop, monitoring this function is the only way for
// VirtualGL to know that the window size has changed.

Status XGetGeometry(Display *dpy, Drawable drawable, Window *root, int *x,
	int *y, unsigned int *width_return, unsigned int *height_return,
	unsigned int *border_width, unsigned int *depth)
{
	Status ret = 0;
	unsigned int width = 0, height = 0;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XGetGeometry(dpy, drawable, root, x, y, width_return,
			height_return, border_width, depth);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XGetGeometry);  PRARGD(dpy);  PRARGX(drawable);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw;
	if((vw = winhash.find(NULL, drawable)) != NULL)
	{
		// Apparently drawable is a GLX drawable ID that backs a window, so we need
		// to request the geometry of the window, not the GLX drawable.  This
		// prevents a BadDrawable error in Steam.
		dpy = vw->getX11Display();
		drawable = vw->getX11Drawable();
	}
	ret = _XGetGeometry(dpy, drawable, root, x, y, &width, &height, border_width,
		depth);

	if((vw = winhash.find(dpy, drawable)) != NULL && width > 0 && height > 0)
		vw->resize(width, height);
	#ifdef EGLBACKEND
	faker::EGLXVirtualWin *eglxvw;
	if((eglxvw = eglxwinhash.find(dpy, drawable)) != NULL && width > 0
		&& height > 0)
		eglxvw->resize(width, height);
	#endif

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(root) PRARGX(*root);  if(x) PRARGI(*x);  if(y) PRARGI(*y);
	PRARGI(width);  PRARGI(height);  if(border_width) PRARGI(*border_width);
	if(depth) PRARGI(*depth);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(width_return) *width_return = width;
	if(height_return) *height_return = height;

	CATCH();
	return ret;
}


// If the pixmap has been used for 3D rendering, then we have to synchronize
// the contents of the 3D pixmap, which resides on the 3D X server, with the
// 2D pixmap on the 2D X server before calling the "real" XGetImage() function.

XImage *XGetImage(Display *dpy, Drawable drawable, int x, int y,
	unsigned int width, unsigned int height, unsigned long plane_mask,
	int format)
{
	XImage *xi = NULL;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XGetImage(dpy, drawable, x, y, width, height, plane_mask, format);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XGetImage);  PRARGD(dpy);  PRARGX(drawable);  PRARGI(x);
	PRARGI(y);  PRARGI(width);  PRARGI(height);  PRARGX(plane_mask);
	PRARGI(format);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	faker::VirtualPixmap *vpm = pmhash.find(dpy, drawable);
	if(vpm) vpm->readback();

	xi = _XGetImage(dpy, drawable, x, y, width, height, plane_mask, format);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return xi;
}


// Tell the application that the GLX extension is present, even if it isn't

char **XListExtensions(Display *dpy, int *next)
{
	char **list = NULL, *listStr = NULL;  int n, i;
	int hasGLX = 0, listLen = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _XListExtensions(dpy, next);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XListExtensions);  PRARGD(dpy);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	list = _XListExtensions(dpy, &n);
	if(list && n > 0)
	{
		for(i = 0; i < n; i++)
		{
			if(list[i])
			{
				listLen += strlen(list[i]) + 1;
				if(!strcmp(list[i], "GLX")) hasGLX = 1;
			}
		}
	}
	if(!hasGLX)
	{
		char **newList = NULL;  int index = 0;
		listLen += 4;  // "GLX" + terminating NULL
		ERRIFNOT(newList = (char **)malloc(sizeof(char *) * (n + 1)))
		ERRIFNOT(listStr = (char *)malloc(listLen + 1))
		memset(listStr, 0, listLen + 1);
		listStr = &listStr[1];  // For compatibility with X.org implementation
		if(list && n > 0)
		{
			for(i = 0; i < n; i++)
			{
				newList[i] = &listStr[index];
				if(list[i])
				{
					memcpy(newList[i], list[i], strlen(list[i]));
					index += strlen(list[i]);
					listStr[index] = '\0';  index++;
				}
			}
			XFreeExtensionList(list);
		}
		newList[n] = &listStr[index];
		memcpy(newList[n], "GLX", 3);  newList[n][3] = '\0';
		list = newList;  n++;
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(n);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	if(next) *next = n;
	return list;
}


static void setupXDisplay(Display *dpy)
{
	XExtCodes *codes;
	XEDataObject obj = { dpy };
	XExtData *extData;
	bool excludeDisplay = faker::isDisplayStringExcluded(DisplayString(dpy));

	// Extension code 1 stores the excluded status for a Display.
	if(!(codes = XAddExtension(dpy))
		|| !(extData = (XExtData *)calloc(1, sizeof(XExtData)))
		|| !(extData->private_data = (XPointer)malloc(sizeof(bool))))
		THROW("Memory allocation error");
	*(bool *)extData->private_data = excludeDisplay;
	extData->number = codes->extension;
	XAddToExtensionList(XEHeadOfExtensionList(obj), extData);

	// Extension code 2 stores the mutex for a Display.
	if(!(codes = XAddExtension(dpy))
		|| !(extData = (XExtData *)calloc(1, sizeof(XExtData))))
		THROW("Memory allocation error");
	extData->private_data = (XPointer)(new util::CriticalSection());
	extData->number = codes->extension;
	extData->free_private = faker::deleteCS;
	XAddToExtensionList(XEHeadOfExtensionList(obj), extData);

	// Extension code 3 stores the visual attribute table for a Screen.
	if(!(codes = XAddExtension(dpy)))
		THROW("Memory allocation error");

	// Extension code 4 stores the FB config attribute table for a Screen.
	if(!(codes = XAddExtension(dpy)))
		THROW("Memory allocation error");

	#ifdef EGLBACKEND
	// Extension code 5 stores the RBO context instance for a Display
	if(!(codes = XAddExtension(dpy))
		|| !(extData = (XExtData *)calloc(1, sizeof(XExtData))))
		THROW("Memory allocation error");
	extData->private_data = (XPointer)(new backend::RBOContext());
	extData->number = 5;
	extData->free_private = faker::deleteRBOContext;
	XAddToExtensionList(XEHeadOfExtensionList(obj), extData);
	#endif

	if(!excludeDisplay && strlen(fconfig.vendor) > 0)
	{
		// Danger, Will Robinson!  We do this to prevent a small memory leak, but
		// we can only can get away with it because we know that Xlib dynamically
		// allocates the vendor string.  Xlib has done so for as long as VirtualGL
		// has been around, so it seems like a safe assumption.
		_XFree(ServerVendor(dpy));
		ServerVendor(dpy) = strdup(fconfig.vendor);
	}
}


// This is normally where VirtualGL initializes, unless a GLX function is
// called first.

Display *XOpenDisplay(_Xconst char *name)
{
	Display *dpy = NULL;

	TRY();

	if(faker::deadYet || faker::getFakerLevel() > 0)
		return _XOpenDisplay(name);

	faker::init();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XOpenDisplay);  PRARGS(name);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	dpy = _XOpenDisplay(name);
	if(dpy) setupXDisplay(dpy);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGD(dpy);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return dpy;
}


// XkbOpenDisplay() calls XOpenDisplay(), but since that function call occurs
// within libX11, VirtualGL cannot intercept it on some platforms.  Thus we
// need to interpose XkbOpenDisplay().

#ifdef LIBX11_18
Display *XkbOpenDisplay(_Xconst char *display_name, int *event_rtrn,
	int *error_rtrn, int *major_in_out, int *minor_in_out, int *reason_rtrn)
#else
Display *XkbOpenDisplay(char *display_name, int *event_rtrn, int *error_rtrn,
	int *major_in_out, int *minor_in_out, int *reason_rtrn)
#endif
{
	Display *dpy = NULL;

	TRY();

	if(faker::deadYet || faker::getFakerLevel() > 0)
		return _XkbOpenDisplay(display_name, event_rtrn, error_rtrn, major_in_out,
			minor_in_out, reason_rtrn);

	faker::init();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XkbOpenDisplay);  PRARGS(display_name);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	dpy = _XkbOpenDisplay(display_name, event_rtrn, error_rtrn, major_in_out,
		minor_in_out, reason_rtrn);
	if(dpy) setupXDisplay(dpy);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGD(dpy);  if(event_rtrn) PRARGI(*event_rtrn);
	if(error_rtrn) PRARGI(*error_rtrn);
	if(major_in_out) PRARGI(*major_in_out);
	if(minor_in_out) PRARGI(*minor_in_out);
	if(reason_rtrn) PRARGI(*reason_rtrn);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return dpy;
}


// Tell the application that the GLX extension is present, even if it isn't.

Bool XQueryExtension(Display *dpy, _Xconst char *name, int *major_opcode,
	int *first_event, int *first_error)
{
	Bool retval = True;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _XQueryExtension(dpy, name, major_opcode, first_event, first_error);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XQueryExtension);  PRARGD(dpy);  PRARGS(name);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(!strcmp(name, "GLX"))
		retval = backend::queryExtension(dpy, major_opcode, first_event,
			first_error);
	else
		retval = _XQueryExtension(dpy, name, major_opcode, first_event,
			first_error);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(major_opcode) PRARGI(*major_opcode);
	if(first_event) PRARGI(*first_event);
	if(first_error) PRARGI(*first_error);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return retval;
}


// This was implemented because of Pro/E Wildfire v2 on SPARC.  Unless the X
// server vendor string was "Sun Microsystems, Inc.", it would assume a remote
// connection and disable OpenGL rendering.

char *XServerVendor(Display *dpy)
{
	TRY();

	if(IS_EXCLUDED(dpy) || !strlen(fconfig.vendor))
		return _XServerVendor(dpy);
	return fconfig.vendor;

	CATCH();

	return NULL;
}


// The following functions are interposed so that VirtualGL can detect window
// resizes, key presses (to pop up the VGL configuration dialog), and window
// delete events from the window manager.

static void handleEvent(Display *dpy, XEvent *xe)
{
	faker::VirtualWin *vw;
	#ifdef EGLBACKEND
	faker::EGLXVirtualWin *eglxvw;
	#endif

	if(IS_EXCLUDED(dpy))
		return;

	if(xe && xe->type == ConfigureNotify)
	{
		if((vw = winhash.find(dpy, xe->xconfigure.window)) != NULL)
		{
			/////////////////////////////////////////////////////////////////////////
			OPENTRACE(handleEvent);  PRARGI(xe->xconfigure.width);
			PRARGI(xe->xconfigure.height);  PRARGX(xe->xconfigure.window);
			STARTTRACE();
			/////////////////////////////////////////////////////////////////////////

			vw->resize(xe->xconfigure.width, xe->xconfigure.height);

			/////////////////////////////////////////////////////////////////////////
			STOPTRACE();  CLOSETRACE();
			/////////////////////////////////////////////////////////////////////////
		}
		#ifdef EGLBACKEND
		if((eglxvw = eglxwinhash.find(dpy, xe->xconfigure.window)) != NULL)
		{
			/////////////////////////////////////////////////////////////////////////
			OPENTRACE(handleEvent);  PRARGI(xe->xconfigure.width);
			PRARGI(xe->xconfigure.height);  PRARGX(xe->xconfigure.window);
			STARTTRACE();
			/////////////////////////////////////////////////////////////////////////

			eglxvw->resize(xe->xconfigure.width, xe->xconfigure.height);

			/////////////////////////////////////////////////////////////////////////
			STOPTRACE();  CLOSETRACE();
			/////////////////////////////////////////////////////////////////////////
		}
		#endif
	}
	else if(xe && xe->type == KeyPress)
	{
		unsigned int state2, state = (xe->xkey.state & (~(LockMask))) & 0xFF;
		state2 = fconfig.guimod;
		if(state2 & Mod1Mask)
		{
			state2 &= (~(Mod1Mask));  state2 |= Mod2Mask;
		}
		if(fconfig.gui
			&& KeycodeToKeysym(dpy, xe->xkey.keycode, 0) == fconfig.guikey
			&& (state == fconfig.guimod || state == state2)
			&& fconfig_getshmid() != -1)
			VGLPOPUP(dpy, fconfig_getshmid());
	}
	else if(xe && xe->type == ClientMessage)
	{
		XClientMessageEvent *cme = (XClientMessageEvent *)xe;
		Atom protoAtom = XInternAtom(dpy, "WM_PROTOCOLS", True);
		Atom deleteAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
		if(protoAtom && deleteAtom && cme->message_type == protoAtom
			&& cme->data.l[0] == (long)deleteAtom)
		{
			if((vw = winhash.find(dpy, cme->window)) != NULL)
				vw->wmDeleted();
			#ifdef EGLBACKEND
			if((eglxvw = eglxwinhash.find(dpy, cme->window)) != NULL)
				eglxvw->wmDeleted();
			#endif
		}
	}
}


Bool XCheckMaskEvent(Display *dpy, long event_mask, XEvent *xe)
{
	Bool retval = 0;
	TRY();

	if((retval = _XCheckMaskEvent(dpy, event_mask, xe)) == True)
		handleEvent(dpy, xe);

	CATCH();
	return retval;
}


Bool XCheckTypedEvent(Display *dpy, int event_type, XEvent *xe)
{
	Bool retval = 0;
	TRY();

	if((retval = _XCheckTypedEvent(dpy, event_type, xe)) == True)
		handleEvent(dpy, xe);

	CATCH();
	return retval;
}


Bool XCheckTypedWindowEvent(Display *dpy, Window win, int event_type,
	XEvent *xe)
{
	Bool retval = 0;
	TRY();

	if((retval = _XCheckTypedWindowEvent(dpy, win, event_type, xe)) == True)
		handleEvent(dpy, xe);

	CATCH();
	return retval;
}


Bool XCheckWindowEvent(Display *dpy, Window win, long event_mask, XEvent *xe)
{
	Bool retval = 0;
	TRY();

	if((retval = _XCheckWindowEvent(dpy, win, event_mask, xe)) == True)
		handleEvent(dpy, xe);

	CATCH();
	return retval;
}


int XConfigureWindow(Display *dpy, Window win, unsigned int value_mask,
	XWindowChanges *values)
{
	int retval = 0;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XConfigureWindow(dpy, win, value_mask, values);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XConfigureWindow);  PRARGD(dpy);  PRARGX(win);
	if(values && (value_mask & CWWidth)) { PRARGI(values->width); }
	if(values && (value_mask & CWHeight)) { PRARGI(values->height); }
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw;
	if((vw = winhash.find(dpy, win)) != NULL && values)
		vw->resize(value_mask & CWWidth ? values->width : 0,
			value_mask & CWHeight ? values->height : 0);
	#ifdef EGLBACKEND
	faker::EGLXVirtualWin *eglxvw;
	if((eglxvw = eglxwinhash.find(dpy, win)) != NULL && values)
		eglxvw->resize(value_mask & CWWidth ? values->width : 0,
			value_mask & CWHeight ? values->height : 0);
	#endif
	retval = _XConfigureWindow(dpy, win, value_mask, values);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return retval;
}


int XMaskEvent(Display *dpy, long event_mask, XEvent *xe)
{
	int retval = 0;
	TRY();

	retval = _XMaskEvent(dpy, event_mask, xe);
	handleEvent(dpy, xe);

	CATCH();
	return retval;
}


int XMoveResizeWindow(Display *dpy, Window win, int x, int y,
	unsigned int width, unsigned int height)
{
	int retval = 0;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XMoveResizeWindow(dpy, win, x, y, width, height);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XMoveResizeWindow);  PRARGD(dpy);  PRARGX(win);  PRARGI(x);
	PRARGI(y);  PRARGI(width);  PRARGI(height);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw;
	if((vw = winhash.find(dpy, win)) != NULL)
		vw->resize(width, height);
	#ifdef EGLBACKEND
	faker::EGLXVirtualWin *eglxvw;
	if((eglxvw = eglxwinhash.find(dpy, win)) != NULL)
		eglxvw->resize(width, height);
	#endif
	retval = _XMoveResizeWindow(dpy, win, x, y, width, height);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return retval;
}


int XNextEvent(Display *dpy, XEvent *xe)
{
	int retval = 0;
	TRY();

	retval = _XNextEvent(dpy, xe);
	handleEvent(dpy, xe);

	CATCH();
	return retval;
}


int XResizeWindow(Display *dpy, Window win, unsigned int width,
	unsigned int height)
{
	int retval = 0;
	TRY();

	if(IS_EXCLUDED(dpy))
		return _XResizeWindow(dpy, win, width, height);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XResizeWindow);  PRARGD(dpy);  PRARGX(win);  PRARGI(width);
	PRARGI(height);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualWin *vw;
	if((vw = winhash.find(dpy, win)) != NULL)
		vw->resize(width, height);
	#ifdef EGLBACKEND
	faker::EGLXVirtualWin *eglxvw;
	if((eglxvw = eglxwinhash.find(dpy, win)) != NULL)
		eglxvw->resize(width, height);
	#endif
	retval = _XResizeWindow(dpy, win, width, height);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return retval;
}


int XWindowEvent(Display *dpy, Window win, long event_mask, XEvent *xe)
{
	int retval = 0;
	TRY();

	retval = _XWindowEvent(dpy, win, event_mask, xe);
	handleEvent(dpy, xe);

	CATCH();
	return retval;
}


}  // extern "C"
