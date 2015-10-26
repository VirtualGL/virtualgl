/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011-2015 D. R. Commander
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

#include <string.h>
#include <limits.h>
#include "Error.h"
#include "vglutil.h"
#define GLX_GLXEXT_PROTOTYPES
#include "ConfigHash.h"
#include "ContextHash.h"
#include "GLXDrawableHash.h"
#include "PixmapHash.h"
#include "ReverseConfigHash.h"
#include "VisualHash.h"
#include "WindowHash.h"
#include "DisplayHash.h"
#include "rr.h"
#include "faker.h"
#include "glxvisual.h"

using namespace vglutil;
using namespace vglserver;


#define dpy3DIsCurrent() (_glXGetCurrentDisplay()==_dpy3D)


// This emulates the behavior of the nVidia drivers
#define VGL_MAX_SWAP_INTERVAL 8


// Applications will sometimes use X11 functions to obtain a list of 2D X
// server visuals, then pass one of those visuals to glXCreateContext(),
// glXGetConfig(), etc.  Since the visual didn't come from glXChooseVisual(),
// VGL has no idea what properties the app is looking for, so if no 3D X server
// FB config is already hashed to the visual, we have to create one using
// default attributes.

#define testattrib(attrib, index, min, max) {  \
	if(!strcmp(argv[i], #attrib) && i<argc-1) {  \
		int temp=atoi(argv[++i]);  \
		if(temp>=min && temp<=max) {  \
			attribs[index++]=attrib;  \
			attribs[index++]=temp;  \
		}  \
	}  \
}

static GLXFBConfig matchConfig(Display *dpy, XVisualInfo *vis,
	bool preferSingleBuffer=false, bool pixmap=false)
{
	GLXFBConfig config=0, *configs=NULL;  int n=0;

	if(!dpy || !vis) return 0;
	if(!(config=vishash.getConfig(dpy, vis))
		&& !(config=vishash.mostRecentConfig(dpy, vis)))
	{
		// Punt.  We can't figure out where the visual came from
		int defaultAttribs[]={GLX_DOUBLEBUFFER, 1, GLX_RED_SIZE, 8,
			GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_STEREO, 0, GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
			GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_DEPTH_SIZE, 1,
			GLX_STENCIL_SIZE, 8, None};
		int attribs[256];

		if(pixmap || fconfig.drawable==RRDRAWABLE_PIXMAP)
			defaultAttribs[13]=GLX_PIXMAP_BIT|GLX_WINDOW_BIT;
		memset(attribs, 0, sizeof(attribs));
		memcpy(attribs, defaultAttribs, sizeof(defaultAttribs));
		if(glxvisual::visAttrib2D(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_STEREO))
			attribs[11]=1;

		if(vis->c_class==DirectColor) attribs[15]=GLX_DIRECT_COLOR;

		// If we're creating a GLX pixmap and we can't determine an exact FB config
		// to map to the visual, then we will make the pixmap single-buffered.
		if(preferSingleBuffer) attribs[1]=0;

		// Allow the default FB config attribs to be manually specified.  This is
		// necessary to support apps that implement their own visual selection
		// mechanisms.  Since those apps don't use glXChooseVisual(), VirtualGL has
		// no idea what 3D visual attributes they need, and thus it is necessary
		// to give it a hint using this environment variable.
		if(strlen(fconfig.defaultfbconfig)>0)
		{
			char *str=strdup(fconfig.defaultfbconfig);
			if(!str) _throwunix();
			char *argv[512];  int argc=0;
			char *arg=strtok(str, " \t,");
			while(arg && argc<512)
			{
				argv[argc]=arg;  argc++;
				arg=strtok(NULL, " \t,");
			}
			for(int i=0, j=20; i<argc && j<256; i++)
			{
				int index;
				index=2;
				testattrib(GLX_RED_SIZE, index, 0, INT_MAX);
				index=4;
				testattrib(GLX_GREEN_SIZE, index, 0, INT_MAX);
				index=6;
				testattrib(GLX_BLUE_SIZE, index, 0, INT_MAX);
				index=16;
				testattrib(GLX_DEPTH_SIZE, index, 0, INT_MAX);
				index=18;
				testattrib(GLX_STENCIL_SIZE, index, 0, INT_MAX);
				testattrib(GLX_ALPHA_SIZE, j, 0, INT_MAX);
				testattrib(GLX_AUX_BUFFERS, j, 0, INT_MAX);
				testattrib(GLX_ACCUM_RED_SIZE, j, 0, INT_MAX);
				testattrib(GLX_ACCUM_GREEN_SIZE, j, 0, INT_MAX);
				testattrib(GLX_ACCUM_BLUE_SIZE, j, 0, INT_MAX);
				testattrib(GLX_ACCUM_ALPHA_SIZE, j, 0, INT_MAX);
				testattrib(GLX_SAMPLE_BUFFERS, j, 0, INT_MAX);
				testattrib(GLX_SAMPLES, j, 0, INT_MAX);
			}
			free(str);
		}

		configs=glXChooseFBConfig(_dpy3D, DefaultScreen(_dpy3D), attribs,
			&n);
		if((!configs || n<1) && attribs[11])
		{
			attribs[11]=0;
			configs=glXChooseFBConfig(_dpy3D, DefaultScreen(_dpy3D), attribs,
				&n);
		}
		if((!configs || n<1) && attribs[1])
		{
			attribs[1]=0;
			configs=glXChooseFBConfig(_dpy3D, DefaultScreen(_dpy3D), attribs,
				&n);
		}
		if(!configs || n<1) return 0;
		config=configs[0];
		XFree(configs);
		if(config)
		{
			vishash.add(dpy, vis, config);
			cfghash.add(dpy, config, vis->visualid);
		}
	}
	return config;
}


// Return the 2D X server visual that was previously hashed to 'config', or
// find and return an appropriate 2D X server visual otherwise.

static VisualID matchVisual(Display *dpy, GLXFBConfig config)
{
	VisualID vid=0;
	if(!dpy || !config) return 0;
	int screen=DefaultScreen(dpy);
	if(!(vid=cfghash.getVisual(dpy, config)))
	{
		// If we get here, then the app is using an FB config that was not obtained
		// through glXChooseFBConfig(), so we have no idea what attributes it is
		// looking for.  We first try to match the FB config with a 2D X Server
		// visual that has the same class, depth, and stereo properties.
		XVisualInfo *vis=_glXGetVisualFromFBConfig(_dpy3D, config);
		if(vis)
		{
			if(vis->depth>=24
				&& (vis->c_class==TrueColor || vis->c_class==DirectColor))
				vid=glxvisual::matchVisual2D(dpy, screen, vis->depth, vis->c_class, 0,
					glxvisual::visAttrib3D(config, GLX_STEREO), 0);
			XFree(vis);
		}
		// Failing that, we try to find a 24-bit TrueColor visual with the same
		// stereo properties.
		if(!vid)
			vid=glxvisual::matchVisual2D(dpy, screen, 24, TrueColor, 0,
				glxvisual::visAttrib3D(config, GLX_STEREO), 0);
		// Failing that, we try to find a 24-bit TrueColor mono visual.
		if(!vid)
			vid=glxvisual::matchVisual2D(dpy, screen, 24, TrueColor, 0, 0, 0);
	}
	if(vid) cfghash.add(dpy, config, vid);
	return vid;
}


// If GLXDrawable is a window ID, then return the ID for its corresponding
// off-screen drawable (if applicable.)

GLXDrawable ServerDrawable(Display *dpy, GLXDrawable draw)
{
	VirtualWin *vw=NULL;
	if(winhash.find(dpy, draw, vw)) return vw->getGLXDrawable();
	else return draw;
}


void setWMAtom(Display *dpy, Window win)
{
	Atom *protocols=NULL, *newProtocols=NULL;  int count=0;

	Atom deleteAtom=XInternAtom(dpy, "WM_DELETE_WINDOW", True);
	if(!deleteAtom) goto bailout;

	if(XGetWMProtocols(dpy, win, &protocols, &count) && protocols && count>0)
	{
		for(int i=0; i<count; i++)
			if(protocols[i]==deleteAtom)
			{
				XFree(protocols);  return;
			}
		newProtocols=(Atom *)malloc(sizeof(Atom)*(count+1));
		if(!newProtocols) goto bailout;
		for(int i=0; i<count; i++)
			newProtocols[i]=protocols[i];
		newProtocols[count]=deleteAtom;
		if(!XSetWMProtocols(dpy, win, newProtocols, count+1)) goto bailout;
		XFree(protocols);
		free(newProtocols);
	}
	else if(!XSetWMProtocols(dpy, win, &deleteAtom, 1)) goto bailout;
	return;

	bailout:
	if(protocols) XFree(protocols);
	if(newProtocols) free(newProtocols);
	static bool alreadyWarned=false;
	if(!alreadyWarned)
	{
		if(fconfig.verbose)
			vglout.print("[VGL] WARNING: Could not set WM_DELETE_WINDOW on window 0x%.8x\n",
				win);
		alreadyWarned=true;
	}
}



