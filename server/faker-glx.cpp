// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011-2022 D. R. Commander
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

#include <string.h>
#include <limits.h>
#include "Error.h"
#include "vglutil.h"
#define GLX_GLXEXT_PROTOTYPES
#include "ContextHash.h"
#include "GLXDrawableHash.h"
#include "PixmapHash.h"
#include "VisualHash.h"
#include "WindowHash.h"
#include "rr.h"
#include "faker.h"
#include "glxvisual.h"
#include <X11/Xmd.h>
#include <GL/glxproto.h>


// This emulates the behavior of the nVidia drivers
#define VGL_MAX_SWAP_INTERVAL  8


static INLINE VGLFBConfig matchConfig(Display *dpy, XVisualInfo *vis)
{
	VGLFBConfig config = 0;

	if(!dpy || !vis) return 0;

	// If the visual was obtained from glXChooseVisual() or
	// glXGetVisualFromFBConfig(), then it should have a corresponding FB config
	// in the visual hash.
	if(!(config = vishash.getConfig(dpy, vis)))
	{
		// Punt.  We can't figure out where the visual came from, so return the
		// default FB config from the visual attribute table.
		config = glxvisual::getDefaultFBConfig(dpy, vis->screen, vis->visualid);
		if(config) config->visualID = vis->visualid;
	}
	return config;
}


void setWMAtom(Display *dpy, Window win, faker::VirtualWin *vw)
{
	Atom *protocols = NULL, *newProtocols = NULL;  int count = 0;

	Atom deleteAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
	if(!deleteAtom) goto bailout;

	if(XGetWMProtocols(dpy, win, &protocols, &count) && protocols && count > 0)
	{
		for(int i = 0; i < count; i++)
			if(protocols[i] == deleteAtom)
			{
				_XFree(protocols);  return;
			}
		newProtocols = (Atom *)malloc(sizeof(Atom) * (count + 1));
		if(!newProtocols) goto bailout;
		for(int i = 0; i < count; i++)
			newProtocols[i] = protocols[i];
		newProtocols[count] = deleteAtom;
		if(!XSetWMProtocols(dpy, win, newProtocols, count + 1)) goto bailout;
		_XFree(protocols);
		free(newProtocols);
	}
	else if(!XSetWMProtocols(dpy, win, &deleteAtom, 1)) goto bailout;
	vw->enableWMDeleteHandler();
	return;

	bailout:
	if(protocols) _XFree(protocols);
	free(newProtocols);
	static bool alreadyWarned = false;
	if(!alreadyWarned)
	{
		if(fconfig.verbose)
			vglout.print("[VGL] WARNING: Could not set WM_DELETE_WINDOW on window 0x%.8x\n",
				win);
		alreadyWarned = true;
	}
}



// Interposed GLX functions

extern "C" {

// Return a set of FB configs from the 3D X server that support off-screen
// rendering and contain the desired GLX attributes.

GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
	const int *attrib_list, int *nelements)
{
	VGLFBConfig *configs = NULL;
	bool fbcidreq = false;
	int drawableType = GLX_WINDOW_BIT;

	TRY();

	// If this is called internally from within another GLX function, then use
	// the real function.
	if(IS_EXCLUDED(dpy))
		return _glXChooseFBConfig(dpy, screen, attrib_list, nelements);

	if(attrib_list)
	{
		for(int i = 0; attrib_list[i] != None && i < MAX_ATTRIBS; i += 2)
		{
			if(attrib_list[i] == GLX_FBCONFIG_ID) fbcidreq = true;
			if(attrib_list[i] == GLX_DRAWABLE_TYPE)
				drawableType = attrib_list[i + 1];
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXChooseFBConfig);  PRARGD(dpy);  PRARGI(screen);
	PRARGAL13(attrib_list);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	int temp;
	if(!nelements) nelements = &temp;
	*nelements = 0;

	// If no attributes are specified, return all FB configs.  If GLX_FBCONFIG_ID
	// is specified, ignore all other attributes.
	if(!attrib_list || fbcidreq)
	{
		configs = glxvisual::chooseFBConfig(dpy, screen, attrib_list, *nelements);
		goto done;
	}

	// Modify the attributes so that only FB configs appropriate for off-screen
	// rendering are considered.
	else configs = glxvisual::configsFromVisAttribs(dpy, screen, attrib_list,
		*nelements, true);

	if(configs && *nelements && drawableType & (GLX_WINDOW_BIT | GLX_PIXMAP_BIT))
	{
		int nv = 0;
		VGLFBConfig *newConfigs =
			(VGLFBConfig *)calloc(*nelements, sizeof(VGLFBConfig));
		if(!newConfigs)
		{
			_XFree(configs);  configs = NULL;
			THROW("Memory allocation error");
		}

		for(int i = 0; i < *nelements; i++)
		{
			if(!configs[i]->visualID) continue;
			newConfigs[nv++] = configs[i];
		}
		*nelements = nv;
		_XFree(configs);
		configs = newConfigs;
		if(!nv) { _XFree(configs);  configs = NULL; }
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(configs && nelements)
	{
		if(*nelements)
			for(int i = 0; i < *nelements; i++)
				vglout.print("configs[%d]=0x%.8lx(0x%.2x) ", i,
					(unsigned long)configs[i], FBCID(configs[i]));
		PRARGI(*nelements);
	}
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return (GLXFBConfig *)configs;
}

GLXFBConfigSGIX *glXChooseFBConfigSGIX(Display *dpy, int screen,
	int *attrib_list, int *nelements)
{
	return glXChooseFBConfig(dpy, screen, attrib_list, nelements);
}


// Obtain a 3D X server FB config that supports off-screen rendering and has
// the desired set of attributes, match it to an appropriate 2D X server
// visual, hash the two, and return the visual.

XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
	XVisualInfo *vis = NULL;  VGLFBConfig config = 0;
	static bool alreadyWarned = false;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXChooseVisual(dpy, screen, attrib_list);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXChooseVisual);  PRARGD(dpy);  PRARGI(screen);
	PRARGAL11(attrib_list);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	// Use the specified set of GLX attributes to obtain an FB config on the 3D X
	// server suitable for off-screen rendering
	VGLFBConfig *configs = NULL, prevConfig;  int n = 0;
	if(!dpy || !attrib_list) goto done;
	if(!(configs = glxvisual::configsFromVisAttribs(dpy, screen, attrib_list, n))
		|| n < 1)
	{
		if(!alreadyWarned && fconfig.verbose && !fconfig.egl)
		{
			alreadyWarned = true;
			vglout.println("[VGL] WARNING: VirtualGL attempted and failed to obtain a true color visual on");
			vglout.println("[VGL]    the 3D X server %s suitable for off-screen rendering.",
				fconfig.localdpystring);
			vglout.println("[VGL]    This is normal if the 3D application is probing for visuals with");
			vglout.println("[VGL]    certain capabilities, but if the application fails to start, then make");
			vglout.println("[VGL]    sure that the 3D X server is configured for true color and has");
			vglout.println("[VGL]    appropriate GPU drivers installed.");
		}
		goto done;
	}
	config = configs[0];
	for(int i = 0; i < n; i++)
	{
		if(configs[i]->visualID)
		{
			config = configs[i];
			break;
		}
	}
	_XFree(configs);

	// Find an appropriate matching visual on the 2D X server.
	if(!config->visualID) goto done;
	vis = glxvisual::visualFromID(dpy, screen, config->visualID);
	if(!vis) goto done;

	if((prevConfig = vishash.getConfig(dpy, vis))
		&& FBCID(config) != FBCID(prevConfig) && fconfig.trace)
		vglout.println("[VGL] WARNING: Visual 0x%.2x was previously mapped to FB config 0x%.2x and is now mapped to 0x%.2x\n",
			vis->visualid, FBCID(prevConfig), FBCID(config));

	// Hash the FB config and the visual so that we can look up the FB config
	// whenever the appplication subsequently passes the visual to
	// glXCreateContext() or other functions.
	vishash.add(dpy, vis, config);

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGV(vis);  PRARGC(config);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return vis;
}


