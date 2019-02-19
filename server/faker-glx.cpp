// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011-2019 D. R. Commander
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
#include "WindowHash.h"
#include "rr.h"
#include "faker.h"
#include "glxvisual.h"

using namespace vglutil;
using namespace vglserver;


// This emulates the behavior of the nVidia drivers
#define VGL_MAX_SWAP_INTERVAL  8


// Applications will sometimes use X11 functions to obtain a list of 2D X
// server visuals, then pass one of those visuals to glXCreateContext(),
// glXGetConfig(), etc.  Since the visual didn't come from glXChooseVisual(),
// VGL has no idea what properties the application is looking for, so if no 3D
// X server FB config is already attached to the visual, we have to create one
// using default attributes.

#define TEST_ATTRIB(attrib, index, min, max) \
{ \
	if(!strcmp(argv[i], #attrib) && i < argc - 1) \
	{ \
		int temp = atoi(argv[++i]); \
		if(temp >= min && temp <= max) \
		{ \
			attribs[index++] = attrib; \
			attribs[index++] = temp; \
		} \
	} \
}

static GLXFBConfig matchConfig(Display *dpy, XVisualInfo *vis,
	bool preferSingleBuffer = false, bool pixmap = false)
{
	GLXFBConfig config = 0, *configs = NULL;  int n = 0;

	if(!dpy || !vis) return 0;
	if(!(config = vglfaker::getConfigForVisual(dpy, vis)))
	{
		// Punt.  We can't figure out where the visual came from
		int defaultAttribs[] = { GLX_DOUBLEBUFFER, 1, GLX_RED_SIZE, 8,
			GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_STEREO, 0, GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT, GLX_X_VISUAL_TYPE,
			GLX_TRUE_COLOR, GLX_DEPTH_SIZE, 1, GLX_STENCIL_SIZE, 8, None };
		int attribs[256];

		if(pixmap || fconfig.drawable == RRDRAWABLE_PIXMAP)
			defaultAttribs[13] = GLX_PIXMAP_BIT | GLX_WINDOW_BIT;
		if(vis->depth == 30)
		{
			defaultAttribs[3] = defaultAttribs[5] = defaultAttribs[7] = 10;
		}
		memset(attribs, 0, sizeof(attribs));
		memcpy(attribs, defaultAttribs, sizeof(defaultAttribs));
		if(glxvisual::visAttrib2D(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_STEREO))
			attribs[11] = 1;

		if(vis->c_class == DirectColor) attribs[15] = GLX_DIRECT_COLOR;

		// If we're creating a GLX pixmap and we can't determine an exact FB config
		// to map to the visual, then we will make the pixmap single-buffered.
		if(preferSingleBuffer) attribs[1] = 0;

		// Allow the default FB config attribs to be manually specified.  This is
		// necessary to support apps that implement their own visual selection
		// mechanisms.  Since those apps don't use glXChooseVisual(), VirtualGL has
		// no idea what 3D visual attributes they need, and thus it is necessary
		// to give it a hint using this environment variable.
		if(strlen(fconfig.defaultfbconfig) > 0)
		{
			char *str = strdup(fconfig.defaultfbconfig);
			if(!str) THROW_UNIX();
			char *argv[512];  int argc = 0;
			char *arg = strtok(str, " \t,");
			while(arg && argc < 512)
			{
				argv[argc] = arg;  argc++;
				arg = strtok(NULL, " \t,");
			}
			for(int i = 0, j = 20; i < argc && j < 256; i++)
			{
				int index;
				index = 2;
				TEST_ATTRIB(GLX_RED_SIZE, index, 0, INT_MAX);
				index = 4;
				TEST_ATTRIB(GLX_GREEN_SIZE, index, 0, INT_MAX);
				index = 6;
				TEST_ATTRIB(GLX_BLUE_SIZE, index, 0, INT_MAX);
				index = 16;
				TEST_ATTRIB(GLX_DEPTH_SIZE, index, 0, INT_MAX);
				index = 18;
				TEST_ATTRIB(GLX_STENCIL_SIZE, index, 0, INT_MAX);
				TEST_ATTRIB(GLX_ALPHA_SIZE, j, 0, INT_MAX);
				TEST_ATTRIB(GLX_AUX_BUFFERS, j, 0, INT_MAX);
				TEST_ATTRIB(GLX_ACCUM_RED_SIZE, j, 0, INT_MAX);
				TEST_ATTRIB(GLX_ACCUM_GREEN_SIZE, j, 0, INT_MAX);
				TEST_ATTRIB(GLX_ACCUM_BLUE_SIZE, j, 0, INT_MAX);
				TEST_ATTRIB(GLX_ACCUM_ALPHA_SIZE, j, 0, INT_MAX);
				TEST_ATTRIB(GLX_SAMPLE_BUFFERS, j, 0, INT_MAX);
				TEST_ATTRIB(GLX_SAMPLES, j, 0, INT_MAX);
			}
			free(str);
		}

			OPENTRACE(Choosing FB config for visual with unknown OpenGL attributes);
			if(fconfig.trace) vglout.print("VGL_DEFAULTFBCONFIG ");
			PRARGAL13(attribs);  STARTTRACE();

		configs = _glXChooseFBConfig(DPY3D, DefaultScreen(DPY3D), attribs, &n);
		if((!configs || n < 1) && attribs[11])
		{
			attribs[11] = 0;
			if(fconfig.trace) vglout.print("[failed, trying mono] ");
			configs = _glXChooseFBConfig(DPY3D, DefaultScreen(DPY3D), attribs, &n);
		}
		if((!configs || n < 1) && attribs[1])
		{
			attribs[1] = 0;
			if(fconfig.trace) vglout.print("[failed, trying single-buffered] ");
			configs = _glXChooseFBConfig(DPY3D, DefaultScreen(DPY3D), attribs, &n);
		}

			STOPTRACE();  PRARGC(configs[0]);  CLOSETRACE();

		if(!configs || n < 1) return 0;
		config = configs[0];
		XFree(configs);
		if(config)
		{
			vglfaker::setConfigForVisual(dpy, vis, config);
			glxvisual::attachVisualID(dpy, vis->screen, config, vis->visualid);
		}
	}
	return config;
}


// If GLXDrawable is a window ID, then return the ID for its corresponding
// off-screen drawable (if applicable.)

GLXDrawable ServerDrawable(Display *dpy, GLXDrawable draw)
{
	VirtualWin *vw = NULL;
	if(winhash.find(dpy, draw, vw)) return vw->getGLXDrawable();
	else return draw;
}


void setWMAtom(Display *dpy, Window win, VirtualWin *vw)
{
	Atom *protocols = NULL, *newProtocols = NULL;  int count = 0;

	Atom deleteAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
	if(!deleteAtom) goto bailout;

	if(XGetWMProtocols(dpy, win, &protocols, &count) && protocols && count > 0)
	{
		for(int i = 0; i < count; i++)
			if(protocols[i] == deleteAtom)
			{
				XFree(protocols);  return;
			}
		newProtocols = (Atom *)malloc(sizeof(Atom) * (count + 1));
		if(!newProtocols) goto bailout;
		for(int i = 0; i < count; i++)
			newProtocols[i] = protocols[i];
		newProtocols[count] = deleteAtom;
		if(!XSetWMProtocols(dpy, win, newProtocols, count + 1)) goto bailout;
		XFree(protocols);
		free(newProtocols);
	}
	else if(!XSetWMProtocols(dpy, win, &deleteAtom, 1)) goto bailout;
	vw->vglWMDelete();
	return;

	bailout:
	if(protocols) XFree(protocols);
	if(newProtocols) free(newProtocols);
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
	GLXFBConfig *configs = NULL;
	bool fbcidreq = false;

	TRY();

	// If this is called internally from within another GLX function, then use
	// the real function.
	if(IS_EXCLUDED(dpy))
		return _glXChooseFBConfig(dpy, screen, attrib_list, nelements);

	if(attrib_list)
	{
		for(int i = 0; attrib_list[i] != None && i <= 254; i += 2)
		{
			if(attrib_list[i] == GLX_FBCONFIG_ID) fbcidreq = true;
		}
	}

		OPENTRACE(glXChooseFBConfig);  PRARGD(dpy);  PRARGI(screen);
		PRARGAL13(attrib_list);  STARTTRACE();

	int temp;
	if(!nelements) nelements = &temp;
	*nelements = 0;

	// If no attributes are specified, return all FB configs.  If GLX_FBCONFIG_ID
	// is specified, ignore all other attributes.
	if(!attrib_list || fbcidreq)
	{
		configs = _glXChooseFBConfig(DPY3D, DefaultScreen(DPY3D), attrib_list,
			nelements);
		goto done;
	}

	// Modify the attributes so that only FB configs appropriate for off-screen
	// rendering are considered.
	else configs = glxvisual::configsFromVisAttribs(attrib_list, *nelements,
		true);

	if(configs && *nelements)
	{
		// Create the FB config table, if it isn't already created.  We do this
		// here because we may not have access to the screen information later.
		glxvisual::getAttachedVisualID(dpy, screen, configs[0]);
	}

	done:
		STOPTRACE();
		if(configs && nelements)
		{
			if(*nelements)
				for(int i = 0; i < *nelements; i++)
					vglout.print("configs[%d]=0x%.8lx(0x%.2x) ", i,
						(unsigned long)configs[i], configs[i] ? FBCID(configs[i]) : 0);
			PRARGI(*nelements);
		}
		CLOSETRACE();

	CATCH();
	return configs;
}

GLXFBConfigSGIX *glXChooseFBConfigSGIX(Display *dpy, int screen,
	int *attrib_list, int *nelements)
{
	return glXChooseFBConfig(dpy, screen, attrib_list, nelements);
}


// Obtain a 3D X server FB config that supports off-screen rendering and has
// the desired set of attributes, match it to an appropriate 2D X server
// visual, attach the two, and return the visual.

XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
	XVisualInfo *vis = NULL;  GLXFBConfig config = 0;
	static bool alreadyWarned = false;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXChooseVisual(dpy, screen, attrib_list);

		OPENTRACE(glXChooseVisual);  PRARGD(dpy);  PRARGI(screen);
		PRARGAL11(attrib_list);  STARTTRACE();

	// Use the specified set of GLX attributes to obtain an FB config on the 3D X
	// server suitable for off-screen rendering
	GLXFBConfig *configs = NULL, prevConfig;  int n = 0;
	VisualID vid = 0;
	if(!dpy || !attrib_list) goto done;
	if(!(configs = glxvisual::configsFromVisAttribs(attrib_list, n)) || n < 1)
	{
		if(!alreadyWarned && fconfig.verbose)
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
	XFree(configs);

	// Find an appropriate matching visual on the 2D X server.
	vid = glxvisual::getAttachedVisualID(dpy, screen, config);
	if(!vid) goto done;
	vis = glxvisual::visualFromID(dpy, screen, vid);
	if(!vis) goto done;

	if((prevConfig = vglfaker::getConfigForVisual(dpy, vis))
		&& FBCID(config) != FBCID(prevConfig) && fconfig.trace)
		vglout.println("[VGL] WARNING: Visual 0x%.2x was previously mapped to FB config 0x%.2x and is now mapped to 0x%.2x\n",
			vis->visualid, FBCID(prevConfig), FBCID(config));

	// Attach the FB config and the visual so that we can look up the FB config
	// whenever the appplication subsequently passes the visual to
	// glXCreateContext() or other functions.
	vglfaker::setConfigForVisual(dpy, vis, config);

	done:
		STOPTRACE();  PRARGV(vis);  PRARGC(config);  CLOSETRACE();

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

	_glXCopyContext(DPY3D, src, dst, mask);

	CATCH();
}


// Create a GLX context on the 3D X server suitable for off-screen rendering.

GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
	GLXContext share_list, Bool direct)
{
	GLXContext ctx = 0;  GLXFBConfig config = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateContext(dpy, vis, share_list, direct);

	if(!fconfig.allowindirect) direct = True;

		OPENTRACE(glXCreateContext);  PRARGD(dpy);  PRARGV(vis);
		PRARGX(share_list);  PRARGI(direct);  STARTTRACE();

	// If 'vis' was obtained through a previous call to glXChooseVisual(), find
	// the corresponding FB config.  Otherwise, we have to fall back to using a
	// default FB config returned from matchConfig().
	if(!(config = matchConfig(dpy, vis)))
		THROW("Could not obtain RGB visual on the server suitable for off-screen rendering.");
	ctx = _glXCreateNewContext(DPY3D, config, GLX_RGBA_TYPE, share_list,
		direct);
	if(ctx)
	{
		int newctxIsDirect = _glXIsDirect(DPY3D, ctx);
		if(!newctxIsDirect && direct)
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

		STOPTRACE();  PRARGC(config);  PRARGX(ctx);  CLOSETRACE();

	CATCH();
	return ctx;
}


GLXContext glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config,
	GLXContext share_context, Bool direct, const int *attribs)
{
	GLXContext ctx = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateContextAttribsARB(dpy, config, share_context, direct,
			attribs);

	if(!fconfig.allowindirect) direct = True;

		OPENTRACE(glXCreateContextAttribsARB);  PRARGD(dpy);  PRARGC(config);
		PRARGX(share_context);  PRARGI(direct);  PRARGAL13(attribs);
		STARTTRACE();

	CHECKSYM_NONFATAL(glXCreateContextAttribsARB)
	if((!attribs || attribs[0] == None) && !__glXCreateContextAttribsARB)
		ctx = _glXCreateNewContext(DPY3D, config, GLX_RGBA_TYPE, share_context,
			direct);
	else
		ctx = _glXCreateContextAttribsARB(DPY3D, config, share_context, direct,
			attribs);
	if(ctx)
	{
		int newctxIsDirect = _glXIsDirect(DPY3D, ctx);
		if(!newctxIsDirect && direct)
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

		STOPTRACE();  PRARGX(ctx);  CLOSETRACE();

	CATCH();
	return ctx;
}


GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config,
	int render_type, GLXContext share_list, Bool direct)
{
	GLXContext ctx = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateNewContext(dpy, config, render_type, share_list, direct);

	if(!fconfig.allowindirect) direct = True;

		OPENTRACE(glXCreateNewContext);  PRARGD(dpy);  PRARGC(config);
		PRARGI(render_type);  PRARGX(share_list);  PRARGI(direct);  STARTTRACE();

	ctx = _glXCreateNewContext(DPY3D, config, GLX_RGBA_TYPE, share_list,
		direct);
	if(ctx)
	{
		int newctxIsDirect = _glXIsDirect(DPY3D, ctx);
		if(!newctxIsDirect && direct)
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

		STOPTRACE();  PRARGX(ctx);  CLOSETRACE();

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

GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config,
	const int *attrib_list)
{
	GLXPbuffer pb = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreatePbuffer(dpy, config, attrib_list);

		OPENTRACE(glXCreatePbuffer);  PRARGD(dpy);  PRARGC(config);
		PRARGAL13(attrib_list);  STARTTRACE();

	pb = _glXCreatePbuffer(DPY3D, config, attrib_list);
	if(dpy && pb) glxdhash.add(pb, dpy);

		STOPTRACE();  PRARGX(pb);  CLOSETRACE();

	CATCH();
	return pb;
}

GLXPbuffer glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config,
	unsigned int width, unsigned int height, int *attrib_list)
{
	int attribs[257], j = 0;
	if(attrib_list)
	{
		for(int i = 0; attrib_list[i] != None && i <= 254; i += 2)
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
	GLXPixmap drawable = 0;  GLXFBConfig config = 0;
	int x = 0, y = 0;  unsigned int width = 0, height = 0, depth = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateGLXPixmap(dpy, vis, pm);

		OPENTRACE(glXCreateGLXPixmap);  PRARGD(dpy);  PRARGV(vis);  PRARGX(pm);
		STARTTRACE();

	Window root;  unsigned int bw;
	_XGetGeometry(dpy, pm, &root, &x, &y, &width, &height, &bw, &depth);
	if(!(config = matchConfig(dpy, vis, true, true)))
		THROW("Could not obtain pixmap-capable RGB visual on the server");
	VirtualPixmap *vpm = new VirtualPixmap(dpy, vis->visual, pm);
	if(vpm)
	{
		// Hash the VirtualPixmap instance to the 2D pixmap and also hash the 2D X
		// display handle to the 3D pixmap.
		vpm->init(width, height, depth, config, NULL);
		pmhash.add(dpy, pm, vpm);
		glxdhash.add(vpm->getGLXDrawable(), dpy);
		drawable = vpm->getGLXDrawable();
	}

		STOPTRACE();  PRARGI(x);  PRARGI(y);  PRARGI(width);  PRARGI(height);
		PRARGI(depth);  PRARGC(config);  PRARGX(drawable);  CLOSETRACE();

	CATCH();
	return drawable;
}


GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pm,
	const int *attribs)
{
	GLXPixmap drawable = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreatePixmap(dpy, config, pm, attribs);

		OPENTRACE(glXCreatePixmap);  PRARGD(dpy);  PRARGC(config);  PRARGX(pm);
		STARTTRACE();

	Window root;  int x, y;  unsigned int w, h, bw, d;
	_XGetGeometry(dpy, pm, &root, &x, &y, &w, &h, &bw, &d);

	VisualID vid = glxvisual::getAttachedVisualID(dpy, DefaultScreen(dpy),
		config);
	VirtualPixmap *vpm = NULL;
	if(vid)
	{
		XVisualInfo *vis = glxvisual::visualFromID(dpy, DefaultScreen(dpy), vid);
		if(vis)
		{
			vpm = new VirtualPixmap(dpy, vis->visual, pm);
			XFree(vis);
		}
	}
	if(vpm)
	{
		vpm->init(w, h, d, config, attribs);
		pmhash.add(dpy, pm, vpm);
		glxdhash.add(vpm->getGLXDrawable(), dpy);
		drawable = vpm->getGLXDrawable();
	}

		STOPTRACE();  PRARGI(x);  PRARGI(y);  PRARGI(w);  PRARGI(h);
		PRARGI(d);  PRARGX(drawable);  CLOSETRACE();

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

GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config, Window win,
	const int *attrib_list)
{
	VirtualWin *vw = NULL;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXCreateWindow(dpy, config, win, attrib_list);

		OPENTRACE(glXCreateWindow);  PRARGD(dpy);  PRARGC(config);  PRARGX(win);
		STARTTRACE();

	XSync(dpy, False);
	vw = winhash.initVW(dpy, win, config);
	if(!vw && !glxdhash.getCurrentDisplay(win))
	{
		// Apparently win was created in another process or using XCB.
		winhash.add(dpy, win);
		vw = winhash.initVW(dpy, win, config);
	}
	if(!vw)
		THROW("Cannot create virtual window for specified X window");

		STOPTRACE();  if(vw) { PRARGX(vw->getGLXDrawable()); }
		CLOSETRACE();

	CATCH();
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

		OPENTRACE(glXDestroyContext);  PRARGD(dpy);  PRARGX(ctx);  STARTTRACE();

	ctxhash.remove(ctx);
	_glXDestroyContext(DPY3D, ctx);

		STOPTRACE();  CLOSETRACE();

	CATCH();
}


// When the Pbuffer is destroyed, remove it from the GLX drawable-to-2D display
// handle hash.

void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
	TRY();

	if(IS_EXCLUDED(dpy)) { _glXDestroyPbuffer(dpy, pbuf);  return; }

		OPENTRACE(glXDestroyPbuffer);  PRARGD(dpy);  PRARGX(pbuf);  STARTTRACE();

	_glXDestroyPbuffer(DPY3D, pbuf);
	if(pbuf) glxdhash.remove(pbuf);

		STOPTRACE();  CLOSETRACE();

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

		OPENTRACE(glXDestroyGLXPixmap);  PRARGD(dpy);  PRARGX(pix);  STARTTRACE();

	VirtualPixmap *vpm = pmhash.find(dpy, pix);
	if(vpm && vpm->isInit()) vpm->readback();

	if(pix) glxdhash.remove(pix);
	if(dpy && pix) pmhash.remove(dpy, pix);

		STOPTRACE();  CLOSETRACE();

	CATCH();
}