// Interposed GLX functions

extern "C" {

// Return a set of FB configs from the 3D X server that support off-screen
// rendering and contain the desired GLX attributes.

GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
	const int *attrib_list, int *nelements)
{
	GLXFBConfig *configs=NULL;
	bool fbcidreq=false;

	TRY();

	// If this is called internally from within another GLX function, then use
	// the real function.
	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXChooseFBConfig(dpy, screen, attrib_list, nelements);

	// If 'attrib_list' specifies properties for transparent overlay rendering,
	// then hand off to the 2D X server.
	if(attrib_list)
	{
		bool overlayreq=false;
		for(int i=0; attrib_list[i]!=None && i<=254; i+=2)
		{
			if(attrib_list[i]==GLX_LEVEL && attrib_list[i+1]==1)
				overlayreq=true;
			if(attrib_list[i]==GLX_FBCONFIG_ID) fbcidreq=true;
		}
		if(overlayreq)
		{
			int dummy;
			if(!_XQueryExtension(dpy, "GLX", &dummy, &dummy, &dummy))
				configs=NULL;
			else configs=_glXChooseFBConfig(dpy, screen, attrib_list, nelements);
			if(configs && nelements && *nelements)
			{
				for(int i=0; i<*nelements; i++) rcfghash.add(dpy, configs[i]);
			}
			return configs;
		}
	}

		opentrace(glXChooseFBConfig);  prargd(dpy);  prargi(screen);
		prargal13(attrib_list);  starttrace();

	int depth=24, c_class=TrueColor, level=0, stereo=0, trans=0, temp;
	if(!nelements) nelements=&temp;
	*nelements=0;

	// If no attributes are specified, return all FB configs.  If GLX_FBCONFIG_ID
	// is specified, ignore all other attributes.
	if(!attrib_list || fbcidreq)
	{
		configs=_glXChooseFBConfig(_dpy3D, DefaultScreen(_dpy3D),
			attrib_list, nelements);
		goto done;
	}

	// Modify the attributes so that only FB configs appropriate for off-screen
	// rendering are considered.
	else configs=glxvisual::configsFromVisAttribs(attrib_list, c_class, level,
		stereo, trans, *nelements, true);

	if(configs && *nelements)
	{
		int nv=0;

		// Get a matching visual from the 2D X server and hash it to every FB
		// config we just obtained.
		for(int i=0; i<*nelements; i++)
		{
			int d=depth;
			XVisualInfo *vis=_glXGetVisualFromFBConfig(_dpy3D, configs[i]);
			if(vis)
			{
				if(vis->depth==32) d=32;
				XFree(vis);
			}

			// Find an appropriate matching visual on the 2D X server.
			VisualID vid=glxvisual::matchVisual2D(dpy, screen, d, c_class, level,
				stereo, trans);
			if(!vid)
			{
				if(depth==32) vid=glxvisual::matchVisual2D(dpy, screen, 24,
					c_class, level, stereo, trans);
				if(!vid) continue;
			}
			nv++;
			cfghash.add(dpy, configs[i], vid);
		}
		if(!nv) { *nelements=0;  XFree(configs);  configs=NULL; }
	}

	done:
		stoptrace();
		if(configs && nelements)
		{
			if(*nelements)
				for(int i=0; i<*nelements; i++)
					vglout.print("configs[%d]=0x%.8lx(0x%.2x) ", i,
						(unsigned long)configs[i], configs[i]? _FBCID(configs[i]):0);
			prargi(*nelements);
		}
		closetrace();

	CATCH();
	return configs;
}

GLXFBConfigSGIX *glXChooseFBConfigSGIX (Display *dpy, int screen,
	int *attrib_list, int *nelements)
{
	return glXChooseFBConfig(dpy, screen, attrib_list, nelements);
}


// Obtain a 3D X server FB config that supports off-screen rendering and has
// the desired set of attributes, match it to an appropriate 2D X server
// visual, hash the two, and return the visual.

XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *attrib_list)
{
	XVisualInfo *vis=NULL;  GLXFBConfig config=0;
	static bool alreadyWarned=false;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXChooseVisual(dpy, screen, attrib_list);

	// If 'attrib_list' specifies properties for transparent overlay rendering,
	// then hand off to the 2D X server.
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
			int dummy;
			if(!_XQueryExtension(dpy, "GLX", &dummy, &dummy, &dummy))
				vis=NULL;
			else vis=_glXChooseVisual(dpy, screen, attrib_list);
			return vis;
		}
	}

		opentrace(glXChooseVisual);  prargd(dpy);  prargi(screen);
		prargal11(attrib_list);  starttrace();

	// Use the specified set of GLX attributes to obtain an FB config on the 3D X
	// server suitable for off-screen rendering
	GLXFBConfig *configs=NULL, prevConfig;  int n=0;
	int depth=24, c_class=TrueColor, level=0, stereo=0, trans=0;
	VisualID vid=0;  XVisualInfo *vtemp=NULL;
	if(!dpy || !attrib_list) goto done;
	if(!(configs=glxvisual::configsFromVisAttribs(attrib_list, c_class, level,
		stereo, trans, n)) || n<1)
	{
		if(!alreadyWarned && fconfig.verbose)
		{
			alreadyWarned=true;
			vglout.println("[VGL] WARNING: VirtualGL attempted and failed to obtain a true color visual on");
			vglout.println("[VGL]    the 3D X server %s suitable for off-screen rendering.", fconfig.localdpystring);
			vglout.println("[VGL]    This is normal if the 3D application is probing for visuals with");
			vglout.println("[VGL]    certain capabilities, but if the app fails to start, then make sure");
			vglout.println("[VGL]    that the 3D X server is configured for true color and has accelerated");
			vglout.println("[VGL]    3D drivers installed.");
		}
		goto done;
	}
	config=configs[0];
	XFree(configs);
	vtemp=_glXGetVisualFromFBConfig(_dpy3D, config);
	if(vtemp)
	{
		if(vtemp->depth==32) depth=32;
		XFree(vtemp);
	}

	// Find an appropriate matching visual on the 2D X server.
	vid=glxvisual::matchVisual2D(dpy, screen, depth, c_class, level, stereo,
		trans);
	if(!vid)
	{
		if(depth==32) vid=glxvisual::matchVisual2D(dpy, screen, 24, c_class, level,
			stereo, trans);
		if(!vid) goto done;
	}
	vis=glxvisual::visualFromID(dpy, screen, vid);
	if(!vis) goto done;

	if((prevConfig=vishash.getConfig(dpy, vis))
		&& _FBCID(config)!=_FBCID(prevConfig) && fconfig.trace)
		vglout.println("[VGL] WARNING: Visual 0x%.2x was previously mapped to FB config 0x%.2x and is now mapped to 0x%.2x\n",
			vis->visualid, _FBCID(prevConfig), _FBCID(config));

	// Hash the FB config and the visual so that we can look up the FB config
	// whenever the app subsequently passes the visual to glXCreateContext() or
	// other functions.
	vishash.add(dpy, vis, config);

	done:
		stoptrace();  prargv(vis);  prargc(config);  closetrace();

	CATCH();
	return vis;
}


// If src or dst is an overlay context, hand off to the 2D X server.
// Otherwise, hand off to the 3D X server without modification.

void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst,
	unsigned long mask)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXCopyContext(dpy, src, dst, mask);

	bool srcOverlay=false, dstOverlay=false;
	if(ctxhash.isOverlay(src)) srcOverlay=true;
	if(ctxhash.isOverlay(dst)) dstOverlay=true;
	if(srcOverlay && dstOverlay)
	{
		_glXCopyContext(dpy, src, dst, mask);  return;
	}
	else if(srcOverlay!=dstOverlay)
		_throw("glXCopyContext() cannot copy between overlay and non-overlay contexts");
	_glXCopyContext(_dpy3D, src, dst, mask);

	CATCH();
}


// Create a GLX context on the 3D X server suitable for off-screen rendering.

GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
	GLXContext share_list, Bool direct)
{
	GLXContext ctx=0;  GLXFBConfig config=0;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXCreateContext(dpy, vis, share_list, direct);

	if(!fconfig.allowindirect) direct=True;

	// If 'vis' is an overlay visual, hand off to the 2D X server.
	if(vis)
	{
		int level=glxvisual::visAttrib2D(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_LEVEL);
		int trans=(glxvisual::visAttrib2D(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_TRANSPARENT_TYPE)==GLX_TRANSPARENT_INDEX);
		if(level && trans)
		{
			int dummy;
			if(!_XQueryExtension(dpy, "GLX", &dummy, &dummy, &dummy))
				ctx=NULL;
			else ctx=_glXCreateContext(dpy, vis, share_list, direct);
			if(ctx) ctxhash.add(ctx, (GLXFBConfig)-1, -1);
			return ctx;
		}
	}

		opentrace(glXCreateContext);  prargd(dpy);  prargv(vis);
		prargx(share_list);  prargi(direct);  starttrace();

	// If 'vis' was obtained through a previous call to glXChooseVisual(), find
	// the corresponding FB config in the hash.  Otherwise, we have to fall back
	// to using a default FB config returned from matchConfig().
	if(!(config=matchConfig(dpy, vis)))
		_throw("Could not obtain RGB visual on the server suitable for off-screen rendering.");
	ctx=_glXCreateNewContext(_dpy3D, config, GLX_RGBA_TYPE, share_list,
		direct);
	if(ctx)
	{
		int newctxIsDirect=_glXIsDirect(_dpy3D, ctx);
		if(!newctxIsDirect && direct)
		{
			vglout.println("[VGL] WARNING: The OpenGL rendering context obtained on X display");
			vglout.println("[VGL]    %s is indirect, which may cause performance to suffer.",
				DisplayString(_dpy3D));
			vglout.println("[VGL]    If %s is a local X display, then the framebuffer device",
				DisplayString(_dpy3D));
			vglout.println("[VGL]    permissions may be set incorrectly.");
		}
		// Hash the FB config to the context so we can use it in subsequent calls
		// to glXMake[Context]Current().
		ctxhash.add(ctx, config, newctxIsDirect);
	}

		stoptrace();  prargc(config);  prargx(ctx);  closetrace();

	CATCH();
	return ctx;
}


GLXContext glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config,
	GLXContext share_context, Bool direct, const int *attribs)
{
	GLXContext ctx=0;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXCreateContextAttribsARB(dpy, config, share_context, direct,
			attribs);

	if(!fconfig.allowindirect) direct=True;

	// Overlay config.  Hand off to 2D X server.
	if(rcfghash.isOverlay(dpy, config))
	{
		ctx=_glXCreateContextAttribsARB(dpy, config, share_context, direct,
			attribs);
		if(ctx) ctxhash.add(ctx, (GLXFBConfig)-1, -1);
		return ctx;
	}

		opentrace(glXCreateContextAttribsARB);  prargd(dpy);  prargc(config);
		prargx(share_context);  prargi(direct);  prargal13(attribs);
		starttrace();

	CHECKSYM_NONFATAL(glXCreateContextAttribsARB)
	if((!attribs || attribs[0]==None) && !__glXCreateContextAttribsARB)
		ctx=_glXCreateNewContext(_dpy3D, config, GLX_RGBA_TYPE, share_context,
			direct);
	else
		ctx=_glXCreateContextAttribsARB(_dpy3D, config, share_context, direct,
			attribs);
	if(ctx)
	{
		int newctxIsDirect=_glXIsDirect(_dpy3D, ctx);
		if(!newctxIsDirect && direct)
		{
			vglout.println("[VGL] WARNING: The OpenGL rendering context obtained on X display");
			vglout.println("[VGL]    %s is indirect, which may cause performance to suffer.",
				DisplayString(_dpy3D));
			vglout.println("[VGL]    If %s is a local X display, then the framebuffer device",
				DisplayString(_dpy3D));
			vglout.println("[VGL]    permissions may be set incorrectly.");
		}
		ctxhash.add(ctx, config, newctxIsDirect);
	}

		stoptrace();  prargx(ctx);  closetrace();

	CATCH();
	return ctx;
}


GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config,
	int render_type, GLXContext share_list, Bool direct)
{
	GLXContext ctx=0;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXCreateNewContext(dpy, config, render_type, share_list, direct);

	if(!fconfig.allowindirect) direct=True;

	 // Overlay config.  Hand off to 2D X server.
	if(rcfghash.isOverlay(dpy, config))
	{
		ctx=_glXCreateNewContext(dpy, config, render_type, share_list, direct);
		if(ctx) ctxhash.add(ctx, (GLXFBConfig)-1, -1);
		return ctx;
	}

		opentrace(glXCreateNewContext);  prargd(dpy);  prargc(config);
		prargi(render_type);  prargx(share_list);  prargi(direct);  starttrace();

	ctx=_glXCreateNewContext(_dpy3D, config, GLX_RGBA_TYPE, share_list, direct);
	if(ctx)
	{
		int newctxIsDirect=_glXIsDirect(_dpy3D, ctx);
		if(!newctxIsDirect && direct)
		{
			vglout.println("[VGL] WARNING: The OpenGL rendering context obtained on X display");
			vglout.println("[VGL]    %s is indirect, which may cause performance to suffer.",
				DisplayString(_dpy3D));
			vglout.println("[VGL]    If %s is a local X display, then the framebuffer device",
				DisplayString(_dpy3D));
			vglout.println("[VGL]    permissions may be set incorrectly.");
		}
		ctxhash.add(ctx, config, newctxIsDirect);
	}

		stoptrace();  prargx(ctx);  closetrace();

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
	GLXPbuffer pb=0;

	TRY();

	if(dpyhash.find(dpy))
		return _glXCreatePbuffer(dpy, config, attrib_list);

		opentrace(glXCreatePbuffer);  prargd(dpy);  prargc(config);
		prargal13(attrib_list);  starttrace();

	pb=_glXCreatePbuffer(_dpy3D, config, attrib_list);
	if(dpy && pb) glxdhash.add(pb, dpy);

		stoptrace();  prargx(pb);  closetrace();

	CATCH();
	return pb;
}

GLXPbuffer glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config,
	unsigned int width, unsigned int height, int *attrib_list)
{
	int attribs[257], j=0;
	if(attrib_list)
	{
		for(int i=0; attrib_list[i]!=None && i<=254; i+=2)
		{
			attribs[j++]=attrib_list[i];  attribs[j++]=attrib_list[i+1];
		}
	}
	attribs[j++]=GLX_PBUFFER_WIDTH;  attribs[j++]=width;
	attribs[j++]=GLX_PBUFFER_HEIGHT;  attribs[j++]=height;
	attribs[j]=None;
	return glXCreatePbuffer(dpy, config, attribs);
}


// Pixmap rendering in VirtualGL is implemented by redirecting rendering into
// a "3D pixmap" stored on the 3D X server.  Thus, we create a 3D pixmap, hash
// it to the "2D pixmap", and return the 3D pixmap handle.

GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pm)
{
	GLXPixmap drawable=0;  GLXFBConfig config=0;
	int x=0, y=0;  unsigned int width=0, height=0, depth=0;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXCreateGLXPixmap(dpy, vis, pm);

	// Not sure whether a transparent pixmap has any meaning, but in any case,
	// we have to hand it off to the 2D X server.
	if(vis)
	{
		int level=glxvisual::visAttrib2D(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_LEVEL);
		int trans=(glxvisual::visAttrib2D(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_TRANSPARENT_TYPE)==GLX_TRANSPARENT_INDEX);
		if(level && trans)
		{
			int dummy;
			if(!_XQueryExtension(dpy, "GLX", &dummy, &dummy, &dummy))
				drawable=0;
			else drawable=_glXCreateGLXPixmap(dpy, vis, pm);
			return drawable;
		}
	}

		opentrace(glXCreateGLXPixmap);  prargd(dpy);  prargv(vis);  prargx(pm);
		starttrace();

	Window root;  unsigned int bw;
	_XGetGeometry(dpy, pm, &root, &x, &y, &width, &height, &bw, &depth);
	if(!(config=matchConfig(dpy, vis, true, true)))
		_throw("Could not obtain pixmap-capable RGB visual on the server");
	VirtualPixmap *vpm=new VirtualPixmap(dpy, vis, pm);
	if(vpm)
	{
		// Hash the VirtualPixmap instance to the 2D pixmap and also hash the 2D X
		// display handle to the 3D pixmap.
		vpm->init(width, height, depth, config, NULL);
		pmhash.add(dpy, pm, vpm);
		glxdhash.add(vpm->getGLXDrawable(), dpy);
		drawable=vpm->getGLXDrawable();
	}

		stoptrace();  prargi(x);  prargi(y);  prargi(width);  prargi(height);
		prargi(depth);  prargc(config);  prargx(drawable);  closetrace();

	CATCH();
	return drawable;
}


GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pm,
	const int *attribs)
{
	GLXPixmap drawable=0;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy) || rcfghash.isOverlay(dpy, config))
		return _glXCreatePixmap(dpy, config, pm, attribs);

		opentrace(glXCreatePixmap);  prargd(dpy);  prargc(config);  prargx(pm);
		starttrace();

	Window root;  int x, y;  unsigned int w, h, bw, d;
	_XGetGeometry(dpy, pm, &root, &x, &y, &w, &h, &bw, &d);

	VisualID vid=matchVisual(dpy, config);
	VirtualPixmap *vpm=NULL;
	if(vid)
	{
		XVisualInfo *vis=glxvisual::visualFromID(dpy, DefaultScreen(dpy), vid);
		if(vis) vpm=new VirtualPixmap(dpy, vis, pm);
	}
	if(vpm)
	{
		vpm->init(w, h, d, config, attribs);
		pmhash.add(dpy, pm, vpm);
		glxdhash.add(vpm->getGLXDrawable(), dpy);
		drawable=vpm->getGLXDrawable();
	}

		stoptrace();  prargi(x);  prargi(y);  prargi(w);  prargi(h);
		prargi(d);  prargx(drawable);  closetrace();

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
	VirtualWin *vw=NULL;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXCreateWindow(dpy, config, win, attrib_list);

	// Overlay config.  Hand off to 2D X server.
	if(rcfghash.isOverlay(dpy, config))
	{
		GLXWindow glxw=_glXCreateWindow(dpy, config, win, attrib_list);
		winhash.setOverlay(dpy, glxw);
		return glxw;
	}

		opentrace(glXCreateWindow);  prargd(dpy);  prargc(config);  prargx(win);
		starttrace();

	XSync(dpy, False);
	vw=winhash.initVW(dpy, win, config);
	if(!vw && !glxdhash.getCurrentDisplay(win))
	{
		// Apparently win was created in another process or using XCB.
		if(!is3D(dpy))
		{
			winhash.add(dpy, win);
			vw=winhash.initVW(dpy, win, config);
		}
	}
	if(!vw)
		_throw("Cannot create virtual window for specified X window");

		stoptrace();  if(vw) { prargx(vw->getGLXDrawable()); }  closetrace();

	CATCH();
	return win;  // Make the client store the original window handle, which we
               // use to find the off-screen drawable in the hash
}


// When the context is destroyed, remove it from the context-to-FB config hash.

void glXDestroyContext(Display* dpy, GLXContext ctx)
{
	TRY();

	if(ctxhash.isOverlay(ctx) || dpyhash.find(dpy))
	{
		_glXDestroyContext(dpy, ctx);  return;
	}

		opentrace(glXDestroyContext);  prargd(dpy);  prargx(ctx);  starttrace();

	ctxhash.remove(ctx);
	_glXDestroyContext(_dpy3D, ctx);

		stoptrace();  closetrace();

	CATCH();
}


// When the Pbuffer is destroyed, remove it from the GLX drawable-to-2D display
// handle hash.

void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
	TRY();

	if(dpyhash.find(dpy)) { _glXDestroyPbuffer(dpy, pbuf);  return; }

		opentrace(glXDestroyPbuffer);  prargd(dpy);  prargx(pbuf);  starttrace();

	_glXDestroyPbuffer(_dpy3D, pbuf);
	if(pbuf) glxdhash.remove(pbuf);

		stoptrace();  closetrace();

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

	if(is3D(dpy) || dpyhash.find(dpy))
	{
		_glXDestroyGLXPixmap(dpy, pix);  return;
	}

		opentrace(glXDestroyGLXPixmap);  prargd(dpy);  prargx(pix);  starttrace();

	VirtualPixmap *vpm=pmhash.find(dpy, pix);
	if(vpm && vpm->isInit()) vpm->readback();

	if(pix) glxdhash.remove(pix);
	if(dpy && pix) pmhash.remove(dpy, pix);

		stoptrace();  closetrace();

	CATCH();
}


void glXDestroyPixmap(Display *dpy, GLXPixmap pix)
{
	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
	{
		_glXDestroyPixmap(dpy, pix);  return;
	}

		opentrace(glXDestroyPixmap);  prargd(dpy);  prargx(pix);  starttrace();

	VirtualPixmap *vpm=pmhash.find(dpy, pix);
	if(vpm && vpm->isInit()) vpm->readback();

	if(pix) glxdhash.remove(pix);
	if(dpy && pix) pmhash.remove(dpy, pix);

		stoptrace();  closetrace();

	CATCH();
}


// 'win' is really an off-screen drawable ID, so the window hash matches it to
// the corresponding VirtualWin instance and shuts down that instance.

void glXDestroyWindow(Display *dpy, GLXWindow win)
{
	TRY();

	if(is3D(dpy) || dpyhash.find(dpy) || winhash.isOverlay(dpy, win))
	{
		_glXDestroyWindow(dpy, win);  return;
	}

		opentrace(glXDestroyWindow);  prargd(dpy);  prargx(win);  starttrace();

	winhash.remove(dpy, win);

		stoptrace();  closetrace();

	CATCH();
}


// Hand off to the 2D X server (overlay rendering) or the 3D X server (opaque
// rendering) without modification.

void glXFreeContextEXT(Display *dpy, GLXContext ctx)
{
	TRY();

	if(dpyhash.find(dpy) || ctxhash.isOverlay(ctx))
	{
		_glXFreeContextEXT(dpy, ctx);  return;
	}
	_glXFreeContextEXT(_dpy3D, ctx);

	CATCH();
}


// Since VirtualGL is effectively its own implementation of GLX, it needs to
// properly report the extensions and GLX version it supports.

#define VGL_GLX_EXTENSIONS \
	"GLX_ARB_get_proc_address GLX_ARB_multisample GLX_EXT_visual_info GLX_EXT_visual_rating GLX_SGI_make_current_read GLX_SGIX_fbconfig GLX_SGIX_pbuffer GLX_SUN_get_transparent_index GLX_EXT_texture_from_pixmap GLX_EXT_swap_control GLX_SGI_swap_control"
#define VGL_GLX_ARB_CTX_EXTENSIONS \
	" GLX_ARB_create_context GLX_ARB_create_context_profile"
static const char *glxextensions=VGL_GLX_EXTENSIONS VGL_GLX_ARB_CTX_EXTENSIONS;
static const char *glxextensions_no_arb_ctx=VGL_GLX_EXTENSIONS;

static const char *getGLXExtensions(void)
{
	CHECKSYM_NONFATAL(glXCreateContextAttribsARB)
	if(__glXCreateContextAttribsARB)
		return glxextensions;
	else
		return glxextensions_no_arb_ctx;
}


const char *glXGetClientString(Display *dpy, int name)
{
	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXGetClientString(dpy, name);

	if(name==GLX_EXTENSIONS) return getGLXExtensions();
	else if(name==GLX_VERSION) return "1.4";
	else if(name==GLX_VENDOR)
	{
		if(strlen(fconfig.glxvendor)>0) return fconfig.glxvendor;
		else return __APPNAME;
	}

	CATCH();
	return NULL;
}


// For the specified 2D X server visual, return a property from the
// corresponding 3D X server FB config.

int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
	GLXFBConfig config;  int retval=0;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXGetConfig(dpy, vis, attrib, value);

	if(!dpy || !vis || !value)
		return GLX_BAD_VALUE;

	// If 'vis' is an overlay visual, hand off to the 2D X server.
	int level=glxvisual::visAttrib2D(dpy, DefaultScreen(dpy), vis->visualid,
		GLX_LEVEL);
	int trans=(glxvisual::visAttrib2D(dpy, DefaultScreen(dpy), vis->visualid,
		GLX_TRANSPARENT_TYPE)==GLX_TRANSPARENT_INDEX);
	if(level && trans && attrib!=GLX_LEVEL && attrib!=GLX_TRANSPARENT_TYPE)
	{
		int dummy;
		if(!_XQueryExtension(dpy, "GLX", &dummy, &dummy, &dummy))
			retval=GLX_NO_EXTENSION;
		else retval=_glXGetConfig(dpy, vis, attrib, value);
		return retval;
	}

		opentrace(glXGetConfig);  prargd(dpy);  prargv(vis);  prargix(attrib);
		starttrace();

	// If 'vis' was obtained through a previous call to glXChooseVisual(), find
	// the corresponding FB config in the hash.  Otherwise, we have to fall back
	// to using a default FB config returned from matchConfig().
	if(!(config=matchConfig(dpy, vis)))
		_throw("Could not obtain RGB visual on the server suitable for off-screen rendering");

	if(attrib==GLX_USE_GL)
	{
		if(vis->c_class==TrueColor || vis->c_class==DirectColor) *value=1;
		else *value=0;
	}
	// Transparent overlay visuals are actual 2D X server visuals, not dummy
	// visuals mapped to 3D X server FB configs, so we obtain their properties
	// from the 2D X server.
	else if(attrib==GLX_LEVEL || attrib==GLX_TRANSPARENT_TYPE
		|| attrib==GLX_TRANSPARENT_INDEX_VALUE
		|| attrib==GLX_TRANSPARENT_RED_VALUE
		|| attrib==GLX_TRANSPARENT_GREEN_VALUE
		|| attrib==GLX_TRANSPARENT_BLUE_VALUE
		|| attrib==GLX_TRANSPARENT_ALPHA_VALUE)
		*value=glxvisual::visAttrib2D(dpy, vis->screen, vis->visualid, attrib);
	else if(attrib==GLX_RGBA)
	{
		if((glxvisual::visAttrib3D(config, GLX_RENDER_TYPE) & GLX_RGBA_BIT)!=0)
			*value=1;
		else *value=0;
	}
	else retval=_glXGetFBConfigAttrib(_dpy3D, config, attrib, value);

		stoptrace();  if(value) { prargix(*value); }  else { prargx(value); }
		closetrace();

	CATCH();
	return retval;
}


