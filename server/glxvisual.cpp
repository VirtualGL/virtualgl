// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2009-2016, 2019 D. R. Commander
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

using namespace vglutil;


namespace glxvisual {

typedef struct
{
	int doubleBuffer, alphaSize, depthSize, stencilSize, samples;
} OGLAttrib;

typedef struct
{
	VisualID visualID;
	VGLFBConfig config;
	int depth, c_class, bpc, nVisuals;
	int isStereo, isDB, isGL;
	OGLAttrib ogl;
} VisAttrib;


#define GET_VA_TABLE() \
	VisAttrib *va;  int vaEntries; \
	XEDataObject obj; \
	XExtData *extData; \
	\
	obj.screen = XScreenOfDisplay(dpy, screen); \
	extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), 3); \
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
	extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), 4); \
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
		if(ca[i].alphaSize < 0 || ca[i].stencilSize < 0 || ca[i].samples < 0)
			continue;
		minAlpha = min(minAlpha, ca[i].alphaSize);
		maxAlpha = max(maxAlpha, ca[i].alphaSize);
		minStencil = min(minStencil, ca[i].stencilSize);
		maxStencil = max(maxStencil, ca[i].stencilSize);
		minSamples = min(minSamples, ca[i].samples);
		maxSamples = max(maxSamples, ca[i].samples);
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
						va[i].ogl.doubleBuffer = db;
						va[i].ogl.alphaSize = alpha;
						va[i].ogl.depthSize = depth;
						va[i].ogl.stencilSize = stencil;
						va[i++].ogl.samples = samples;
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

	if(dpy == vglfaker::dpy3D)
		THROW("glxvisual::buildVisAttribTable() called with 3D X server handle (this should never happen)");

	try
	{
		CriticalSection::SafeLock l(vglfaker::getDisplayCS(dpy));

		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), 3);
		if(extData && extData->private_data) return true;

		if(fconfig.probeglx
			&& _XQueryExtension(dpy, "GLX", &majorOpcode, &firstEvent, &firstError)
			&& majorOpcode >= 0 && firstEvent >= 0 && firstError >= 0)
			clientGLX = 1;
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
			va[i].ogl.alphaSize = va[i].ogl.depthSize = va[i].ogl.stencilSize =
				va[i].ogl.samples = -1;
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
		XFree(depths);
		XFree(visuals);

		if(!(extData = (XExtData *)calloc(1, sizeof(XExtData))))
			THROW("Memory allocation error");
		extData->private_data = (XPointer)va;
		extData->number = 3;
		XAddToExtensionList(XEHeadOfExtensionList(obj), extData);
	}
	catch(...)
	{
		if(visuals) XFree(visuals);
		if(va) free(va);
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

	XVisualInfo *vis = _glXGetVisualFromFBConfig(DPY3D, config->glxConfig);
	if(vis)
	{
		if(vis->depth >= 24
			&& (vis->c_class == TrueColor || vis->c_class == DirectColor))
		{
			// We first try to match the FB config with a 2D X Server visual that has
			// the same class, depth, and stereo properties.
			vid = matchVisual2D(dpy, screen, vis->depth, vis->c_class,
				vis->bits_per_rgb, config->stereo, true);
			if(!vid)
				vid = matchVisual2D(dpy, screen, vis->depth, vis->c_class,
					vis->bits_per_rgb, config->stereo, false);
			// Failing that, we try to find a mono visual.
			if(!vid && config->stereo)
				vid = matchVisual2D(dpy, screen, vis->depth, vis->c_class,
					vis->bits_per_rgb, 0, true);
			if(!vid && config->stereo)
				vid = matchVisual2D(dpy, screen, vis->depth, vis->c_class,
					vis->bits_per_rgb, 0, false);
		}
		XFree(vis);
	}

	return vid;
}


static void buildCfgAttribTable(Display *dpy, int screen)
{
	int nConfigs = 0;
	GLXFBConfig *glxConfigs = NULL;
	struct _VGLFBConfig *ca = NULL;
	XEDataObject obj;
	XExtData *extData;
	obj.screen = XScreenOfDisplay(dpy, screen);

	if(dpy == vglfaker::dpy3D)
		THROW("glxvisual::buildCfgAttribTable() called with 3D X server handle (this should never happen)");

	try
	{
		CriticalSection::SafeLock l(vglfaker::getDisplayCS(dpy));

		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), 4);
		if(extData && extData->private_data) return;

		if(!(glxConfigs = _glXGetFBConfigs(DPY3D, DefaultScreen(DPY3D),
			&nConfigs)))
			THROW("No FB configs found");

		if(!(ca =
			(struct _VGLFBConfig *)calloc(nConfigs, sizeof(struct _VGLFBConfig))))
			THROW("Memory allocation error");

		for(int i = 0; i < nConfigs; i++)
		{
			ca[i].id = visAttrib3D(glxConfigs[i], GLX_FBCONFIG_ID);
			ca[i].screen = screen;
			ca[i].nConfigs = nConfigs;
			int drawableType = visAttrib3D(glxConfigs[i], GLX_DRAWABLE_TYPE);
			if((drawableType & (GLX_PBUFFER_BIT | GLX_WINDOW_BIT))
				== (GLX_PBUFFER_BIT | GLX_WINDOW_BIT))
			{
				ca[i].alphaSize = visAttrib3D(glxConfigs[i], GLX_ALPHA_SIZE);
				ca[i].stencilSize = visAttrib3D(glxConfigs[i], GLX_STENCIL_SIZE);
				ca[i].samples = visAttrib3D(glxConfigs[i], GLX_SAMPLES);
			}
			else
			{
				ca[i].alphaSize = ca[i].stencilSize = ca[i].samples = -1;
			}
			ca[i].stereo = visAttrib3D(glxConfigs[i], GLX_STEREO);
			ca[i].glxConfig = glxConfigs[i];
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

		XFree(glxConfigs);
	}
	catch(...)
	{
		if(glxConfigs) XFree(glxConfigs);
		if(ca) free(ca);
		throw;
	}
}