void glXDestroyPixmap(Display *dpy, GLXPixmap pix)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXDestroyPixmap(dpy, pix);  return;
	}

		OPENTRACE(glXDestroyPixmap);  PRARGD(dpy);  PRARGX(pix);  STARTTRACE();

	VirtualPixmap *vpm = pmhash.find(dpy, pix);
	if(vpm && vpm->isInit()) vpm->readback();

	if(pix) glxdhash.remove(pix);
	if(dpy && pix) pmhash.remove(dpy, pix);

		STOPTRACE();  CLOSETRACE();

	CATCH();
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

		OPENTRACE(glXDestroyWindow);  PRARGD(dpy);  PRARGX(win);  STARTTRACE();

	winhash.remove(dpy, win);

		STOPTRACE();  CLOSETRACE();

	CATCH();
}


// Hand off to the 3D X server without modification.

void glXFreeContextEXT(Display *dpy, GLXContext ctx)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXFreeContextEXT(dpy, ctx);  return;
	}
	_glXFreeContextEXT(DPY3D, ctx);

	CATCH();
}


// Since VirtualGL is effectively its own implementation of GLX, it needs to
// properly report the extensions and GLX version it supports.

#define VGL_GLX_EXTENSIONS \
	"GLX_ARB_get_proc_address GLX_ARB_multisample GLX_EXT_visual_info GLX_EXT_visual_rating GLX_SGI_make_current_read GLX_SGIX_fbconfig GLX_SGIX_pbuffer"
