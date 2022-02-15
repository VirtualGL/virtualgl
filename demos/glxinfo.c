/*
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * Copyright (C) 2005-2007  Sun Microsystems, Inc.   All Rights Reserved.
 * Copyright (C) 2011, 2013, 2015, 2019, 2022  D. R. Commander
 *                                             All Rights Reserved.
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


/*
 * This program is a work-alike of the IRIX glxinfo program.
 * Command line options:
 *  -t                     print wide table
 *  -v                     print verbose information
 *  -display DisplayName   specify the X display to interogate
 *  -B                     brief, print only the basics
 *  -b                     only print ID of "best" visual on screen 0
 *  -i                     use indirect rendering connection only
 *  -l                     print interesting OpenGL limits (added 5 Sep 2002)
 *
 * Brian Paul  26 January 2000
 */

#define GLX_GLXEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "glinfo_common.h"


#ifndef GLX_NONE_EXT
#define GLX_NONE_EXT  0x8000
#endif

#ifndef GLX_TRANSPARENT_RGB
#define GLX_TRANSPARENT_RGB 0x8008
#endif

#ifndef GLX_RGBA_BIT
#define GLX_RGBA_BIT                    0x00000001
#endif

#ifndef GLX_COLOR_INDEX_BIT
#define GLX_COLOR_INDEX_BIT             0x00000002
#endif


struct visual_attribs
{
   /* X visual attribs */
   int id;              /* May be visual ID or FBConfig ID */
   int vis_id;          /* Visual ID.  Only set for FBConfigs */
   int klass;
   int depth;
   int redMask, greenMask, blueMask;
   int colormapSize;
   int bitsPerRGB;

   /* GL visual attribs */
   int supportsGL;
   int drawableType;
   int transparentType;
   int transparentRedValue;
   int transparentGreenValue;
   int transparentBlueValue;
   int transparentAlphaValue;
   int transparentIndexValue;
   int bufferSize;
   int level;
   int render_type;
   int doubleBuffer;
   int stereo;
   int auxBuffers;
   int redSize, greenSize, blueSize, alphaSize;
   int depthSize;
   int stencilSize;
   int accumRedSize, accumGreenSize, accumBlueSize, accumAlphaSize;
   int numSamples, numMultisample;
   int visualCaveat;
   int floatComponents;
   int packedfloatComponents;
   int srgb;
   int bindToTextureRGB, bindToTextureRGBA;
};


/**
 * Version of the context that was created
 *
 * 20, 21, 30, 31, 32, etc.
 */
static int version;

/**
 * GL Error checking/warning.
 */
static void
CheckError(int line)
{
   int n;
   n = glGetError();
   if (n)
      printf("Warning: GL error 0x%x at line %d\n", n, line);
}


static void
print_display_info(Display *dpy)
{
   printf("name of display: %s\n", DisplayString(dpy));
}


/**
 * Choose a simple FB Config.
 */
static GLXFBConfig *
choose_fb_config(Display *dpy, int scrnum)
{
   int fbAttribSingle[] = {
      GLX_RENDER_TYPE,   GLX_RGBA_BIT,
      GLX_RED_SIZE,      1,
      GLX_GREEN_SIZE,    1,
      GLX_BLUE_SIZE,     1,
      GLX_DOUBLEBUFFER,  False,
      None };
   int fbAttribDouble[] = {
      GLX_RENDER_TYPE,   GLX_RGBA_BIT,
      GLX_RED_SIZE,      1,
      GLX_GREEN_SIZE,    1,
      GLX_BLUE_SIZE,     1,
      GLX_DOUBLEBUFFER,  True,
      None };
   GLXFBConfig *configs;
   int nConfigs;

   configs = glXChooseFBConfig(dpy, scrnum, fbAttribSingle, &nConfigs);
   if (!configs)
      configs = glXChooseFBConfig(dpy, scrnum, fbAttribDouble, &nConfigs);

   return configs;
}


static Bool CreateContextErrorFlag;

static int
create_context_error_handler(Display *dpy, XErrorEvent *error)
{
   (void) dpy;
   (void) error->error_code;
   CreateContextErrorFlag = True;
   return 0;
}


/**
 * Try to create a GLX context of the given version with flags/options.
 * Note: A version number is required in order to get a core profile
 * (at least w/ NVIDIA).
 */
static GLXContext
create_context_flags(Display *dpy, GLXFBConfig fbconfig, int major, int minor,
                     int contextFlags, int profileMask, Bool direct)
{
#ifdef GLX_ARB_create_context
   static PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB_func = 0;
   static Bool firstCall = True;
   int (*old_handler)(Display *, XErrorEvent *);
   GLXContext context;
   int attribs[20];
   int n = 0;

   if (firstCall) {
      /* See if we have GLX_ARB_create_context_profile and get pointer to
       * glXCreateContextAttribsARB() function.
       */
      const char *glxExt = glXQueryExtensionsString(dpy, 0);
      if (extension_supported("GLX_ARB_create_context_profile", glxExt)) {
         glXCreateContextAttribsARB_func = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
            glXGetProcAddress((const GLubyte *) "glXCreateContextAttribsARB");
      }
      firstCall = False;
   }

   if (!glXCreateContextAttribsARB_func)
      return 0;

   /* setup attribute array */
   if (major) {
      attribs[n++] = GLX_CONTEXT_MAJOR_VERSION_ARB;
      attribs[n++] = major;
      attribs[n++] = GLX_CONTEXT_MINOR_VERSION_ARB;
      attribs[n++] = minor;
   }
   if (contextFlags) {
      attribs[n++] = GLX_CONTEXT_FLAGS_ARB;
      attribs[n++] = contextFlags;
   }
#ifdef GLX_ARB_create_context_profile
   if (profileMask) {
      attribs[n++] = GLX_CONTEXT_PROFILE_MASK_ARB;
      attribs[n++] = profileMask;
   }
#endif
   attribs[n++] = 0;

   /* install X error handler */
   old_handler = XSetErrorHandler(create_context_error_handler);
   CreateContextErrorFlag = False;

   /* try creating context */
   context = glXCreateContextAttribsARB_func(dpy,
                                             fbconfig,
                                             0, /* share_context */
                                             direct,
                                             attribs);

   /* restore error handler */
   XSetErrorHandler(old_handler);

   if (CreateContextErrorFlag)
      context = 0;

   if (context && direct) {
      if (!glXIsDirect(dpy, context)) {
         glXDestroyContext(dpy, context);
         return 0;
      }
   }

   return context;
#else
   return 0;
#endif
}