VGLFBConfig *configsFromVisAttribs(Display *dpy, int screen,
	const int attribs[], int &nElements, bool glx13)
{
	int glxattribs[257], j = 0;
	int doubleBuffer = glx13 ? -1 : 0, redSize = -1, greenSize = -1,
		blueSize = -1, alphaSize = -1, samples = -1, stereo = 0,
		renderType = glx13 ? -1 : GLX_COLOR_INDEX_BIT,
		drawableType = glx13 ? -1 : GLX_WINDOW_BIT | GLX_PIXMAP_BIT,
		visualType = -1;

	for(int i = 0; attribs[i] != None && i <= 254; i++)
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
		else if(attribs[i] == GLX_LEVEL && attribs[++i] != 0)
			return NULL;
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
		else if(attribs[i] == GLX_TRANSPARENT_TYPE && attribs[++i] != GLX_NONE)
			return NULL;
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
		if(fconfig.drawable == RRDRAWABLE_PIXMAP)
			drawableType |= GLX_PIXMAP_BIT | GLX_WINDOW_BIT;
		else
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


int visAttrib2D(Display *dpy, int screen, VisualID vid, int attribute)
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


int visAttrib3D(GLXFBConfig config, int attribute)
{
	int value = 0;
	_glXGetFBConfigAttrib(DPY3D, config, attribute, &value);
	return value;
}


int visAttrib3D(VGLFBConfig config, int attribute)
{
	return visAttrib3D(GLXFBC(config), attribute);
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


VGLFBConfig *chooseFBConfig(Display *dpy, int screen, const int attribs[],
	int &nElements)
{
	GLXFBConfig *glxConfigs = NULL;
	VGLFBConfig *configs = NULL;

	if(!dpy || screen < 0) return NULL;

	glxConfigs = _glXChooseFBConfig(DPY3D, DefaultScreen(DPY3D), attribs,
		&nElements);
	if(!glxConfigs) goto bailout;

	buildCfgAttribTable(dpy, screen);
	GET_CA_TABLE()

	configs = (VGLFBConfig *)calloc(nElements, sizeof(VGLFBConfig));
	if(!configs) goto bailout;

	for(int i = 0; i < nElements; i++)
	{
		for(int j = 0; j < caEntries; j++)
		{
			if(ca[j].glxConfig == glxConfigs[i])
			{
				configs[i] = &ca[j];
				break;
			}
		}
	}

	bailout:
	if(glxConfigs) XFree(glxConfigs);
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

			if(va[i].ogl.doubleBuffer < 0 || va[i].ogl.alphaSize < 0
				|| va[i].ogl.depthSize < 0 || va[i].ogl.stencilSize < 0
				|| va[i].ogl.samples < 0)
				return NULL;

			int glxattribs[] = { GLX_DOUBLEBUFFER, va[i].ogl.doubleBuffer,
				GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, va[i].ogl.alphaSize, GLX_RENDER_TYPE, GLX_RGBA_BIT,
				GLX_STEREO, va[i].isStereo,
				GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT |
					(va[i].ogl.samples ? 0 : GLX_PIXMAP_BIT) | GLX_WINDOW_BIT,
				GLX_X_VISUAL_TYPE,
				va[i].c_class == DirectColor ? GLX_DIRECT_COLOR : GLX_TRUE_COLOR,
				GLX_DEPTH_SIZE, va[i].ogl.depthSize,
				GLX_STENCIL_SIZE, va[i].ogl.stencilSize,
				GLX_SAMPLES, va[i].ogl.samples, None };
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
				int actualDB = visAttrib3D(configs[0], GLX_DOUBLEBUFFER);
				int actualDepth = visAttrib3D(configs[0], GLX_DEPTH_SIZE);

				if(configs[0]->alphaSize >= 0
					&& !!configs[0]->alphaSize == !!va[i].ogl.alphaSize
					&& !!actualDB == !!va[i].ogl.doubleBuffer
					&& configs[0]->stencilSize >= 0
					&& !!configs[0]->stencilSize == !!va[i].ogl.stencilSize
					&& !!actualDepth == !!va[i].ogl.depthSize
					&& configs[0]->samples >= 0
					&& configs[0]->samples == va[i].ogl.samples)
				{
					if(fconfig.trace)
						vglout.println("[VGL] Visual 0x%.2x has default FB config 0x%.2x",
							(unsigned int)va[i].visualID, configs[0]->id);
					va[i].config = configs[0];
				}
				XFree(configs);
			}
			return va[i].config;
		}
	}

	return 0;
}

}  // namespace