// Allow enough space here for all of the extensions
static char glxextensions[1024] = VGL_GLX_EXTENSIONS;

static const char *getGLXExtensions(void)
{
	CHECKSYM_NONFATAL(glXCreateContextAttribsARB)
	if(__glXCreateContextAttribsARB
		&& !strstr(glxextensions, "GLX_ARB_create_context"))
		strncat(glxextensions,
			" GLX_ARB_create_context GLX_ARB_create_context_profile",
			1023 - strlen(glxextensions));

	CHECKSYM_NONFATAL(glXFreeContextEXT)
	CHECKSYM_NONFATAL(glXImportContextEXT)
	CHECKSYM_NONFATAL(glXQueryContextInfoEXT)
	if(__glXFreeContextEXT && __glXImportContextEXT && __glXQueryContextInfoEXT
		&& !strstr(glxextensions, "GLX_EXT_import_context"))
		strncat(glxextensions, " GLX_EXT_import_context",
			1023 - strlen(glxextensions));

	CHECKSYM_NONFATAL(glXSwapIntervalEXT)
	if(__glXSwapIntervalEXT && !strstr(glxextensions, "GLX_EXT_swap_control"))
		strncat(glxextensions, " GLX_EXT_swap_control",
			1023 - strlen(glxextensions));

	CHECKSYM_NONFATAL(glXBindTexImageEXT)
	CHECKSYM_NONFATAL(glXReleaseTexImageEXT)
	if(__glXBindTexImageEXT && __glXReleaseTexImageEXT
		&& !strstr(glxextensions, "GLX_EXT_texture_from_pixmap"))
		strncat(glxextensions, " GLX_EXT_texture_from_pixmap",
			1023 - strlen(glxextensions));

	CHECKSYM_NONFATAL(glXBindSwapBarrierNV)
	CHECKSYM_NONFATAL(glXJoinSwapGroupNV)
	CHECKSYM_NONFATAL(glXQueryFrameCountNV)
	CHECKSYM_NONFATAL(glXQueryMaxSwapGroupsNV)
	CHECKSYM_NONFATAL(glXQuerySwapGroupNV)
	CHECKSYM_NONFATAL(glXResetFrameCountNV)
	if(__glXBindSwapBarrierNV && __glXJoinSwapGroupNV && __glXQueryFrameCountNV
		&& __glXQueryMaxSwapGroupsNV && __glXQuerySwapGroupNV
		&& __glXResetFrameCountNV && !strstr(glxextensions, "GLX_NV_swap_group"))
		strncat(glxextensions, " GLX_NV_swap_group", 1023 - strlen(glxextensions));

	CHECKSYM_NONFATAL(glXSwapIntervalSGI)
	if(__glXSwapIntervalSGI && !strstr(glxextensions, "GLX_SGI_swap_control"))
		strncat(glxextensions, " GLX_SGI_swap_control",
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
	GLXFBConfig config;  int retval = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetConfig(dpy, vis, attrib, value);

	if(!dpy || !vis || !value)
		return GLX_BAD_VALUE;

		OPENTRACE(glXGetConfig);  PRARGD(dpy);  PRARGV(vis);  PRARGIX(attrib);
		STARTTRACE();

	// If 'vis' was obtained through a previous call to glXChooseVisual(), find
	// the corresponding FB config in the hash.  Otherwise, we have to fall back
	// to using a default FB config returned from matchConfig().
	if(!(config = matchConfig(dpy, vis)))
		THROW("Could not obtain RGB visual on the server suitable for off-screen rendering");

	if(attrib == GLX_USE_GL)
	{
		if(vis->c_class == TrueColor || vis->c_class == DirectColor) *value = 1;
		else *value = 0;
	}
	else if(attrib == GLX_RGBA)
	{
		if((glxvisual::visAttrib3D(config, GLX_RENDER_TYPE) & GLX_RGBA_BIT) != 0)
			*value = 1;
		else *value = 0;
	}
	else retval = _glXGetFBConfigAttrib(DPY3D, config, attrib, value);

		STOPTRACE();  if(value) { PRARGIX(*value); }  else { PRARGX(value); }
		CLOSETRACE();

	CATCH();
	return retval;
}


// This returns the 2D X display handle associated with the current drawable,
// that is, the 2D X display handle passed to whatever function (such as
// XCreateWindow(), glXCreatePbuffer(), etc.) that was used to create the
// drawable.

Display *glXGetCurrentDisplay(void)
{
	Display *dpy = NULL;  VirtualWin *vw = NULL;

	if(vglfaker::getExcludeCurrent()) return _glXGetCurrentDisplay();

	TRY();

		OPENTRACE(glXGetCurrentDisplay);  STARTTRACE();

	GLXDrawable curdraw = _glXGetCurrentDrawable();
	if(winhash.find(curdraw, vw)) dpy = vw->getX11Display();
	else
	{
		if(curdraw) dpy = glxdhash.getCurrentDisplay(curdraw);
	}

		STOPTRACE();  PRARGD(dpy);  CLOSETRACE();

	CATCH();
	return dpy;
}


// As far as the application is concerned, it is rendering to a window and not
// an off-screen drawable, so we must maintain that illusion and pass it back
// the window ID instead of the GLX drawable ID.

GLXDrawable glXGetCurrentDrawable(void)
{
	VirtualWin *vw = NULL;  GLXDrawable draw = _glXGetCurrentDrawable();

	if(vglfaker::getExcludeCurrent()) return draw;

	TRY();

		OPENTRACE(glXGetCurrentDrawable);  STARTTRACE();

	if(winhash.find(draw, vw)) draw = vw->getX11Drawable();

		STOPTRACE();  PRARGX(draw);  CLOSETRACE();

	CATCH();
	return draw;
}

GLXDrawable glXGetCurrentReadDrawable(void)
{
	VirtualWin *vw = NULL;  GLXDrawable read = _glXGetCurrentReadDrawable();

	if(vglfaker::getExcludeCurrent()) return read;

	TRY();

		OPENTRACE(glXGetCurrentReadDrawable);  STARTTRACE();

	if(winhash.find(read, vw)) read = vw->getX11Drawable();

		STOPTRACE();  PRARGX(read);  CLOSETRACE();

	CATCH();
	return read;
}

GLXDrawable glXGetCurrentReadDrawableSGI(void)
{
	return glXGetCurrentReadDrawable();
}


// Return a property from the specified FB config.

int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute,
	int *value)
{
	VisualID vid = 0;  int retval = 0;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetFBConfigAttrib(dpy, config, attribute, value);

		OPENTRACE(glXGetFBConfigAttrib);  PRARGD(dpy);  PRARGC(config);
		PRARGIX(attribute);  STARTTRACE();

	if(!dpy || !config || !value)
	{
		retval = GLX_BAD_VALUE;
		goto done;
	}

	retval = _glXGetFBConfigAttrib(DPY3D, config, attribute, value);

	if(attribute == GLX_DRAWABLE_TYPE && retval == Success)
	{
		int temp = *value;
		*value = 0;
		if((fconfig.drawable == RRDRAWABLE_PBUFFER
			&& glxvisual::visAttrib3D(config, GLX_VISUAL_ID) != 0
			&& temp & GLX_PBUFFER_BIT)
			|| (fconfig.drawable == RRDRAWABLE_PIXMAP && temp & GLX_WINDOW_BIT
				&& temp & GLX_PIXMAP_BIT))
			*value |= GLX_WINDOW_BIT;
		if(temp & GLX_PIXMAP_BIT && temp & GLX_WINDOW_BIT)
			*value |= GLX_PIXMAP_BIT;
		if(temp & GLX_PBUFFER_BIT) *value |= GLX_PBUFFER_BIT;
	}

	// If there is a corresponding 2D X server visual attached to this FB config,
	// then that means it was obtained via glXChooseFBConfig(), and we can
	// return attributes that take into account the interaction between visuals
	// on the 2D X Server and FB Configs on the 3D X Server.
	if((vid = glxvisual::getAttachedVisualID(dpy, DefaultScreen(dpy),
		config)) != 0 && attribute == GLX_VISUAL_ID)
		*value = vid;

	done:
		STOPTRACE();  if(value) { PRARGIX(*value); }  else { PRARGX(value); }
		CLOSETRACE();

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
	else return matchConfig(dpy, vis);

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

		OPENTRACE(glXGetFBConfigs);  PRARGD(dpy);  PRARGI(screen);
		STARTTRACE();

	configs = _glXGetFBConfigs(DPY3D, DefaultScreen(DPY3D), nelements);

		STOPTRACE();  if(configs && nelements) PRARGI(*nelements);
		CLOSETRACE();

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

		OPENTRACE(glXBindTexImageEXT);  PRARGD(dpy);  PRARGX(drawable);
		PRARGI(buffer);  PRARGAL13(attrib_list);  STARTTRACE();

	VirtualPixmap *vpm = NULL;
	if((vpm = pmhash.find(dpy, drawable)) == NULL)
		// If we get here, then the drawable wasn't created with
		// glXCreate[GLX]Pixmap().  Thus, we set it to 0 so _glXBindTexImageEXT()
		// will throw a GLXBadPixmap error for us.
		drawable = 0;
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
			// Also trigger GLXBadPixmap error
			drawable = 0;
		if(gc) XFreeGC(DPY3D, gc);
		if(image) XDestroyImage(image);
	}

	_glXBindTexImageEXT(DPY3D, drawable, buffer, attrib_list);

		STOPTRACE();  CLOSETRACE();

	CATCH();
}


void glXReleaseTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXReleaseTexImageEXT(dpy, drawable, buffer);

		OPENTRACE(glXReleaseTexImageEXT);  PRARGD(dpy);  PRARGX(drawable);
		PRARGI(buffer);  STARTTRACE();

	_glXReleaseTexImageEXT(DPY3D, drawable, buffer);

		STOPTRACE();  CLOSETRACE();

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

	vglfaker::init();

		OPENTRACE(glXGetProcAddressARB);  PRARGS((char *)procName);  STARTTRACE();

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
		CHECK_OPT_FAKED(glXImportContextEXT)
		CHECK_OPT_FAKED(glXQueryContextInfoEXT)

		// GLX_EXT_swap_control
		CHECK_OPT_FAKED(glXSwapIntervalEXT)

		// GLX_EXT_texture_from_pixmap
		CHECK_OPT_FAKED(glXBindTexImageEXT)
		CHECK_OPT_FAKED(glXReleaseTexImageEXT)

		// GLX_NV_swap_group
		CHECK_OPT_FAKED(glXBindSwapBarrierNV)
		CHECK_OPT_FAKED(glXJoinSwapGroupNV)
		CHECK_OPT_FAKED(glXQueryFrameCountNV)
		CHECK_OPT_FAKED(glXQueryMaxSwapGroupsNV)
		CHECK_OPT_FAKED(glXQuerySwapGroupNV)
		CHECK_OPT_FAKED(glXResetFrameCountNV)

		// GLX_SGI_make_current_read
		CHECK_FAKED(glXGetCurrentReadDrawableSGI)
		CHECK_FAKED(glXMakeCurrentReadSGI)

		// GLX_SGI_swap_control
		CHECK_OPT_FAKED(glXSwapIntervalSGI)

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
		CHECK_FAKED(glFinish)
		CHECK_FAKED(glFlush)
		CHECK_FAKED(glViewport)
		CHECK_FAKED(glDrawBuffer)
		CHECK_FAKED(glPopAttrib)
	}
	if(!retval)
	{
		if(fconfig.trace) vglout.print("[passed through]");
		retval = _glXGetProcAddress(procName);
	}

		STOPTRACE();  CLOSETRACE();

	return retval;
}

