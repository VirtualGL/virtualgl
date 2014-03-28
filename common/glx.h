#ifdef SYSGLXHEADERS
#include <GL/glx.h>
#else

#ifndef __glx_h__
#define __glx_h__

/*
** The contents of this file are subject to the GLX Public License Version 1.0
** (the "License"). You may not use this file except in compliance with the
** License. You may obtain a copy of the License at Silicon Graphics, Inc.,
** attn: Legal Services, 2011 N. Shoreline Blvd., Mountain View, CA 94043
** or at http://www.sgi.com/software/opensource/glx/license.html.
**
** Software distributed under the License is distributed on an "AS IS"
** basis. ALL WARRANTIES ARE DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY
** IMPLIED WARRANTIES OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR
** PURPOSE OR OF NON- INFRINGEMENT. See the License for the specific
** language governing rights and limitations under the License.
**
** The Original Software is GLX version 1.2 source code, released February,
** 1999. The developer of the Original Software is Silicon Graphics, Inc.
** Those portions of the Subject Software created by Silicon Graphics, Inc.
** are Copyright (c) 1991-9 Silicon Graphics, Inc. All Rights Reserved.
**
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Names for attributes to glXGetConfig.
 */
#define GLX_USE_GL              1       /* support GLX rendering */
#define GLX_BUFFER_SIZE         2       /* depth of the color buffer */
#define GLX_LEVEL               3       /* level in plane stacking */
#define GLX_RGBA                4       /* true if RGBA mode */
#define GLX_DOUBLEBUFFER        5       /* double buffering supported */
#define GLX_STEREO              6       /* stereo buffering supported */
#define GLX_AUX_BUFFERS         7       /* number of aux buffers */
#define GLX_RED_SIZE            8       /* number of red component bits */
#define GLX_GREEN_SIZE          9       /* number of green component bits */
#define GLX_BLUE_SIZE           10      /* number of blue component bits */
#define GLX_ALPHA_SIZE          11      /* number of alpha component bits */
#define GLX_DEPTH_SIZE          12      /* number of depth bits */
#define GLX_STENCIL_SIZE        13      /* number of stencil bits */
#define GLX_ACCUM_RED_SIZE      14      /* number of red accum bits */
#define GLX_ACCUM_GREEN_SIZE    15      /* number of green accum bits */
#define GLX_ACCUM_BLUE_SIZE     16      /* number of blue accum bits */
#define GLX_ACCUM_ALPHA_SIZE    17      /* number of alpha accum bits */

/*
 * Error return values from glXGetConfig.  Success is indicated by
 * a value of 0.
 */
#define GLX_BAD_SCREEN                  1  /* screen # is bad */
#define GLX_BAD_ATTRIBUTE               2  /* attribute to get is bad */
#define GLX_NO_EXTENSION                3  /* no glx extension on server */
#define GLX_BAD_VISUAL                  4  /* visual # not known by GLX */
#define GLX_BAD_CONTEXT                 5
#define GLX_BAD_VALUE                   6
#define GLX_BAD_ENUM                    7

/*
 * Names for attributes to glXGetClientString.
 */
#define GLX_VENDOR                      0x1
#define GLX_VERSION                     0x2
#define GLX_EXTENSIONS                  0x3

