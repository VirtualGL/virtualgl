/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
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

int __vglConfigDepth(GLXFBConfig);

int __vglConfigClass(GLXFBConfig);

GLXFBConfig *__vglConfigsFromVisAttribs(const int attribs[],
	int &, int &, int &, int &, int &, int &, bool glx13=false);

int __vglClientVisualAttrib(Display *, int, VisualID, int);

int __vglServerVisualAttrib(GLXFBConfig, int);

int __vglVisualDepth(Display *, int, VisualID);

int __vglVisualClass(Display *, int, VisualID);

double __vglVisualGamma(Display *, int, VisualID);

bool __vglHasGCVisuals(Display *, int);

VisualID __vglMatchVisual(Display *, int, int, int, int, int, int);

XVisualInfo *__vglVisualFromVisualID(Display *, int, VisualID);

Status _XSolarisGetVisualGamma(Display *, int, Visual *, double *);

#define _FBCID(c) __vglServerVisualAttrib(c, GLX_FBCONFIG_ID)

#endif
