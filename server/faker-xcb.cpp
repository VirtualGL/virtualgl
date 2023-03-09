// Copyright (C)2014-2016, 2018-2023 D. R. Commander
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

#ifdef FAKEXCB

#include "WindowHash.h"
#include "XCBConnHash.h"
#ifdef EGLBACKEND
#include "EGLXDisplayHash.h"
#include "EGLXWindowHash.h"
#endif
#include "faker.h"
#include "vglconfigLauncher.h"


// This interposes enough of XCB to make Qt 5 work.  It may be necessary to
// expand this in the future, but at the moment there is no XCB equivalent for
// the GLX API, so XCB applications that want to use OpenGL will have to use
// Xlib-xcb.  Thus, we only need to interpose enough of XCB to make the
// application think that GLX is always available and to intercept window
// resize events.


extern "C" {


xcb_void_cookie_t xcb_create_window(xcb_connection_t *conn, uint8_t depth,
	xcb_window_t wid, xcb_window_t parent, int16_t x, int16_t y, uint16_t width,
	uint16_t height, uint16_t border_width, uint16_t _class,
	xcb_visualid_t visual, uint32_t value_mask, const void *value_list)
{
	xcb_void_cookie_t cookie = { 0 };

	TRY();

	if(!fconfig.fakeXCB || IS_EXCLUDED(xcbconnhash.getX11Display(conn)))
		return _xcb_create_window(conn, depth, wid, parent, x, y, width, height,
			border_width, _class, visual, value_mask, value_list);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_create_window);  PRARGX(conn);  PRARGI(depth);  PRARGX(wid);
	PRARGX(parent);  PRARGI(x);  PRARGI(y);  PRARGI(width);  PRARGI(height);
	PRARGI(border_width);  PRARGI(_class);  PRARGX(visual);  PRARGX(value_mask);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	cookie = _xcb_create_window(conn, depth, wid, parent, x, y, width, height,
		border_width, _class, visual, value_mask, value_list);
	if(cookie.sequence) winhash.add(xcbconnhash.getX11Display(conn), wid);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
}


xcb_void_cookie_t xcb_create_window_aux(xcb_connection_t *conn, uint8_t depth,
	xcb_window_t wid, xcb_window_t parent, int16_t x, int16_t y, uint16_t width,
	uint16_t height, uint16_t border_width, uint16_t _class,
	xcb_visualid_t visual, uint32_t value_mask,
	const xcb_create_window_value_list_t *value_list)
{
	xcb_void_cookie_t cookie = { 0 };

	TRY();

	if(!fconfig.fakeXCB || IS_EXCLUDED(xcbconnhash.getX11Display(conn)))
		return _xcb_create_window_aux(conn, depth, wid, parent, x, y, width,
			height, border_width, _class, visual, value_mask, value_list);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_create_window_aux);  PRARGX(conn);  PRARGI(depth);
	PRARGX(wid);  PRARGX(parent);  PRARGI(x);  PRARGI(y);  PRARGI(width);
	PRARGI(height);  PRARGI(border_width);  PRARGI(_class);  PRARGX(visual);
	PRARGX(value_mask);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	cookie = _xcb_create_window_aux(conn, depth, wid, parent, x, y, width,
		height, border_width, _class, visual, value_mask, value_list);
	if(cookie.sequence) winhash.add(xcbconnhash.getX11Display(conn), wid);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
}


xcb_void_cookie_t xcb_create_window_aux_checked(xcb_connection_t *conn,
	uint8_t depth, xcb_window_t wid, xcb_window_t parent, int16_t x, int16_t y,
	uint16_t width, uint16_t height, uint16_t border_width, uint16_t _class,
	xcb_visualid_t visual, uint32_t value_mask,
	const xcb_create_window_value_list_t *value_list)
{
	xcb_void_cookie_t cookie = { 0 };

	TRY();

	if(!fconfig.fakeXCB || IS_EXCLUDED(xcbconnhash.getX11Display(conn)))
		return _xcb_create_window_aux_checked(conn, depth, wid, parent, x, y,
			width, height, border_width, _class, visual, value_mask, value_list);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_create_window_aux_checked);  PRARGX(conn);  PRARGI(depth);
	PRARGX(wid);  PRARGX(parent);  PRARGI(x);  PRARGI(y);  PRARGI(width);
	PRARGI(height);  PRARGI(border_width);  PRARGI(_class);  PRARGX(visual);
	PRARGX(value_mask);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	cookie = _xcb_create_window_aux_checked(conn, depth, wid, parent, x, y,
		width, height, border_width, _class, visual, value_mask, value_list);
	if(cookie.sequence) winhash.add(xcbconnhash.getX11Display(conn), wid);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
}


