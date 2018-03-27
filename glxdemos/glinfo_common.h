/*
 * Copyright (C) 1999-2014  Brian Paul   All Rights Reserved.
 * Copyright (C) 2018  D. R. Commander   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * Common code shared by glxinfo and wglinfo.
 */

#ifndef GLINFO_COMMON_H
#define GLINFO_COMMON_H


#ifdef _WIN32
/* GL/glext.h is not commonly available on Windows. */
#include <GL/glew.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

typedef void (APIENTRY * GETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef const GLubyte *(APIENTRY * GETSTRINGIPROC) (GLenum name, GLuint index);
typedef void (APIENTRY * GETCONVOLUTIONPARAMETERIVPROC) (GLenum target, GLenum pname, GLint *params);


/**
 * Ext functions needed in common code but must be provided by
 * glxinfo or wglinfo.
 */
struct ext_functions
{
   GETPROGRAMIVARBPROC GetProgramivARB;
   GETSTRINGIPROC GetStringi;
   GETCONVOLUTIONPARAMETERIVPROC GetConvolutionParameteriv;
};


#define ELEMENTS(array) (sizeof(array) / sizeof(array[0]))


struct bit_info
{
   int bit;
   const char *name;
};


typedef enum
{
   Normal,
   Wide,
   Verbose,
   Brief
} InfoMode;


struct options
{
   InfoMode mode;
   GLboolean findBest;
   GLboolean limits;
   GLboolean singleLine;
   /* GLX only */
   char *displayName;
   GLboolean allowDirect;
   GLboolean fbConfigs;
};


/** list of known OpenGL versions */
static const struct { int major, minor; } gl_versions[] = {
   {4, 6},
   {4, 5},
   {4, 4},
   {4, 3},
   {4, 2},
   {4, 1},
   {4, 0},

   {3, 3},
   {3, 2},
   {3, 1},
   {3, 0},

   {2, 1},
   {2, 0},

   {1, 5},
   {1, 4},
   {1, 3},
   {1, 2},
   {1, 1},
   {1, 0},

   {0, 0} /* end of list */
};


void
print_extension_list(const char *ext, GLboolean singleLine);

char *
build_core_profile_extension_list(const struct ext_functions *extfuncs);

GLboolean
extension_supported(const char *ext, const char *extensionsList);

void
print_limits(const char *extensions, const char *oglstring, int version,
             const struct ext_functions *extfuncs);

const char *
bitmask_to_string(const struct bit_info bits[], int numBits, int mask);

const char *
profile_mask_string(int mask);

const char *
context_flags_string(int mask);


void
parse_args(int argc, char *argv[], struct options *options);

void
print_gpu_memory_info(const char *glExtensions);

#endif /* GLINFO_COMMON_H */