// Hand off to the 3D X server without modification.

void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst,
	unsigned long mask)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCopyContext(dpy, src, dst, mask);

	if(fconfig.egl)
	{
		vglout.println("[VGL] ERROR: glXCopyContext() requires the GLX back end");
		faker::sendGLXError(dpy, X_GLXCopyContext, BadRequest, true);
		return;
	}
	_glXCopyContext(DPY3D, src, dst, mask);

	CATCH();
}


// Create a GLX context on the 3D X server suitable for off-screen rendering.

GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
	GLXContext share_list, Bool direct)
{
	GLXContext ctx = 0;  VGLFBConfig config = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateContext(dpy, vis, share_list, direct);

	if(!fconfig.allowindirect) direct = True;

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXCreateContext);  PRARGD(dpy);  PRARGV(vis);  PRARGX(share_list);
	PRARGI(direct);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(!(config = matchConfig(dpy, vis)))
	{
		faker::sendGLXError(dpy, X_GLXCreateContext, BadValue, true);
		goto done;
	}
	ctx = backend::createContext(dpy, config, share_list, direct, NULL);
	if(ctx)
	{
		int newctxIsDirect = backend::isDirect(ctx);
		if(!fconfig.egl && !newctxIsDirect && direct)
		{
			vglout.println("[VGL] WARNING: The OpenGL rendering context obtained on X display");
			vglout.println("[VGL]    %s is indirect, which may cause performance to suffer.",
				DisplayString(DPY3D));
			vglout.println("[VGL]    If %s is a local X display, then the framebuffer device",
				DisplayString(DPY3D));
			vglout.println("[VGL]    permissions may be set incorrectly.");
		}
		// Hash the FB config to the context so we can use it in subsequent calls
		// to glXMake[Context]Current().
		ctxhash.add(ctx, config, newctxIsDirect);
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGC(config);  PRARGX(ctx);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return ctx;
}


GLXContext glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config_,
	GLXContext share_context, Bool direct, const int *attribs)
{
	GLXContext ctx = 0;
	VGLFBConfig config = (VGLFBConfig)config_;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateContextAttribsARB(dpy, config_, share_context, direct,
			attribs);

	if(!fconfig.allowindirect) direct = True;

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXCreateContextAttribsARB);  PRARGD(dpy);  PRARGC(config);
	PRARGX(share_context);  PRARGI(direct);  PRARGAL13(attribs);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	ctx = backend::createContext(dpy, config, share_context, direct, attribs);
	if(ctx)
	{
		int newctxIsDirect = backend::isDirect(ctx);
		if(!fconfig.egl && !newctxIsDirect && direct)
		{
			vglout.println("[VGL] WARNING: The OpenGL rendering context obtained on X display");
			vglout.println("[VGL]    %s is indirect, which may cause performance to suffer.",
				DisplayString(DPY3D));
			vglout.println("[VGL]    If %s is a local X display, then the framebuffer device",
				DisplayString(DPY3D));
			vglout.println("[VGL]    permissions may be set incorrectly.");
		}
		ctxhash.add(ctx, config, newctxIsDirect);
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(ctx);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return ctx;
}


GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config_,
	int render_type, GLXContext share_list, Bool direct)
{
	GLXContext ctx = 0;
	VGLFBConfig config = (VGLFBConfig)config_;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateNewContext(dpy, config_, render_type, share_list, direct);

	if(!fconfig.allowindirect) direct = True;

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXCreateNewContext);  PRARGD(dpy);  PRARGC(config);
	PRARGI(render_type);  PRARGX(share_list);  PRARGI(direct);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	ctx = backend::createContext(dpy, config, share_list, direct, NULL);
	if(ctx)
	{
		int newctxIsDirect = backend::isDirect(ctx);
		if(!fconfig.egl && !newctxIsDirect && direct)
		{
			vglout.println("[VGL] WARNING: The OpenGL rendering context obtained on X display");
			vglout.println("[VGL]    %s is indirect, which may cause performance to suffer.",
				DisplayString(DPY3D));
			vglout.println("[VGL]    If %s is a local X display, then the framebuffer device",
				DisplayString(DPY3D));
			vglout.println("[VGL]    permissions may be set incorrectly.");
		}
		ctxhash.add(ctx, config, newctxIsDirect);
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(ctx);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return ctx;
}

// On Linux, GLXFBConfigSGIX is typedef'd to GLXFBConfig

GLXContext glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config,
	int render_type, GLXContext share_list, Bool direct)
{
	return glXCreateNewContext(dpy, config, render_type, share_list, direct);
}


// We maintain a hash of GLX drawables to 2D X display handles for two reasons:
// (1) so we can determine in glXMake[Context]Current() whether or not a
// drawable ID is a window, a GLX drawable, or a window created in another
// application, and (2) so, if the application is rendering to a Pbuffer or
// a pixmap, we can return the correct 2D X display handle if it calls
// glXGetCurrentDisplay().

GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config_,
	const int *attrib_list)
{
	GLXPbuffer pb = 0;
	VGLFBConfig config = (VGLFBConfig)config_;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreatePbuffer(dpy, config_, attrib_list);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXCreatePbuffer);  PRARGD(dpy);  PRARGC(config);
	PRARGAL13(attrib_list);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	pb = backend::createPbuffer(dpy, config, attrib_list);
	if(dpy && pb) glxdhash.add(pb, dpy);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(pb);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return pb;
}

GLXPbuffer glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config,
	unsigned int width, unsigned int height, int *attrib_list)
{
	int attribs[MAX_ATTRIBS + 1], j = 0;
	if(attrib_list)
	{
		for(int i = 0; attrib_list[i] != None && i < MAX_ATTRIBS - 2; i += 2)
		{
			attribs[j++] = attrib_list[i];  attribs[j++] = attrib_list[i + 1];
		}
	}
	attribs[j++] = GLX_PBUFFER_WIDTH;  attribs[j++] = width;
	attribs[j++] = GLX_PBUFFER_HEIGHT;  attribs[j++] = height;
	attribs[j] = None;
	return glXCreatePbuffer(dpy, config, attribs);
}


// Pixmap rendering in VirtualGL is implemented by redirecting rendering into
// a "3D pixmap" stored on the 3D X server.  Thus, we create a 3D pixmap, hash
// it to the "2D pixmap", and return the 3D pixmap handle.

GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pm)
{
	GLXPixmap drawable = 0;  VGLFBConfig config = 0;
	int x = 0, y = 0;  unsigned int width = 0, height = 0, depth = 0;
	faker::VirtualPixmap *vpm = NULL;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateGLXPixmap(dpy, vis, pm);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXCreateGLXPixmap);  PRARGD(dpy);  PRARGV(vis);  PRARGX(pm);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	Window root;  unsigned int bw;
	if(!(config = matchConfig(dpy, vis)))
	{
		faker::sendGLXError(dpy, X_GLXCreateGLXPixmap, BadValue, true);
		goto done;
	}
	if(!pm
		|| !_XGetGeometry(dpy, pm, &root, &x, &y, &width, &height, &bw, &depth))
	{
		faker::sendGLXError(dpy, X_GLXCreateGLXPixmap, BadPixmap, true);
		goto done;
	}
	if((vpm = new faker::VirtualPixmap(dpy, vis->visual, pm)) != NULL)
	{
		// Hash the VirtualPixmap instance to the 2D pixmap and also hash the 2D X
		// display handle to the 3D pixmap.
		vpm->init(width, height, depth, config, NULL);
		pmhash.add(dpy, pm, vpm);
		glxdhash.add(vpm->getGLXDrawable(), dpy);
		drawable = vpm->getGLXDrawable();
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(x);  PRARGI(y);  PRARGI(width);  PRARGI(height);
	PRARGI(depth);  PRARGC(config);  PRARGX(drawable);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return drawable;
}


GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config_, Pixmap pm,
	const int *attribs)
{
	GLXPixmap drawable = 0;
	VGLFBConfig config = (VGLFBConfig)config_;
	faker::VirtualPixmap *vpm = NULL;
	XVisualInfo *vis = NULL;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreatePixmap(dpy, config_, pm, attribs);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXCreatePixmap);  PRARGD(dpy);  PRARGC(config);  PRARGX(pm);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(!VALID_CONFIG(config))
	{
		faker::sendGLXError(dpy, X_GLXCreatePixmap, GLXBadFBConfig, false);
		goto done;
	}
	Window root;  int x, y;  unsigned int w, h, bw, d;
	if(!pm
		|| !_XGetGeometry(dpy, pm, &root, &x, &y, &w, &h, &bw, &d))
	{
		faker::sendGLXError(dpy, X_GLXCreatePixmap, BadPixmap, true);
		goto done;
	}

	if(!config->visualID)
	{
		faker::sendGLXError(dpy, X_GLXCreatePixmap, BadMatch, true);
		goto done;
	}
	if((vis = glxvisual::visualFromID(dpy, config->screen,
		config->visualID)) != NULL)
	{
		vpm = new faker::VirtualPixmap(dpy, vis->visual, pm);
		_XFree(vis);
	}
	if(vpm)
	{
		vpm->init(w, h, d, config, attribs);
		pmhash.add(dpy, pm, vpm);
		glxdhash.add(vpm->getGLXDrawable(), dpy);
		drawable = vpm->getGLXDrawable();
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(x);  PRARGI(y);  PRARGI(w);  PRARGI(h);  PRARGI(d);
	PRARGX(drawable);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return drawable;
}

GLXPixmap glXCreateGLXPixmapWithConfigSGIX(Display *dpy,
	GLXFBConfigSGIX config, Pixmap pixmap)
{
	return glXCreatePixmap(dpy, config, pixmap, NULL);
}


// Fake out the application into thinking it's getting a window drawable, but
// really it's getting an off-screen drawable.

GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config_, Window win,
	const int *attrib_list)
{
	faker::VirtualWin *vw = NULL;
	VGLFBConfig config = (VGLFBConfig)config_;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateWindow(dpy, config_, win, attrib_list);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXCreateWindow);  PRARGD(dpy);  PRARGC(config);  PRARGX(win);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	XSync(dpy, False);
	if(!VALID_CONFIG(config))
	{
		faker::sendGLXError(dpy, X_GLXCreateWindow, GLXBadFBConfig, false);
		win = 0;
		goto done;
	}
	if(!win)
	{
		faker::sendGLXError(dpy, X_GLXCreateWindow, BadWindow, true);
		goto done;
	}
	try
	{
		vw = winhash.initVW(dpy, win, config);
		if(!vw && !glxdhash.getCurrentDisplay(win))
		{
			// Apparently win was created in another process or using XCB.
			winhash.add(dpy, win);
			vw = winhash.initVW(dpy, win, config);
		}
	}
	catch(std::exception &e)
	{
		if(!strcmp(GET_METHOD(e), "VirtualWin")
			&& !strcmp(e.what(), "Invalid window"))
		{
			faker::sendGLXError(dpy, X_GLXCreateWindow, BadWindow, true);
			win = 0;
			goto done;
		}
		throw;
	}
	if(!vw)
		THROW("Cannot create virtual window for specified X window");

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(vw) { PRARGX(vw->getGLXDrawable()); }
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return win;  // Make the client store the original window handle, which we
	             // use to find the off-screen drawable in the hash
}


// When the context is destroyed, remove it from the context-to-FB config hash.

void glXDestroyContext(Display *dpy, GLXContext ctx)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXDestroyContext(dpy, ctx);  return;
	}

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXDestroyContext);  PRARGD(dpy);  PRARGX(ctx);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	ctxhash.remove(ctx);
	backend::destroyContext(dpy, ctx);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


// When the Pbuffer is destroyed, remove it from the GLX drawable-to-2D display
// handle hash.

void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
	TRY();

	if(IS_EXCLUDED(dpy)) { _glXDestroyPbuffer(dpy, pbuf);  return; }

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXDestroyPbuffer);  PRARGD(dpy);  PRARGX(pbuf);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	backend::destroyPbuffer(dpy, pbuf);
	if(pbuf) glxdhash.remove(pbuf);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}

void glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf)
{
	glXDestroyPbuffer(dpy, pbuf);
}


// Some applications will destroy the GLX pixmap handle but then try to use X11
// functions on the X11 pixmap handle, so we sync the contents of the 3D pixmap
// with the 2D pixmap before we delete the 3D pixmap.

void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXDestroyGLXPixmap(dpy, pix);  return;
	}

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXDestroyGLXPixmap);  PRARGD(dpy);  PRARGX(pix);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	faker::VirtualPixmap *vpm = pmhash.find(dpy, pix);
	if(vpm && vpm->isInit()) vpm->readback();

	if(pix) glxdhash.remove(pix);
	if(dpy && pix) pmhash.remove(dpy, pix);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
}


void glXDestroyPixmap(Display *dpy, GLXPixmap pix)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXDestroyPixmap(dpy, pix);  return;
	}

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXDestroyPixmap);  PRARGD(dpy);  PRARGX(pix);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	faker::VirtualPixmap *vpm = pmhash.find(dpy, pix);
	if(vpm && vpm->isInit()) vpm->readback();

	if(pix) glxdhash.remove(pix);
	if(dpy && pix) pmhash.remove(dpy, pix);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
}


// 'win' is really an off-screen drawable ID, so the window hash matches it to
// the corresponding VirtualWin instance and shuts down that instance.

void glXDestroyWindow(Display *dpy, GLXWindow win)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXDestroyWindow(dpy, win);  return;
	}

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXDestroyWindow);  PRARGD(dpy);  PRARGX(win);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	winhash.remove(dpy, win);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
}


// Hand off to the 3D X server without modification.

void glXFreeContextEXT(Display *dpy, GLXContext ctx)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXFreeContextEXT(dpy, ctx);  return;
	}
	if(fconfig.egl) THROW("glXFreeContextEXT() requires the GLX back end");
	_glXFreeContextEXT(DPY3D, ctx);

	CATCH();
}


// Since VirtualGL is effectively its own implementation of GLX, it needs to
// properly report the extensions and GLX version it supports.

#define VGL_GLX_EXTENSIONS \
	"GLX_ARB_get_proc_address GLX_ARB_multisample GLX_EXT_swap_control GLX_EXT_visual_info GLX_EXT_visual_rating GLX_SGI_make_current_read GLX_SGI_swap_control GLX_SGIX_fbconfig GLX_SGIX_pbuffer"
// Allow enough space here for all of the extensions
static char glxextensions[1024] = VGL_GLX_EXTENSIONS;