xcb_void_cookie_t xcb_create_window_checked(xcb_connection_t *conn,
	uint8_t depth, xcb_window_t wid, xcb_window_t parent, int16_t x, int16_t y,
	uint16_t width, uint16_t height, uint16_t border_width, uint16_t _class,
	xcb_visualid_t visual, uint32_t value_mask, const void *value_list)
{
	xcb_void_cookie_t cookie = { 0 };

	TRY();

	if(!fconfig.fakeXCB || IS_EXCLUDED(xcbconnhash.getX11Display(conn)))
		return _xcb_create_window_checked(conn, depth, wid, parent, x, y, width,
			height, border_width, _class, visual, value_mask, value_list);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_create_window_checked);  PRARGX(conn);  PRARGI(depth);
	PRARGX(wid);  PRARGX(parent);  PRARGI(x);  PRARGI(y);  PRARGI(width);
	PRARGI(height);  PRARGI(border_width);  PRARGI(_class);  PRARGX(visual);
	PRARGX(value_mask);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	cookie = _xcb_create_window_checked(conn, depth, wid, parent, x, y, width,
		height, border_width, _class, visual, value_mask, value_list);
	if(cookie.sequence) winhash.add(xcbconnhash.getX11Display(conn), wid);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
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


xcb_void_cookie_t xcb_destroy_subwindows(xcb_connection_t *conn,
	xcb_window_t window)
{
	xcb_void_cookie_t cookie = { 0 };

	TRY();

	Display *dpy = xcbconnhash.getX11Display(conn);

	if(!fconfig.fakeXCB || IS_EXCLUDED(dpy))
		return _xcb_destroy_subwindows(conn, window);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_destroy_subwindows);  PRARGX(conn);  PRARGX(window);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(dpy && window) DeleteWindow(dpy, window, true);
	cookie = _xcb_destroy_subwindows(conn, window);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
}


xcb_void_cookie_t xcb_destroy_subwindows_checked(xcb_connection_t *conn,
	xcb_window_t window)
{
	xcb_void_cookie_t cookie = { 0 };

	TRY();

	Display *dpy = xcbconnhash.getX11Display(conn);

	if(!fconfig.fakeXCB || IS_EXCLUDED(dpy))
		return _xcb_destroy_subwindows_checked(conn, window);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_destroy_subwindows_checked);  PRARGX(conn);  PRARGX(window);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(dpy && window) DeleteWindow(dpy, window, true);
	cookie = _xcb_destroy_subwindows_checked(conn, window);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
}


xcb_void_cookie_t xcb_destroy_window(xcb_connection_t *conn,
	xcb_window_t window)
{
	xcb_void_cookie_t cookie = { 0 };

	TRY();

	Display *dpy = xcbconnhash.getX11Display(conn);

	if(!fconfig.fakeXCB || IS_EXCLUDED(dpy))
		return _xcb_destroy_window(conn, window);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_destroy_window);  PRARGX(conn);  PRARGX(window);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(dpy && window) DeleteWindow(dpy, window);
	cookie = _xcb_destroy_window(conn, window);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
}


xcb_void_cookie_t xcb_destroy_window_checked(xcb_connection_t *conn,
	xcb_window_t window)
{
	xcb_void_cookie_t cookie = { 0 };

	TRY();

	Display *dpy = xcbconnhash.getX11Display(conn);

	if(!fconfig.fakeXCB || IS_EXCLUDED(dpy))
		return _xcb_destroy_window_checked(conn, window);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_destroy_window_checked);  PRARGX(conn);  PRARGX(window);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(dpy && window) DeleteWindow(dpy, window);
	cookie = _xcb_destroy_window_checked(conn, window);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
}


