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
#include "Error.h"
#include "Mutex.h"
#include "faker.h"

using namespace vglutil;


typedef struct
{
	VisualID visualID;
	int depth, c_class, bpc;
	int level, isStereo, isDB, isGL, isTrans;
	int transIndex, transRed, transGreen, transBlue, transAlpha;
} VisAttrib;

static Display *vaDisplay = NULL;
static int vaScreen = -1, vaEntries = 0;
static CriticalSection vaMutex;
static VisAttrib *va;


static bool buildVisAttribTable(Display *dpy, int screen)
{
	int clientGLX = 0, majorOpcode = -1, firstEvent = -1, firstError = -1,
		nVisuals = 0;
	XVisualInfo *visuals = NULL, vtemp;
	Atom atom = 0;
	int len = 10000;

	try
	{
		CriticalSection::SafeLock l(vaMutex);

		if(dpy == vaDisplay && screen == vaScreen) return true;
		if(fconfig.probeglx
			&& _XQueryExtension(dpy, "GLX", &majorOpcode, &firstEvent, &firstError)
			&& majorOpcode >= 0 && firstEvent >= 0 && firstError >= 0)
			clientGLX = 1;
		vtemp.screen = screen;
		if(!(visuals = XGetVisualInfo(dpy, VisualScreenMask, &vtemp, &nVisuals))
			|| nVisuals == 0)
			THROW("No visuals found on display");

		delete [] va;
		NEWCHECK(va = new VisAttrib[nVisuals]);
		vaEntries = nVisuals;
		memset(va, 0, sizeof(VisAttrib) * nVisuals);

		for(int i = 0; i < nVisuals; i++)
		{
			va[i].visualID = visuals[i].visualid;
			va[i].depth = visuals[i].depth;
			va[i].c_class = visuals[i].c_class;
			va[i].bpc = visuals[i].bits_per_rgb;
		}

		if((atom = XInternAtom(dpy, "SERVER_OVERLAY_VISUALS", True)) != None)
		{
			struct overlay_info
			{
				unsigned long visualID;
				long transType, transPixel, level;
			} *olprop = NULL;
			unsigned long nop = 0, bytesLeft = 0;
			int actualFormat = 0;
			Atom actualType = 0;

			do
			{
				nop = 0;  actualFormat = 0;  actualType = 0;
				unsigned char *olproptemp = NULL;
				if(XGetWindowProperty(dpy, RootWindow(dpy, screen), atom, 0, len,
					False, atom, &actualType, &actualFormat, &nop, &bytesLeft,
					&olproptemp) != Success || nop < 4 || actualFormat != 32
					|| actualType != atom)
					goto done;
				olprop = (struct overlay_info *)olproptemp;
				len += (bytesLeft + 3) / 4;
				if(bytesLeft && olprop) { XFree(olprop);  olprop = NULL; }
			} while(bytesLeft);

			for(unsigned long i = 0; i < nop / 4; i++)
			{
				for(int j = 0; j < nVisuals; j++)
				{
					if(olprop[i].visualID == va[j].visualID)
					{
						va[j].isTrans = 1;
						if(olprop[i].transType == 1)  // Transparent pixel
							va[j].transIndex = olprop[i].transPixel;
						else if(olprop[i].transType == 2)  // Transparent mask
						{
							// Is this right??
							va[j].transRed = olprop[i].transPixel & 0xFF;
							va[j].transGreen = olprop[i].transPixel & 0x00FF;
							va[j].transBlue = olprop[i].transPixel & 0x0000FF;
							va[j].transAlpha = olprop[i].transPixel & 0x000000FF;
						}
						va[j].level = olprop[i].level;
					}
				}
			}

			done:
			if(olprop) { XFree(olprop);  olprop = NULL; }
		}

		for(int i = 0; i < nVisuals; i++)
		{
			if(clientGLX)
			{
				_glXGetConfig(dpy, &visuals[i], GLX_DOUBLEBUFFER, &va[i].isDB);
				_glXGetConfig(dpy, &visuals[i], GLX_USE_GL, &va[i].isGL);
				_glXGetConfig(dpy, &visuals[i], GLX_STEREO, &va[i].isStereo);
			}
		}

		XFree(visuals);
		vaDisplay = dpy;  vaScreen = screen;
	}
	catch(...)
	{
		if(visuals) XFree(visuals);
		delete [] va;  va = NULL;
		vaDisplay = NULL;  vaScreen = -1;  vaEntries = 0;
		return false;
	}
	return true;
}


namespace glxvisual {

GLXFBConfig *configsFromVisAttribs(const int attribs[], int &level,
	int &stereo, int &trans, int &nElements, bool glx13)
{
	int glxattribs[257], j = 0;
	int doubleBuffer = glx13 ? -1 : 0, redSize = -1, greenSize = -1,
		blueSize = -1, alphaSize = -1, samples = -1,
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
		else if(attribs[i] == GLX_LEVEL)
		{
			level = attribs[i + 1];  i++;
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
			if(attribs[i + 1] == GLX_TRANSPARENT_INDEX
				|| attribs[i + 1] == GLX_TRANSPARENT_RGB)
				trans = 1;
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

	return _glXChooseFBConfig(DPY3D, DefaultScreen(DPY3D), glxattribs,
		&nElements);
}


int visAttrib2D(Display *dpy, int screen, VisualID vid, int attribute)
{
	if(!buildVisAttribTable(dpy, screen)) return 0;

	for(int i = 0; i < vaEntries; i++)
	{
		if(va[i].visualID == vid)
		{
			if(attribute == GLX_LEVEL) return va[i].level;
			if(attribute == GLX_TRANSPARENT_TYPE)
			{
				if(va[i].isTrans)
				{
					if(va[i].c_class == TrueColor || va[i].c_class == DirectColor)
						return GLX_TRANSPARENT_RGB;
					else return GLX_TRANSPARENT_INDEX;
				}
				else return GLX_NONE;
			}
			if(attribute == GLX_TRANSPARENT_INDEX_VALUE)
			{
				if(fconfig.transpixel >= 0) return fconfig.transpixel;
				else return va[i].transIndex;
			}
			if(attribute == GLX_TRANSPARENT_RED_VALUE) return va[i].transRed;
			if(attribute == GLX_TRANSPARENT_GREEN_VALUE) return va[i].transGreen;
			if(attribute == GLX_TRANSPARENT_BLUE_VALUE) return va[i].transBlue;
			if(attribute == GLX_TRANSPARENT_ALPHA_VALUE) return va[i].transAlpha;
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
	int bpc, int level, int stereo, int trans, bool strictMatch)
{
	int i, tryStereo;
	if(!dpy) return 0;

	if(!buildVisAttribTable(dpy, screen)) return 0;

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
			if(level != va[i].level) match = 0;
			if(trans && !va[i].isTrans) match = 0;
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