static const char *getGLXExtensions(void)
{
	const char *realGLXExtensions = fconfig.egl ? "" :
		_glXQueryExtensionsString(DPY3D, DefaultScreen(DPY3D));

	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		faker::init3D();
		if((faker::eglMajor > 1 || (faker::eglMajor == 1 && faker::eglMinor >= 5))
			&& !strstr(glxextensions, "GLX_ARB_create_context"))
			strncat(glxextensions,
				" GLX_ARB_create_context GLX_ARB_create_context_profile GLX_EXT_framebuffer_sRGB",
				1023 - strlen(glxextensions));
		return glxextensions;
	}
	else
	#endif
	{
		CHECKSYM_NONFATAL(glXCreateContextAttribsARB)
		if(__glXCreateContextAttribsARB
			&& !strstr(glxextensions, "GLX_ARB_create_context"))
			strncat(glxextensions,
				" GLX_ARB_create_context GLX_ARB_create_context_profile",
				1023 - strlen(glxextensions));
	}

	if(strstr(realGLXExtensions, "GLX_ARB_create_context_robustness")
		&& !strstr(glxextensions, "GLX_ARB_create_context_robustness"))
		strncat(glxextensions, " GLX_ARB_create_context_robustness",
			1023 - strlen(glxextensions));

	if(strstr(realGLXExtensions, "GLX_ARB_fbconfig_float")
		&& !strstr(glxextensions, "GLX_ARB_fbconfig_float"))
		strncat(glxextensions, " GLX_ARB_fbconfig_float",
			1023 - strlen(glxextensions));

	if(strstr(realGLXExtensions, "GLX_EXT_create_context_es2_profile")
		&& !strstr(glxextensions, "GLX_EXT_create_context_es2_profile"))
		strncat(glxextensions, " GLX_EXT_create_context_es2_profile",
			1023 - strlen(glxextensions));

	if(strstr(realGLXExtensions, "GLX_EXT_fbconfig_packed_float")
		&& !strstr(glxextensions, "GLX_EXT_fbconfig_packed_float"))
		strncat(glxextensions, " GLX_EXT_fbconfig_packed_float",
			1023 - strlen(glxextensions));

	if(strstr(realGLXExtensions, "GLX_EXT_framebuffer_sRGB")
		&& !strstr(glxextensions, "GLX_EXT_framebuffer_sRGB"))
		strncat(glxextensions, " GLX_EXT_framebuffer_sRGB",
			1023 - strlen(glxextensions));

	CHECKSYM_NONFATAL(glXFreeContextEXT)
	CHECKSYM_NONFATAL(glXImportContextEXT)
	CHECKSYM_NONFATAL(glXQueryContextInfoEXT)
	if(__glXFreeContextEXT && __glXImportContextEXT && __glXQueryContextInfoEXT
		&& !strstr(glxextensions, "GLX_EXT_import_context"))
		strncat(glxextensions, " GLX_EXT_import_context",
			1023 - strlen(glxextensions));

	CHECKSYM_NONFATAL(glXBindTexImageEXT)
	CHECKSYM_NONFATAL(glXReleaseTexImageEXT)
	if(__glXBindTexImageEXT && __glXReleaseTexImageEXT
		&& !strstr(glxextensions, "GLX_EXT_texture_from_pixmap"))
		strncat(glxextensions, " GLX_EXT_texture_from_pixmap",
			1023 - strlen(glxextensions));

	if(strstr(realGLXExtensions, "GLX_NV_float_buffer")
		&& !strstr(glxextensions, "GLX_NV_float_buffer"))
		strncat(glxextensions, " GLX_NV_float_buffer",
			1023 - strlen(glxextensions));

	return glxextensions;
}


const char *glXGetClientString(Display *dpy, int name)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetClientString(dpy, name);

	if(name == GLX_EXTENSIONS) return getGLXExtensions();
	else if(name == GLX_VERSION) return "1.4";
	else if(name == GLX_VENDOR)
	{
		if(strlen(fconfig.glxvendor) > 0) return fconfig.glxvendor;
		else return __APPNAME;
	}

	CATCH();
	return NULL;
}


// For the specified 2D X server visual, return a property from the
// corresponding 3D X server FB config.

int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
	VGLFBConfig config;  int retval = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetConfig(dpy, vis, attrib, value);

	if(!dpy) return GLX_NO_EXTENSION;
	if(!vis) return GLX_BAD_VISUAL;
	if(!value) return GLX_BAD_VALUE;

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetConfig);  PRARGD(dpy);  PRARGV(vis);  PRARGIX(attrib);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if((config = matchConfig(dpy, vis)) != 0)
	{
		if(attrib == GLX_USE_GL)
		{
			if(vis->c_class == TrueColor || vis->c_class == DirectColor) *value = 1;
			else *value = 0;
		}
		else if(attrib == GLX_RGBA)
		{
			if((glxvisual::getFBConfigAttrib(dpy, config,
				GLX_RENDER_TYPE) & GLX_RGBA_BIT) != 0)
				*value = 1;
			else *value = 0;
		}
		else retval = backend::getFBConfigAttrib(dpy, config, attrib, value);
	}
	else
	{
		*value = 0;
		if(attrib != GLX_USE_GL) retval = GLX_BAD_VISUAL;
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(value) { PRARGIX(*value); }  else { PRARGX(value); }
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return retval;
}


GLXContext glXGetCurrentContext(void)
{
	GLXContext ctx = 0;

	if(faker::getGLXExcludeCurrent()) return _glXGetCurrentContext();

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetCurrentContext);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	ctx = backend::getCurrentContext();

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(ctx);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return ctx;
}


// This returns the 2D X display handle associated with the current drawable,
// that is, the 2D X display handle passed to whatever function (such as
// XCreateWindow(), glXCreatePbuffer(), etc.) that was used to create the
// drawable.

Display *glXGetCurrentDisplay(void)
{
	Display *dpy = NULL;  faker::VirtualWin *vw;

	if(faker::getGLXExcludeCurrent()) return _glXGetCurrentDisplay();

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetCurrentDisplay);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	GLXDrawable curdraw = backend::getCurrentDrawable();
	if((vw = winhash.find(NULL, curdraw)) != NULL)
		dpy = vw->getX11Display();
	else if(curdraw)
		dpy = glxdhash.getCurrentDisplay(curdraw);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGD(dpy);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return dpy;
}

Display *glXGetCurrentDisplayEXT(void)
{
	return glXGetCurrentDisplay();
}


// As far as the application is concerned, it is rendering to a window and not
// an off-screen drawable, so we must maintain that illusion and pass it back
// the window ID instead of the GLX drawable ID.

GLXDrawable glXGetCurrentDrawable(void)
{
	faker::VirtualWin *vw;  GLXDrawable draw = 0;

	if(faker::getGLXExcludeCurrent()) return _glXGetCurrentDrawable();

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetCurrentDrawable);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	draw = backend::getCurrentDrawable();
	if((vw = winhash.find(NULL, draw)) != NULL)
		draw = vw->getX11Drawable();

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(draw);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return draw;
}

GLXDrawable glXGetCurrentReadDrawable(void)
{
	faker::VirtualWin *vw;  GLXDrawable read = 0;

	if(faker::getGLXExcludeCurrent()) return _glXGetCurrentReadDrawable();

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetCurrentReadDrawable);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	read = backend::getCurrentReadDrawable();
	if((vw = winhash.find(NULL, read)) != NULL)
		read = vw->getX11Drawable();

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(read);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return read;
}

GLXDrawable glXGetCurrentReadDrawableSGI(void)
{
	return glXGetCurrentReadDrawable();
}


// Return a property from the specified FB config.

int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config_, int attribute,
	int *value)
{
	int retval = 0;
	VGLFBConfig config = (VGLFBConfig)config_;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetFBConfigAttrib(dpy, config_, attribute, value);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetFBConfigAttrib);  PRARGD(dpy);  PRARGC(config);
	PRARGIX(attribute);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(!dpy)
	{
		retval = GLX_NO_EXTENSION;  goto done;
	}
	if(!VALID_CONFIG(config))
	{
		retval = GLX_BAD_VISUAL;  goto done;
	}
	if(!value)
	{
		retval = GLX_BAD_VALUE;  goto done;
	}

	// If there is a corresponding 2D X server visual attached to this FB config,
	// then that means it was obtained via glXChooseFBConfig(), and we can
	// return attributes that take into account the interaction between visuals
	// on the 2D X Server and FB Configs on the 3D X Server.
	if(attribute == GLX_VISUAL_ID)
		*value = config->visualID;
	else
		retval = backend::getFBConfigAttrib(dpy, config, attribute, value);

	if(attribute == GLX_DRAWABLE_TYPE && retval == Success)
	{
		int temp = *value;
		*value = 0;
		if(fconfig.egl
			|| (glxvisual::getFBConfigAttrib(dpy, config, GLX_VISUAL_ID) != 0
				&& config->visualID))
		{
			if(temp & GLX_PBUFFER_BIT)
				*value |= GLX_WINDOW_BIT;
			if((fconfig.egl && (temp & GLX_PBUFFER_BIT))
				|| (!fconfig.egl
					&& (temp & GLX_PIXMAP_BIT) && (temp & GLX_WINDOW_BIT)))
				*value |= GLX_PIXMAP_BIT;
		}
		if(temp & GLX_PBUFFER_BIT) *value |= GLX_PBUFFER_BIT;
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(value) { PRARGIX(*value); }  else { PRARGX(value); }
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return retval;
}

int glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config,
	int attribute, int *value_return)
{
	return glXGetFBConfigAttrib(dpy, config, attribute, value_return);
}


// See notes for matchConfig()

GLXFBConfigSGIX glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetFBConfigFromVisualSGIX(dpy, vis);
	else return (GLXFBConfig)matchConfig(dpy, vis);

	CATCH();
	return 0;
}


// Hand off to the 3D X server without modification.

GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
	GLXFBConfig *configs = NULL;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetFBConfigs(dpy, screen, nelements);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetFBConfigs);  PRARGD(dpy);  PRARGI(screen);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	configs = (GLXFBConfig *)glxvisual::getFBConfigs(dpy, screen, *nelements);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(configs && nelements) PRARGI(*nelements);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return configs;
}


// GLX_EXT_texture_from_pixmap support.  drawable is a GLX pixmap that was
// previously created on the 3D X server, so passing it to glXBindTexImageEXT()
// or glXReleaseTexImageEXT() should "just work."  However, we first have to
// copy the pixels from the corresponding 2D pixmap, which is on the 2D X
// server.  Thus, this extension will probably not perform well except with an
// X proxy.

void glXBindTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer,
	const int *attrib_list)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXBindTexImageEXT(dpy, drawable, buffer, attrib_list);

	// TODO: Is there a way to implement this with EGL?
	if(fconfig.egl) THROW("glXBindTexImageEXT() requires the GLX back end");

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXBindTexImageEXT);  PRARGD(dpy);  PRARGX(drawable);
	PRARGI(buffer);  PRARGAL13(attrib_list);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::VirtualPixmap *vpm = NULL;
	if((vpm = pmhash.find(dpy, drawable)) == NULL)
	{
		// If we get here, then the drawable wasn't created with
		// glXCreate[GLX]Pixmap().
		faker::sendGLXError(dpy, X_GLXVendorPrivate, GLXBadPixmap, false);
		goto done;
	}
	else
	{
		// Transfer pixels from the 2D Pixmap (stored on the 2D X server) to the
		// 3D Pixmap (stored on the 3D X server.)
		XImage *image = _XGetImage(dpy, vpm->getX11Drawable(), 0, 0,
			vpm->getWidth(), vpm->getHeight(), AllPlanes, ZPixmap);
		GC gc = XCreateGC(DPY3D, vpm->get3DX11Pixmap(), 0, NULL);
		if(gc && image)
			XPutImage(DPY3D, vpm->get3DX11Pixmap(), gc, image, 0, 0, 0, 0,
				vpm->getWidth(), vpm->getHeight());
		else
		{
			faker::sendGLXError(dpy, X_GLXVendorPrivate, GLXBadPixmap, false);
			goto done;
		}
		if(gc) XFreeGC(DPY3D, gc);
		if(image) XDestroyImage(image);
	}

	_glXBindTexImageEXT(DPY3D, drawable, buffer, attrib_list);

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


void glXReleaseTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXReleaseTexImageEXT(dpy, drawable, buffer);

	// TODO: Is there a way to implement this with EGL?
	if(fconfig.egl) THROW("glXReleaseTexImageEXT() requires the GLX back end");

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXReleaseTexImageEXT);  PRARGD(dpy);  PRARGX(drawable);
	PRARGI(buffer);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	_glXReleaseTexImageEXT(DPY3D, drawable, buffer);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


// If an application uses glXGetProcAddressARB() to obtain the address of a
// function that we're interposing, we need to return the address of the
// interposed function.