#ifndef GLX_VERSION_1_3
#define GLX_WINDOW_BIT                     0x00000001
#define GLX_PIXMAP_BIT                     0x00000002
#define GLX_PBUFFER_BIT                    0x00000004
#define GLX_RGBA_BIT                       0x00000001
#define GLX_COLOR_INDEX_BIT                0x00000002
#define GLX_PBUFFER_CLOBBER_MASK           0x08000000
#define GLX_FRONT_LEFT_BUFFER_BIT          0x00000001
#define GLX_FRONT_RIGHT_BUFFER_BIT         0x00000002
#define GLX_BACK_LEFT_BUFFER_BIT           0x00000004
#define GLX_BACK_RIGHT_BUFFER_BIT          0x00000008
#define GLX_AUX_BUFFERS_BIT                0x00000010
#define GLX_DEPTH_BUFFER_BIT               0x00000020
#define GLX_STENCIL_BUFFER_BIT             0x00000040
#define GLX_ACCUM_BUFFER_BIT               0x00000080
#define GLX_CONFIG_CAVEAT                  0x20
#define GLX_X_VISUAL_TYPE                  0x22
#define GLX_TRANSPARENT_TYPE               0x23
#define GLX_TRANSPARENT_INDEX_VALUE        0x24
#define GLX_TRANSPARENT_RED_VALUE          0x25
#define GLX_TRANSPARENT_GREEN_VALUE        0x26
#define GLX_TRANSPARENT_BLUE_VALUE         0x27
#define GLX_TRANSPARENT_ALPHA_VALUE        0x28
#define GLX_DONT_CARE                      0xFFFFFFFF
#define GLX_NONE                           0x8000
#define GLX_SLOW_CONFIG                    0x8001
#define GLX_TRUE_COLOR                     0x8002
#define GLX_DIRECT_COLOR                   0x8003
#define GLX_PSEUDO_COLOR                   0x8004
#define GLX_STATIC_COLOR                   0x8005
#define GLX_GRAY_SCALE                     0x8006
#define GLX_STATIC_GRAY                    0x8007
#define GLX_TRANSPARENT_RGB                0x8008
#define GLX_TRANSPARENT_INDEX              0x8009
#define GLX_VISUAL_ID                      0x800B
#define GLX_SCREEN                         0x800C
#define GLX_NON_CONFORMANT_CONFIG          0x800D
#define GLX_DRAWABLE_TYPE                  0x8010
#define GLX_RENDER_TYPE                    0x8011
#define GLX_X_RENDERABLE                   0x8012
#define GLX_FBCONFIG_ID                    0x8013
#define GLX_RGBA_TYPE                      0x8014
#define GLX_COLOR_INDEX_TYPE               0x8015
#define GLX_MAX_PBUFFER_WIDTH              0x8016
#define GLX_MAX_PBUFFER_HEIGHT             0x8017
#define GLX_MAX_PBUFFER_PIXELS             0x8018
#define GLX_PRESERVED_CONTENTS             0x801B
#define GLX_LARGEST_PBUFFER                0x801C
#define GLX_WIDTH                          0x801D
#define GLX_HEIGHT                         0x801E
#define GLX_EVENT_MASK                     0x801F
#define GLX_DAMAGED                        0x8020
#define GLX_SAVED                          0x8021
#define GLX_WINDOW                         0x8022
#define GLX_PBUFFER                        0x8023
#define GLX_PBUFFER_HEIGHT                 0x8040
#define GLX_PBUFFER_WIDTH                  0x8041
#endif

#ifndef GLX_ARB_get_proc_address
typedef void (*__GLXextFuncPtr)(void);
#endif

/*
 * GLX resources.
 */
typedef XID GLXContextID;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef XID GLXPbuffer;
typedef XID GLXWindow;
typedef XID GLXFBConfigID;

/*
 * GLXContext is a pointer to opaque data.
 */
typedef struct __GLXcontextRec *GLXContext;

/*
 * GLXFBConfig is a pointer to opaque data.
 */
typedef struct __GLXFBConfigRec *GLXFBConfig;


/**********************************************************************/

/*
 * GLX 1.0 functions.
 */
extern XVisualInfo* glXChooseVisual(Display *dpy, int screen,
                                    int *attrib_list);

extern void glXCopyContext(Display *dpy, GLXContext src,
                           GLXContext dst, unsigned long mask);

extern GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
                                   GLXContext share_list, Bool direct);

extern GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis,
                                    Pixmap pixmap);

extern void glXDestroyContext(Display *dpy, GLXContext ctx);

extern void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pix);

extern int glXGetConfig(Display *dpy, XVisualInfo *vis,
                        int attrib, int *value);

extern GLXContext glXGetCurrentContext(void);

extern GLXDrawable glXGetCurrentDrawable(void);

extern Bool glXIsDirect(Display *dpy, GLXContext ctx);

extern Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable,
                           GLXContext ctx);

extern Bool glXQueryExtension(Display *dpy, int *error_base, int *event_base);

extern Bool glXQueryVersion(Display *dpy, int *major, int *minor);

extern void glXSwapBuffers(Display *dpy, GLXDrawable drawable);

extern void glXUseXFont(Font font, int first, int count, int list_base);

extern void glXWaitGL(void);

extern void glXWaitX(void);


#ifndef GLX_VERSION_1_1
#define GLX_VERSION_1_1         1
/*
 * GLX 1.1 functions.
 */
extern const char *glXGetClientString(Display *dpy, int name);

extern const char *glXQueryServerString(Display *dpy, int screen, int name);

extern const char *glXQueryExtensionsString(Display *dpy, int screen);
#endif


#ifndef GLX_VERSION_1_2
#define GLX_VERSION_1_2         1
/*
 * GLX 1.2 functions.
 */
extern Display *glXGetCurrentDisplay(void);
#endif

#ifndef GLX_VERSION_1_3
#define GLX_VERSION_1_3 1
/*
 * GLX 1.3 functions.
 */
extern GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen,
                                      const int *attrib_list, int *nelements);

extern GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config,
                                      int render_type, GLXContext share_list,
                                      Bool direct);

extern GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config,
                                   const int *attrib_list);

extern GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config,
                                 Pixmap pixmap, const int *attrib_list);