/**
 * Try to create a GLX context of the newest version.
 */
static GLXContext
create_context_with_config(Display *dpy, GLXFBConfig config,
                           Bool coreProfile, Bool es2Profile, Bool direct)
{
   GLXContext ctx = 0;

   if (coreProfile) {
      /* Try to create a core profile, starting with the newest version of
       * GL that we're aware of.  If we don't specify the version
       */
      int i;
      for (i = 0; gl_versions[i].major > 0; i++) {
          /* don't bother below GL 3.0 */
          if (gl_versions[i].major == 3 &&
              gl_versions[i].minor == 0)
             return 0;
         ctx = create_context_flags(dpy, config,
                                    gl_versions[i].major,
                                    gl_versions[i].minor,
                                    0x0,
                                    GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                                    direct);
         if (ctx)
            return ctx;
      }
      /* couldn't get core profile context */
      return 0;
   }

   if (es2Profile) {
#ifdef GLX_CONTEXT_ES2_PROFILE_BIT_EXT
      if (extension_supported("GLX_EXT_create_context_es2_profile",
                              glXQueryExtensionsString(dpy, 0))) {
         ctx = create_context_flags(dpy, config, 2, 0, 0x0,
                                    GLX_CONTEXT_ES2_PROFILE_BIT_EXT,
                                    direct);
         return ctx;
      }
#endif
      return 0;
   }

   /* GLX should return a context of the latest GL version that supports
    * the full profile.
    */
   ctx = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, NULL, direct);

   /* make sure the context is direct, if direct was requested */
   if (ctx && direct) {
      if (!glXIsDirect(dpy, ctx)) {
         glXDestroyContext(dpy, ctx);
         return 0;
      }
   }

   return ctx;
}


static XVisualInfo *
choose_xvisinfo(Display *dpy, int scrnum)
{
   int attribSingle[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      None };
   int attribDouble[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DOUBLEBUFFER,
      None };
   XVisualInfo *visinfo;

   visinfo = glXChooseVisual(dpy, scrnum, attribSingle);
   if (!visinfo)
      visinfo = glXChooseVisual(dpy, scrnum, attribDouble);

   return visinfo;
}


static void
query_renderer(void)
{
#ifdef GLX_MESA_query_renderer
    PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC queryInteger;
    PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC queryString;
    unsigned int v[3];

    queryInteger = (PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC)
        glXGetProcAddressARB((const GLubyte *)
                             "glXQueryCurrentRendererIntegerMESA");
    queryString = (PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC)
        glXGetProcAddressARB((const GLubyte *)
                             "glXQueryCurrentRendererStringMESA");

    printf("Extended renderer info (GLX_MESA_query_renderer):\n");
    queryInteger(GLX_RENDERER_VENDOR_ID_MESA, v);
    printf("    Vendor: %s (0x%x)\n",
           queryString(GLX_RENDERER_VENDOR_ID_MESA), *v);
    queryInteger(GLX_RENDERER_DEVICE_ID_MESA, v);
    printf("    Device: %s (0x%x)\n",
           queryString(GLX_RENDERER_DEVICE_ID_MESA), *v);
    queryInteger(GLX_RENDERER_VERSION_MESA, v);
    printf("    Version: %d.%d.%d\n", v[0], v[1], v[2]);
    queryInteger(GLX_RENDERER_ACCELERATED_MESA, v);
    printf("    Accelerated: %s\n", *v ? "yes" : "no");
    queryInteger(GLX_RENDERER_VIDEO_MEMORY_MESA, v);
    printf("    Video memory: %dMB\n", *v);
    queryInteger(GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA, v);
    printf("    Unified memory: %s\n", *v ? "yes" : "no");
    queryInteger(GLX_RENDERER_PREFERRED_PROFILE_MESA, v);
    printf("    Preferred profile: %s (0x%x)\n",
           *v == GLX_CONTEXT_CORE_PROFILE_BIT_ARB ? "core" :
           *v == GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB ? "compat" :
           "unknown", *v);
    queryInteger(GLX_RENDERER_OPENGL_CORE_PROFILE_VERSION_MESA, v);
    printf("    Max core profile version: %d.%d\n", v[0], v[1]);
    queryInteger(GLX_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_MESA, v);
    printf("    Max compat profile version: %d.%d\n", v[0], v[1]);
    queryInteger(GLX_RENDERER_OPENGL_ES_PROFILE_VERSION_MESA, v);
    printf("    Max GLES1 profile version: %d.%d\n", v[0], v[1]);
    queryInteger(GLX_RENDERER_OPENGL_ES2_PROFILE_VERSION_MESA, v);
    printf("    Max GLES[23] profile version: %d.%d\n", v[0], v[1]);
#endif
}


