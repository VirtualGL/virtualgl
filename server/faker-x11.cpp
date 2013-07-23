/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011-2013 D. R. Commander
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

// Interposed X11 functions


extern "C" {

// XCloseDisplay() implicitly closes all windows and subwindows that were
// attached to the display handle, so we have to make sure that the
// corresponding pbwin instances are shut down.

int XCloseDisplay(Display *dpy)
{
	// For reasons unexplained, MainWin apps will sometimes call XCloseDisplay()
	// after the global instances have been destroyed, so if this has occurred,
	// we can't access fconfig or rrout or winh without causing deadlocks or
	// other issues.
	if(__shutdown) return _XCloseDisplay(dpy);

	int retval=0;
	TRY();

		opentrace(XCloseDisplay);  prargd(dpy);  starttrace();

	winh.remove(dpy);
	retval=_XCloseDisplay(dpy);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


// We have to override this function in order to handle GLX pixmap rendering

int XCopyArea(Display *dpy, Drawable src, Drawable dst, GC gc, int src_x,
	int src_y, unsigned int w, unsigned int h, int dest_x, int dest_y)
{
	TRY();
	pbdrawable *pbsrc=NULL;  pbdrawable *pbdst=NULL;
	bool srcwin=false, dstwin=false;
	bool copy2d=true, copy3d=false, triggerrb=false;
	int retval=0;
	GLXDrawable glxsrc=0, glxdst=0;

	if(src==0 || dst==0) return BadDrawable;

		opentrace(XCopyArea);  prargd(dpy);  prargx(src);  prargx(dst);
		prargx(gc); prargi(src_x);  prargi(src_y);  prargi(w);  prargi(h);
		prargi(dest_x);  prargi(dest_y);  starttrace();

	if(!(pbsrc=(pbdrawable *)pmh.find(dpy, src)))
	{
		pbsrc=(pbdrawable *)winh.findwin(dpy, src);
		if(pbsrc) srcwin=true;
	};
	if(!(pbdst=(pbdrawable *)pmh.find(dpy, dst)))
	{
		pbdst=(pbdrawable *)winh.findwin(dpy, dst);
		if(pbdst) dstwin=true;
	}

	// GLX (3D) Pixmap --> non-GLX (2D) drawable
	// Sync pixels from the 3D pixmap (on the 3D X Server) to the corresponding
	// 2D pixmap (on the 2D X Server) and let the "real" XCopyArea() do the rest.
	if(pbsrc && !srcwin && !pbdst) ((pbpm *)pbsrc)->readback();

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
	// We assume that glFinish() or another synchronization function has been
	// called prior to XCopyArea(), in order to copy the pixels from the
	// off-screen drawable on the 3D X Server to the corresponding window on the
	// 2D X Server.  Thus, we defer to the real XCopyArea() function (but this
	// may not work properly without VGL_SYNC=1.)
	{}

	// GLX (3D) Window --> GLX (3D) drawable
	// GLX (3D) Pixmap --> GLX (3D) Pixmap
	// Sync both 2D and 3D pixels.
	if(pbsrc && srcwin && pbdst) copy3d=true;
	if(pbsrc && !srcwin && pbdst && !dstwin) copy3d=true;

	// GLX (3D) Pixmap --> GLX (3D) Window
	// Copy 3D pixels to the window's corresponding off-screen drawable, then
	// trigger a VirtualGL readback to deliver the pixels from the off-screen
	// drawable to the window.
	if(pbsrc && !srcwin && pbdst && dstwin)
	{
		copy2d=false;  copy3d=true;  triggerrb=true;
	}

	if(copy2d) retval=_XCopyArea(dpy, src, dst, gc, src_x, src_y, w, h, dest_x,
		dest_y);

	if(copy3d)
	{
		glxsrc=pbsrc->getglxdrawable();
		glxdst=pbdst->getglxdrawable();
		pbsrc->copypixels(src_x, src_y, w, h, dest_x, dest_y, glxdst);
		if(triggerrb) ((pbwin *)pbdst)->readback(GL_FRONT, false, fconfig.sync);
	}

		stoptrace();  if(copy3d) prargx(glxsrc);  if(copy3d) prargx(glxdst);
		closetrace();

	CATCH();
	return 0;
}


// When a window is created, add it to the hash.  A pbwin instance does not
// get created and hashed to the window until/unless the window is made
// current in OpenGL.

Window XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	unsigned long border, unsigned long background)
{
	Window win=0;
	TRY();

		opentrace(XCreateSimpleWindow);  prargd(dpy);  prargx(parent);  prargi(x);
		prargi(y);  prargi(width);  prargi(height);  starttrace();

	win=_XCreateSimpleWindow(dpy, parent, x, y, width, height, border_width,
		border, background);
	if(win && _isremote(dpy)) winh.add(dpy, win);

		stoptrace();  prargx(win);  closetrace();

	CATCH();
	return win;
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

	win=_XCreateWindow(dpy, parent, x, y, width, height, border_width,
		depth, c_class, visual, valuemask, attributes);
	if(win && _isremote(dpy)) winh.add(dpy, win);

		stoptrace();  prargx(win);  closetrace();

	CATCH();
	return win;
}


// When a window is destroyed, we shut down the corresponding pbwin instance,
// but we also have to walk the window tree to ensure that pbwin instances
// attached to subwindows are also shut down.

static void DeleteWindow(Display *dpy, Window win, bool subonly=false)
{
	Window root, parent, *children=NULL;  unsigned int n=0;

	if(!subonly) winh.remove(dpy, win);
	if(XQueryTree(dpy, win, &root, &parent, &children, &n)
		&& children && n>0)
	{
		for(unsigned int i=0; i<n; i++) DeleteWindow(dpy, children[i]);
	}
}


int XDestroySubwindows(Display *dpy, Window win)
{
	int retval=0;
	TRY();

		opentrace(XDestroySubwindows);  prargd(dpy);  prargx(win);  starttrace();

	if(dpy && win) DeleteWindow(dpy, win, true);
	retval=_XDestroySubwindows(dpy, win);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


int XDestroyWindow(Display *dpy, Window win)
{
	int retval=0;
	TRY();

		opentrace(XDestroyWindow);  prargd(dpy);  prargx(win);  starttrace();

	if(dpy && win) DeleteWindow(dpy, win);
	retval=_XDestroyWindow(dpy, win);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


// If we're freeing a visual that is hashed to an FB config, then remove the
// corresponding hash entry.

int XFree(void *data)
{
	int ret=0;
	TRY();
	ret=_XFree(data);
	if(data && !isdead()) vish.remove(NULL, (XVisualInfo *)data);
	CATCH();
	return ret;
}


// Chromium is mainly to blame for this one.  Since it is using separate
// processes to do 3D and X11 rendering, the 3D process will call
// XGetGeometry() repeatedly to obtain the window size, and since the 3D
// process has no X event loop, monitoring this function is the only way for
// VirtualGL to know that the window size has changed.

Status XGetGeometry(Display *display, Drawable drawable, Window *root, int *x,
	int *y, unsigned int *width, unsigned int *height,
	unsigned int *border_width, unsigned int *depth)
{
	Status ret=0;
	unsigned int w=0, h=0;

		opentrace(XGetGeometry);  prargx(display);  prargx(drawable);
		starttrace();

	pbwin *pbw=NULL;
	if(winh.findpb(drawable, pbw))
	{
		// Apparently drawable is a GLX drawable ID that backs a window, so we need
		// to request the geometry of the window, not the GLX drawable.  This
		// prevents a BadDrawable error in Steam.
		display=pbw->get2ddpy();
		drawable=pbw->getx11drawable();
	}
	ret=_XGetGeometry(display, drawable, root, x, y, &w, &h, border_width,
		depth);
	if(winh.findpb(display, drawable, pbw) && w>0 && h>0)
		pbw->resize(w, h);

		stoptrace();  if(root) prargx(*root);  if(x) prargi(*x);  if(y) prargi(*y);
		prargi(w);  prargi(h);
		if(border_width) prargi(*border_width);  if(depth) prargi(*depth);
		closetrace();

	if(width) *width=w;  if(height) *height=h;
	return ret;
}


// If the pixmap has been used for 3D rendering, then we have to synchronize
// the contents of the 3D pixmap, which resides on the 3D X server, with the
// 2D pixmap on the 2D X server before calling the "real" XGetImage() function.

XImage *XGetImage(Display *display, Drawable d, int x, int y,
	unsigned int width, unsigned int height, unsigned long plane_mask,
	int format)
{
	XImage *xi=NULL;

		opentrace(XGetImage);  prargd(display);  prargx(d);  prargi(x);  prargi(y);
		prargi(width);  prargi(height);  prargx(plane_mask);  prargi(format);
		starttrace();

	pbpm *pbp=pmh.find(display, d);
	if(pbp) pbp->readback();

	xi=_XGetImage(display, d, x, y, width, height, plane_mask, format);

		stoptrace();  closetrace();

	return xi;
}


// Tell the application that the GLX extension is present, even if it isn't

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


// This is normally where VirtualGL initializes, unless a GLX function is
// called first.

Display *XOpenDisplay(_Xconst char* name)
{
	Display *dpy=NULL;
	TRY();

		opentrace(XOpenDisplay);  prargs(name);  starttrace();

	__vgl_fakerinit();
	dpy=_XOpenDisplay(name);
	if(dpy && strlen(fconfig.vendor)>0) ServerVendor(dpy)=strdup(fconfig.vendor);

		stoptrace();  prargd(dpy);  closetrace();

	CATCH();
	return dpy;
}


// Tell the application that the GLX extension is present, even if it isn't.

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


// This was implemented because of Pro/E Wildfire v2 on SPARC.  Unless the X
// server vendor string was "Sun Microsystems, Inc.", it would assume a remote
// connection and disable OpenGL rendering.

char *XServerVendor(Display *dpy)
{
	if(strlen(fconfig.vendor)>0) return fconfig.vendor;
	else return _XServerVendor(dpy);
}


// The following functions are interposed so that VirtualGL can detect window
// resizes, key presses (to pop up the VGL configuration dialog), and window
// delete events from the window manager.

static void _HandleEvent(Display *dpy, XEvent *xe)
{
	pbwin *pbw=NULL;
	if(xe && xe->type==ConfigureNotify)
	{
		if(winh.findpb(dpy, xe->xconfigure.window, pbw))
		{
				opentrace(_HandleEvent);  prargi(xe->xconfigure.width);
				prargi(xe->xconfigure.height);  prargx(xe->xconfigure.window);
				starttrace();

			pbw->resize(xe->xconfigure.width, xe->xconfigure.height);

				stoptrace();  closetrace();
		}
	}
	else if(xe && xe->type==KeyPress)
	{
		unsigned int state2, state=(xe->xkey.state)&(~(LockMask));
		state2=fconfig.guimod;
		if(state2&Mod1Mask) {state2&=(~(Mod1Mask));  state2|=Mod2Mask;}
		if(fconfig.gui
			&& XKeycodeToKeysym(dpy, xe->xkey.keycode, 0)==fconfig.guikey
			&& (state==fconfig.guimod || state==state2)
			&& fconfig_getshmid()!=-1)
			vglpopup(dpy, fconfig_getshmid());
	}
	else if(xe && xe->type==ClientMessage)
	{
		XClientMessageEvent *cme=(XClientMessageEvent *)xe;
		Atom protoatom=XInternAtom(dpy, "WM_PROTOCOLS", True);
		Atom deleteatom=XInternAtom(dpy, "WM_DELETE_WINDOW", True);
		if(protoatom && deleteatom && cme->message_type==protoatom
			&& cme->data.l[0]==(long)deleteatom
			&& winh.findpb(dpy, cme->window, pbw))
			pbw->wmdelete();
	}
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


Bool XCheckTypedWindowEvent(Display *dpy, Window win, int event_type,
	XEvent *xe)
{
	Bool retval=0;
	TRY();
	if((retval=_XCheckTypedWindowEvent(dpy, win, event_type, xe))==True)
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


int XConfigureWindow(Display *dpy, Window win, unsigned int value_mask,
	XWindowChanges *values)
{
	int retval=0;
	TRY();

		opentrace(XConfigureWindow);  prargd(dpy);  prargx(win);
		if(values && (value_mask&CWWidth)) {prargi(values->width);}
		if(values && (value_mask&CWHeight)) {prargi(values->height);}
		starttrace();

	pbwin *pbw=NULL;
	if(winh.findpb(dpy, win, pbw) && values)
		pbw->resize(value_mask&CWWidth? values->width:0,
			value_mask&CWHeight?values->height:0);
	retval=_XConfigureWindow(dpy, win, value_mask, values);

		stoptrace();  closetrace();

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


int XMoveResizeWindow(Display *dpy, Window win, int x, int y,
	unsigned int width, unsigned int height)
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


int XNextEvent(Display *dpy, XEvent *xe)
{
	int retval=0;
	TRY();
	retval=_XNextEvent(dpy, xe);
	_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}


int XResizeWindow(Display *dpy, Window win, unsigned int width,
	unsigned int height)
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


int XWindowEvent(Display *dpy, Window win, long event_mask, XEvent *xe)
{
	int retval=0;
	TRY();
	retval=_XWindowEvent(dpy, win, event_mask, xe);
	_HandleEvent(dpy, xe);
	CATCH();
	return retval;
}


} // extern "C"