extern GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config,
                                 Window win, const int *attrib_list);

extern void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf);

extern void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap);

extern void glXDestroyWindow(Display *dpy, GLXWindow win);

extern GLXDrawable glXGetCurrentReadDrawable(void);

extern int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config,
                                int attribute, int *value);

extern GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements);

extern void glXGetSelectedEvent(Display *dpy, GLXDrawable draw,
                                unsigned long *event_mask);

extern XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config);

extern Bool glXMakeContextCurrent(Display *display, GLXDrawable draw,
                                  GLXDrawable read, GLXContext ctx);

extern int glXQueryContext(Display *dpy, GLXContext ctx,
                           int attribute, int *value);

extern void glXQueryDrawable(Display *dpy, GLXDrawable draw,
                             int attribute, unsigned int *value);

extern void glXSelectEvent(Display *dpy, GLXDrawable draw,
                           unsigned long event_mask);

typedef GLXFBConfig * ( * PFNGLXGETFBCONFIGSPROC) (Display *dpy, int screen, int *nelements);
typedef GLXFBConfig * ( * PFNGLXCHOOSEFBCONFIGPROC) (Display *dpy, int screen, const int *attrib_list, int *nelements);
typedef int ( * PFNGLXGETFBCONFIGATTRIBPROC) (Display *dpy, GLXFBConfig config, int attribute, int *value);
typedef XVisualInfo * ( * PFNGLXGETVISUALFROMFBCONFIGPROC) (Display *dpy, GLXFBConfig config);
typedef GLXWindow ( * PFNGLXCREATEWINDOWPROC) (Display *dpy, GLXFBConfig config, Window win, const int *attrib_list);
typedef void ( * PFNGLXDESTROYWINDOWPROC) (Display *dpy, GLXWindow win);
typedef GLXPixmap ( * PFNGLXCREATEPIXMAPPROC) (Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list);
typedef void ( * PFNGLXDESTROYPIXMAPPROC) (Display *dpy, GLXPixmap pixmap);
typedef GLXPbuffer ( * PFNGLXCREATEPBUFFERPROC) (Display *dpy, GLXFBConfig config, const int *attrib_list);
typedef void ( * PFNGLXDESTROYPBUFFERPROC) (Display *dpy, GLXPbuffer pbuf);
typedef void ( * PFNGLXQUERYDRAWABLEPROC) (Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
typedef GLXContext ( * PFNGLXCREATENEWCONTEXTPROC) (Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
typedef Bool ( * PFNGLXMAKECONTEXTCURRENTPROC) (Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
typedef GLXDrawable ( * PFNGLXGETCURRENTREADDRAWABLEPROC) (void);
typedef Display * ( * PFNGLXGETCURRENTDISPLAYPROC) (void);
typedef int ( * PFNGLXQUERYCONTEXTPROC) (Display *dpy, GLXContext ctx, int attribute, int *value);
typedef void ( * PFNGLXSELECTEVENTPROC) (Display *dpy, GLXDrawable draw, unsigned long event_mask);
typedef void ( * PFNGLXGETSELECTEDEVENTPROC) (Display *dpy, GLXDrawable draw, unsigned long *event_mask);
#endif

/* GLX 1.4 and later */
extern void (*glXGetProcAddress(const GLubyte *procname))(void);

/**********************************************************************/

/*
 * ARB_get_proc_address
 */
#ifndef GLX_ARB_get_proc_address
#define GLX_ARB_get_proc_address 1
/* Don't wrap this in GLX_GLXEXT_PROTOTYPES! */
extern void (*glXGetProcAddressARB(const GLubyte *procName))(void);
typedef __GLXextFuncPtr ( * PFNGLXGETPROCADDRESSARBPROC) (const GLubyte *procName);
#endif

/**********************************************************************/

/*** Should these go here, or in another header? */
/*
 * GLX Events
 */
typedef struct {
    int event_type;             /* GLX_DAMAGED or GLX_SAVED */
    int draw_type;              /* GLX_WINDOW or GLX_PBUFFER */
    unsigned long serial;       /* # of last request processed by server */
    Bool send_event;            /* true if this came for SendEvent request */
    Display *display;           /* display the event was read from */
    GLXDrawable drawable;       /* XID of Drawable */
    unsigned int buffer_mask;   /* mask indicating which buffers are affected */
    unsigned int aux_buffer;    /* which aux buffer was affected */
    int x, y;
    int width, height;
    int count;                  /* if nonzero, at least this many more */
} GLXPbufferClobberEvent;

typedef union __GLXEvent {
    GLXPbufferClobberEvent glxpbufferclobber;
    long pad[24];
} GLXEvent;

#ifndef GLX_GLXEXT_LEGACY
#include "glxext.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* !__glx_h__ */

#endif /* SYSGLXHEADERS */
