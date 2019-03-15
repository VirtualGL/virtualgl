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

// This class attempts to manage visual properties across Windows and Pbuffers
// and across GLX 1.0 and 1.3 in a sane manner

#include "glxvisual.h"
#include <stdio.h>
#include <stdlib.h>
#include "Error.h"
#include "Mutex.h"
#include "faker.h"

using namespace vglutil;


namespace glxvisual {

typedef struct
{
	VisualID visualID;
	int depth, c_class, nVisuals;
	int isStereo, isDB, isGL;
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


static void buildVisAttribTable(Display *dpy, int screen)
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
		if(extData && extData->private_data) return;

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
			va[i].nVisuals = nVisuals;

			if(clientGLX)
			{
				_glXGetConfig(dpy, &visuals[i], GLX_DOUBLEBUFFER, &va[i].isDB);
				_glXGetConfig(dpy, &visuals[i], GLX_USE_GL, &va[i].isGL);
				_glXGetConfig(dpy, &visuals[i], GLX_STEREO, &va[i].isStereo);
			}
		}

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
		throw;
	}
}


// This function finds a 2D X server visual that matches the given
// visual parameters.  As with the above functions, it uses the cached
// attributes from the 2D X server, or it caches them if they have not
// already been read.

static VisualID matchVisual2D(Display *dpy, int screen, int depth, int c_class,
	int stereo)
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
			if(va[i].depth != depth) match = 0;
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

static VisualID matchVisual2D(Display *dpy, int screen, GLXFBConfig config,
	bool stereo)
{
	VisualID vid = 0;

	if(!dpy || screen < 0 || !config) return 0;

	XVisualInfo *vis = _glXGetVisualFromFBConfig(DPY3D, config);
	if(vis)
	{
		// We first try to match the FB config with a 2D X Server visual that has
		// the same class, depth, and stereo properties.
		if(vis->depth >= 24
			&& (vis->c_class == TrueColor || vis->c_class == DirectColor))
			vid = matchVisual2D(dpy, screen, vis->depth, vis->c_class, stereo);
		XFree(vis);

		// Failing that, we try to find a TrueColor visual with the same stereo
		// properties, using the default depth of the 2D X server.
		if(!vid)
			vid = matchVisual2D(dpy, screen, DefaultDepth(dpy, screen), TrueColor,
				stereo);
		// Failing that, we try to find a TrueColor mono visual.
		if(!vid)
			vid = matchVisual2D(dpy, screen, DefaultDepth(dpy, screen), TrueColor,
				0);
	}

	return vid;
}


typedef struct
{
	int configID, nConfigs;
	VisualID visualID;
} CfgAttrib;


#define GET_CA_TABLE() \
	CfgAttrib *ca;  int caEntries; \
	XEDataObject obj; \
	XExtData *extData; \
	\
	obj.screen = XScreenOfDisplay(dpy, screen); \
	extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), 5); \
	if(!extData) \
		THROW("Could not retrieve FB config attribute table for screen"); \
	ca = (CfgAttrib *)extData->private_data; \
	caEntries = ca[0].nConfigs;


static void buildCfgAttribTable(Display *dpy, int screen)
{
	int nConfigs = 0;
	GLXFBConfig *configs = NULL;
	CfgAttrib *ca = NULL;
	XEDataObject obj;
	XExtData *extData;
	obj.screen = XScreenOfDisplay(dpy, screen);

	if(dpy == vglfaker::dpy3D)
		THROW("glxvisual::buildCfgAttribTable() called with 3D X server handle (this should never happen)");

	try
	{
		CriticalSection::SafeLock l(vglfaker::getDisplayCS(dpy));

		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), 5);
		if(extData && extData->private_data) return;

		if(!(configs = _glXGetFBConfigs(DPY3D, DefaultScreen(DPY3D), &nConfigs)))
			THROW("No FB configs found");

		if(!(ca = (CfgAttrib *)calloc(nConfigs, sizeof(CfgAttrib))))
			THROW("Memory allocation error");

		for(int i = 0; i < nConfigs; i++)
		{
			ca[i].configID = FBCID(configs[i]);
			ca[i].visualID = matchVisual2D(dpy, screen, configs[i],
				visAttrib3D(configs[i], GLX_STEREO));
			ca[i].nConfigs = nConfigs;
		}

		XFree(configs);

		if(!(extData = (XExtData *)calloc(1, sizeof(XExtData))))
			THROW("Memory allocation error");
		extData->private_data = (XPointer)ca;
		extData->number = 5;
		XAddToExtensionList(XEHeadOfExtensionList(obj), extData);
	}
	catch(...)
	{
		if(configs) XFree(configs);
		if(ca) free(ca);
		throw;
	}
}


GLXFBConfig *configsFromVisAttribs(const int attribs[], int &nElements,
	bool glx13)
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
		else if(attribs[i] == GLX_LEVEL) i++;
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
		else if(attribs[i] == GLX_TRANSPARENT_TYPE) i++;
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

	return _glXChooseFBConfig(DPY3D, DefaultScreen(DPY3D), glxattribs,
		&nElements);
}


int visAttrib2D(Display *dpy, int screen, VisualID vid, int attribute)
{
	buildVisAttribTable(dpy, screen);
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


XVisualInfo *visualFromID(Display *dpy, int screen, VisualID vid)
{
	XVisualInfo vtemp;  int n = 0;
	vtemp.visualid = vid;
	vtemp.screen = screen;
	return XGetVisualInfo(dpy, VisualIDMask | VisualScreenMask, &vtemp, &n);
}


VisualID getAttachedVisualID(Display *dpy, int screen, GLXFBConfig config)
{
	if(!dpy || screen < 0 || !config) return 0;

	buildCfgAttribTable(dpy, screen);
	GET_CA_TABLE()

	int fbcid = FBCID(config);
	for(int i = 0; i < caEntries; i++)
	{
		if(fbcid == ca[i].configID)
			return ca[i].visualID;
	}

	return 0;
}


void attachVisualID(Display *dpy, int screen, GLXFBConfig config, VisualID vid)
{
	if(!dpy || screen < 0 || !config || !vid) return;

	buildCfgAttribTable(dpy, screen);
	GET_CA_TABLE()

	int fbcid = FBCID(config);
	for(int i = 0; i < caEntries; i++)
	{
		if(fbcid == ca[i].configID)
		{
			ca[i].visualID = vid;
			return;
		}
	}
}

}  // namespace
