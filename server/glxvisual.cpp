// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2009-2016, 2019-2023 D. R. Commander
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

#include "glxvisual.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "Error.h"
#include "Mutex.h"
#include "faker.h"
#include "vglutil.h"
#ifdef EGLBACKEND
#include "TempContextEGL.h"
#endif

using namespace util;


namespace glxvisual {

typedef struct
{
	VisualID visualID;
	VGLFBConfig config;
	int depth, c_class, bpc, nVisuals;
	int isStereo, isDB, isGL;
	GLXAttrib glx;
} VisAttrib;


#define GET_VA_TABLE() \
	VisAttrib *va;  int vaEntries; \
	XEDataObject obj; \
	XExtData *extData; \
	\
	obj.screen = XScreenOfDisplay(dpy, screen); \
	int minExtensionNumber = \
		XFindOnExtensionList(XEHeadOfExtensionList(obj), 0) ? 0 : 1; \
	extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), \
		minExtensionNumber + 2); \
	if(!extData) \
		THROW("Could not retrieve visual attribute table for screen"); \
	va = (VisAttrib *)extData->private_data; \
	vaEntries = va[0].nVisuals;


#define GET_CA_TABLE() \
	struct _VGLFBConfig *ca;  int caEntries; \
	XEDataObject obj; \
	XExtData *extData; \
	\
	obj.screen = XScreenOfDisplay(dpy, screen); \
	int minExtensionNumber = \
		XFindOnExtensionList(XEHeadOfExtensionList(obj), 0) ? 0 : 1; \
	extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), \
		minExtensionNumber + 3); \
	if(!extData) \
		THROW("Could not retrieve FB config attribute table for screen"); \
	ca = (struct _VGLFBConfig *)extData->private_data; \
	caEntries = ca[0].nConfigs;


// This function assigns, as much as possible, various permutations of common
// OpenGL rendering attributes to the available 2D X server visuals.  This is
// necessary because 3D applications sometimes "hunt for visuals"; they use X11
// functions to obtain a list of 2D X server visuals, then they iterate through
// the list with glXGetConfig() until they find a visual with a desired set of
// OpenGL rendering attributes.  However, if a visual doesn't originate from
// glXChooseVisual() or glXGetVisualFromFBConfig(), then VirtualGL has no idea
// which OpenGL rendering attributes should be assigned to it.  Historically,
// when VGL encountered such an "unknown visual", it assigned a default FB
// config to it and allowed the rendering attributes of that default FB config
// to be specified in an environment variable (VGL_DEFAULTFBCONFIG.)  Instead,
// VirtualGL now pre-assigns OpenGL rendering attributes to all available 2D X
// server visuals.  Furthermore, VGL assigns all of the common OpenGL rendering
// attributes to the first 2D X server visual, in order to maximize the odds
// that a "visual hunting" 3D application will work with a 2D X server that
// exports only one or two visuals.  VGL_DEFAULTFBCONFIG can still be used to
// modify the OpenGL rendering attributes of all visuals, but the usefulness of
// that feature is now very limited.