static Bool
print_screen_info(Display *dpy, int scrnum,
                  const struct options *opts,
                  Bool coreProfile, Bool es2Profile, Bool limits,
                  Bool coreWorked)
{
   Window win;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   GLXContext ctx = NULL;
   XVisualInfo *visinfo;
   int width = 100, height = 100;
   GLXFBConfig *fbconfigs;
   const char *oglstring = coreProfile ? "OpenGL core profile" :
                           es2Profile ? "OpenGL ES profile" : "OpenGL";

   root = RootWindow(dpy, scrnum);

   /*
    * Choose FBConfig or XVisualInfo and create a context.
    */
   fbconfigs = choose_fb_config(dpy, scrnum);
   if (fbconfigs) {
      ctx = create_context_with_config(dpy, fbconfigs[0],
                                       coreProfile, es2Profile,
                                       opts->allowDirect);
      if (!ctx && opts->allowDirect && !coreProfile) {
         /* try indirect */
         ctx = create_context_with_config(dpy, fbconfigs[0],
                                          coreProfile, es2Profile, False);
      }

      visinfo = glXGetVisualFromFBConfig(dpy, fbconfigs[0]);
      XFree(fbconfigs);
   }
   else if (!coreProfile && !es2Profile) {
      visinfo = choose_xvisinfo(dpy, scrnum);
      if (visinfo)
         ctx = glXCreateContext(dpy, visinfo, NULL, opts->allowDirect);
   } else
      visinfo = NULL;

   if (!visinfo && !coreProfile && !es2Profile) {
      fprintf(stderr, "Error: couldn't find RGB GLX visual or fbconfig\n");
      return False;
   }

   if (!ctx) {
      if (!coreProfile && !es2Profile)
         fprintf(stderr, "Error: glXCreateContext failed\n");
      XFree(visinfo);
      return False;
   }

   /*
    * Create a window so that we can just bind the context.
    */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
   win = XCreateWindow(dpy, root, 0, 0, width, height,
                       0, visinfo->depth, InputOutput,
                       visinfo->visual, mask, &attr);

   if (glXMakeCurrent(dpy, win, ctx)) {
      const char *serverVendor = glXQueryServerString(dpy, scrnum, GLX_VENDOR);
      const char *serverVersion = glXQueryServerString(dpy, scrnum, GLX_VERSION);
      const char *serverExtensions = glXQueryServerString(dpy, scrnum, GLX_EXTENSIONS);
      const char *clientVendor = glXGetClientString(dpy, GLX_VENDOR);
      const char *clientVersion = glXGetClientString(dpy, GLX_VERSION);
      const char *clientExtensions = glXGetClientString(dpy, GLX_EXTENSIONS);
      const char *glxExtensions = glXQueryExtensionsString(dpy, scrnum);
      const char *glVendor = (const char *) glGetString(GL_VENDOR);
      const char *glRenderer = (const char *) glGetString(GL_RENDERER);
      const char *glVersion = (const char *) glGetString(GL_VERSION);
      char *glExtensions = NULL;
      int glxVersionMajor = 0;
      int glxVersionMinor = 0;
      char *displayName = NULL;
      char *colon = NULL, *period = NULL;
      struct ext_functions extfuncs;

      CheckError(__LINE__);

      /* Get some ext functions */
      extfuncs.GetProgramivARB = (GETPROGRAMIVARBPROC)
         glXGetProcAddressARB((GLubyte *) "glGetProgramivARB");
      extfuncs.GetStringi = (GETSTRINGIPROC)
         glXGetProcAddressARB((GLubyte *) "glGetStringi");
      extfuncs.GetConvolutionParameteriv = (GETCONVOLUTIONPARAMETERIVPROC)
         glXGetProcAddressARB((GLubyte *) "glGetConvolutionParameteriv");

      if (!glXQueryVersion(dpy, & glxVersionMajor, & glxVersionMinor)) {
         fprintf(stderr, "Error: glXQueryVersion failed\n");
         exit(1);
      }

      /* Get list of GL extensions */
      if (coreProfile && extfuncs.GetStringi)
         glExtensions = build_core_profile_extension_list(&extfuncs);
      if (!glExtensions) {
         coreProfile = False;
         glExtensions = (char *) glGetString(GL_EXTENSIONS);
      }

      CheckError(__LINE__);

      if (!coreWorked) {
         /* Strip the screen number from the display name, if present. */
         if (!(displayName = (char *) malloc(strlen(DisplayString(dpy)) + 1))) {
            fprintf(stderr, "Error: malloc() failed\n");
            exit(1);
         }
         strcpy(displayName, DisplayString(dpy));
         colon = strrchr(displayName, ':');
         if (colon) {
            period = strchr(colon, '.');
            if (period)
               *period = '\0';
         }

         printf("display: %s  screen: %d\n", displayName, scrnum);
         free(displayName);
         printf("direct rendering: ");
         if (glXIsDirect(dpy, ctx)) {
            printf("Yes\n");
         }
         else {
            if (!opts->allowDirect) {
               printf("No (-i specified)\n");
            }
            else if (getenv("LIBGL_ALWAYS_INDIRECT")) {
               printf("No (LIBGL_ALWAYS_INDIRECT set)\n");
            }
            else {
               printf("No (If you want to find out why, try setting "
                      "LIBGL_DEBUG=verbose)\n");
            }
         }
         if (opts->mode != Brief) {
            printf("server glx vendor string: %s\n", serverVendor);
            printf("server glx version string: %s\n", serverVersion);
            printf("server glx extensions:\n");
            print_extension_list(serverExtensions, opts->singleLine);
            printf("client glx vendor string: %s\n", clientVendor);
            printf("client glx version string: %s\n", clientVersion);
            printf("client glx extensions:\n");
            print_extension_list(clientExtensions, opts->singleLine);
            printf("GLX version: %u.%u\n", glxVersionMajor, glxVersionMinor);
            printf("GLX extensions:\n");
            print_extension_list(glxExtensions, opts->singleLine);
         }
         if (strstr(glxExtensions, "GLX_MESA_query_renderer"))
            query_renderer();
         print_gpu_memory_info(glExtensions);
         printf("OpenGL vendor string: %s\n", glVendor);
         printf("OpenGL renderer string: %s\n", glRenderer);
      } else
         printf("\n");

      printf("%s version string: %s\n", oglstring, glVersion);

      version = (glVersion[0] - '0') * 10 + (glVersion[2] - '0');

      CheckError(__LINE__);

#ifdef GL_VERSION_2_0
      if (version >= 20) {
         char *v = (char *) glGetString(GL_SHADING_LANGUAGE_VERSION);
         printf("%s shading language version string: %s\n", oglstring, v);
      }
#endif
      CheckError(__LINE__);
#ifdef GL_VERSION_3_0
      if (version >= 30 && !es2Profile) {
         GLint flags;
         glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
         printf("%s context flags: %s\n", oglstring, context_flags_string(flags));
      }
#endif
      CheckError(__LINE__);
#ifdef GL_VERSION_3_2
      if (version >= 32 && !es2Profile) {
         GLint profileMask;
         glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
         printf("%s profile mask: %s\n", oglstring,
                profile_mask_string(profileMask));
      }
#endif

      CheckError(__LINE__);

      if (opts->mode != Brief) {
         printf("%s extensions:\n", oglstring);
         print_extension_list(glExtensions, opts->singleLine);
      }

      if (limits) {
         print_limits(glExtensions, oglstring, version, &extfuncs);
      }

      if (coreProfile)
         free(glExtensions);
   }
   else {
      fprintf(stderr, "Error: glXMakeCurrent failed\n");
   }

   glXDestroyContext(dpy, ctx);
   XFree(visinfo);
   XDestroyWindow(dpy, win);
   XSync(dpy, 1);
   return True;
}