const xcb_query_extension_reply_t *
	xcb_get_extension_data(xcb_connection_t *conn, xcb_extension_t *ext)
{
	const xcb_query_extension_reply_t *reply = NULL;

	TRY();

	if(fconfig.fakeXCB && ext && !strcmp(ext->name, "GLX")
		&& !IS_EXCLUDED(xcbconnhash.getX11Display(conn)))
	{
		///////////////////////////////////////////////////////////////////////////
		OPENTRACE(xcb_get_extension_data);  PRARGX(conn);  PRARGS(ext->name);
		PRARGI(ext->global_id);  STARTTRACE();
		///////////////////////////////////////////////////////////////////////////

		xcb_connection_t *conn3D =
			(fconfig.egl ? conn : _XGetXCBConnection(DPY3D));
		if(conn3D != NULL)
			reply = _xcb_get_extension_data(conn3D, _xcb_glx_id());

		///////////////////////////////////////////////////////////////////////////
		STOPTRACE();
		if(reply)
		{
			PRARGI(reply->present);  PRARGI(reply->major_opcode);
			PRARGI(reply->first_event);  PRARGI(reply->first_error);
		}
		else PRARGX(reply);
		CLOSETRACE();
		///////////////////////////////////////////////////////////////////////////
	}
	else
		reply = _xcb_get_extension_data(conn, ext);
	CATCH();

	return reply;
}


xcb_glx_query_version_cookie_t
	xcb_glx_query_version(xcb_connection_t *conn, uint32_t major_version,
		uint32_t minor_version)
{
	xcb_glx_query_version_cookie_t cookie = { 0 };

	TRY();

	// Note that we have to hand off to the underlying XCB libraries if
	// faker::deadYet==true, because MainWin calls X11 functions (which in turn
	// call XCB functions) from one of its shared library destructors, which is
	// executed after the VirtualGL Faker has shut down.
	if(!fconfig.fakeXCB || IS_EXCLUDED(xcbconnhash.getX11Display(conn)))
		return _xcb_glx_query_version(conn, major_version, minor_version);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_glx_query_version);  PRARGX(conn);  PRARGI(major_version);
	PRARGI(minor_version);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	xcb_connection_t *conn3D = (fconfig.egl ? conn : _XGetXCBConnection(DPY3D));
	if(conn3D != NULL)
		cookie = _xcb_glx_query_version(conn3D, major_version, minor_version);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return cookie;
}


xcb_glx_query_version_reply_t *
	xcb_glx_query_version_reply(xcb_connection_t *conn,
		xcb_glx_query_version_cookie_t cookie, xcb_generic_error_t **error)
{
	xcb_glx_query_version_reply_t *reply = NULL;

	TRY();

	if(!fconfig.fakeXCB || IS_EXCLUDED(xcbconnhash.getX11Display(conn)))
		return _xcb_glx_query_version_reply(conn, cookie, error);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(xcb_glx_query_version_reply);  PRARGX(conn);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	xcb_connection_t *conn3D = (fconfig.egl ? conn : _XGetXCBConnection(DPY3D));
	if(conn3D != NULL)
		reply = _xcb_glx_query_version_reply(conn3D, cookie, error);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(error)
	{
		if(*error) PRARGERR(*error)  else PRARGX(*error);
	}
	else PRARGX(error);
	if(reply)
	{
		PRARGI(reply->major_version);  PRARGI(reply->minor_version);
	}
	else PRARGX(reply);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();

	return reply;
}


// The following functions are interposed so that VirtualGL can detect window
// resizes, key presses (to pop up the VGL configuration dialog), and window
// delete events from the window manager.

