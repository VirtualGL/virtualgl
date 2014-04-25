/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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

#ifndef __GLXVISUAL_H__
#define __GLXVISUAL_H__

#include "faker-sym.h"


namespace glxvisual
{
	// Helper function used in glXChooseVisual() and glXChooseFBConfig().  It
	// returns a list of "VirtualGL-friendly" FB configs that fit the given
	// attribute list.  It also returns the visual parameters derived from that
	// same attribute list, so those parameters can be used to match an
	// appropriate visual on the 2D X server.
	GLXFBConfig *configsFromVisAttribs(const int attribs[], int &depth,
		int &c_class, int &level, int &stereo, int &trans, int &nElements,
		bool glx13=false);

	// These functions return attributes for visuals on the 2D X server (those
	// attributes are read from the 2D X server and cached on first access, so
	// only the first call to any of these will result in a round trip to the
	// 2D X server.)
	int visAttrib2D(Display *dpy, int screen, VisualID vid, int attribute);

	int visDepth2D(Display *dpy, int screen, VisualID vid);

	int visClass2D(Display *dpy, int screen, VisualID vid);

	// This function finds a 2D X server visual that matches the given
	// visual parameters.  As with the above functions, it uses the cached
	// attributes from the 2D X server, or it caches them if they have not
	// already been read.
	VisualID matchVisual2D(Display *dpy, int screen, int depth, int c_class,
		int level, int stereo, int trans);

	// Simple helper function that obtains an attribute for a GLXFBConfig on the
	// 3D X server.
	int visAttrib3D(GLXFBConfig config, int attribute);

	// This is just a convenience wrapper for XGetVisualInfo()
	XVisualInfo *visualFromID(Display *dpy, int screen, VisualID vid);
}


#define _FBCID(c) glxvisual::visAttrib3D(c, GLX_FBCONFIG_ID)

#endif // __GLXVISUAL_H__