static const char *
visual_class_name(int cls)
{
   switch (cls) {
      case StaticColor:
         return "StaticColor";
      case PseudoColor:
         return "PseudoColor";
      case StaticGray:
         return "StaticGray";
      case GrayScale:
         return "GrayScale";
      case TrueColor:
         return "TrueColor";
      case DirectColor:
         return "DirectColor";
      default:
         return "Unknown";
   }
}

static const char *
visual_drawable_type(int type)
{
   static const struct bit_info bits[] = {
      { GLX_WINDOW_BIT, "Window" },
      { GLX_PIXMAP_BIT, "Pixmap" },
      { GLX_PBUFFER_BIT, "Pbuffer" }
   };

   return bitmask_to_string(bits, ELEMENTS(bits), type);
}

static const char *
visual_class_abbrev(int cls)
{
   switch (cls) {
      case StaticColor:
         return "sc";
      case PseudoColor:
         return "pc";
      case StaticGray:
         return "sg";
      case GrayScale:
         return "gs";
      case TrueColor:
         return "tc";
      case DirectColor:
         return "dc";
      default:
         return "??";
   }
}

static const char *
visual_render_type_name(int type)
{
   switch (type) {
      case GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT:
         return "ufloat";
      case GLX_RGBA_FLOAT_BIT_ARB:
         return "float";
      case GLX_RGBA_BIT:
         return "rgba";
      case GLX_COLOR_INDEX_BIT:
         return "ci";
      case GLX_RGBA_BIT | GLX_COLOR_INDEX_BIT:
         return "rgba|ci";
      default:
         return "";
   }
}

static const char *
visual_render_type_space(int type)
{
   switch (type) {
      case GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT:
         return " ";
      case GLX_RGBA_FLOAT_BIT_ARB:
         return "  ";
      case GLX_RGBA_BIT:
         return "   ";
      case GLX_COLOR_INDEX_BIT:
         return "     ";
      default:
         return "";
   }
}

static const char *
caveat_string(int caveat)
{
   switch (caveat) {
#ifdef GLX_EXT_visual_rating
      case GLX_SLOW_VISUAL_EXT:
         return "Slow";
      case GLX_NON_CONFORMANT_VISUAL_EXT:
         return "Ncon";
      case GLX_NONE_EXT:
         /* fall-through */
#endif
      case 0:
         /* fall-through */
      default:
         return "None";
   }
}


static Bool
get_visual_attribs(Display *dpy, XVisualInfo *vInfo,
                   struct visual_attribs *attribs)
{
   const char *ext = glXQueryExtensionsString(dpy, vInfo->screen);
   int rgba;

   memset(attribs, 0, sizeof(struct visual_attribs));

   attribs->id = vInfo->visualid;
#if defined(__cplusplus) || defined(c_plusplus)
   attribs->klass = vInfo->c_class;
#else
   attribs->klass = vInfo->class;
#endif
   attribs->depth = vInfo->depth;
   attribs->redMask = vInfo->red_mask;
   attribs->greenMask = vInfo->green_mask;
   attribs->blueMask = vInfo->blue_mask;
   attribs->colormapSize = vInfo->colormap_size;
   attribs->bitsPerRGB = vInfo->bits_per_rgb;

   if (glXGetConfig(dpy, vInfo, GLX_USE_GL, &attribs->supportsGL) != 0 ||
       !attribs->supportsGL)
      return False;
   glXGetConfig(dpy, vInfo, GLX_BUFFER_SIZE, &attribs->bufferSize);
   glXGetConfig(dpy, vInfo, GLX_LEVEL, &attribs->level);
   glXGetConfig(dpy, vInfo, GLX_RGBA, &rgba);
   if (rgba)
      attribs->render_type = GLX_RGBA_BIT;
   else
      attribs->render_type = GLX_COLOR_INDEX_BIT;

