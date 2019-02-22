/* Copyright (C)2017, 2019 D. R. Commander
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <GL/glx.h>


#define _throw(m) \
{ \
	retval = -1;  fprintf(stderr, "ERROR: %s\n", m);  goto bailout; \
}

#define _try(f) \
{ \
	if((f) < 0) \
	{ \
		retval = -1;  goto bailout; \
	} \
}


#define _glXChooseVisual  glXChooseVisual
#define _glClear  glClear
#define _glClearColor  glClearColor
#define _glXCreateContext  glXCreateContext
#define _glXDestroyContext  glXDestroyContext
#define _glXMakeCurrent  glXMakeCurrent
#define _glXSwapBuffers  glXSwapBuffers
#include "dlfakerut-test.c"