#define CHECK_FAKED(f) \
	if(!strcmp((char *)procName, #f)) \
	{ \
		retval = (void (*)(void))f; \
		if(fconfig.trace) vglout.print("[INTERPOSED]"); \
	}

// For optional libGL symbols, check that the underlying function
// actually exists in libGL before returning the interposed version of it.

#define CHECK_OPT_FAKED(f) \
	if(!strcmp((char *)procName, #f)) \
	{ \
		CHECKSYM_NONFATAL(f) \
		if(__##f) \
		{ \
			retval = (void (*)(void))f; \
			if(fconfig.trace) vglout.print("[INTERPOSED]"); \
		} \
	}

void (*glXGetProcAddressARB(const GLubyte *procName))(void)
{
	void (*retval)(void) = NULL;

	faker::init();

	if(faker::getGLXExcludeCurrent()) return _glXGetProcAddressARB(procName);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetProcAddressARB);  PRARGS((char *)procName);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(procName)
	{
		// GLX 1.0
		CHECK_FAKED(glXChooseVisual)
		CHECK_FAKED(glXCopyContext)
		CHECK_FAKED(glXCreateContext)
		CHECK_FAKED(glXCreateGLXPixmap)
		CHECK_FAKED(glXDestroyContext)
		CHECK_FAKED(glXDestroyGLXPixmap)
		CHECK_FAKED(glXGetConfig)
		CHECK_FAKED(glXGetCurrentContext)
		CHECK_FAKED(glXGetCurrentDrawable)
		CHECK_FAKED(glXIsDirect)
		CHECK_FAKED(glXMakeCurrent)
		CHECK_FAKED(glXQueryExtension)
		CHECK_FAKED(glXQueryVersion)
		CHECK_FAKED(glXSwapBuffers)
		CHECK_FAKED(glXUseXFont)
		CHECK_FAKED(glXWaitGL)

		// GLX 1.1
		CHECK_FAKED(glXGetClientString)
		CHECK_FAKED(glXQueryServerString)
		CHECK_FAKED(glXQueryExtensionsString)

		// GLX 1.2
		CHECK_FAKED(glXGetCurrentDisplay)

		// GLX 1.3
		CHECK_FAKED(glXChooseFBConfig)
		CHECK_FAKED(glXCreateNewContext)
		CHECK_FAKED(glXCreatePbuffer)
		CHECK_FAKED(glXCreatePixmap)
		CHECK_FAKED(glXCreateWindow)
		CHECK_FAKED(glXDestroyPbuffer)
		CHECK_FAKED(glXDestroyPixmap)
		CHECK_FAKED(glXDestroyWindow)
		CHECK_FAKED(glXGetCurrentReadDrawable)
		CHECK_FAKED(glXGetFBConfigAttrib)
		CHECK_FAKED(glXGetFBConfigs)
		CHECK_FAKED(glXGetSelectedEvent)
		CHECK_FAKED(glXGetVisualFromFBConfig)
		CHECK_FAKED(glXMakeContextCurrent)
		CHECK_FAKED(glXQueryContext)
		CHECK_FAKED(glXQueryDrawable)
		CHECK_FAKED(glXSelectEvent)

		// GLX 1.4
		CHECK_FAKED(glXGetProcAddress)

		// GLX_ARB_create_context
		CHECK_OPT_FAKED(glXCreateContextAttribsARB)

		// GLX_ARB_get_proc_address
		CHECK_FAKED(glXGetProcAddressARB)

		// GLX_EXT_import_context
		CHECK_OPT_FAKED(glXFreeContextEXT)
		CHECK_FAKED(glXGetCurrentDisplayEXT)
		CHECK_OPT_FAKED(glXImportContextEXT)
		CHECK_OPT_FAKED(glXQueryContextInfoEXT)

		// GLX_EXT_swap_control
		CHECK_FAKED(glXSwapIntervalEXT)

		// GLX_EXT_texture_from_pixmap
		CHECK_OPT_FAKED(glXBindTexImageEXT)
		CHECK_OPT_FAKED(glXReleaseTexImageEXT)

		// GLX_SGI_make_current_read
		CHECK_FAKED(glXGetCurrentReadDrawableSGI)
		CHECK_FAKED(glXMakeCurrentReadSGI)

		// GLX_SGI_swap_control
		CHECK_FAKED(glXSwapIntervalSGI)

		// GLX_SGIX_fbconfig
		CHECK_FAKED(glXChooseFBConfigSGIX)
		CHECK_FAKED(glXCreateContextWithConfigSGIX)
		CHECK_FAKED(glXCreateGLXPixmapWithConfigSGIX)
		CHECK_FAKED(glXGetFBConfigAttribSGIX)
		CHECK_FAKED(glXGetFBConfigFromVisualSGIX)
		CHECK_FAKED(glXGetVisualFromFBConfigSGIX)

		// GLX_SGIX_pbuffer
		CHECK_FAKED(glXCreateGLXPbufferSGIX)
		CHECK_FAKED(glXDestroyGLXPbufferSGIX)
		CHECK_FAKED(glXGetSelectedEventSGIX)
		CHECK_FAKED(glXQueryGLXPbufferSGIX)
		CHECK_FAKED(glXSelectEventSGIX)

		// OpenGL
		CHECK_FAKED(glBindFramebuffer)
		CHECK_FAKED(glBindFramebufferEXT)
		CHECK_FAKED(glDeleteFramebuffers)
		CHECK_FAKED(glDeleteFramebuffersEXT)
		CHECK_FAKED(glFinish)
		CHECK_FAKED(glFlush)
		CHECK_FAKED(glDrawBuffer)
		CHECK_FAKED(glDrawBuffers)
		CHECK_FAKED(glDrawBuffersARB)
		CHECK_FAKED(glDrawBuffersATI)
		CHECK_FAKED(glFramebufferDrawBufferEXT)
		CHECK_FAKED(glFramebufferDrawBuffersEXT)
		CHECK_FAKED(glFramebufferReadBufferEXT)
		CHECK_FAKED(glGetBooleanv)
		CHECK_FAKED(glGetDoublev)
		CHECK_FAKED(glGetFloatv)
		CHECK_FAKED(glGetFramebufferAttachmentParameteriv)
		CHECK_FAKED(glGetFramebufferParameteriv)
		CHECK_FAKED(glGetIntegerv)
		CHECK_FAKED(glGetInteger64v)
		CHECK_FAKED(glGetNamedFramebufferParameteriv)
		CHECK_FAKED(glGetString)
		CHECK_FAKED(glGetStringi)
		CHECK_FAKED(glNamedFramebufferDrawBuffer)
		CHECK_FAKED(glNamedFramebufferDrawBuffers)
		CHECK_FAKED(glNamedFramebufferReadBuffer)
		CHECK_FAKED(glPopAttrib)
		CHECK_FAKED(glReadBuffer)
		CHECK_FAKED(glReadPixels)
		CHECK_FAKED(glViewport)
	}
	if(!retval)
	{
		// GL_EXT_x11_sync_object does not currently work with VirtualGL.
		if(!strcmp((char *)procName, "glImportSyncEXT"))
		{
			if(fconfig.trace) vglout.print("[NOT IMPLEMENTED]");
			retval = NULL;
		}
		else
		{
			if(fconfig.trace) vglout.print("[passed through]");
			retval = _glXGetProcAddress(procName);
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	return retval;
}

void (*glXGetProcAddress(const GLubyte *procName))(void)
{
	return glXGetProcAddressARB(procName);
}


// Hand off to the 3D X server without modification, except that 'draw' is
// replaced with its corresponding off-screen drawable ID.  See notes for
// glXSelectEvent().

void glXGetSelectedEvent(Display *dpy, GLXDrawable draw,
	unsigned long *event_mask)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetSelectedEvent(dpy, draw, event_mask);

	if(!event_mask) return;

	if(!draw)
	{
		faker::sendGLXError(dpy, X_GLXGetDrawableAttributes, GLXBadDrawable,
			false);
		return;
	}

	faker::VirtualWin *vw;
	if((vw = winhash.find(dpy, draw)) != NULL)
		*event_mask = vw->getEventMask();
	else if(glxdhash.getCurrentDisplay(draw))
		*event_mask = glxdhash.getEventMask(draw);
	else
		faker::sendGLXError(dpy, X_GLXGetDrawableAttributes, GLXBadDrawable,
			false);

	CATCH();
}

void glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable,
	unsigned long *mask)
{
	glXGetSelectedEvent(dpy, drawable, mask);
}


// Return the 2D X server visual that is attached to the 3D X server FB config.

XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config_)
{
	XVisualInfo *vis = NULL;
	VGLFBConfig config = (VGLFBConfig)config_;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetVisualFromFBConfig(dpy, config_);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXGetVisualFromFBConfig);  PRARGD(dpy);  PRARGC(config);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(!dpy || !VALID_CONFIG(config)) goto done;
	if(!config->visualID) goto done;
	vis = glxvisual::visualFromID(dpy, config->screen, config->visualID);
	if(!vis) goto done;
	vishash.add(dpy, vis, config);

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGV(vis);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return vis;
}


XVisualInfo *glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config)
{
	return glXGetVisualFromFBConfig(dpy, config);
}



// Hand off to the 3D X server without modification.

GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXImportContextEXT(dpy, contextID);

	if(fconfig.egl) THROW("glXImportContextEXT() requires the GLX back end");
	return _glXImportContextEXT(DPY3D, contextID);

	CATCH();
	return 0;
}


// Hand off to the 3D X server without modification.

Bool glXIsDirect(Display *dpy, GLXContext ctx)
{
	Bool direct = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXIsDirect(dpy, ctx);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXIsDirect);  PRARGD(dpy);  PRARGX(ctx);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	direct = backend::isDirect(ctx);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(direct);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return direct;
}