   glXGetConfig(dpy, vInfo, GLX_DRAWABLE_TYPE, &attribs->drawableType);
   glXGetConfig(dpy, vInfo, GLX_DOUBLEBUFFER, &attribs->doubleBuffer);
   glXGetConfig(dpy, vInfo, GLX_STEREO, &attribs->stereo);
   glXGetConfig(dpy, vInfo, GLX_AUX_BUFFERS, &attribs->auxBuffers);
   glXGetConfig(dpy, vInfo, GLX_RED_SIZE, &attribs->redSize);
   glXGetConfig(dpy, vInfo, GLX_GREEN_SIZE, &attribs->greenSize);
   glXGetConfig(dpy, vInfo, GLX_BLUE_SIZE, &attribs->blueSize);
   glXGetConfig(dpy, vInfo, GLX_ALPHA_SIZE, &attribs->alphaSize);
   glXGetConfig(dpy, vInfo, GLX_DEPTH_SIZE, &attribs->depthSize);
   glXGetConfig(dpy, vInfo, GLX_STENCIL_SIZE, &attribs->stencilSize);
   glXGetConfig(dpy, vInfo, GLX_ACCUM_RED_SIZE, &attribs->accumRedSize);
   glXGetConfig(dpy, vInfo, GLX_ACCUM_GREEN_SIZE, &attribs->accumGreenSize);
   glXGetConfig(dpy, vInfo, GLX_ACCUM_BLUE_SIZE, &attribs->accumBlueSize);
   glXGetConfig(dpy, vInfo, GLX_ACCUM_ALPHA_SIZE, &attribs->accumAlphaSize);

   /* get transparent pixel stuff */
   glXGetConfig(dpy, vInfo,GLX_TRANSPARENT_TYPE, &attribs->transparentType);
   if (attribs->transparentType == GLX_TRANSPARENT_RGB) {
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_RED_VALUE, &attribs->transparentRedValue);
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_GREEN_VALUE, &attribs->transparentGreenValue);
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_BLUE_VALUE, &attribs->transparentBlueValue);
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_ALPHA_VALUE, &attribs->transparentAlphaValue);
   }
   else if (attribs->transparentType == GLX_TRANSPARENT_INDEX) {
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_INDEX_VALUE, &attribs->transparentIndexValue);
   }

   /* multisample attribs */
#ifdef GLX_ARB_multisample
   if (ext && strstr(ext, "GLX_ARB_multisample")) {
      glXGetConfig(dpy, vInfo, GLX_SAMPLE_BUFFERS_ARB, &attribs->numMultisample);
      glXGetConfig(dpy, vInfo, GLX_SAMPLES_ARB, &attribs->numSamples);
   }
#endif
   else {
      attribs->numSamples = 0;
      attribs->numMultisample = 0;
   }

#if defined(GLX_EXT_visual_rating)
   if (ext && strstr(ext, "GLX_EXT_visual_rating")) {
      glXGetConfig(dpy, vInfo, GLX_VISUAL_CAVEAT_EXT, &attribs->visualCaveat);
   }
   else {
      attribs->visualCaveat = GLX_NONE_EXT;
   }
#else
   attribs->visualCaveat = 0;
#endif

#if defined(GLX_EXT_framebuffer_sRGB)
   if (ext && strstr(ext, "GLX_EXT_framebuffer_sRGB")) {
      glXGetConfig(dpy, vInfo, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, &attribs->srgb);
   }
#endif

   return True;
}

#ifdef GLX_VERSION_1_3

static int
glx_token_to_visual_class(int visual_type)
{
   switch (visual_type) {
   case GLX_TRUE_COLOR:
      return TrueColor;
   case GLX_DIRECT_COLOR:
      return DirectColor;
   case GLX_PSEUDO_COLOR:
      return PseudoColor;
   case GLX_STATIC_COLOR:
      return StaticColor;
   case GLX_GRAY_SCALE:
      return GrayScale;
   case GLX_STATIC_GRAY:
      return StaticGray;
   case GLX_NONE:
   default:
      return -1;
   }
}

static Bool
get_fbconfig_attribs(Display *dpy, int scrnum, GLXFBConfig fbconfig,
                     struct visual_attribs *attribs)
{
   const char *ext = glXQueryExtensionsString(dpy, scrnum);
   int visual_type;
   XVisualInfo *vInfo;

   memset(attribs, 0, sizeof(struct visual_attribs));

   glXGetFBConfigAttrib(dpy, fbconfig, GLX_FBCONFIG_ID, &attribs->id);

   vInfo = glXGetVisualFromFBConfig(dpy, fbconfig);

   if (vInfo != NULL) {
      attribs->vis_id = vInfo->visualid;
      attribs->depth = vInfo->depth;
      attribs->redMask = vInfo->red_mask;
      attribs->greenMask = vInfo->green_mask;
      attribs->blueMask = vInfo->blue_mask;
      attribs->colormapSize = vInfo->colormap_size;
      attribs->bitsPerRGB = vInfo->bits_per_rgb;
   }

   glXGetFBConfigAttrib(dpy, fbconfig, GLX_X_VISUAL_TYPE, &visual_type);
   attribs->klass = glx_token_to_visual_class(visual_type);