void (*glXGetProcAddress(const GLubyte *procName))(void)
{
	return glXGetProcAddressARB(procName);
}


// Hand off to the 3D X server without modification, except that 'draw' is
// replaced with its corresponding off-screen drawable ID.

void glXGetSelectedEvent(Display *dpy, GLXDrawable draw,
	unsigned long *event_mask)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetSelectedEvent(dpy, draw, event_mask);

	_glXGetSelectedEvent(DPY3D, ServerDrawable(dpy, draw), event_mask);

	CATCH();
}

void glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable,
	unsigned long *mask)
{
	glXGetSelectedEvent(dpy, drawable, mask);
}


// Return the 2D X server visual that was attached to the 3D X server FB config
// during a previous call to glXChooseFBConfig(), or pick out an appropriate
// visual and return it.

XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
	XVisualInfo *vis = NULL;

	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXGetVisualFromFBConfig(dpy, config);

		OPENTRACE(glXGetVisualFromFBConfig);  PRARGD(dpy);  PRARGC(config);
		STARTTRACE();

	VisualID vid = 0;
	if(!dpy || !config) goto done;
	vid = glxvisual::getAttachedVisualID(dpy, DefaultScreen(dpy), config);
	if(!vid) goto done;
	vis = glxvisual::visualFromID(dpy, DefaultScreen(dpy), vid);
	if(!vis) goto done;
	vglfaker::setConfigForVisual(dpy, vis, config);

	done:
		STOPTRACE();  PRARGV(vis);  CLOSETRACE();

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

		OPENTRACE(glXIsDirect);  PRARGD(dpy);  PRARGX(ctx);
		STARTTRACE();

	direct = _glXIsDirect(DPY3D, ctx);

		STOPTRACE();  PRARGI(direct);  CLOSETRACE();

	CATCH();
	return direct;
}