#define TEST_ATTRIB(attrib, var, min, max) \
{ \
	if(!strcmp(argv[i], #attrib) && i < argc - 1) \
	{ \
		int temp = atoi(argv[++i]); \
		if(temp >= min && temp <= max) \
			var = temp; \
	} \
}

static void buildCfgAttribTable(Display *dpy, int screen);

static void assignDefaultFBConfigAttribs(Display *dpy, int screen,
	XVisualInfo *visuals, int nVisuals, int visualDepth, int visualClass,
	int visualBPC, bool stereo, VisAttrib *va)
{
	int alphaSize = -1, doubleBuffer = -1, stencilSize = -1, depthSize = -1,
		numSamples = -1;

	if(nVisuals < 1) return;

	buildCfgAttribTable(dpy, screen);
	GET_CA_TABLE()

	// Allow the default FB config attribs to be manually specified.
	if(strlen(fconfig.defaultfbconfig) > 0)
	{
		char *str = strdup(fconfig.defaultfbconfig);
		if(!str) THROW_UNIX();
		char *argv[512];  int argc = 0;
		char *arg = strtok(str, ", \t");
		while(arg && argc < 512)
		{
		  argv[argc] = arg;  argc++;
		  arg = strtok(NULL, ", \t");
		}
		for(int i = 0; i < argc; i++)
		{
			TEST_ATTRIB(GLX_ALPHA_SIZE, alphaSize, 0, INT_MAX);
			TEST_ATTRIB(GLX_DOUBLEBUFFER, doubleBuffer, 0, 1);
			TEST_ATTRIB(GLX_STENCIL_SIZE, stencilSize, 0, INT_MAX);
			TEST_ATTRIB(GLX_DEPTH_SIZE, depthSize, 0, INT_MAX);
			TEST_ATTRIB(GLX_SAMPLES, numSamples, 0, INT_MAX);
		}
		free(str);
	}

	if(fconfig.samples >= 0) numSamples = fconfig.samples;
	if(fconfig.forcealpha) alphaSize = 1;

	// Determine the range of values that the FB configs provide.
	int minAlpha = INT_MAX, maxAlpha = 0, minDB = 0, maxDB = 1,
		minStencil = INT_MAX, maxStencil = 0, minDepth = 24, maxDepth = 24,
		minSamples = INT_MAX, maxSamples = 0;
	for(int i = 0; i < caEntries; i++)
	{
		if(ca[i].attr.alphaSize < 0 || ca[i].attr.stencilSize < 0
			|| ca[i].attr.samples < 0)
			continue;
		minAlpha = min(minAlpha, ca[i].attr.alphaSize);
		maxAlpha = max(maxAlpha, ca[i].attr.alphaSize);
		minStencil = min(minStencil, ca[i].attr.stencilSize);
		maxStencil = max(maxStencil, ca[i].attr.stencilSize);
		minSamples = min(minSamples, ca[i].attr.samples);
		maxSamples = max(maxSamples, ca[i].attr.samples);
	}
	if(minAlpha < 0) minAlpha = 0;
	minAlpha = !!minAlpha;  maxAlpha = !!maxAlpha;
	if(minStencil < 0) minStencil = 0;
	minStencil = 8 * !!minStencil;  maxStencil = 8 * !!maxStencil;
	if(minSamples < 0) minSamples = 0;
	if(maxSamples > 64) maxSamples = 64;

	if(alphaSize >= 0) minAlpha = maxAlpha = alphaSize;
	if(doubleBuffer >= 0) minDB = maxDB = doubleBuffer;
	if(stencilSize >= 0) minStencil = maxStencil = stencilSize;
	if(depthSize >= 0) minDepth = maxDepth = depthSize;
	if(numSamples >= 0) minSamples = maxSamples = numSamples;

	// Assign the default FB config attributes.
	int i = 0;
	for(int samples = minSamples; samples <= maxSamples;
		samples = (samples ? samples * 2 : 2))
	{
		for(int depth = maxDepth; depth >= minDepth; depth -= 24)
		{
			for(int stencil = maxStencil; stencil >= minStencil; stencil -= 8)
			{
				if(!depth && stencil) continue;
				for(int db = maxDB; db >= minDB; db--)
				{
					for(int alpha = maxAlpha; alpha >= minAlpha; alpha--)
					{
						while(va[i].c_class != visualClass || va[i].depth != visualDepth
							|| (va[i].bpc != visualBPC && visualDepth == 32)
							|| va[i].isStereo != stereo)
						{
							i++;  if(i >= nVisuals) return;
						}
						va[i].glx.doubleBuffer = db;
						va[i].glx.alphaSize = alpha;
						va[i].glx.depthSize = depth;
						va[i].glx.stencilSize = stencil;
						va[i++].glx.samples = samples;
						if(i >= nVisuals) return;
					}
				}
			}
		}
	}
}


static bool buildVisAttribTable(Display *dpy, int screen)
{
	int clientGLX = 0, majorOpcode = -1, firstEvent = -1, firstError = -1,
		nVisuals = 0;
	XVisualInfo *visuals = NULL, vtemp;
	VisAttrib *va = NULL;
	XEDataObject obj;
	XExtData *extData;
	obj.screen = XScreenOfDisplay(dpy, screen);

	if(!fconfig.egl && dpy == faker::dpy3D)
		THROW("glxvisual::buildVisAttribTable() called with 3D X server handle (this should never happen)");

	try
	{
		CriticalSection::SafeLock l(faker::getDisplayCS(dpy));

		int minExtensionNumber =
			XFindOnExtensionList(XEHeadOfExtensionList(obj), 0) ? 0 : 1;
		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj),
			minExtensionNumber + 2);
		if(extData && extData->private_data) return true;

		fconfig_setprobeglxfromdpy(dpy);
		if(fconfig.probeglx
			&& _XQueryExtension(dpy, "GLX", &majorOpcode, &firstEvent, &firstError)
			&& majorOpcode >= 0 && firstEvent >= 0 && firstError >= 0)
		{
			if(fconfig.verbose)
				vglout.println("[VGL] NOTICE: Probing 2D X server for stereo visuals");
			clientGLX = 1;
		}
		vtemp.screen = screen;
		if(!(visuals = XGetVisualInfo(dpy, VisualScreenMask, &vtemp, &nVisuals))
			|| nVisuals == 0)
			THROW("No visuals found on display");

		if(!(va = (VisAttrib *)calloc(nVisuals, sizeof(VisAttrib))))
			THROW("Memory allocation error");

		for(int i = 0; i < nVisuals; i++)
		{
			va[i].visualID = visuals[i].visualid;
			va[i].depth = visuals[i].depth;
			va[i].c_class = visuals[i].c_class;
			va[i].bpc = visuals[i].bits_per_rgb;
			va[i].nVisuals = nVisuals;

			if(clientGLX)
			{
				_glXGetConfig(dpy, &visuals[i], GLX_DOUBLEBUFFER, &va[i].isDB);
				_glXGetConfig(dpy, &visuals[i], GLX_USE_GL, &va[i].isGL);
				_glXGetConfig(dpy, &visuals[i], GLX_STEREO, &va[i].isStereo);
			}
			va[i].glx.alphaSize = va[i].glx.depthSize = va[i].glx.stencilSize =
				va[i].glx.samples = -1;
		}

		int nDepths, *depths = XListDepths(dpy, screen, &nDepths);
		if(!depths) THROW("Memory allocation error");
		for(int i = 0; i < nDepths; i++)
		{
			if(depths[i] >= 24)
			{
				int minBPC = 8, maxBPC = 8;
				if(depths[i] == 30) minBPC = 10;
				if(depths[i] >= 30) maxBPC = 10;

				for(int bpc = minBPC; bpc <= maxBPC; bpc += 2)
				{
					assignDefaultFBConfigAttribs(dpy, screen, visuals, nVisuals,
						depths[i], TrueColor, bpc, true, va);
					assignDefaultFBConfigAttribs(dpy, screen, visuals, nVisuals,
						depths[i], DirectColor, bpc, true, va);
					assignDefaultFBConfigAttribs(dpy, screen, visuals, nVisuals,
						depths[i], TrueColor, bpc, false, va);
					assignDefaultFBConfigAttribs(dpy, screen, visuals, nVisuals,
						depths[i], DirectColor, bpc, false, va);
				}
			}
		}
		_XFree(depths);
		_XFree(visuals);

		if(!(extData = (XExtData *)calloc(1, sizeof(XExtData))))
			THROW("Memory allocation error");
		extData->private_data = (XPointer)va;
		extData->number = 3;
		XAddToExtensionList(XEHeadOfExtensionList(obj), extData);
	}
	catch(...)
	{
		if(visuals) _XFree(visuals);
		free(va);
		return false;
	}
	return true;
}