// This returns the 2D X display handle associated with the current drawable,
// that is, the 2D X display handle passed to whatever function (such as
// XCreateWindow(), glXCreatePbuffer(), etc.) that was used to create the
// drawable.

Display *glXGetCurrentDisplay(void)
{
	Display *dpy=NULL;  VirtualWin *vw=NULL;

	if(vglfaker::getExcludeCurrent()) return _glXGetCurrentDisplay();

	TRY();

		opentrace(glXGetCurrentDisplay);  starttrace();

	GLXDrawable curdraw=_glXGetCurrentDrawable();
	if(winhash.find(curdraw, vw)) dpy=vw->getX11Display();
	else
	{
		if(curdraw) dpy=glxdhash.getCurrentDisplay(curdraw);
	}

		stoptrace();  prargd(dpy);  closetrace();

	CATCH();
	return dpy;
}


// As far as the application is concerned, it is rendering to a window and not
// an off-screen drawable, so we must maintain that illusion and pass it back
// the window ID instead of the GLX drawable ID.

GLXDrawable glXGetCurrentDrawable(void)
{
	VirtualWin *vw=NULL;  GLXDrawable draw=_glXGetCurrentDrawable();

	if(vglfaker::getExcludeCurrent()) return draw;

	TRY();

		opentrace(glXGetCurrentDrawable);  starttrace();

	if(winhash.find(draw, vw)) draw=vw->getX11Drawable();

		stoptrace();  prargx(draw);  closetrace();

	CATCH();
	return draw;
}

GLXDrawable glXGetCurrentReadDrawable(void)
{
	VirtualWin *vw=NULL;  GLXDrawable read=_glXGetCurrentReadDrawable();

	if(vglfaker::getExcludeCurrent()) return read;

	TRY();

		opentrace(glXGetCurrentReadDrawable);  starttrace();

	if(winhash.find(read, vw)) read=vw->getX11Drawable();

		stoptrace();  prargx(read);  closetrace();

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
	VisualID vid=0;  int retval=0;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy) || rcfghash.isOverlay(dpy, config))
		return _glXGetFBConfigAttrib(dpy, config, attribute, value);

	int screen=dpy? DefaultScreen(dpy):0;

		opentrace(glXGetFBConfigAttrib);  prargd(dpy);  prargc(config);
		prargix(attribute);  starttrace();

	if(!dpy || !config || !value)
	{
		retval=GLX_BAD_VALUE;
		goto done;
	}

	retval=_glXGetFBConfigAttrib(_dpy3D, config, attribute, value);

	if(attribute==GLX_DRAWABLE_TYPE && retval==Success)
	{
		int temp=*value;
		*value=0;
		if((fconfig.drawable==RRDRAWABLE_PBUFFER && temp&GLX_PBUFFER_BIT)
			|| (fconfig.drawable==RRDRAWABLE_PIXMAP && temp&GLX_WINDOW_BIT
				&& temp&GLX_PIXMAP_BIT))
			*value|=GLX_WINDOW_BIT;
		if(temp&GLX_PIXMAP_BIT && temp&GLX_WINDOW_BIT) *value|=GLX_PIXMAP_BIT;
		if(temp&GLX_PBUFFER_BIT) *value|=GLX_PBUFFER_BIT;
	}

	// If there is a corresponding 2D X server visual hashed to this FB config,
	// then that means it was obtained via glXChooseFBConfig(), and we can
	// return attributes that take into account the interaction between visuals
	// on the 2D X Server and FB Configs on the 3D X Server.

	if((vid=cfghash.getVisual(dpy, config))!=0)
	{
		// Transparent overlay FB configs are located on the 2D X server, not the
		// 3D X server.
		if(attribute==GLX_LEVEL || attribute==GLX_TRANSPARENT_TYPE
			|| attribute==GLX_TRANSPARENT_INDEX_VALUE
			|| attribute==GLX_TRANSPARENT_RED_VALUE
			|| attribute==GLX_TRANSPARENT_GREEN_VALUE
			|| attribute==GLX_TRANSPARENT_BLUE_VALUE
			|| attribute==GLX_TRANSPARENT_ALPHA_VALUE)
			*value=glxvisual::visAttrib2D(dpy, screen, vid, attribute);
		else if(attribute==GLX_VISUAL_ID)
			*value=vid;
	}

	done:
		stoptrace();  if(value) { prargix(*value); }  else { prargx(value); }
		closetrace();

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

	if(dpyhash.find(dpy))
		return _glXGetFBConfigFromVisualSGIX(dpy, vis);
	else return matchConfig(dpy, vis);

	CATCH();
	return 0;
}


// Hand off to the 3D X server without modification

GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
	GLXFBConfig *configs=NULL;

	TRY();

	if(dpyhash.find(dpy))
		return _glXGetFBConfigs(dpy, screen, nelements);

		opentrace(glXGetFBConfigs);  prargd(dpy);  prargi(screen);
		starttrace();

	configs=_glXGetFBConfigs(_dpy3D, DefaultScreen(_dpy3D), nelements);

		stoptrace();  if(configs && nelements) prargi(*nelements);
		closetrace();

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

	if(dpyhash.find(dpy))
		return _glXBindTexImageEXT(dpy, drawable, buffer, attrib_list);

		opentrace(glXBindTexImageEXT);  prargd(dpy);  prargx(drawable);
		prargi(buffer);  prargal13(attrib_list);  starttrace();

	VirtualPixmap *vpm=NULL;
	if((vpm=pmhash.find(dpy, drawable))==NULL)
		// If we get here, then the drawable wasn't created with
		// glXCreate[GLX]Pixmap().  Thus, we set it to 0 so _glXBindTexImageEXT()
		// will throw a GLXBadPixmap error for us.
		drawable=0;
	else
	{
		// Transfer pixels from the 2D Pixmap (stored on the 2D X server) to the
		// 3D Pixmap (stored on the 3D X server.)
		XImage *image=_XGetImage(dpy, vpm->getX11Drawable(), 0, 0, vpm->getWidth(),
			vpm->getHeight(), AllPlanes, ZPixmap);
		GC gc=XCreateGC(_dpy3D, vpm->get3DX11Pixmap(), 0, NULL);
		if(gc && image)
			XPutImage(_dpy3D, vpm->get3DX11Pixmap(), gc, image, 0, 0, 0, 0,
				vpm->getWidth(), vpm->getHeight());
		else
			// Also trigger GLXBadPixmap error
			drawable=0;
		if(gc) XFreeGC(_dpy3D, gc);
		if(image) XDestroyImage(image);
	}

	_glXBindTexImageEXT(_dpy3D, drawable, buffer, attrib_list);

		stoptrace();  closetrace();

	CATCH();
}


void glXReleaseTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXReleaseTexImageEXT(dpy, drawable, buffer);

		opentrace(glXReleaseTexImageEXT);  prargd(dpy);  prargx(drawable);
		prargi(buffer);  starttrace();

	_glXReleaseTexImageEXT(_dpy3D, drawable, buffer);

		stoptrace();  closetrace();

	CATCH();
}


// If an application uses glXGetProcAddressARB() to obtain the address of a
// function that we're interposing, we need to return the address of the
// interposed function.

#define checkfaked(f) if(!strcmp((char *)procName, #f)) {  \
	retval=(void (*)(void))f;  if(fconfig.trace) vglout.print("[INTERPOSED]");}

// For optional libGL symbols, check that the underlying function
// actually exists in libGL before returning the interposed version of it.

