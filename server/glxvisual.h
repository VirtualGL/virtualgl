// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2014, 2019-2021 D. R. Commander
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

#ifndef __GLXVISUAL_H__
#define __GLXVISUAL_H__

#include "faker-sym.h"
#ifdef EGLBACKEND
#include "RBOContext.h"
#endif


typedef struct
{
	int doubleBuffer, stereo, redSize, greenSize, blueSize, alphaSize, depthSize,
		stencilSize, samples;
} GLXAttrib;

typedef struct _VGLFBConfig
{
	// GLX back end only
	GLXFBConfig glx;

	int id, screen, nConfigs;
	VisualID visualID;
	GLXAttrib attr;
	int c_class, depth;
	int bufSize;  // For sorting purposes only

	// EGL back end only
	int maxPBWidth, maxPBHeight;
} *VGLFBConfig;


namespace glxvisual
{
	// Helper function used in glXChooseVisual() and glXChooseFBConfig().  It
	// returns a list of "VirtualGL-friendly" FB configs that fit the given
	// attribute list.
	VGLFBConfig *configsFromVisAttribs(Display *dpy, int screen,
		const int attribs[], int &nElements, bool glx13 = false);

	// These functions return attributes for visuals on the 2D X server (those
	// attributes are read from the 2D X server and cached on first access, so
	// only the first call to any of these will result in a round trip to the
	// 2D X server.)
	int visAttrib(Display *dpy, int screen, VisualID vid, int attribute);

	// This just wraps backend::getFBConfigAttrib() to allow an FB config
	// attribute to be obtained with a one-liner.
	int INLINE getFBConfigAttrib(Display *dpy, VGLFBConfig config, int attribute)
	{
		int value = 0;
		backend::getFBConfigAttrib(dpy, config, attribute, &value);
		return value;
	}

	// This is just a convenience wrapper for XGetVisualInfo()
	XVisualInfo *visualFromID(Display *dpy, int screen, VisualID vid);

	// This function : VGLFBConfig :: glXChooseFBConfig() : GLXFBConfig
	VGLFBConfig *chooseFBConfig(Display *dpy, int screen, const int attribs[],
		int &nElements);

	// This function : VGLFBConfig :: glXGetFBConfigs() : GLXFBConfig
	VGLFBConfig *getFBConfigs(Display *dpy, int screen, int &nElements);

	// This function returns the default FB config attached to a given visual ID
	// in the visual attribute table.
	VGLFBConfig getDefaultFBConfig(Display *dpy, int screen, VisualID vid);
}


#define GLXFBC(c)  ((c) ? (c)->glx : 0)
#define FBCID(c)  ((c) ? (c)->id : 0)
#define VALID_CONFIG(c) \
	((c) && ((!fconfig.egl && (c)->glx) || (fconfig.egl && (c)->id > 0)))

#endif  // __GLXVISUAL_H__