// See notes

Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
	Bool retval = False;  const char *renderer = "Unknown";
	VirtualWin *vw;  GLXFBConfig config = 0;

	if(vglfaker::deadYet || vglfaker::getFakerLevel() > 0)
		return _glXMakeCurrent(dpy, drawable, ctx);

	TRY();

	// Find the FB config that was previously hashed to this context when it was
	// created.
	if(ctx) config = ctxhash.findConfig(ctx);
	if(vglfaker::isDisplayExcluded(dpy))
	{
		vglfaker::setExcludeCurrent(true);
		return _glXMakeCurrent(dpy, drawable, ctx);
	}
	vglfaker::setExcludeCurrent(false);

		OPENTRACE(glXMakeCurrent);  PRARGD(dpy);  PRARGX(drawable);  PRARGX(ctx);
		STARTTRACE();

	// glXMakeCurrent() implies a glFinish() on the previous context, which is
	// why we read back the front buffer here if it is dirty.
	GLXDrawable curdraw = _glXGetCurrentDrawable();
	if(_glXGetCurrentContext() && _glXGetCurrentDisplay() == DPY3D
		&& curdraw && winhash.find(curdraw, vw))
	{
		VirtualWin *newvw;
		if(drawable == 0 || !winhash.find(dpy, drawable, newvw)
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

	retval = _glXMakeContextCurrent(DPY3D, drawable, drawable, ctx);
	if(fconfig.trace && retval)
		renderer = (const char *)_glGetString(GL_RENDERER);
	// The pixels in a new off-screen drawable are undefined, so we have to clear
	// it.
	if(winhash.find(drawable, vw)) { vw->clear();  vw->cleanup(); }
	VirtualPixmap *vpm;
	if((vpm = pmhash.find(dpy, drawable)) != NULL)
	{
		vpm->clear();
		vpm->setDirect(direct);
	}

	done:
		STOPTRACE();  PRARGC(config);  PRARGX(drawable);  PRARGS(renderer);
		CLOSETRACE();

	CATCH();
	return retval;
}


Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx)
{
	Bool retval = False;  const char *renderer = "Unknown";
	VirtualWin *vw;  GLXFBConfig config = 0;

	if(vglfaker::deadYet || vglfaker::getFakerLevel() > 0)
		return _glXMakeContextCurrent(dpy, draw, read, ctx);

	TRY();

	if(ctx) config = ctxhash.findConfig(ctx);
	if(vglfaker::isDisplayExcluded(dpy))
	{
		vglfaker::setExcludeCurrent(true);
		return _glXMakeContextCurrent(dpy, draw, read, ctx);
	}
	vglfaker::setExcludeCurrent(false);

		OPENTRACE(glXMakeContextCurrent);  PRARGD(dpy);  PRARGX(draw);
		PRARGX(read);  PRARGX(ctx);  STARTTRACE();

	// glXMakeContextCurrent() implies a glFinish() on the previous context,
	// which is why we read back the front buffer here if it is dirty.
	GLXDrawable curdraw = _glXGetCurrentDrawable();
	if(_glXGetCurrentContext() && _glXGetCurrentDisplay() == DPY3D && curdraw
		&& winhash.find(curdraw, vw))
	{
		VirtualWin *newvw;
		if(draw == 0 || !winhash.find(dpy, draw, newvw)
			|| newvw->getGLXDrawable() != curdraw)
		{
			if(DrawingToFront() || vw->dirty)
				vw->readback(GL_FRONT, false, fconfig.sync);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to an off-screen drawable.
	VirtualWin *drawVW, *readVW;
	int direct = ctxhash.isDirect(ctx);
	if(dpy && (draw || read) && ctx)
	{
		if(!config)
		{
			vglout.PRINTLN("[VGL] WARNING: glXMakeContextCurrent() called with a previously-destroyed context");
			goto done;
		}

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
	retval = _glXMakeContextCurrent(DPY3D, draw, read, ctx);
	if(fconfig.trace && retval)
		renderer = (const char *)_glGetString(GL_RENDERER);
	if(winhash.find(draw, drawVW)) { drawVW->clear();  drawVW->cleanup(); }
	if(winhash.find(read, readVW)) readVW->cleanup();
	VirtualPixmap *vpm;
	if((vpm = pmhash.find(dpy, draw)) != NULL)
	{
		vpm->clear();
		vpm->setDirect(direct);
	}

	done:
		STOPTRACE();  PRARGC(config);  PRARGX(draw);  PRARGX(read);
		PRARGS(renderer);  CLOSETRACE();

	CATCH();
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

		OPENTRACE(glXQueryContext);  PRARGD(dpy);  PRARGX(ctx);
		PRARGIX(attribute);  STARTTRACE();

	retval = _glXQueryContext(DPY3D, ctx, attribute, value);

		STOPTRACE();  if(value) PRARGIX(*value);  CLOSETRACE();

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

		OPENTRACE(glXQueryContextInfoEXT);  PRARGD(dpy);  PRARGX(ctx);
		PRARGIX(attribute);  STARTTRACE();

	retval = _glXQueryContextInfoEXT(DPY3D, ctx, attribute, value);

		STOPTRACE();  if(value) PRARGIX(*value);  CLOSETRACE();

	CATCH();
	return retval;
}


// Hand off to the 3D X server without modification, except that 'draw' is
// replaced with its corresponding off-screen drawable ID and
// GLX_SWAP_INTERVAL_INT is emulated.

void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute,
	unsigned int *value)
{
	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXQueryDrawable(dpy, draw, attribute, value);
		return;
	}

		OPENTRACE(glXQueryDrawable);  PRARGD(dpy);  PRARGX(draw);
		PRARGIX(attribute);  STARTTRACE();

	// GLX_EXT_swap_control attributes
	if(attribute == GLX_SWAP_INTERVAL_EXT && value)
	{
		VirtualWin *vw = NULL;
		if(winhash.find(dpy, draw, vw))
			*value = vw->getSwapInterval();
		else
			*value = 0;
		goto done;
	}
	else if(attribute == GLX_MAX_SWAP_INTERVAL_EXT && value)
	{
		*value = VGL_MAX_SWAP_INTERVAL;
		goto done;
	}

	_glXQueryDrawable(DPY3D, ServerDrawable(dpy, draw), attribute, value);

	done:
		STOPTRACE();  PRARGX(ServerDrawable(dpy, draw));
		if(value) { PRARGIX(*value); }  else { PRARGX(value); }
		CLOSETRACE();

	CATCH();
}

int glXQueryGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf, int attribute,
	unsigned int *value)
{
	glXQueryDrawable(dpy, pbuf, attribute, value);
	return 0;
}