// This function finds a 2D X server visual that matches the given
// visual parameters.  As with the above functions, it uses the cached
// attributes from the 2D X server, or it caches them if they have not
// already been read.

static VisualID matchVisual2D(Display *dpy, int screen, int depth, int c_class,
	int bpc, int stereo, bool strictMatch)
{
	int i, tryStereo;
	if(!dpy) return 0;

	buildVisAttribTable(dpy, screen);
	GET_VA_TABLE()

	// Try to find an exact match
	for(tryStereo = 1; tryStereo >= 0; tryStereo--)
	{
		for(i = 0; i < vaEntries; i++)
		{
			int match = 1;
			if(va[i].c_class != c_class) match = 0;
			if(strictMatch)
			{
				if(va[i].depth != depth) match = 0;
				if(va[i].bpc != bpc && va[i].depth > 30) match = 0;
			}
			else
			{
				if(depth == 24 && (va[i].depth != 24
					&& (va[i].depth != 32 || va[i].bpc != 8)))
					match = 0;
				if(depth == 30 && (va[i].depth != 30
					&& (va[i].depth != 32 || va[i].bpc != 10)))
					match = 0;
				if(depth == 32 && bpc == 8 && (va[i].depth != 24
					&& (va[i].depth != depth || va[i].bpc != bpc)))
					match = 0;
				if(depth == 32 && bpc == 10 && (va[i].depth != 30
					&& (va[i].depth != depth || va[i].bpc != bpc)))
					match = 0;
			}
			if(fconfig.stereo == RRSTEREO_QUADBUF && tryStereo)
			{
				if(stereo != va[i].isStereo) match = 0;
				if(stereo && !va[i].isDB) match = 0;
				if(stereo && !va[i].isGL) match = 0;
				if(stereo && va[i].c_class != TrueColor
					&& va[i].c_class != DirectColor)
					match = 0;
			}
			if(match) return va[i].visualID;
		}
	}

	return 0;
}


// This function finds a 2D X server visual that is suitable for use with a
// particular 3D X server FB config.

static VisualID matchVisual2D(Display *dpy, int screen, VGLFBConfig config)
{
	VisualID vid = 0;

	if(!dpy || screen < 0 || !config) return 0;

	buildVisAttribTable(dpy, screen);
	GET_VA_TABLE()

	// Check if the FB config is already the default FB config for a visual and,
	// if so, return that visual.
	for(int i = 0; i < vaEntries; i++)
	{
		if(va[i].config && config->id == va[i].config->id)
			return va[i].visualID;
	}

	int depth, c_class, bpc;
	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		depth = config->depth;
		c_class = config->c_class;
		bpc = config->attr.redSize;
	}
	else
	#endif
	{
		XVisualInfo *vis = _glXGetVisualFromFBConfig(DPY3D, config->glx);
		if(!vis) return 0;
		depth = vis->depth;
		c_class = vis->c_class;
		bpc = vis->bits_per_rgb;
		_XFree(vis);
	}

	if(depth >= 24 && (c_class == TrueColor || c_class == DirectColor))
	{
		// We first try to match the FB config with a 2D X Server visual that
		// has the same class, depth, and stereo properties.
		vid = matchVisual2D(dpy, screen, depth, c_class, bpc, config->attr.stereo,
			true);
		if(!vid)
			vid = matchVisual2D(dpy, screen, depth, c_class, bpc,
				config->attr.stereo, false);
		// Failing that, we try to find a mono visual.
		if(!vid && config->attr.stereo)
			vid = matchVisual2D(dpy, screen, depth, c_class, bpc, 0, true);
		if(!vid && config->attr.stereo)
			vid = matchVisual2D(dpy, screen, depth, c_class, bpc, 0, false);
	}

	return vid;
}


