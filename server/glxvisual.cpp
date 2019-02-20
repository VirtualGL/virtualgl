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


namespace glxvisual {

GLXFBConfig *configsFromVisAttribs(const int attribs[], int &c_class,
	int &stereo, int &nElements, bool glx13)
{
	int glxattribs[257], j = 0;
	int doubleBuffer = 0, redSize = -1, greenSize = -1, blueSize = -1,
		alphaSize = -1, samples = -1,
		renderType = glx13 ? GLX_RGBA_BIT : GLX_COLOR_INDEX_BIT,
		visualType = GLX_TRUE_COLOR;

	c_class = TrueColor;

	for(int i = 0; attribs[i] != None && i <= 254; i++)
	{
		if(attribs[i] == GLX_DOUBLEBUFFER)
		{
			if(glx13) { doubleBuffer = attribs[i + 1];  i++; }
			else doubleBuffer = 1;
		}
		else if(attribs[i] == GLX_RGBA) renderType = GLX_RGBA_BIT;
		else if(attribs[i] == GLX_RENDER_TYPE)
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
		else if(attribs[i] == GLX_DRAWABLE_TYPE) i++;
		else if(attribs[i] == GLX_X_VISUAL_TYPE)
		{
			int temp = attribs[i + 1];  i++;
			if(temp == GLX_DIRECT_COLOR)
			{
				visualType = temp;  c_class = DirectColor;
			}
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
	glxattribs[j++] = GLX_DOUBLEBUFFER;  glxattribs[j++] = doubleBuffer;
	glxattribs[j++] = GLX_RENDER_TYPE;  glxattribs[j++] = renderType;
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
	glxattribs[j++] = GLX_DRAWABLE_TYPE;
	if(fconfig.drawable == RRDRAWABLE_PIXMAP)
		glxattribs[j++] = GLX_PIXMAP_BIT | GLX_WINDOW_BIT;
	else
	{
		// Multisampling cannot be used with Pixmap rendering, and on nVidia GPUs,
		// there are no multisample-enabled FB configs that support GLX_PIXMAP_BIT.
		if(samples > 0)
			glxattribs[j++] = GLX_PBUFFER_BIT;
		else
			glxattribs[j++] = GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;
	}
	glxattribs[j++] = GLX_X_VISUAL_TYPE;  glxattribs[j++] = visualType;
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


VisualID matchVisual2D(Display *dpy, int screen, int depth, int c_class,
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


XVisualInfo *visualFromID(Display *dpy, int screen, VisualID vid)
{
	XVisualInfo vtemp;  int n = 0;
	vtemp.visualid = vid;
	vtemp.screen = screen;
	return XGetVisualInfo(dpy, VisualIDMask | VisualScreenMask, &vtemp, &n);
}

}  // namespace