   glXGetFBConfigAttrib(dpy, fbconfig, GLX_DRAWABLE_TYPE, &attribs->drawableType);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_BUFFER_SIZE, &attribs->bufferSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_LEVEL, &attribs->level);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_RENDER_TYPE, &attribs->render_type);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_DOUBLEBUFFER, &attribs->doubleBuffer);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_STEREO, &attribs->stereo);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_AUX_BUFFERS, &attribs->auxBuffers);

   glXGetFBConfigAttrib(dpy, fbconfig, GLX_RED_SIZE, &attribs->redSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_GREEN_SIZE, &attribs->greenSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_BLUE_SIZE, &attribs->blueSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_ALPHA_SIZE, &attribs->alphaSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_DEPTH_SIZE, &attribs->depthSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_STENCIL_SIZE, &attribs->stencilSize);

   glXGetFBConfigAttrib(dpy, fbconfig, GLX_ACCUM_RED_SIZE, &attribs->accumRedSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_ACCUM_GREEN_SIZE, &attribs->accumGreenSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_ACCUM_BLUE_SIZE, &attribs->accumBlueSize);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_ACCUM_ALPHA_SIZE, &attribs->accumAlphaSize);

   /* get transparent pixel stuff */
   glXGetFBConfigAttrib(dpy, fbconfig,GLX_TRANSPARENT_TYPE, &attribs->transparentType);
   if (attribs->transparentType == GLX_TRANSPARENT_RGB) {
     glXGetFBConfigAttrib(dpy, fbconfig, GLX_TRANSPARENT_RED_VALUE, &attribs->transparentRedValue);
     glXGetFBConfigAttrib(dpy, fbconfig, GLX_TRANSPARENT_GREEN_VALUE, &attribs->transparentGreenValue);
     glXGetFBConfigAttrib(dpy, fbconfig, GLX_TRANSPARENT_BLUE_VALUE, &attribs->transparentBlueValue);
     glXGetFBConfigAttrib(dpy, fbconfig, GLX_TRANSPARENT_ALPHA_VALUE, &attribs->transparentAlphaValue);
   }
   else if (attribs->transparentType == GLX_TRANSPARENT_INDEX) {
     glXGetFBConfigAttrib(dpy, fbconfig, GLX_TRANSPARENT_INDEX_VALUE, &attribs->transparentIndexValue);
   }

   glXGetFBConfigAttrib(dpy, fbconfig, GLX_SAMPLE_BUFFERS, &attribs->numMultisample);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_SAMPLES, &attribs->numSamples);
   glXGetFBConfigAttrib(dpy, fbconfig, GLX_CONFIG_CAVEAT, &attribs->visualCaveat);

#if defined(GLX_NV_float_buffer)
   if (ext && strstr(ext, "GLX_NV_float_buffer")) {
      glXGetFBConfigAttrib(dpy, fbconfig, GLX_FLOAT_COMPONENTS_NV, &attribs->floatComponents);
   }
#endif
#if defined(GLX_ARB_fbconfig_float)
   if (ext && strstr(ext, "GLX_ARB_fbconfig_float")) {
      if (attribs->render_type & GLX_RGBA_FLOAT_BIT_ARB) {
         attribs->floatComponents = True;
      }
   }
#endif
#if defined(GLX_EXT_fbconfig_packed_float)
   if (ext && strstr(ext, "GLX_EXT_fbconfig_packed_float")) {
      if (attribs->render_type & GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT) {
         attribs->packedfloatComponents = True;
      }
   }
#endif

#if defined(GLX_EXT_framebuffer_sRGB)
   if (ext && strstr(ext, "GLX_EXT_framebuffer_sRGB")) {
      glXGetFBConfigAttrib(dpy, fbconfig, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, &attribs->srgb);
   }
#endif

#if defined(GLX_EXT_texture_from_pixmap)
   if (ext && strstr(ext, "GLX_EXT_texture_from_pixmap")) {
      glXGetFBConfigAttrib(dpy, fbconfig, GLX_BIND_TO_TEXTURE_RGB_EXT,
                           &attribs->bindToTextureRGB);
      glXGetFBConfigAttrib(dpy, fbconfig, GLX_BIND_TO_TEXTURE_RGBA_EXT,
                           &attribs->bindToTextureRGBA);
   }
#endif
   return True;
}

#endif



static void
print_visual_attribs_verbose(const struct visual_attribs *attribs,
                             int fbconfigs)
{
   if (fbconfigs) {
      printf("FBConfig ID: %x  Visual ID=%x  depth=%d  class=%s\n",
             attribs->id, attribs->vis_id, attribs->depth,
             visual_class_name(attribs->klass));
      printf("    Drawable Types=%s\n",
             visual_drawable_type(attribs->drawableType));
   }
   else {
      printf("Visual ID: %x  depth=%d  class=%s\n",
             attribs->id, attribs->depth, visual_class_name(attribs->klass));
   }
   printf("    bufferSize=%d level=%d renderType=%s doubleBuffer=%d stereo=%d\n",
          attribs->bufferSize, attribs->level,
          visual_render_type_name(attribs->render_type),
          attribs->doubleBuffer, attribs->stereo);
   printf("    rgba: redSize=%d greenSize=%d blueSize=%d alphaSize=%d float=%c sRGB=%c\n",
          attribs->redSize, attribs->greenSize,
          attribs->blueSize, attribs->alphaSize,
          attribs->packedfloatComponents ? 'P' : attribs->floatComponents ? 'Y' : 'N',
          attribs->srgb ? 'Y' : 'N');
   printf("    auxBuffers=%d depthSize=%d stencilSize=%d\n",
          attribs->auxBuffers, attribs->depthSize, attribs->stencilSize);
   printf("    accum: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n",
          attribs->accumRedSize, attribs->accumGreenSize,
          attribs->accumBlueSize, attribs->accumAlphaSize);
   printf("    multiSample=%d  multiSampleBuffers=%d\n",
          attribs->numSamples, attribs->numMultisample);
#ifdef GLX_EXT_visual_rating
   if (attribs->visualCaveat == GLX_NONE_EXT || attribs->visualCaveat == 0)
      printf("    visualCaveat=None\n");
   else if (attribs->visualCaveat == GLX_SLOW_VISUAL_EXT)
      printf("    visualCaveat=Slow\n");
   else if (attribs->visualCaveat == GLX_NON_CONFORMANT_VISUAL_EXT)
      printf("    visualCaveat=Nonconformant\n");
#endif
   if (attribs->transparentType == GLX_NONE) {
     printf("    Opaque.\n");
   }
   else if (attribs->transparentType == GLX_TRANSPARENT_RGB) {
     printf("    Transparent RGB: Red=%d Green=%d Blue=%d Alpha=%d\n",attribs->transparentRedValue,attribs->transparentGreenValue,attribs->transparentBlueValue,attribs->transparentAlphaValue);
   }
   else if (attribs->transparentType == GLX_TRANSPARENT_INDEX) {
     printf("    Transparent index=%d\n",attribs->transparentIndexValue);
   }
#if defined(GLX_EXT_texture_from_pixmap)
   if (attribs->bindToTextureRGB || attribs->bindToTextureRGBA) {
      printf("    Bind to texture:");
      if (attribs->bindToTextureRGB)
         printf(" RGB");
      if (attribs->bindToTextureRGBA)
         printf(" RGBA");
      printf("\n");
   }
#endif
}