static INLINE int getGLXFBConfigAttrib(GLXFBConfig config, int attribute)
{
	int value = 0;
	_glXGetFBConfigAttrib(DPY3D, config, attribute, &value);
	return value;
}


static void buildCfgAttribTable(Display *dpy, int screen)
{
	int nConfigs = 0;
	GLXFBConfig *glxConfigs = NULL;
	#ifdef EGLBACKEND
	EGLContext ctx = 0;
	#endif
	struct _VGLFBConfig *ca = NULL;
	XEDataObject obj;
	XExtData *extData;
	obj.screen = XScreenOfDisplay(dpy, screen);

	if(!fconfig.egl && dpy == faker::dpy3D)
		THROW("glxvisual::buildCfgAttribTable() called with 3D X server handle (this should never happen)");

	try
	{
		CriticalSection::SafeLock l(faker::getDisplayCS(dpy));

		int minExtensionNumber =
			XFindOnExtensionList(XEHeadOfExtensionList(obj), 0) ? 0 : 1;
		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj),
			minExtensionNumber + 3);
		if(extData && extData->private_data) return;

		#ifdef EGLBACKEND
		if(fconfig.egl)
		{
			int i = 0;
			int defaultDepth = DefaultDepth(dpy, screen);
			int nbpcs = defaultDepth == 30 ? 2 : 1;
			int bpcs[] = { defaultDepth == 30 ? 10 : 8, defaultDepth == 30 ? 8 : 0 };
			int maxSamples = 0, maxPBWidth = 32768, maxPBHeight = 32768, nsamps = 1;

			if(!_eglBindAPI(EGL_OPENGL_API))
				THROW("Could not enable OpenGL API");
			if(!(ctx = _eglCreateContext(EDPY, (EGLConfig)0, NULL, NULL)))
				THROW("Could not create temporary EGL context");
			{
				backend::TempContextEGL tc(ctx);

				_glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
				if(maxSamples > 0)
				{
					int temp = maxSamples;
					while(temp >>= 1) nsamps++;
				}
				else maxSamples = 0;

				GLint dims[2] = { -1, -1 };
				_glGetIntegerv(GL_MAX_VIEWPORT_DIMS, dims);
				if(dims[0] > 0) maxPBWidth = max(dims[0], maxPBWidth);
				if(dims[1] > 0) maxPBHeight = max(dims[1], maxPBHeight);
			}
			_eglDestroyContext(EDPY, ctx);  ctx = 0;

			nConfigs =
				2 *       // visual classes
				2 *       // stereo options
				2 *       // alpha channel options
				2 *       // double buffering options
				3 *       // stencil & depth buffer options
				nsamps *  // multisampling options (0, 2, 4, 8, 16, 32, 64)
				2 *       // visual depths
				nbpcs;    // bits-per-component options

			if(!(ca =
				(struct _VGLFBConfig *)calloc(nConfigs, sizeof(struct _VGLFBConfig))))
				THROW("Memory allocation error");

			for(int bpcIndex = 0; bpcIndex < nbpcs; bpcIndex++)
			{
				int bpc = bpcs[bpcIndex];
				for(int depth = bpc * 3; depth <= 32; depth += 32 - bpc * 3)
				{
					for(int samples = 0; samples <= maxSamples;
						samples = (samples ? samples * 2 : 2))
					{
						for(int depthBuffer = 1; depthBuffer >= 0; depthBuffer--)
						{
							for(int stencil = depthBuffer ? 1 : 0; stencil >= 0; stencil--)
							{
								for(int doubleBuffer = 1; doubleBuffer >= 0; doubleBuffer--)
								{
									for(int alpha = 0; alpha <= 1; alpha++)
									{
										for(int stereo = 0; stereo <= 1; stereo++)
										{
											for(int c_class = TrueColor; c_class <= DirectColor;
												c_class++)
											{
												ca[i].id = i + 1;
												ca[i].screen = screen;
												ca[i].nConfigs = nConfigs;

												ca[i].attr.doubleBuffer = doubleBuffer;
												ca[i].attr.stereo = stereo;
												ca[i].attr.redSize = ca[i].attr.greenSize =
													ca[i].attr.blueSize = bpc;
												ca[i].attr.alphaSize = alpha * (32 - bpc * 3);
												ca[i].attr.depthSize = depthBuffer * 24;
												ca[i].attr.stencilSize = stencil * 8;
												ca[i].attr.samples = samples;

												ca[i].c_class = c_class;
												ca[i].depth = depth;

												ca[i].maxPBWidth = maxPBWidth;
												ca[i].maxPBHeight = maxPBHeight;

												i++;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else  // fconfig.egl
		#endif
		{
			if(!(glxConfigs = _glXGetFBConfigs(DPY3D, DefaultScreen(DPY3D),
				&nConfigs)))
				THROW("No FB configs found");

			if(!(ca =
				(struct _VGLFBConfig *)calloc(nConfigs, sizeof(struct _VGLFBConfig))))
				THROW("Memory allocation error");

			for(int i = 0; i < nConfigs; i++)
			{
				ca[i].id = getGLXFBConfigAttrib(glxConfigs[i], GLX_FBCONFIG_ID);
				ca[i].screen = screen;
				ca[i].nConfigs = nConfigs;
				ca[i].attr.stereo = getGLXFBConfigAttrib(glxConfigs[i], GLX_STEREO);
				int drawableType =
					getGLXFBConfigAttrib(glxConfigs[i], GLX_DRAWABLE_TYPE);
				if((drawableType & (GLX_PBUFFER_BIT | GLX_WINDOW_BIT))
					== (GLX_PBUFFER_BIT | GLX_WINDOW_BIT))
				{
					ca[i].attr.alphaSize =
						getGLXFBConfigAttrib(glxConfigs[i], GLX_ALPHA_SIZE);
					ca[i].attr.stencilSize =
						getGLXFBConfigAttrib(glxConfigs[i], GLX_STENCIL_SIZE);
					ca[i].attr.samples =
						getGLXFBConfigAttrib(glxConfigs[i], GLX_SAMPLES);
				}
				else
				{
					ca[i].attr.alphaSize = ca[i].attr.stencilSize = ca[i].attr.samples =
						-1;
				}
				ca[i].glx = glxConfigs[i];
			}
		}

		if(!(extData = (XExtData *)calloc(1, sizeof(XExtData))))
			THROW("Memory allocation error");
		extData->private_data = (XPointer)ca;
		extData->number = 4;
		XAddToExtensionList(XEHeadOfExtensionList(obj), extData);

		for(int i = 0; i < nConfigs; i++)
		{
			ca[i].visualID = matchVisual2D(dpy, screen, &ca[i]);
			if(fconfig.trace && ca[i].visualID)
				vglout.println("[VGL] FB config 0x%.2x has attached visual 0x%.2x",
					ca[i].id, (unsigned int)ca[i].visualID);
		}

		_XFree(glxConfigs);
	}
	catch(...)
	{
		if(glxConfigs) _XFree(glxConfigs);
		#ifdef EGLBACKEND
		if(ctx) _eglDestroyContext(EDPY, ctx);
		#endif
		free(ca);
		throw;
	}
}


VGLFBConfig *configsFromVisAttribs(Display *dpy, int screen,
	const int attribs[], int &nElements, bool glx13)
{
	int glxattribs[MAX_ATTRIBS + 1], j = 0;
	int doubleBuffer = glx13 ? -1 : 0, redSize = -1, greenSize = -1,
		blueSize = -1, alphaSize = -1, samples = -1, stereo = 0,
		renderType = glx13 ? -1 : GLX_COLOR_INDEX_BIT,
		drawableType = glx13 ? -1 : GLX_WINDOW_BIT | GLX_PIXMAP_BIT,
		visualType = -1;

	for(int i = 0; attribs[i] != None && i < MAX_ATTRIBS; i++)
	{
		if(attribs[i] == GLX_DOUBLEBUFFER)
		{
			if(glx13) { doubleBuffer = attribs[i + 1];  i++; }
			else doubleBuffer = 1;
		}
		else if(attribs[i] == GLX_RGBA && !glx13) renderType = GLX_RGBA_BIT;
		else if(attribs[i] == GLX_RENDER_TYPE && glx13)
		{
			renderType = attribs[i + 1];  i++;
		}
		else if(attribs[i] == GLX_LEVEL)
		{
			if(attribs[i + 1] != 0) return NULL;
			i++;
		}
		else if(attribs[i] == GLX_STEREO)
		{
			if(glx13) { stereo = attribs[i + 1];  i++; }
			else stereo = 1;
		}
		else if(attribs[i] == GLX_RED_SIZE)
		{
			redSize = attribs[i + 1];  i++;
		}
		else if(attribs[i] == GLX_GREEN_SIZE)
		{
			greenSize = attribs[i + 1];  i++;
		}
		else if(attribs[i] == GLX_BLUE_SIZE)
		{
			blueSize = attribs[i + 1];  i++;
		}
		else if(attribs[i] == GLX_ALPHA_SIZE)
		{
			alphaSize = attribs[i + 1];  i++;
		}
		else if(attribs[i] == GLX_TRANSPARENT_TYPE)
		{
			if(attribs[i + 1] != GLX_NONE) return NULL;
			i++;
		}
		else if(attribs[i] == GLX_SAMPLES)
		{
			samples = attribs[i + 1];  i++;
		}
		else if(attribs[i] == GLX_X_VISUAL_TYPE)
		{
			visualType = attribs[i + 1];  i++;
		}
		else if(attribs[i] == GLX_VISUAL_ID) i++;
		else if(attribs[i] == GLX_X_RENDERABLE) i++;
		else if(attribs[i] == GLX_TRANSPARENT_INDEX_VALUE) i++;
		else if(attribs[i] == GLX_TRANSPARENT_RED_VALUE) i++;
		else if(attribs[i] == GLX_TRANSPARENT_GREEN_VALUE) i++;
		else if(attribs[i] == GLX_TRANSPARENT_BLUE_VALUE) i++;
		else if(attribs[i] == GLX_TRANSPARENT_ALPHA_VALUE) i++;
		else if(attribs[i] != GLX_USE_GL)
		{
			glxattribs[j++] = attribs[i];  glxattribs[j++] = attribs[i + 1];
			i++;
		}
	}
	if(renderType == GLX_COLOR_INDEX_BIT) return NULL;
	if(doubleBuffer >= 0)
	{
		glxattribs[j++] = GLX_DOUBLEBUFFER;  glxattribs[j++] = doubleBuffer;
	}
	if(fconfig.forcealpha == 1 && redSize > 0 && greenSize > 0 && blueSize > 0
		&& alphaSize < 1)
		alphaSize = 1;
	if(redSize >= 0)
	{
		glxattribs[j++] = GLX_RED_SIZE;  glxattribs[j++] = redSize;
	}
	if(greenSize >= 0)
	{
		glxattribs[j++] = GLX_GREEN_SIZE;  glxattribs[j++] = greenSize;
	}
	if(blueSize >= 0)
	{
		glxattribs[j++] = GLX_BLUE_SIZE;  glxattribs[j++] = blueSize;
	}
	if(alphaSize >= 0)
	{
		glxattribs[j++] = GLX_ALPHA_SIZE;  glxattribs[j++] = alphaSize;
	}
	if(fconfig.samples >= 0) samples = fconfig.samples;
	if(samples >= 0)
	{
		glxattribs[j++] = GLX_SAMPLES;  glxattribs[j++] = samples;
	}
	if(stereo)
	{
		glxattribs[j++] = GLX_STEREO;  glxattribs[j++] = stereo;
	}
	if(drawableType >= 0 && drawableType & GLX_WINDOW_BIT)
	{
		drawableType &= ~GLX_WINDOW_BIT;
		drawableType |= GLX_PBUFFER_BIT;
		if(visualType >= 0)
			drawableType |= GLX_WINDOW_BIT;
		if(samples >= 0) drawableType &= ~GLX_PIXMAP_BIT;
		renderType = GLX_RGBA_BIT;
	}
	if(renderType >= 0)
	{
		glxattribs[j++] = GLX_RENDER_TYPE;  glxattribs[j++] = renderType;
	}
	if(drawableType >= 0)
	{
		glxattribs[j++] = GLX_DRAWABLE_TYPE;  glxattribs[j++] = drawableType;
	}
	if(visualType >= 0)
	{
		glxattribs[j++] = GLX_X_VISUAL_TYPE;  glxattribs[j++] = visualType;
	}
	glxattribs[j] = None;

	if(fconfig.trace) PRARGAL13(glxattribs);

	return chooseFBConfig(dpy, screen, glxattribs, nElements);
}


int visAttrib(Display *dpy, int screen, VisualID vid, int attribute)
{
	if(!buildVisAttribTable(dpy, screen)) return 0;
	GET_VA_TABLE()

	for(int i = 0; i < vaEntries; i++)
	{
		if(va[i].visualID == vid)
		{
			if(attribute == GLX_STEREO)
			{
				return va[i].isStereo && va[i].isGL && va[i].isDB;
			}
			if(attribute == GLX_X_VISUAL_TYPE) return va[i].c_class;
		}
	}
	return 0;
}


XVisualInfo *visualFromID(Display *dpy, int screen, VisualID vid)
{
	XVisualInfo vtemp;  int n = 0;
	vtemp.visualid = vid;
	vtemp.screen = screen;
	return XGetVisualInfo(dpy, VisualIDMask | VisualScreenMask, &vtemp, &n);
}


VGLFBConfig *getFBConfigs(Display *dpy, int screen, int &nElements)
{
	VGLFBConfig *configs = NULL;

	if(!dpy || screen < 0) return NULL;

	buildCfgAttribTable(dpy, screen);
	GET_CA_TABLE()

	configs = (VGLFBConfig *)calloc(caEntries, sizeof(VGLFBConfig));
	if(!configs) return NULL;

	nElements = caEntries;
	for(int i = 0; i < nElements; i++)
		configs[i] = &ca[i];

	return configs;
}


#ifdef EGLBACKEND

static int compareFBConfig(const void *arg1, const void *arg2)
{
	VGLFBConfig *_c1 = (VGLFBConfig *)arg1, *_c2 = (VGLFBConfig *)arg2,
		c1 = *_c1, c2 = *_c2;

	// Prefer larger RGBA size
	if(c1->bufSize != c2->bufSize)
		return c2->bufSize - c1->bufSize;

	// Prefer smaller sample count
	if(c1->attr.samples != c2->attr.samples)
		return c1->attr.samples - c2->attr.samples;

	// Prefer larger depth buffer
	if(c1->attr.depthSize != c2->attr.depthSize)
		return c2->attr.depthSize - c1->attr.depthSize;

	// Prefer smaller stencil buffer
	if(c1->attr.stencilSize != c2->attr.stencilSize)
		return c1->attr.stencilSize - c2->attr.stencilSize;

	return 0;
}

static int compareFBConfigNoDepth(const void *arg1, const void *arg2)
{
	VGLFBConfig *_c1 = (VGLFBConfig *)arg1, *_c2 = (VGLFBConfig *)arg2,
		c1 = *_c1, c2 = *_c2;

	// Prefer smaller depth buffer
	if(c1->attr.depthSize != c2->attr.depthSize)
		return c1->attr.depthSize - c2->attr.depthSize;

	return compareFBConfig(arg1, arg2);
}

#define PARSE_ATTRIB(name, var, min, max) \
	case name: \
		var = attribs[++i]; \
		if(var < min || var > max) goto bailout; \
		break;

#define PARSE_ATTRIB_DC(name, var, min, max) \
	case name: \
		var = attribs[++i]; \
		if(var != (int)GLX_DONT_CARE && (var < min || var > max)) goto bailout; \
		break;

#endif

VGLFBConfig *chooseFBConfig(Display *dpy, int screen, const int attribs[],
	int &nElements)
{
	GLXFBConfig *glxConfigs = NULL;
	VGLFBConfig *configs = NULL;

	if(!dpy || screen < 0) return NULL;

	#ifdef EGLBACKEND
	if(fconfig.egl)
	{
		int fbConfigID = GLX_DONT_CARE, doubleBuffer = GLX_DONT_CARE, redSize = 0,
			greenSize = 0, blueSize = 0, alphaSize = 0, depthSize = 0,
			stencilSize = 0, samples = 0, drawableType = GLX_WINDOW_BIT,
			xRenderable = GLX_DONT_CARE, visualType = GLX_DONT_CARE, stereo = 0,
			sRGB = GLX_DONT_CARE;

		if(!attribs) return getFBConfigs(dpy, screen, nElements);

		buildCfgAttribTable(dpy, screen);
		GET_CA_TABLE()

		for(int i = 0; attribs[i] != None && i < MAX_ATTRIBS; i++)
		{
			switch(attribs[i])
			{
				PARSE_ATTRIB_DC(GLX_FBCONFIG_ID, fbConfigID, 1, caEntries)
				PARSE_ATTRIB_DC(GLX_DOUBLEBUFFER, doubleBuffer, 0, 1)
				PARSE_ATTRIB(GLX_STEREO, stereo, 0, 1)
				PARSE_ATTRIB_DC(GLX_RED_SIZE, redSize, 0, 10)
				PARSE_ATTRIB_DC(GLX_GREEN_SIZE, greenSize, 0, 10)
				PARSE_ATTRIB_DC(GLX_BLUE_SIZE, blueSize, 0, 10)
				PARSE_ATTRIB_DC(GLX_ALPHA_SIZE, alphaSize, 0, 8)
				PARSE_ATTRIB(GLX_DEPTH_SIZE, depthSize, 0, 24)
				PARSE_ATTRIB(GLX_STENCIL_SIZE, stencilSize, 0, 8)
				PARSE_ATTRIB(GLX_SAMPLES, samples, 0, 64)
				case GLX_DRAWABLE_TYPE:
					drawableType = attribs[++i];
					if(drawableType != (int)GLX_DONT_CARE && (drawableType &
						~(GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT)))
						goto bailout;
					break;
				PARSE_ATTRIB_DC(GLX_X_RENDERABLE, xRenderable, 0, 1)
				PARSE_ATTRIB_DC(GLX_X_VISUAL_TYPE, visualType, GLX_TRUE_COLOR,
					GLX_DIRECT_COLOR)
				PARSE_ATTRIB_DC(GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, sRGB, 0, 1);

				case GLX_LEVEL:
				case GLX_AUX_BUFFERS:
				case GLX_ACCUM_RED_SIZE:
				case GLX_ACCUM_GREEN_SIZE:
				case GLX_ACCUM_BLUE_SIZE:
				case GLX_ACCUM_ALPHA_SIZE:
					if(attribs[++i] != 0) goto bailout;
					break;
				case GLX_RENDER_TYPE:
					if(attribs[++i] != GLX_RGBA_BIT) goto bailout;
					break;
				case GLX_CONFIG_CAVEAT:
				case GLX_TRANSPARENT_TYPE:
					if(attribs[++i] != GLX_NONE) goto bailout;
					break;
				default:
					i++;
			}
		}

		configs = (VGLFBConfig *)calloc(caEntries, sizeof(VGLFBConfig));
		if(!configs) goto bailout;

		nElements = 0;
		for(int i = 0; i < caEntries; i++)
		{
			if(fbConfigID != (int)GLX_DONT_CARE && ca[i].id != fbConfigID)
				continue;
			if(doubleBuffer != (int)GLX_DONT_CARE
				&& ca[i].attr.doubleBuffer != doubleBuffer)
				continue;
			if(ca[i].attr.stereo != stereo)
				continue;
			if(redSize && redSize != (int)GLX_DONT_CARE
				&& ca[i].attr.redSize < redSize)
				continue;
			if(greenSize && greenSize != (int)GLX_DONT_CARE
				&& ca[i].attr.greenSize < greenSize)
				continue;
			if(blueSize && blueSize != (int)GLX_DONT_CARE
				&& ca[i].attr.blueSize < blueSize)
				continue;
			if(alphaSize && alphaSize != (int)GLX_DONT_CARE
				&& ca[i].attr.alphaSize < alphaSize)
				continue;
			if(ca[i].attr.depthSize < depthSize)
				continue;
			if(ca[i].attr.stencilSize < stencilSize)
				continue;
			if(ca[i].attr.samples < samples)
				continue;
			if((drawableType & (GLX_PIXMAP_BIT | GLX_WINDOW_BIT)) && !ca[i].visualID)
				continue;
			if(xRenderable != (int)GLX_DONT_CARE
				&& !!xRenderable != !!ca[i].visualID)
				continue;
			if(visualType != (int)GLX_DONT_CARE
				&& ((visualType == GLX_TRUE_COLOR && ca[i].c_class != TrueColor)
					|| (visualType == GLX_DIRECT_COLOR && ca[i].c_class != DirectColor)))
				continue;
			if(sRGB != (int)GLX_DONT_CARE
				&& sRGB != !!(ca[i].attr.redSize + ca[i].attr.greenSize +
					ca[i].attr.blueSize == 24))
				continue;

			configs[nElements] = &ca[i];
			configs[nElements++]->bufSize = ca[i].attr.redSize +
				ca[i].attr.greenSize + ca[i].attr.blueSize +
				(alphaSize && alphaSize != (int)GLX_DONT_CARE ?
					ca[i].attr.alphaSize : 0);
		}
		if(!nElements)
		{
			_XFree(configs);  return NULL;
		}

		configs = (VGLFBConfig *)realloc(configs, nElements * sizeof(VGLFBConfig));
		if(!configs) goto bailout;
		qsort(configs, nElements, sizeof(VGLFBConfig),
			depthSize == 0 ? compareFBConfigNoDepth : compareFBConfig);
	}
	else  // fconfig.egl
	#endif
	{
		glxConfigs = _glXChooseFBConfig(DPY3D, DefaultScreen(DPY3D), attribs,
			&nElements);
		if(!glxConfigs) goto bailout;

		buildCfgAttribTable(dpy, screen);
		GET_CA_TABLE()

		configs = (VGLFBConfig *)calloc(nElements, sizeof(VGLFBConfig));
		if(!configs) goto bailout;

		// Some Mesa implementations of glXChooseFBConfig() can return an FB config
		// that is not returned by glXGetFBConfigs().  Work around that by
		// filtering out any FB configs returned by glXChooseFBConfig() that do not
		// match an entry in the config attribute table.
		int nValidConfigs = 0;
		for(int i = 0; i < nElements; i++)
		{
			for(int j = 0; j < caEntries; j++)
			{
				if(ca[j].glx == glxConfigs[i])
				{
					configs[nValidConfigs++] = &ca[j];
					break;
				}
			}
		}
		nElements = nValidConfigs;
	}

	bailout:
	if(glxConfigs) _XFree(glxConfigs);
	return configs;
}


VGLFBConfig getDefaultFBConfig(Display *dpy, int screen, VisualID vid)
{
	if(!buildVisAttribTable(dpy, screen)) return NULL;
	GET_VA_TABLE()

	for(int i = 0; i < vaEntries; i++)
	{
		if(va[i].visualID == vid)
		{
			if(va[i].config) return va[i].config;

			if(va[i].glx.doubleBuffer < 0 || va[i].glx.alphaSize < 0
				|| va[i].glx.depthSize < 0 || va[i].glx.stencilSize < 0
				|| va[i].glx.samples < 0)
				return NULL;

			int glxattribs[] = { GLX_DOUBLEBUFFER, va[i].glx.doubleBuffer,
				GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, va[i].glx.alphaSize, GLX_RENDER_TYPE, GLX_RGBA_BIT,
				GLX_STEREO, va[i].isStereo,
				GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT |
					(va[i].glx.samples ? 0 : GLX_PIXMAP_BIT) | GLX_WINDOW_BIT,
				GLX_X_VISUAL_TYPE,
				va[i].c_class == DirectColor ? GLX_DIRECT_COLOR : GLX_TRUE_COLOR,
				GLX_DEPTH_SIZE, va[i].glx.depthSize,
				GLX_STENCIL_SIZE, va[i].glx.stencilSize,
				GLX_SAMPLES, va[i].glx.samples, None };
			if(va[i].depth == 30 || (va[i].depth == 32 && va[i].bpc == 10))
			{
				glxattribs[3] = glxattribs[5] = glxattribs[7] = 10;
			}

			int n;
			VGLFBConfig *configs = chooseFBConfig(dpy, screen, glxattribs, n);
			if(configs)
			{
				// Make sure that the FB config actually has the requested
				// attributes, i.e. that its attributes are unique among the
				// list of visuals.  Otherwise, skip it.
				int actualDB = getFBConfigAttrib(dpy, configs[0], GLX_DOUBLEBUFFER);
				int actualDepth = getFBConfigAttrib(dpy, configs[0], GLX_DEPTH_SIZE);

				if(configs[0]->attr.alphaSize >= 0
					&& !!configs[0]->attr.alphaSize == !!va[i].glx.alphaSize
					&& !!actualDB == !!va[i].glx.doubleBuffer
					&& configs[0]->attr.stencilSize >= 0
					&& !!configs[0]->attr.stencilSize == !!va[i].glx.stencilSize
					&& !!actualDepth == !!va[i].glx.depthSize
					&& configs[0]->attr.samples >= 0
					&& configs[0]->attr.samples == va[i].glx.samples)
				{
					if(fconfig.trace)
						vglout.println("[VGL] Visual 0x%.2x has default FB config 0x%.2x",
							(unsigned int)va[i].visualID, configs[0]->id);
					va[i].config = configs[0];
				}
				_XFree(configs);
			}
			return va[i].config;
		}
	}

	return 0;
}

}  // namespace