static void handleXCBEvent(xcb_connection_t *conn, xcb_generic_event_t *ev)
{
	faker::VirtualWin *vw = NULL;
	#ifdef EGLBACKEND
	faker::EGLXVirtualWin *eglxvw;
	#endif

	if(!ev || faker::deadYet || !fconfig.fakeXCB || faker::getFakerLevel() > 0)
		return;

	switch(ev->response_type & ~0x80)
	{
		case XCB_CONFIGURE_NOTIFY:
		{
			xcb_configure_notify_event_t *cne = (xcb_configure_notify_event_t *)ev;
			Display *dpy = xcbconnhash.getX11Display(conn);

			if(!dpy || faker::isDisplayExcluded(dpy)) break;

			if((vw = winhash.find(dpy, cne->window)) != NULL)
			{
				///////////////////////////////////////////////////////////////////////
				OPENTRACE(handleXCBEvent);  PRARGX(cne->window);  PRARGI(cne->width);
				PRARGI(cne->height);  STARTTRACE();
				///////////////////////////////////////////////////////////////////////

				vw->resize(cne->width, cne->height);

				///////////////////////////////////////////////////////////////////////
				STOPTRACE();  CLOSETRACE();
				///////////////////////////////////////////////////////////////////////
			}
			#ifdef EGLBACKEND
			if((eglxvw = eglxwinhash.find(dpy, cne->window)) != NULL)
			{
				///////////////////////////////////////////////////////////////////////
				OPENTRACE(handleXCBEvent);  PRARGX(cne->window);  PRARGI(cne->width);
				PRARGI(cne->height);  STARTTRACE();
				///////////////////////////////////////////////////////////////////////

				eglxvw->resize(cne->width, cne->height);

				///////////////////////////////////////////////////////////////////////
				STOPTRACE();  CLOSETRACE();
				///////////////////////////////////////////////////////////////////////
			}
			#endif
			break;
		}
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t *kpe = (xcb_key_press_event_t *)ev;
			Display *dpy = xcbconnhash.getX11Display(conn);

			if(!dpy || !fconfig.gui || faker::isDisplayExcluded(dpy)) break;

			xcb_key_symbols_t *keysyms = _xcb_key_symbols_alloc(conn);
			if(!keysyms) break;

			xcb_keysym_t keysym =
				_xcb_key_symbols_get_keysym(keysyms, kpe->detail, 0);
			unsigned int state2;
			unsigned int state = (kpe->state & (~(XCB_MOD_MASK_LOCK))) & 0xFF;
			state2 = fconfig.guimod;
			if(state2 & Mod1Mask)
			{
				state2 &= (~(Mod1Mask));  state2 |= Mod2Mask;
			}
			if(keysym == fconfig.guikey
				&& (state == fconfig.guimod || state == state2)
				&& fconfig_getshmid() != -1)
				VGLPOPUP(dpy, fconfig_getshmid());

			_xcb_key_symbols_free(keysyms);

			break;
		}
		case XCB_CLIENT_MESSAGE:
		{
			xcb_client_message_event_t *cme = (xcb_client_message_event_t *)ev;
			xcb_atom_t protoAtom = 0, deleteAtom = 0;

			Display *dpy = xcbconnhash.getX11Display(conn);
			protoAtom = xcbconnhash.getProtoAtom(conn);
			deleteAtom = xcbconnhash.getDeleteAtom(conn);

			if(!dpy || !protoAtom || !deleteAtom
				|| cme->type != protoAtom || cme->data.data32[0] != deleteAtom
				|| faker::isDisplayExcluded(dpy))
				break;

			if((vw = winhash.find(dpy, cme->window)) != NULL)
				vw->wmDeleted();
			#ifdef EGLBACKEND
			if((eglxvw = eglxwinhash.find(dpy, cme->window)) != NULL)
				eglxvw->wmDeleted();
			#endif

			break;
		}
	}
}


xcb_generic_event_t *xcb_poll_for_event(xcb_connection_t *conn)
{
	xcb_generic_event_t *ev = NULL;

	TRY();

	if((ev = _xcb_poll_for_event(conn)) != NULL)
		handleXCBEvent(conn, ev);

	CATCH();

	return ev;
}


xcb_generic_event_t *xcb_poll_for_queued_event(xcb_connection_t *conn)
{
	xcb_generic_event_t *ev = NULL;

	TRY();

	if((ev = _xcb_poll_for_queued_event(conn)) != NULL)
		handleXCBEvent(conn, ev);

	CATCH();

	return ev;
}


xcb_generic_event_t *xcb_wait_for_event(xcb_connection_t *conn)
{
	xcb_generic_event_t *ev = NULL;

	TRY();

	if((ev = _xcb_wait_for_event(conn)) != NULL)
		handleXCBEvent(conn, ev);

	CATCH();

	return ev;
}


void XSetEventQueueOwner(Display *dpy, enum XEventQueueOwner owner)
{
	xcb_connection_t *conn = NULL;

	TRY();

	if(faker::deadYet || faker::isDisplayExcluded(dpy))
		return _XSetEventQueueOwner(dpy, owner);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(XSetEventQueueOwner);  PRARGD(dpy);  PRARGI(owner);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(fconfig.fakeXCB)
	{
		conn = _XGetXCBConnection(dpy);
		if(conn)
		{
			if(owner == XCBOwnsEventQueue) xcbconnhash.add(conn, dpy);
			else xcbconnhash.remove(conn);
		}
	}
	_XSetEventQueueOwner(dpy, owner);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(fconfig.fakeXCB) PRARGX(conn);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}

}  // extern "C"


#endif  // FAKEXCB