static void
print_visual_attribs_short_header(Bool fbconfigs)
{
 printf("    visual  x   bf lv rg d st  colorbuffer  ax dp st accumbuffer  ms  cav");
 if (fbconfigs) printf("  drw");
 printf("\n");
 printf("  id dep cl sp  sz l  ci b ro  r  g  b  a F bf th cl  r  g  b  a ns b eat");
 if (fbconfigs) printf("  typ");
 printf("\n");
 printf("--------------------------------------------------------------------------");
 if (fbconfigs) printf("----");
 printf("\n");
}


static void
print_visual_attribs_short(const struct visual_attribs *attribs,
                           Bool fbconfigs)
{
   const char *caveat = caveat_string(attribs->visualCaveat);

   printf("0x%03x %2d %2s %2d %3d %2d %c%c %c  %c %2d %2d %2d %2d %c %2d %2d %2d",
          attribs->id,
          attribs->depth,
          visual_class_abbrev(attribs->klass),
          attribs->transparentType != GLX_NONE,
          attribs->bufferSize,
          attribs->level,
          (attribs->render_type & GLX_RGBA_BIT) ? (attribs->srgb ? 's' : 'r') :
                                                  ' ',
          (attribs->render_type & GLX_COLOR_INDEX_BIT) ? 'c' : ' ',
          attribs->doubleBuffer ? 'y' : '.',
          attribs->stereo ? 'y' : '.',
          attribs->redSize, attribs->greenSize,
          attribs->blueSize, attribs->alphaSize,
          attribs->packedfloatComponents ? 'u' : attribs->floatComponents ? 'f' : '.',
          attribs->auxBuffers,
          attribs->depthSize,
          attribs->stencilSize
          );

   printf(" %2d %2d %2d %2d %2d %1d %s",
          attribs->accumRedSize, attribs->accumGreenSize,
          attribs->accumBlueSize, attribs->accumAlphaSize,
          attribs->numSamples, attribs->numMultisample,
          caveat
          );

   if (fbconfigs) {
      printf(" ");
      if (attribs->drawableType & GLX_PBUFFER_BIT)
         printf("P");
      else
         printf(".");
      if (attribs->drawableType & GLX_PIXMAP_BIT) {
#if defined(GLX_EXT_texture_from_pixmap)
         if (attribs->bindToTextureRGBA)
            printf("A");
         else if (attribs->bindToTextureRGB)
            printf("R");
         else
#endif
         printf("X");
      } else
         printf(".");
      if (attribs->drawableType & GLX_WINDOW_BIT)
         printf("W");
      else
         printf(".");
  }
  printf("\n");
}


static void
print_visual_attribs_long_header(void)
{
 printf("Vis   Vis   Visual Trans  buff lev render DB ste  r   g   b   a      s  aux dep ste  accum buffer   MS   MS         \n");
 printf(" ID  Depth   Type  parent size el   type     reo sz  sz  sz  sz flt rgb buf th  ncl  r   g   b   a  num bufs caveats\n");
 printf("--------------------------------------------------------------------------------------------------------------------\n");
}


static void
print_visual_attribs_long(const struct visual_attribs *attribs)
{
   const char *caveat = caveat_string(attribs->visualCaveat);

   printf("0x%3x %2d %-11s %2d    %3d %2d  %s%s%1d %3d %3d %3d %3d %3d",
          attribs->id,
          attribs->depth,
          visual_class_name(attribs->klass),
          attribs->transparentType != GLX_NONE,
          attribs->bufferSize,
          attribs->level,
          visual_render_type_name(attribs->render_type),
          visual_render_type_space(attribs->render_type),
          attribs->doubleBuffer,
          attribs->stereo,
          attribs->redSize, attribs->greenSize,
          attribs->blueSize, attribs->alphaSize
          );

   printf("  %c   %c %3d %4d %2d %3d %3d %3d %3d  %2d  %2d %6s\n",
          attribs->floatComponents ? 'f' : '.',
          attribs->srgb ? 's' : '.',
          attribs->auxBuffers,
          attribs->depthSize,
          attribs->stencilSize,
          attribs->accumRedSize, attribs->accumGreenSize,
          attribs->accumBlueSize, attribs->accumAlphaSize,
          attribs->numSamples, attribs->numMultisample,
          caveat
          );
}


static void
print_visual_info(Display *dpy, int scrnum, InfoMode mode)
{
   XVisualInfo theTemplate;
   XVisualInfo *visuals;
   int numVisuals, numGlxVisuals;
   long mask;
   int i;
   struct visual_attribs attribs;

   /* get list of all visuals on this screen */
   theTemplate.screen = scrnum;
   mask = VisualScreenMask;
   visuals = XGetVisualInfo(dpy, mask, &theTemplate, &numVisuals);

   numGlxVisuals = 0;
   for (i = 0; i < numVisuals; i++) {
      if (get_visual_attribs(dpy, &visuals[i], &attribs))
         numGlxVisuals++;
   }

   if (numGlxVisuals == 0)
      return;

   printf("%d GLX Visuals\n", numGlxVisuals);

   if (mode == Normal)
      print_visual_attribs_short_header(False);
   else if (mode == Wide)
      print_visual_attribs_long_header();

   for (i = 0; i < numVisuals; i++) {
      if (!get_visual_attribs(dpy, &visuals[i], &attribs))
         continue;

      if (mode == Verbose)
         print_visual_attribs_verbose(&attribs, False);
      else if (mode == Normal)
         print_visual_attribs_short(&attribs, False);
      else if (mode == Wide)
         print_visual_attribs_long(&attribs);
   }
   printf("\n");

   XFree(visuals);
}