// See notes

Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
	Bool retval = False;  const char *renderer = "Unknown";
	faker::VirtualWin *vw;  VGLFBConfig config = 0;

	if(faker::deadYet || faker::getFakerLevel() > 0)
		return _glXMakeCurrent(dpy, drawable, ctx);

	TRY();

	// Find the FB config that was previously hashed to this context when it was
	// created.
	if(ctx) config = ctxhash.findConfig(ctx);
	if(faker::isDisplayExcluded(dpy))
	{
		faker::setGLXExcludeCurrent(true);
		faker::setOGLExcludeCurrent(true);
		return _glXMakeCurrent(dpy, drawable, ctx);
	}
	faker::setGLXExcludeCurrent(false);
	faker::setOGLExcludeCurrent(false);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXMakeCurrent);  PRARGD(dpy);  PRARGX(drawable);  PRARGX(ctx);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	// glXMakeCurrent() implies a glFinish() on the previous context, which is
	// why we read back the front buffer here if it is dirty.
	GLXDrawable curdraw = backend::getCurrentDrawable();
	if(backend::getCurrentContext() && curdraw
		&& (vw = winhash.find(NULL, curdraw)) != NULL)
	{
		faker::VirtualWin *newvw;
		if(drawable == 0 || !(newvw = winhash.find(dpy, drawable))
			|| newvw->getGLXDrawable() != curdraw)
		{
			if(DrawingToFront() || vw->dirty)
				vw->readback(GL_FRONT, false, fconfig.sync);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to an off-screen drawable.
	int direct = ctxhash.isDirect(ctx);
	if(dpy && drawable && ctx)
	{
		if(!config)
		{
			vglout.PRINTLN("[VGL] WARNING: glXMakeCurrent() called with a previously-destroyed context.");
			goto done;
		}
		try
		{
			vw = winhash.initVW(dpy, drawable, config);
			if(vw)
			{
				setWMAtom(dpy, drawable, vw);
				drawable = vw->updateGLXDrawable();
				vw->setDirect(direct);
			}
			else if(!glxdhash.getCurrentDisplay(drawable))
			{
				// Apparently it isn't a Pbuffer or a Pixmap, so it must be a window
				// that was created in another process.
				winhash.add(dpy, drawable);
				vw = winhash.initVW(dpy, drawable, config);
				if(vw)
				{
					drawable = vw->updateGLXDrawable();
					vw->setDirect(direct);
				}
			}
		}
		catch(std::exception &e)
		{
			if(!strcmp(GET_METHOD(e), "VirtualWin")
				&& !strcmp(e.what(), "Invalid window"))
			{
				faker::sendGLXError(dpy, X_GLXMakeCurrent, GLXBadDrawable, false);
				goto done;
			}
			throw;
		}
	}
	else if(drawable && !ctx)
	{
		// This situation would cause _glXMakeContextCurrent() to trigger an X11
		// error anyhow, but the error it triggers is implementation-dependent.
		// It's better to provide consistent behavior to the calling program.
		faker::sendGLXError(dpy, X_GLXMakeCurrent, GLXBadContext, false);
		goto done;
	}

	retval = backend::makeCurrent(dpy, drawable, drawable, ctx);
	if(fconfig.trace && retval)
		renderer = (const char *)_glGetString(GL_RENDERER);
	// The pixels in a new off-screen drawable are undefined, so we have to clear
	// it.
	if((vw = winhash.find(NULL, drawable)) != NULL)
	{
		vw->clear();  vw->cleanup();
	}
	faker::VirtualPixmap *vpm;
	if((vpm = pmhash.find(dpy, drawable)) != NULL)
	{
		vpm->clear();
		vpm->setDirect(direct);
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGC(config);  PRARGX(drawable);  PRARGS(renderer);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx)
{
	Bool retval = False;  const char *renderer = "Unknown";
	faker::VirtualWin *vw;  VGLFBConfig config = 0;

	if(faker::deadYet || faker::getFakerLevel() > 0)
		return _glXMakeContextCurrent(dpy, draw, read, ctx);

	TRY();

	if(ctx) config = ctxhash.findConfig(ctx);
	if(faker::isDisplayExcluded(dpy))
	{
		faker::setGLXExcludeCurrent(true);
		faker::setOGLExcludeCurrent(true);
		return _glXMakeContextCurrent(dpy, draw, read, ctx);
	}
	faker::setGLXExcludeCurrent(false);
	faker::setOGLExcludeCurrent(false);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXMakeContextCurrent);  PRARGD(dpy);  PRARGX(draw);  PRARGX(read);
	PRARGX(ctx);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	// glXMakeContextCurrent() implies a glFinish() on the previous context,
	// which is why we read back the front buffer here if it is dirty.
	GLXDrawable curdraw = backend::getCurrentDrawable();
	if(backend::getCurrentContext() && curdraw
		&& (vw = winhash.find(NULL, curdraw)) != NULL)
	{
		faker::VirtualWin *newvw;
		if(draw == 0 || !(newvw = winhash.find(dpy, draw))
			|| newvw->getGLXDrawable() != curdraw)
		{
			if(DrawingToFront() || vw->dirty)
				vw->readback(GL_FRONT, false, fconfig.sync);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to an off-screen drawable.
	faker::VirtualWin *drawVW, *readVW;
	int direct = ctxhash.isDirect(ctx);
	if(dpy && (draw || read) && ctx)
	{
		if(!config)
		{
			vglout.PRINTLN("[VGL] WARNING: glXMakeContextCurrent() called with a previously-destroyed context");
			goto done;
		}
		try
		{
			bool differentReadDrawable = (read && read != draw);

			if(draw)
			{
				drawVW = winhash.initVW(dpy, draw, config);
				if(drawVW)
				{
					setWMAtom(dpy, draw, drawVW);
					draw = drawVW->updateGLXDrawable();
					drawVW->setDirect(direct);
				}
				else if(!glxdhash.getCurrentDisplay(draw))
				{
					// Apparently it isn't a Pbuffer or a Pixmap, so it must be a window
					// that was created in another process.
					winhash.add(dpy, draw);
					drawVW = winhash.initVW(dpy, draw, config);
					if(drawVW)
					{
						draw = drawVW->updateGLXDrawable();
						drawVW->setDirect(direct);
					}
				}
			}

			if(differentReadDrawable)
			{
				readVW = winhash.initVW(dpy, read, config);
				if(readVW)
				{
					setWMAtom(dpy, read, readVW);
					read = readVW->updateGLXDrawable();
					readVW->setDirect(direct);
				}
				else if(!glxdhash.getCurrentDisplay(read))
				{
					// Apparently it isn't a Pbuffer or a Pixmap, so it must be a window
					// that was created in another process.
					winhash.add(dpy, read);
					readVW = winhash.initVW(dpy, read, config);
					if(readVW)
					{
						read = readVW->updateGLXDrawable();
						readVW->setDirect(direct);
					}
				}
			}
			else if(read) read = draw;
		}
		catch(std::exception &e)
		{
			if(!strcmp(GET_METHOD(e), "VirtualWin")
				&& !strcmp(e.what(), "Invalid window"))
			{
				faker::sendGLXError(dpy, X_GLXMakeContextCurrent, GLXBadDrawable,
					false);
				goto done;
			}
			throw;
		}
	}
	else if((draw || read) && !ctx)
	{
		// This situation would cause _glXMakeContextCurrent() to trigger an X11
		// error anyhow, but the error it triggers is implementation-dependent.
		// It's better to provide consistent behavior to the calling program.
		faker::sendGLXError(dpy, X_GLXMakeContextCurrent, GLXBadContext, false);
		goto done;
	}

	retval = backend::makeCurrent(dpy, draw, read, ctx);
	if(fconfig.trace && retval)
		renderer = (const char *)_glGetString(GL_RENDERER);
	if((drawVW = winhash.find(NULL, draw)) != NULL)
	{
		drawVW->clear();  drawVW->cleanup();
	}
	if((readVW = winhash.find(NULL, read)) != NULL)
		readVW->cleanup();
	faker::VirtualPixmap *vpm;
	if((vpm = pmhash.find(dpy, draw)) != NULL)
	{
		vpm->clear();
		vpm->setDirect(direct);
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGC(config);  PRARGX(draw);  PRARGX(read);
	PRARGS(renderer);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


Bool glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx)
{
	return glXMakeContextCurrent(dpy, draw, read, ctx);
}


// Hand off to the 3D X server without modification.

int glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	int retval = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryContext(dpy, ctx, attribute, value);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXQueryContext);  PRARGD(dpy);  PRARGX(ctx);  PRARGIX(attribute);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	VGLFBConfig config;

	if(ctx && attribute == GLX_SCREEN && value
		&& (config = ctxhash.findConfig(ctx)) != NULL)
	{
		*value = config->screen;
		retval = Success;
	}
	else retval = backend::queryContext(dpy, ctx, attribute, value);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(value) PRARGIX(*value);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return retval;
}


int glXQueryContextInfoEXT(Display *dpy, GLXContext ctx, int attribute,
	int *value)
{
	int retval = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryContextInfoEXT(dpy, ctx, attribute, value);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXQueryContextInfoEXT);  PRARGD(dpy);  PRARGX(ctx);
	PRARGIX(attribute);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	VGLFBConfig config;

	if(fconfig.egl) THROW("glXQueryContextInfoEXT() requires the GLX back end");

	if(ctx && attribute == GLX_SCREEN_EXT && value
		&& (config = ctxhash.findConfig(ctx)) != NULL)
	{
		*value = config->screen;
		retval = Success;
	}
	else if(ctx && attribute == GLX_VISUAL_ID_EXT && value
		&& (config = ctxhash.findConfig(ctx)) != NULL)
	{
		*value = config->visualID;
		retval = Success;
	}
	else retval = _glXQueryContextInfoEXT(DPY3D, ctx, attribute, value);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(value) PRARGIX(*value);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	return retval;
}


// Hand off to the 3D X server without modification, except that 'draw' is
// replaced with its corresponding off-screen drawable ID and
// GLX_SWAP_INTERVAL_EXT is emulated.

void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute,
	unsigned int *value)
{
	GLXDrawable glxDraw = draw;

	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXQueryDrawable(dpy, draw, attribute, value);
		return;
	}

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXQueryDrawable);  PRARGD(dpy);  PRARGX(draw);
	PRARGIX(attribute);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(!value) goto done;

	// GLX_EXT_swap_control attributes
	if(attribute == GLX_SWAP_INTERVAL_EXT)
	{
		faker::VirtualWin *vw;
		if((vw = winhash.find(dpy, draw)) != NULL)
			*value = vw->getSwapInterval();
		else
			*value = 0;
		goto done;
	}
	else if(attribute == GLX_MAX_SWAP_INTERVAL_EXT)
	{
		*value = VGL_MAX_SWAP_INTERVAL;
		goto done;
	}
	else
	{
		faker::VirtualWin *vw;  faker::VirtualPixmap *vpm;

		if((vw = winhash.find(dpy, draw)) != NULL)
			glxDraw = vw->getGLXDrawable();
		else if((vpm = pmhash.find(dpy, draw)) != NULL)
			glxDraw = vpm->getGLXDrawable();

		backend::queryDrawable(dpy, glxDraw, attribute, value);
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(glxDraw);
	if(value) { PRARGIX(*value); }  else { PRARGX(value); }
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}

