
/*
 * OpenGL pbuffers utility functions.
 *
 * Brian Paul
 * April 1997
 */


#ifndef PBUTIL_H
#define PBUTIL_H


#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#ifndef GLX_SGIX_pbuffer
#define GLX_SGIX_pbuffer
#endif

#ifndef GLX_RGBA_BIT
#define GLX_RGBA_BIT 0x00000001
#endif
#ifndef GLX_LARGEST_PBUFFER
#define GLX_LARGEST_PBUFFER 0x801C
#endif
#ifndef GLX_PBUFFER_WIDTH
#define GLX_PBUFFER_WIDTH 0x8041
#endif
#ifndef GLX_PBUFFER_HEIGHT
#define GLX_PBUFFER_HEIGHT 0x8040
#endif

extern int
QueryPbuffers(Display *dpy, int screen);


extern void
PrintFBConfigInfo(Display *dpy, GLXFBConfig fbConfig, Bool horizFormat);


extern GLXPbuffer
CreatePbuffer( Display *dpy, GLXFBConfig fbConfig,
	       int *pbAttribs );


#endif  /*PBUTIL_H*/