#define checkoptfaked(f)  \
	if(!strcmp((char *)procName, #f)) {  \
		CHECKSYM_NONFATAL(f)  \
		if(__##f) {  \
			retval=(void (*)(void))f;  \
			if(fconfig.trace) vglout.print("[INTERPOSED]");  \
		}  \
	}

void (*glXGetProcAddressARB(const GLubyte *procName))(void)
{
	void (*retval)(void)=NULL;

	vglfaker::init();

		opentrace(glXGetProcAddressARB);  prargs((char *)procName);  starttrace();

	if(procName)
	{
		// GLX 1.0
		checkfaked(glXChooseVisual)
		checkfaked(glXCopyContext)
		checkfaked(glXCreateContext)
		checkfaked(glXCreateGLXPixmap)
		checkfaked(glXDestroyContext)
		checkfaked(glXDestroyGLXPixmap)
		checkfaked(glXGetConfig)
		checkfaked(glXGetCurrentDrawable)
		checkfaked(glXIsDirect)
		checkfaked(glXMakeCurrent)
		checkfaked(glXQueryExtension)
		checkfaked(glXQueryVersion)
		checkfaked(glXSwapBuffers)
		checkfaked(glXUseXFont)
		checkfaked(glXWaitGL)

		// GLX 1.1
		checkfaked(glXGetClientString)
		checkfaked(glXQueryServerString)
		checkfaked(glXQueryExtensionsString)

		// GLX 1.2
		checkfaked(glXGetCurrentDisplay)

		// GLX 1.3
		checkfaked(glXChooseFBConfig)
		checkfaked(glXCreateNewContext)
		checkfaked(glXCreatePbuffer)
		checkfaked(glXCreatePixmap)
		checkfaked(glXCreateWindow)
		checkfaked(glXDestroyPbuffer)
		checkfaked(glXDestroyPixmap)
		checkfaked(glXDestroyWindow)
		checkfaked(glXGetCurrentReadDrawable)
		checkfaked(glXGetFBConfigAttrib)
		checkfaked(glXGetFBConfigs)
		checkfaked(glXGetSelectedEvent)
		checkfaked(glXGetVisualFromFBConfig)
		checkfaked(glXMakeContextCurrent)
		checkfaked(glXQueryContext)
		checkfaked(glXQueryDrawable)
		checkfaked(glXSelectEvent)

		// GLX 1.4
		checkfaked(glXGetProcAddress)

		// GLX_ARB_create_context
		checkoptfaked(glXCreateContextAttribsARB)

		// GLX_ARB_get_proc_address
		checkfaked(glXGetProcAddressARB)

		// GLX_EXT_import_context
		checkoptfaked(glXFreeContextEXT)
		checkoptfaked(glXImportContextEXT)
		checkoptfaked(glXQueryContextInfoEXT)

		// GLX_EXT_swap_control
		checkoptfaked(glXSwapIntervalEXT)

		// GLX_EXT_texture_from_pixmap
		checkoptfaked(glXBindTexImageEXT)
		checkoptfaked(glXReleaseTexImageEXT)

		// GLX_NV_swap_group
		checkoptfaked(glXBindSwapBarrierNV)
		checkoptfaked(glXJoinSwapGroupNV)
		checkoptfaked(glXQueryFrameCountNV)
		checkoptfaked(glXQueryMaxSwapGroupsNV)
		checkoptfaked(glXQuerySwapGroupNV)
		checkoptfaked(glXResetFrameCountNV)

		// GLX_SGI_make_current_read
		checkfaked(glXGetCurrentReadDrawableSGI)
		checkfaked(glXMakeCurrentReadSGI)

		// GLX_SGI_swap_control
		checkoptfaked(glXSwapIntervalSGI)

		// GLX_SGIX_fbconfig
		checkfaked(glXChooseFBConfigSGIX)
		checkfaked(glXCreateContextWithConfigSGIX)
		checkfaked(glXCreateGLXPixmapWithConfigSGIX)
		checkfaked(glXGetFBConfigAttribSGIX)
		checkfaked(glXGetFBConfigFromVisualSGIX)
		checkfaked(glXGetVisualFromFBConfigSGIX)

		// GLX_SGIX_pbuffer
		checkfaked(glXCreateGLXPbufferSGIX)
		checkfaked(glXDestroyGLXPbufferSGIX)
		checkfaked(glXGetSelectedEventSGIX)
		checkfaked(glXQueryGLXPbufferSGIX)
		checkfaked(glXSelectEventSGIX)

		// GLX_SUN_get_transparent_index
		checkfaked(glXGetTransparentIndexSUN)

		// OpenGL
		checkfaked(glFinish)
		checkfaked(glFlush)
		checkfaked(glViewport)
		checkfaked(glDrawBuffer)
		checkfaked(glPopAttrib)
	}
	if(!retval)
	{
		if(fconfig.trace) vglout.print("[passed through]");
		retval=_glXGetProcAddress(procName);
	}

		stoptrace();  closetrace();

	return retval;
}

void (*glXGetProcAddress(const GLubyte *procName))(void)
{
	return glXGetProcAddressARB(procName);
}


// Hand off to the 2D X server (overlay rendering) or the 3D X server (opaque
// rendering) without modification, except that, if 'draw' is not an overlay
// window, we replace it with its corresponding off-screen drawable ID.

void glXGetSelectedEvent(Display *dpy, GLXDrawable draw,
	unsigned long *event_mask)
{
	TRY();

	if(dpyhash.find(dpy) || winhash.isOverlay(dpy, draw))
		return _glXGetSelectedEvent(dpy, draw, event_mask);

	_glXGetSelectedEvent(_dpy3D, ServerDrawable(dpy, draw), event_mask);

	CATCH();
}

void glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable,
	unsigned long *mask)
{
	glXGetSelectedEvent(dpy, drawable, mask);
}


// Return the 2D X server visual that was hashed to the 3D X server FB config
// during a previous call to glXChooseFBConfig(), or pick out an appropriate
// visual and return it.

XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
	XVisualInfo *vis=NULL;

	TRY();

	if(is3D(dpy) || dpyhash.find(dpy) || rcfghash.isOverlay(dpy, config))
		return _glXGetVisualFromFBConfig(dpy, config);

		opentrace(glXGetVisualFromFBConfig);  prargd(dpy);  prargc(config);
		starttrace();

	VisualID vid=0;
	if(!dpy || !config) goto done;
	vid=matchVisual(dpy, config);
	if(!vid) goto done;
	vis=glxvisual::visualFromID(dpy, DefaultScreen(dpy), vid);
	if(!vis) goto done;
	vishash.add(dpy, vis, config);

	done:
		stoptrace();  prargv(vis);  closetrace();

	CATCH();
	return vis;
}


XVisualInfo *glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config)
{
	return glXGetVisualFromFBConfig(dpy, config);
}



// Hand off to the 3D X server without modification

GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXImportContextEXT(dpy, contextID);

	return _glXImportContextEXT(_dpy3D, contextID);

	CATCH();
	return 0;
}


// Hand off to the 2D X server (overlay rendering) or the 3D X server (opaque
// rendering) without modification

Bool glXIsDirect(Display *dpy, GLXContext ctx)
{
	Bool direct=0;

	TRY();

	if(dpyhash.find(dpy) || ctxhash.isOverlay(ctx))
		return _glXIsDirect(dpy, ctx);

		opentrace(glXIsDirect);  prargd(dpy);  prargx(ctx);
		starttrace();

	direct=_glXIsDirect(_dpy3D, ctx);

		stoptrace();  prargi(direct);  closetrace();

	CATCH();
	return direct;
}