#ifdef GLX_VERSION_1_3

static void
print_fbconfig_info(Display *dpy, int scrnum, InfoMode mode)
{
   int numFBConfigs = 0;
   struct visual_attribs attribs;
   GLXFBConfig *fbconfigs;
   int i;

   /* get list of all fbconfigs on this screen */
   fbconfigs = glXGetFBConfigs(dpy, scrnum, &numFBConfigs);

   if (numFBConfigs == 0) {
      XFree(fbconfigs);
      return;
   }

   printf("%d GLXFBConfigs:\n", numFBConfigs);
   if (mode == Normal)
      print_visual_attribs_short_header(True);
   else if (mode == Wide)
      print_visual_attribs_long_header();

   for (i = 0; i < numFBConfigs; i++) {
      get_fbconfig_attribs(dpy, scrnum, fbconfigs[i], &attribs);

      if (mode == Verbose)
         print_visual_attribs_verbose(&attribs, True);
      else if (mode == Normal)
         print_visual_attribs_short(&attribs, True);
      else if (mode == Wide)
         print_visual_attribs_long(&attribs);
   }
   printf("\n");

   XFree(fbconfigs);
}

#endif

/*
 * Stand-alone Mesa doesn't really implement the GLX protocol so it
 * doesn't really know the GLX attributes associated with an X visual.
 * The first time a visual is presented to Mesa's pseudo-GLX it
 * attaches ancilliary buffers to it (like depth and stencil).
 * But that usually only works if glXChooseVisual is used.
 * This function calls glXChooseVisual() to sort of "prime the pump"
 * for Mesa's GLX so that the visuals that get reported actually
 * reflect what applications will see.
 * This has no effect when using true GLX.
 */
static void
mesa_hack(Display *dpy, int scrnum)
{
   static int attribs[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DEPTH_SIZE, 1,
      GLX_STENCIL_SIZE, 1,
      GLX_DOUBLEBUFFER,
      None
   };
   XVisualInfo *visinfo;

   visinfo = glXChooseVisual(dpy, scrnum, attribs);
   if (visinfo)
      XFree(visinfo);
}


/*
 * Examine all visuals to find the so-called best one.
 * We prefer deepest RGBA buffer with depth, stencil and accum
 * that has no caveats.
 */
static int
find_best_visual(Display *dpy, int scrnum)
{
   XVisualInfo theTemplate;
   XVisualInfo *visuals;
   int numVisuals;
   long mask;
   int i;
   struct visual_attribs bestVis;

   /* get list of all visuals on this screen */
   theTemplate.screen = scrnum;
   mask = VisualScreenMask;
   visuals = XGetVisualInfo(dpy, mask, &theTemplate, &numVisuals);

   /* init bestVis with first visual info */
   get_visual_attribs(dpy, &visuals[0], &bestVis);

   /* try to find a "better" visual */
   for (i = 1; i < numVisuals; i++) {
      struct visual_attribs vis;

      get_visual_attribs(dpy, &visuals[i], &vis);

      /* always skip visuals with caveats */
      if (vis.visualCaveat != GLX_NONE_EXT)
         continue;

      /* see if this vis is better than bestVis */
      if ((!bestVis.supportsGL && vis.supportsGL) ||
          (bestVis.visualCaveat != GLX_NONE_EXT) ||
          (!(bestVis.render_type & GLX_RGBA_BIT) && (vis.render_type & GLX_RGBA_BIT)) ||
          (!bestVis.doubleBuffer && vis.doubleBuffer) ||
          (bestVis.redSize < vis.redSize) ||
          (bestVis.greenSize < vis.greenSize) ||
          (bestVis.blueSize < vis.blueSize) ||
          (bestVis.alphaSize < vis.alphaSize) ||
          (bestVis.depthSize < vis.depthSize) ||
          (bestVis.stencilSize < vis.stencilSize) ||
          (bestVis.accumRedSize < vis.accumRedSize)) {
         /* found a better visual */
         bestVis = vis;
      }
   }

   XFree(visuals);

   return bestVis.id;
}


int
main(int argc, char *argv[])
{
   Display *dpy;
   int numScreens, scrnum;
   struct options opts;
   Bool coreWorked;

   parse_args(argc, argv, &opts);

   dpy = XOpenDisplay(opts.displayName);
   if (!dpy) {
      fprintf(stderr, "Error: unable to open display %s\n",
              XDisplayName(opts.displayName));
      return -1;
   }

   if (opts.findBest) {
      int b;
      mesa_hack(dpy, 0);
      b = find_best_visual(dpy, 0);
      printf("%d\n", b);
   }
   else {
      numScreens = ScreenCount(dpy);
      print_display_info(dpy);
      for (scrnum = 0; scrnum < numScreens; scrnum++) {
         mesa_hack(dpy, scrnum);
         coreWorked = print_screen_info(dpy, scrnum, &opts,
                                        True, False, opts.limits, False);
         print_screen_info(dpy, scrnum, &opts, False, False,
                           opts.limits, coreWorked);
         print_screen_info(dpy, scrnum, &opts, False, True, False, True);

         printf("\n");

         if (opts.mode != Brief) {
#ifdef GLX_VERSION_1_3
            if (opts.fbConfigs)
               print_fbconfig_info(dpy, scrnum, opts.mode);
            else
#endif
            print_visual_info(dpy, scrnum, opts.mode);
         }

         if (scrnum + 1 < numScreens)
            printf("\n\n");
      }
   }

   XCloseDisplay(dpy);

   return 0;
}