// Hand off to the 3D X server without modification.

Bool glXQueryExtension(Display *dpy, int *error_base, int *event_base)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryExtension(dpy, error_base, event_base);

	return _glXQueryExtension(DPY3D, error_base, event_base);

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

	return _glXQueryVersion(DPY3D, major, minor);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXSelectEvent(dpy, draw, event_mask);

	_glXSelectEvent(DPY3D, ServerDrawable(dpy, draw), event_mask);

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
	VirtualWin *vw = NULL;
	static Timer timer;  Timer sleepTimer;
	static double err = 0.;  static bool first = true;

	TRY();

	if(IS_EXCLUDED(dpy))
	{
		_glXSwapBuffers(dpy, drawable);
		return;
	}

		OPENTRACE(glXSwapBuffers);  PRARGD(dpy);  PRARGX(drawable);  STARTTRACE();

	fconfig.flushdelay = 0.;
	if(winhash.find(dpy, drawable, vw))
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
	else _glXSwapBuffers(DPY3D, drawable);

		STOPTRACE();  if(vw) { PRARGX(vw->getGLXDrawable()); }
		CLOSETRACE();

	CATCH();
}


// Hand off to the 3D X server without modification, except that 'drawable' is
// replaced with its corresponding off-screen drawable ID.

Bool glXJoinSwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint group)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXJoinSwapGroupNV(dpy, drawable, group);

	return _glXJoinSwapGroupNV(DPY3D, ServerDrawable(dpy, drawable), group);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