// See notes

Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
	Bool retval=False;  const char *renderer="Unknown";
	VirtualWin *vw;  GLXFBConfig config=0;

	if(is3D(dpy)) return _glXMakeCurrent(dpy, drawable, ctx);

	TRY();

	// Find the FB config that was previously hashed to this context when it was
	// created.
	if(ctx) config=ctxhash.findConfig(ctx);
	if(config==(GLXFBConfig)-1)
	{
		// Overlay context.  Hand off to the 2D X server.
		retval=_glXMakeCurrent(dpy, drawable, ctx);
		winhash.setOverlay(dpy, drawable);
		vglfaker::setExcludeCurrent(true);
		return retval;
	}
 	if(dpyhash.find(dpy))
	{
		vglfaker::setExcludeCurrent(true);
		return _glXMakeCurrent(dpy, drawable, ctx);
	}
	vglfaker::setExcludeCurrent(false);

		opentrace(glXMakeCurrent);  prargd(dpy);  prargx(drawable);  prargx(ctx);
		starttrace();

	// glXMakeCurrent() implies a glFinish() on the previous context, which is
	// why we read back the front buffer here if it is dirty.
	GLXDrawable curdraw=_glXGetCurrentDrawable();
	if(_glXGetCurrentContext() && dpy3DIsCurrent()
		&& curdraw && winhash.find(curdraw, vw))
	{
		VirtualWin *newvw;
		if(drawable==0 || !winhash.find(dpy, drawable, newvw)
			|| newvw->getGLXDrawable()!=curdraw)
		{
			if(drawingToFront() || vw->dirty)
				vw->readback(GL_FRONT, false, fconfig.sync);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to an off-screen drawable.
	int direct=ctxhash.isDirect(ctx);
	if(dpy && drawable && ctx)
	{
		if(!config)
		{
			vglout.PRINTLN("[VGL] WARNING: glXMakeCurrent() called with a previously-destroyed context.");
			goto done;
		}
		vw=winhash.initVW(dpy, drawable, config);
		if(vw)
		{
			setWMAtom(dpy, drawable);
			drawable=vw->updateGLXDrawable();
			vw->setDirect(direct);
		}
		else if(!glxdhash.getCurrentDisplay(drawable))
		{
			// Apparently it isn't a Pbuffer or a Pixmap, so it must be a window
			// that was created in another process.
			if(!is3D(dpy))
			{
				winhash.add(dpy, drawable);
				vw=winhash.initVW(dpy, drawable, config);
				if(vw)
				{
					drawable=vw->updateGLXDrawable();
					vw->setDirect(direct);
				}
			}
		}
	}

	retval=_glXMakeContextCurrent(_dpy3D, drawable, drawable, ctx);
	if(fconfig.trace && retval) renderer=(const char *)_glGetString(GL_RENDERER);
	// The pixels in a new off-screen drawable are undefined, so we have to clear
	// it.
	if(winhash.find(drawable, vw)) { vw->clear();  vw->cleanup(); }
	VirtualPixmap *vpm;
	if((vpm=pmhash.find(dpy, drawable))!=NULL)
	{
		vpm->clear();
		vpm->setDirect(direct);
	}

	done:
		stoptrace();  prargc(config);  prargx(drawable);  prargs(renderer);
		closetrace();

	CATCH();
	return retval;
}


Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx)
{
	Bool retval=False;  const char *renderer="Unknown";
	VirtualWin *vw;  GLXFBConfig config=0;

	if(is3D(dpy)) return _glXMakeContextCurrent(dpy, draw, read, ctx);

	TRY();

	if(ctx) config=ctxhash.findConfig(ctx);
	if(config==(GLXFBConfig)-1)
	{
		// Overlay config.  Hand off to 2D X server.
		retval=_glXMakeContextCurrent(dpy, draw, read, ctx);
		winhash.setOverlay(dpy, draw);
		winhash.setOverlay(dpy, read);
		vglfaker::setExcludeCurrent(true);
		return retval;
	}
	if(dpyhash.find(dpy))
	{
		vglfaker::setExcludeCurrent(true);
		return _glXMakeContextCurrent(dpy, draw, read, ctx);
	}
	vglfaker::setExcludeCurrent(false);

		opentrace(glXMakeContextCurrent);  prargd(dpy);  prargx(draw);
		prargx(read);  prargx(ctx);  starttrace();

	// glXMakeContextCurrent() implies a glFinish() on the previous context,
	// which is why we read back the front buffer here if it is dirty.
	GLXDrawable curdraw=_glXGetCurrentDrawable();
	if(_glXGetCurrentContext() && dpy3DIsCurrent() && curdraw
		&& winhash.find(curdraw, vw))
	{
		VirtualWin *newvw;
		if(draw==0 || !winhash.find(dpy, draw, newvw)
			|| newvw->getGLXDrawable()!=curdraw)
		{
			if(drawingToFront() || vw->dirty)
				vw->readback(GL_FRONT, false, fconfig.sync);
		}
	}

	// If the drawable isn't a window, we pass it through unmodified, else we
	// map it to an off-screen drawable.
	VirtualWin *drawVW, *readVW;
	int direct=ctxhash.isDirect(ctx);
	if(dpy && (draw || read) && ctx)
	{
		if(!config)
		{
			vglout.PRINTLN("[VGL] WARNING: glXMakeContextCurrent() called with a previously-destroyed context");
			goto done;
		}

		drawVW=winhash.initVW(dpy, draw, config);
		if(drawVW)
		{
			setWMAtom(dpy, draw);
			draw=drawVW->updateGLXDrawable();
			drawVW->setDirect(direct);
		}
		else if(!glxdhash.getCurrentDisplay(draw))
		{
			// Apparently it isn't a Pbuffer or a Pixmap, so it must be a window
			// that was created in another process.
			if(!is3D(dpy))
			{
				winhash.add(dpy, draw);
				drawVW=winhash.initVW(dpy, draw, config);
				if(drawVW)
				{
					draw=drawVW->updateGLXDrawable();
					drawVW->setDirect(direct);
				}
			}
		}

		readVW=winhash.initVW(dpy, read, config);
		if(readVW)
		{
			setWMAtom(dpy, read);
			read=readVW->updateGLXDrawable();
			readVW->setDirect(direct);
		}
		else if(!glxdhash.getCurrentDisplay(read))
		{
			// Apparently it isn't a Pbuffer or a Pixmap, so it must be a window
			// that was created in another process.
			if(!is3D(dpy))
			{
				winhash.add(dpy, read);
				readVW=winhash.initVW(dpy, read, config);
				if(readVW)
				{
					read=readVW->updateGLXDrawable();
					readVW->setDirect(direct);
				}
			}
		}
	}
	retval=_glXMakeContextCurrent(_dpy3D, draw, read, ctx);
	if(fconfig.trace && retval) renderer=(const char *)_glGetString(GL_RENDERER);
	if(winhash.find(draw, drawVW)) { drawVW->clear();  drawVW->cleanup(); }
	if(winhash.find(read, readVW)) readVW->cleanup();
	VirtualPixmap *vpm;
	if((vpm=pmhash.find(dpy, draw))!=NULL)
	{
		vpm->clear();
		vpm->setDirect(direct);
	}

	done:
		stoptrace();  prargc(config);  prargx(draw);  prargx(read);
		prargs(renderer);  closetrace();

	CATCH();
	return retval;
}


Bool glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx)
{
	return glXMakeContextCurrent(dpy, draw, read, ctx);
}


// Hand off to the 2D X server (overlay rendering) or the 3D X server (opaque
// rendering) without modification

int glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	int retval=0;

	TRY();

	if(dpyhash.find(dpy) || ctxhash.isOverlay(ctx))
		return _glXQueryContext(dpy, ctx, attribute, value);

		opentrace(glXQueryContext);  prargd(dpy);  prargx(ctx);  prargix(attribute);
		starttrace();

	retval=_glXQueryContext(_dpy3D, ctx, attribute, value);

		stoptrace();  if(value) prargix(*value);  closetrace();

	CATCH();
	return retval;
}


int glXQueryContextInfoEXT(Display *dpy, GLXContext ctx, int attribute,
	int *value)
{
	int retval=0;

	TRY();

	if(dpyhash.find(dpy) || ctxhash.isOverlay(ctx))
		return _glXQueryContextInfoEXT(dpy, ctx, attribute, value);

		opentrace(glXQueryContextInfoEXT);  prargd(dpy);  prargx(ctx);
		prargix(attribute); starttrace();

	retval=_glXQueryContextInfoEXT(_dpy3D, ctx, attribute, value);

		stoptrace();  if(value) prargix(*value);  closetrace();

	CATCH();
	return retval;
}


// Hand off to the 2D X server (overlay rendering) or the 3D X server (opaque
// rendering) without modification, except that, if 'draw' is not an overlay
// window, we replace it with its corresponding off-screen drawable ID.

void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute,
	unsigned int *value)
{
	TRY();

	if(dpyhash.find(dpy) || winhash.isOverlay(dpy, draw))
	{
		_glXQueryDrawable(dpy, draw, attribute, value);
		return;
	}

		opentrace(glXQueryDrawable);  prargd(dpy);  prargx(draw);
		prargix(attribute);  starttrace();

	// GLX_EXT_swap_control attributes
	if(attribute==GLX_SWAP_INTERVAL_EXT && value)
	{
		VirtualWin *vw=NULL;
		if(winhash.find(dpy, draw, vw))
			*value=vw->getSwapInterval();
		else
			*value=0;
		goto done;
	}
	else if(attribute==GLX_MAX_SWAP_INTERVAL_EXT && value)
	{
		*value=VGL_MAX_SWAP_INTERVAL;
		goto done;
	}

	_glXQueryDrawable(_dpy3D, ServerDrawable(dpy, draw), attribute, value);

	done:
		stoptrace();  prargx(ServerDrawable(dpy, draw));
		if(value) { prargix(*value); }  else { prargx(value); }  closetrace();

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

	if(dpyhash.find(dpy))
		return _glXQueryExtension(dpy, error_base, event_base);

	return _glXQueryExtension(_dpy3D, error_base, event_base);

	CATCH();
	return False;
}