#if defined(GLX_GLXEXT_VERSION) && GLX_GLXEXT_VERSION >= 20190000
void glXQueryGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf, int attribute,
	unsigned int *value)
#else
int glXQueryGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf, int attribute,
	unsigned int *value)
#endif
{
	glXQueryDrawable(dpy, pbuf, attribute, value);
	#if !defined(GLX_GLXEXT_VERSION) || GLX_GLXEXT_VERSION < 20190000
	return 0;
	#endif
}


// Hand off to the 3D X server without modification.

Bool glXQueryExtension(Display *dpy, int *error_base, int *event_base)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryExtension(dpy, error_base, event_base);

	int majorOpcode, eventBase, errorBase;
	Bool retval = backend::queryExtension(dpy, &majorOpcode, &eventBase,
		&errorBase);
	if(error_base) *error_base = errorBase;
	if(event_base) *event_base = eventBase;
	return retval;

	CATCH();
	return False;
}


// Same as glXGetClientString(GLX_EXTENSIONS)

const char *glXQueryExtensionsString(Display *dpy, int screen)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryExtensionsString(dpy, screen);

	return getGLXExtensions();

	CATCH();
	return NULL;
}


// Same as glXGetClientString() in our case

const char *glXQueryServerString(Display *dpy, int screen, int name)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryServerString(dpy, screen, name);

	if(name == GLX_EXTENSIONS) return getGLXExtensions();
	else if(name == GLX_VERSION) return "1.4";
	else if(name == GLX_VENDOR)
	{
		if(strlen(fconfig.glxvendor) > 0) return fconfig.glxvendor;
		else return __APPNAME;
	}

	CATCH();
	return NULL;
}


// Hand off to the 3D X server without modification.

Bool glXQueryVersion(Display *dpy, int *major, int *minor)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryVersion(dpy, major, minor);

	if(major && minor)
	{
		*major = 1;  *minor = 4;
		return True;
	}

	CATCH();
	return False;
}


// Modern GLX implementations don't seem to do anything meaningful with this
// function.  Mesa stores the event mask for later retrieval by
// glXGetSelectedEvent(), but a GLXPbufferClobberEvent is never actually
// generated.  nVidia's implementation doesn't even store the event mask; it
// appears to be just a no-op.  The number of applications that use this
// function seems to be approximately zero, because the reasons behind it
// (Pbuffer clobbering due to a resource conflict) are not generally valid with
// modern systems.  However, we emulate Mesa's behavior in the interest of
// conformance.

void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXSelectEvent(dpy, draw, event_mask);

	event_mask &= GLX_PBUFFER_CLOBBER_MASK;

	faker::VirtualWin *vw;
	if((vw = winhash.find(dpy, draw)) != NULL)
		vw->setEventMask(event_mask);
	else if(glxdhash.getCurrentDisplay(draw))
		glxdhash.setEventMask(draw, event_mask);
	else
		faker::sendGLXError(dpy, X_GLXChangeDrawableAttributes, GLXBadDrawable,
			false);

	CATCH();
}

void glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
	glXSelectEvent(dpy, drawable, mask);
}


// If the application is rendering to the back buffer, VirtualGL will read
// back and transport the contents of the buffer whenever glXSwapBuffers() is
// called.

void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
	faker::VirtualWin *vw = NULL;
	static util::Timer timer;  util::Timer sleepTimer;
	static double err = 0.;  static bool first = true;

	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXSwapBuffers(dpy, drawable);
		return;
	}

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXSwapBuffers);  PRARGD(dpy);  PRARGX(drawable);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	fconfig.flushdelay = 0.;
	if((vw = winhash.find(dpy, drawable)) != NULL)
	{
		vw->readback(GL_BACK, false, fconfig.sync);
		vw->swapBuffers();
		int interval = vw->getSwapInterval();
		if(interval > 0)
		{
			double elapsed = timer.elapsed();
			if(first) first = false;
			else
			{
				double fps = fconfig.refreshrate / (double)interval;
				if(fps > 0.0 && elapsed < 1. / fps)
				{
					sleepTimer.start();
					long usec = (long)((1. / fps - elapsed - err) * 1000000.);
					if(usec > 0) usleep(usec);
					double sleepTime = sleepTimer.elapsed();
					err = sleepTime - (1. / fps - elapsed - err);  if(err < 0.) err = 0.;
				}
			}
			timer.start();
		}
	}
	else backend::swapBuffers(dpy, drawable);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(vw) { PRARGX(vw->getGLXDrawable()); }
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
}


// Vertical refresh rate has no meaning with an off-screen drawable, but we
// emulate it using an internal timer so that we can provide a reasonable
// implementation of the swap control extensions, which are used by some
// applications as a way of governing the frame rate.

void glXSwapIntervalEXT(Display *dpy, GLXDrawable drawable, int interval)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXSwapIntervalEXT(dpy, drawable, interval);
		return;
	}

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXSwapIntervalEXT);  PRARGD(dpy);  PRARGX(drawable);
	PRARGI(interval);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(interval > VGL_MAX_SWAP_INTERVAL) interval = VGL_MAX_SWAP_INTERVAL;
	if(interval < 0)
		// NOTE:  Technically, this should trigger a BadValue error, but nVidia's
		// implementation doesn't, so we emulate their behavior.
		interval = 1;

	faker::VirtualWin *vw;
	if((vw = winhash.find(dpy, drawable)) != NULL)
		vw->setSwapInterval(interval);
	// NOTE:  Technically, a BadWindow error should be triggered if drawable
	// isn't a GLX window, but nVidia's implementation doesn't, so we emulate
	// their behavior.

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
}


// This is basically the same as calling glXSwapIntervalEXT() with the current
// drawable.

int glXSwapIntervalSGI(int interval)
{
	int retval = 0;

	if(faker::getGLXExcludeCurrent()) return _glXSwapIntervalSGI(interval);

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXSwapIntervalSGI);  PRARGI(interval);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	TRY();

	faker::VirtualWin *vw;  GLXDrawable draw = backend::getCurrentDrawable();
	if(interval < 0) retval = GLX_BAD_VALUE;
	else if(!draw || !(vw = winhash.find(NULL, draw)))
		retval = GLX_BAD_CONTEXT;
	else vw->setSwapInterval(interval);

	CATCH();

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	return retval;
}


#include "xfonts.c"

// We use a tweaked out version of the Mesa glXUseXFont() implementation.

void glXUseXFont(Font font, int first, int count, int list_base)
{
	if(faker::getGLXExcludeCurrent())
	{
		_glXUseXFont(font, first, count, list_base);  return;
	}

	TRY();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(glXUseXFont);  PRARGX(font);  PRARGI(first);  PRARGI(count);
	PRARGI(list_base);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	DISABLE_FAKER();

	Fake_glXUseXFont(font, first, count, list_base);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
}


}  // extern "C"