Bool glXBindSwapBarrierNV(Display *dpy, GLuint group, GLuint barrier)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXBindSwapBarrierNV(dpy, group, barrier);

	return _glXBindSwapBarrierNV(DPY3D, group, barrier);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification, except that 'drawable' is
// replaced with its corresponding off-screen drawable ID.

Bool glXQuerySwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint *group,
	GLuint *barrier)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQuerySwapGroupNV(dpy, drawable, group, barrier);

	return _glXQuerySwapGroupNV(DPY3D, ServerDrawable(dpy, drawable), group,
		barrier);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

Bool glXQueryMaxSwapGroupsNV(Display *dpy, int screen, GLuint *maxGroups,
	GLuint *maxBarriers)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryMaxSwapGroupsNV(dpy, screen, maxGroups, maxBarriers);

	return _glXQueryMaxSwapGroupsNV(DPY3D, DefaultScreen(DPY3D), maxGroups,
		maxBarriers);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

Bool glXQueryFrameCountNV(Display *dpy, int screen, GLuint *count)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXQueryFrameCountNV(dpy, screen, count);

	return _glXQueryFrameCountNV(DPY3D, DefaultScreen(DPY3D), count);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

Bool glXResetFrameCountNV(Display *dpy, int screen)
{
	TRY();

	if(IS_EXCLUDED(dpy))
		return _glXResetFrameCountNV(dpy, screen);

	return _glXResetFrameCountNV(DPY3D, DefaultScreen(DPY3D));

	CATCH();
	return False;
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

		OPENTRACE(glXSwapIntervalEXT);  PRARGD(dpy);  PRARGX(drawable);
		PRARGI(interval);  STARTTRACE();

	if(interval > VGL_MAX_SWAP_INTERVAL) interval = VGL_MAX_SWAP_INTERVAL;
	if(interval < 0)
		// NOTE:  Technically, this should trigger a BadValue error, but nVidia's
		// implementation doesn't, so we emulate their behavior.
		interval = 1;

	VirtualWin *vw = NULL;
	if(winhash.find(dpy, drawable, vw))
		vw->setSwapInterval(interval);
	// NOTE:  Technically, a BadWindow error should be triggered if drawable
	// isn't a GLX window, but nVidia's implementation doesn't, so we emulate
	// their behavior.

		STOPTRACE();  CLOSETRACE();

	CATCH();
}


// This is basically the same as calling glXSwapIntervalEXT() with the current
// drawable.

int glXSwapIntervalSGI(int interval)
{
	int retval = 0;

	if(vglfaker::getExcludeCurrent()) return _glXSwapIntervalSGI(interval);

		OPENTRACE(glXSwapIntervalSGI);  PRARGI(interval);  STARTTRACE();

	TRY();

	VirtualWin *vw = NULL;  GLXDrawable draw = _glXGetCurrentDrawable();
	if(interval < 0) retval = GLX_BAD_VALUE;
	else if(!draw || !winhash.find(draw, vw))
		retval = GLX_BAD_CONTEXT;
	else vw->setSwapInterval(interval);

	CATCH();

		STOPTRACE();  CLOSETRACE();

	return retval;
}


#include "xfonts.c"

// We use a tweaked out version of the Mesa glXUseXFont() implementation.

void glXUseXFont(Font font, int first, int count, int list_base)
{
	if(vglfaker::getExcludeCurrent())
	{
		_glXUseXFont(font, first, count, list_base);  return;
	}

	TRY();

		OPENTRACE(glXUseXFont);  PRARGX(font);  PRARGI(first);  PRARGI(count);
		PRARGI(list_base);  STARTTRACE();

	Fake_glXUseXFont(font, first, count, list_base);

		STOPTRACE();  CLOSETRACE();

	CATCH();
}


}  // extern "C"