// Same as glXGetClientString(GLX_EXTENSIONS)

const char *glXQueryExtensionsString(Display *dpy, int screen)
{
	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXQueryExtensionsString(dpy, screen);

	return getGLXExtensions();

	CATCH();
	return NULL;
}


// Same as glXGetClientString() in our case

const char *glXQueryServerString(Display *dpy, int screen, int name)
{
	TRY();

	if(is3D(dpy) || dpyhash.find(dpy))
		return _glXQueryServerString(dpy, screen, name);

	if(name==GLX_EXTENSIONS) return getGLXExtensions();
	else if(name==GLX_VERSION) return "1.4";
	else if(name==GLX_VENDOR)
	{
		if(strlen(fconfig.glxvendor)>0) return fconfig.glxvendor;
		else return __APPNAME;
	}

	CATCH();
	return NULL;
}


// Hand off to the 3D X server without modification.

Bool glXQueryVersion(Display *dpy, int *major, int *minor)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXQueryVersion(dpy, major, minor);

	return _glXQueryVersion(_dpy3D, major, minor);

	CATCH();
	return False;
}


// Hand off to the 2D X server (overlay rendering) or the 3D X server (opaque
// rendering) without modification.

void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
	TRY();

	if(dpyhash.find(dpy) || winhash.isOverlay(dpy, draw))
		return _glXSelectEvent(dpy, draw, event_mask);

	_glXSelectEvent(_dpy3D, ServerDrawable(dpy, draw), event_mask);

	CATCH();
}

void glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
	glXSelectEvent(dpy, drawable, mask);
}


// If the application is rendering to the back buffer, VirtualGL will read
// back and send the buffer whenever glXSwapBuffers() is called.

void glXSwapBuffers(Display* dpy, GLXDrawable drawable)
{
	VirtualWin *vw=NULL;
	static Timer timer;  Timer sleepTimer;
	static double err=0.;  static bool first=true;

	TRY();

	if(dpyhash.find(dpy) || winhash.isOverlay(dpy, drawable))
	{
		_glXSwapBuffers(dpy, drawable);
		return;
	}

		opentrace(glXSwapBuffers);  prargd(dpy);  prargx(drawable);  starttrace();

	fconfig.flushdelay=0.;
	if(!is3D(dpy) && winhash.find(dpy, drawable, vw))
	{
		vw->readback(GL_BACK, false, fconfig.sync);
		vw->swapBuffers();
		int interval=vw->getSwapInterval();
		if(interval>0)
		{
			double elapsed=timer.elapsed();
			if(first) first=false;
			else
			{
				double fps=fconfig.refreshrate/(double)interval;
				if(fps>0.0 && elapsed<1./fps)
				{
					sleepTimer.start();
					long usec=(long)((1./fps-elapsed-err)*1000000.);
					if(usec>0) usleep(usec);
					double sleepTime=sleepTimer.elapsed();
					err=sleepTime-(1./fps-elapsed-err);  if(err<0.) err=0.;
				}
			}
			timer.start();
		}
	}
	else _glXSwapBuffers(_dpy3D, drawable);

		stoptrace();  if(!is3D(dpy) && vw) { prargx(vw->getGLXDrawable()); }
		closetrace();

	CATCH();
}


// Returns the transparent index from the overlay visual on the 2D X server

int glXGetTransparentIndexSUN(Display *dpy, Window overlay,
	Window underlay, long *transparentIndex)
{
	int retval=False;
	XWindowAttributes xwa;
	if(!transparentIndex) return False;

	TRY();

	if(dpyhash.find(dpy))
		return _glXGetTransparentIndexSUN(dpy, overlay, underlay,
			transparentIndex);

		opentrace(glXGetTransparentIndexSUN);  prargd(dpy);  prargx(overlay);
		prargx(underlay);  starttrace();

	if(fconfig.transpixel>=0)
		*transparentIndex=fconfig.transpixel;
	else
	{
		if(!dpy || !overlay) goto done;
		XGetWindowAttributes(dpy, overlay, &xwa);
		*transparentIndex=glxvisual::visAttrib2D(dpy, DefaultScreen(dpy),
			xwa.visual->visualid, GLX_TRANSPARENT_INDEX_VALUE);
	}
	retval=True;

	done:
		stoptrace();  if(transparentIndex) { prargi(*transparentIndex); }
		else { prargx(transparentIndex); }  closetrace();

	CATCH();
	return retval;
}


// Hand off to the 3D X server without modification, except that 'drawable' is
// replaced with its corresponding off-screen drawable ID.

Bool glXJoinSwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint group)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXJoinSwapGroupNV(dpy, drawable, group);

	return _glXJoinSwapGroupNV(_dpy3D, ServerDrawable(dpy, drawable), group);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

Bool glXBindSwapBarrierNV(Display *dpy, GLuint group, GLuint barrier)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXBindSwapBarrierNV(dpy, group, barrier);

	return _glXBindSwapBarrierNV(_dpy3D, group, barrier);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification, except that 'drawable' is
// replaced with its corresponding off-screen drawable ID.

Bool glXQuerySwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint *group,
	GLuint *barrier)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXQuerySwapGroupNV(dpy, drawable, group, barrier);

	return _glXQuerySwapGroupNV(_dpy3D, ServerDrawable(dpy, drawable), group,
		barrier);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

Bool glXQueryMaxSwapGroupsNV(Display *dpy, int screen, GLuint *maxGroups,
	GLuint *maxBarriers)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXQueryMaxSwapGroupsNV(dpy, screen, maxGroups, maxBarriers);

	return _glXQueryMaxSwapGroupsNV(_dpy3D, DefaultScreen(_dpy3D),
		maxGroups, maxBarriers);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

Bool glXQueryFrameCountNV(Display *dpy, int screen, GLuint *count)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXQueryFrameCountNV(dpy, screen, count);

	return _glXQueryFrameCountNV(_dpy3D, DefaultScreen(_dpy3D), count);

	CATCH();
	return False;
}


// Hand off to the 3D X server without modification.

Bool glXResetFrameCountNV(Display *dpy, int screen)
{
	TRY();

	if(dpyhash.find(dpy))
		return _glXResetFrameCountNV(dpy, screen);

	return _glXResetFrameCountNV(_dpy3D, DefaultScreen(_dpy3D));

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

	// If drawable is an overlay, hand off to the 2D X Server
	if(dpyhash.find(dpy) || winhash.isOverlay(dpy, drawable))
	{
		_glXSwapIntervalEXT(dpy, drawable, interval);
		return;
	}

		opentrace(glXSwapIntervalEXT);  prargd(dpy);  prargx(drawable);
		prargi(interval);  starttrace();

	if(interval>VGL_MAX_SWAP_INTERVAL) interval=VGL_MAX_SWAP_INTERVAL;
	if(interval<0)
		// NOTE:  Technically, this should trigger a BadValue error, but nVidia's
		// implementation doesn't, so we emulate their behavior.
		interval=1;

	VirtualWin *vw=NULL;
	if(winhash.find(dpy, drawable, vw))
		vw->setSwapInterval(interval);
	// NOTE:  Technically, a BadWindow error should be triggered if drawable
	// isn't a GLX window, but nVidia's implementation doesn't, so we emulate
	// their behavior.

		stoptrace();  closetrace();

	CATCH();
}


// This is basically the same as calling glXSwapIntervalEXT() with the current
// drawable.

int glXSwapIntervalSGI(int interval)
{
	int retval=0;

	if(vglfaker::getExcludeCurrent()) return _glXSwapIntervalSGI(interval);

		opentrace(glXSwapIntervalSGI);  prargi(interval);  starttrace();

	TRY();

	VirtualWin *vw=NULL;  GLXDrawable draw=_glXGetCurrentDrawable();
	if(interval<0) retval=GLX_BAD_VALUE;
	else if(!draw || !winhash.find(draw, vw))
		retval=GLX_BAD_CONTEXT;
	else vw->setSwapInterval(interval);

	CATCH();

		stoptrace();  closetrace();

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

		opentrace(glXUseXFont);  prargx(font);  prargi(first);  prargi(count);
		prargi(list_base);  starttrace();

	Fake_glXUseXFont(font, first, count, list_base);

		stoptrace();  closetrace();

	CATCH();
}


} // extern "C"
