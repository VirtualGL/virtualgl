/*
 * Copyright (C) 1999-2014  Brian Paul   All Rights Reserved.
 * Copyright (C) 2005-2007  Sun Microsystems, Inc.   All Rights Reserved.
 * Copyright (C) 2011, 2013, 2015, 2019-2023  D. R. Commander
 *                                            All Rights Reserved.
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

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <assert.h>
#include <GL/gl.h>
#include <EGL/egl.h>
#ifdef EGLX
#include <GL/glx.h>
#else
#include <EGL/eglext.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "glinfo_common.h"


#ifndef EGL_COLOR_COMPONENT_TYPE_EXT
#define EGL_COLOR_COMPONENT_TYPE_EXT 0x3339
#endif

#ifndef EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT
#define EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT 0x333B
#endif


struct config_attribs
{
   /* EGL config attribs */
   int id;
#ifdef EGLX
   int nativeRenderable;
   int vis_id;
   int klass;
#endif
   int surfaceType;
   int transparentType;
   int transparentRedValue;
   int transparentGreenValue;
   int transparentBlueValue;
   int bufferSize;
   int level;
   int colorBufferType;
   int redSize, greenSize, blueSize, alphaSize;
   int depthSize;
   int stencilSize;
   int numSamples, numMultisample;
   int configCaveat;
   int floatComponents;
   int srgb;
   int bindToTextureRGB, bindToTextureRGBA;
   int renderableType;
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


/**
 * EGL Error checking/warning
 */

static const char *eglErrorString(EGLint error)
{
   switch (error) {
      case EGL_SUCCESS:
         return "Success";
      case EGL_NOT_INITIALIZED:
         return "EGL is not or could not be initialized";
      case EGL_BAD_ACCESS:
         return "EGL cannot access a requested resource";
      case EGL_BAD_ALLOC:
         return "EGL failed to allocate resources for the requested operation";
      case EGL_BAD_ATTRIBUTE:
         return "An unrecognized attribute or attribute value was passed in the attribute list";
      case EGL_BAD_CONTEXT:
         return "An EGLContext argument does not name a valid EGL rendering context";
      case EGL_BAD_CONFIG:
         return "An EGLConfig argument does not name a valid EGL frame buffer configuration";
      case EGL_BAD_CURRENT_SURFACE:
         return "The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid";
      case EGL_BAD_DISPLAY:
         return "An EGLDisplay argument does not name a valid EGL display connection";
      case EGL_BAD_SURFACE:
         return "An EGLSurface argument does not name a valid surface configured for GL rendering";
      case EGL_BAD_MATCH:
         return "Arguments are inconsistent";
      case EGL_BAD_PARAMETER:
         return "One or more argument values are invalid";
      case EGL_BAD_NATIVE_PIXMAP:
         return "A NativePixmapType argument does not refer to a valid native pixmap";
      case EGL_BAD_NATIVE_WINDOW:
         return "A NativeWindowType argument does not refer to a valid native window";
      case EGL_CONTEXT_LOST:
         return "The application must destroy all contexts and reinitialise";
   }
   return "UNKNOWN EGL ERROR";
}

static void
CheckEGLError(const char *function)
{
   int n;
   n = eglGetError();
   if (n != EGL_SUCCESS)
      fprintf(stderr, "Error: %s failed:\n%s\n", function,
              eglErrorString(n));
}


/**
 * Choose a simple FB Config.
 */
static EGLConfig *
choose_config(EGLDisplay edpy)
{
   int configAttrib[] = {
      EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
      EGL_SURFACE_TYPE,      EGL_PBUFFER_BIT,
      EGL_RED_SIZE,          1,
      EGL_GREEN_SIZE,        1,
      EGL_BLUE_SIZE,         1,
      EGL_NONE };
   EGLConfig config;
   int nConfigs;

   if (!eglChooseConfig(edpy, configAttrib, &config, 1, &nConfigs)) {
      CheckEGLError("eglChooseConfig()");
      return 0;
   }

   return config;
}


/**
 * Try to create a EGL context of the given version with flags/options.
 * Note: A version number is required in order to get a core profile
 * (at least w/ NVIDIA).
 */
static EGLContext
create_context_flags(EGLDisplay edpy, EGLConfig config, int major, int minor,
                     int profileMask)
{
   EGLContext context;
   int attribs[20];
   int n = 0;

   /* setup attribute array */
   if (major) {
      attribs[n++] = EGL_CONTEXT_MAJOR_VERSION;
      attribs[n++] = major;
      attribs[n++] = EGL_CONTEXT_MINOR_VERSION;
      attribs[n++] = minor;
   }
   if (profileMask) {
      attribs[n++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
      attribs[n++] = profileMask;
   }
   attribs[n++] = EGL_NONE;

   /* try creating context */
   if (!eglBindAPI(EGL_OPENGL_API))
      return NULL;
   context = eglCreateContext(edpy, config, 0, /* share_context */
                              attribs);

   return context;
}


/**
 * Try to create an EGL context of the newest version.
 */
static EGLContext
create_context_with_config(EGLDisplay edpy, EGLConfig config,
                           GLboolean coreProfile)
{
   EGLContext ctx = 0;

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
         ctx = create_context_flags(edpy, config,
                                    gl_versions[i].major,
                                    gl_versions[i].minor,
                                    EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT);
         if (ctx)
            return ctx;
      }
      /* couldn't get core profile context */
      return 0;
   }

   /* EGL should return a context of the latest GL version that supports
    * the full profile.
    */
   ctx = eglCreateContext(edpy, config, NULL, NULL);

   return ctx;
}


static GLboolean
print_display_info(EGLDisplay edpy,
                   const struct options *opts, int eglDeviceID,
                   GLboolean coreProfile, GLboolean limits,
                   GLboolean coreWorked)
{
   EGLSurface pb;
   EGLContext ctx = NULL;
   EGLConfig config;
   const char *oglstring = coreProfile ? "OpenGL core profile" : "OpenGL";
   EGLint pbattribs[] = { EGL_WIDTH, 100, EGL_HEIGHT, 100, EGL_NONE };

   /*
    * Choose config and create a context.
    */
   config = choose_config(edpy);
   if (config)
      ctx = create_context_with_config(edpy, config, coreProfile);

   if (!ctx) {
      if (!coreProfile)
         CheckEGLError("eglCreateContext()");
      return GL_FALSE;
   }

   /*
    * Create a Pbuffer so that we can just bind the context.
    */
   pb = eglCreatePbufferSurface(edpy, config, pbattribs);
   if (pb == EGL_NO_SURFACE) {
      CheckEGLError("eglCreatePbufferSurface()");
      return GL_FALSE;
   }

   if (eglMakeCurrent(edpy, pb, pb, ctx)) {
      const char *eglAPIs = eglQueryString(edpy, EGL_CLIENT_APIS);
      const char *eglVendor = eglQueryString(edpy, EGL_VENDOR);
      const char *eglVersion = eglQueryString(edpy, EGL_VERSION);
      const char *displayExtensions = eglQueryString(edpy, EGL_EXTENSIONS);
      const char *clientExtensions =
         eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
      const char *glVendor = (const char *) glGetString(GL_VENDOR);
      const char *glRenderer = (const char *) glGetString(GL_RENDERER);
      const char *glVersion = (const char *) glGetString(GL_VERSION);
      char *glExtensions = NULL;
      int eglVersionMajor = 0;
      int eglVersionMinor = 0;
      struct ext_functions extfuncs;

      CheckError(__LINE__);

      /* Get some ext functions */
      extfuncs.GetProgramivARB = (GETPROGRAMIVARBPROC)
         eglGetProcAddress("glGetProgramivARB");
      extfuncs.GetStringi = (GETSTRINGIPROC)
         eglGetProcAddress("glGetStringi");
      extfuncs.GetConvolutionParameteriv = (GETCONVOLUTIONPARAMETERIVPROC)
         eglGetProcAddress("glGetConvolutionParameteriv");

      if (!eglInitialize(edpy, & eglVersionMajor, & eglVersionMinor)) {
         fprintf(stderr, "Error: eglInitialize() failed\n");
         exit(1);
      }

      /* Get list of GL extensions */
      if (coreProfile && extfuncs.GetStringi)
         glExtensions = build_core_profile_extension_list(&extfuncs);
      if (!glExtensions) {
         coreProfile = GL_FALSE;
         glExtensions = (char *) glGetString(GL_EXTENSIONS);
      }

      CheckError(__LINE__);

      if (!coreWorked) {
#ifdef EGLX
         printf("display: %s\n", opts->displayName);
#else
         printf("EGL device ID: egl%d", eglDeviceID);
         if (eglDeviceID == 0)
            printf(" or egl");
         printf("\n");
         if (opts->displayName)
            printf("DRI device path: %s\n", opts->displayName);
#endif
         if (opts->mode != Brief) {
            printf("EGL client APIs string: %s\n", eglAPIs);
            printf("EGL vendor string: %s\n", eglVendor);
            printf("EGL version string: %s\n", eglVersion);
            printf("display EGL extensions:\n");
            print_extension_list(displayExtensions, opts->singleLine);
            printf("client EGL extensions:\n");
            print_extension_list(clientExtensions, opts->singleLine);
            printf("EGL version: %u.%u\n", eglVersionMajor, eglVersionMinor);
         }
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
   else
      CheckEGLError("eglMakeCurrent()");

   eglDestroyContext(edpy, ctx);
   eglDestroySurface(edpy, pb);
   return GL_TRUE;
}


#ifdef EGLX

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

#endif

static const char *
config_surface_type(int type)
{
   static const struct bit_info bits[] = {
      { EGL_WINDOW_BIT, "Window" },
      { EGL_PIXMAP_BIT, "Pixmap" },
      { EGL_PBUFFER_BIT, "Pbuffer" }
   };

   return bitmask_to_string(bits, ELEMENTS(bits), type);
}

static const char *
config_color_buffer_type_name(int type)
{
   switch (type) {
      case EGL_RGB_BUFFER:
         return "rgb";
      case EGL_LUMINANCE_BUFFER:
         return "lum";
      default:
         return "???";
   }
}

static const char *
caveat_string(int caveat)
{
   switch (caveat) {
      case EGL_SLOW_CONFIG:
         return "Slow";
      case EGL_NON_CONFORMANT_CONFIG:
         return "Ncon";
      default:
         return "None";
   }
}


static GLboolean
get_config_attribs(EGLDisplay edpy, EGLConfig config,
                   struct config_attribs *attribs)
{
   const char *ext = eglQueryString(edpy, EGL_EXTENSIONS);
#ifdef EGLX
   int visual_type;
#endif

   memset(attribs, 0, sizeof(struct config_attribs));

   eglGetConfigAttrib(edpy, config, EGL_CONFIG_ID, &attribs->id);
#ifdef EGLX
   eglGetConfigAttrib(edpy, config, EGL_NATIVE_RENDERABLE,
                      &attribs->nativeRenderable);
   eglGetConfigAttrib(edpy, config, EGL_NATIVE_VISUAL_ID, &attribs->vis_id);
   eglGetConfigAttrib(edpy, config, EGL_NATIVE_VISUAL_TYPE, &visual_type);
   /* Some EGL implementations use the GLX_X_VISUAL_TYPE tokens. */
   if (visual_type & 0x8000)
     attribs->klass = glx_token_to_visual_class(visual_type);
   else
     attribs->klass = visual_type;
#endif

   eglGetConfigAttrib(edpy, config, EGL_SURFACE_TYPE, &attribs->surfaceType);
   eglGetConfigAttrib(edpy, config, EGL_LEVEL, &attribs->level);
   eglGetConfigAttrib(edpy, config, EGL_COLOR_BUFFER_TYPE,
                      &attribs->colorBufferType);
   if (attribs->colorBufferType == EGL_RGB_BUFFER)
      eglGetConfigAttrib(edpy, config, EGL_BUFFER_SIZE, &attribs->bufferSize);
   else if (attribs->colorBufferType == EGL_LUMINANCE_BUFFER)
      eglGetConfigAttrib(edpy, config, EGL_LUMINANCE_SIZE,
                         &attribs->bufferSize);

   eglGetConfigAttrib(edpy, config, EGL_RED_SIZE, &attribs->redSize);
   eglGetConfigAttrib(edpy, config, EGL_GREEN_SIZE, &attribs->greenSize);
   eglGetConfigAttrib(edpy, config, EGL_BLUE_SIZE, &attribs->blueSize);
   eglGetConfigAttrib(edpy, config, EGL_ALPHA_SIZE, &attribs->alphaSize);
   eglGetConfigAttrib(edpy, config, EGL_DEPTH_SIZE, &attribs->depthSize);
   eglGetConfigAttrib(edpy, config, EGL_STENCIL_SIZE, &attribs->stencilSize);

   /* get transparent pixel stuff */
   eglGetConfigAttrib(edpy, config, EGL_TRANSPARENT_TYPE,
                      &attribs->transparentType);
   if (attribs->transparentType == EGL_TRANSPARENT_RGB) {
      eglGetConfigAttrib(edpy, config, EGL_TRANSPARENT_RED_VALUE,
                         &attribs->transparentRedValue);
      eglGetConfigAttrib(edpy, config, EGL_TRANSPARENT_GREEN_VALUE,
                         &attribs->transparentGreenValue);
      eglGetConfigAttrib(edpy, config, EGL_TRANSPARENT_BLUE_VALUE,
                         &attribs->transparentBlueValue);
   }

   eglGetConfigAttrib(edpy, config, EGL_SAMPLE_BUFFERS,
                      &attribs->numMultisample);
   eglGetConfigAttrib(edpy, config, EGL_SAMPLES, &attribs->numSamples);
   eglGetConfigAttrib(edpy, config, EGL_CONFIG_CAVEAT, &attribs->configCaveat);

   if (ext && strstr(ext, "EGL_EXT_pixel_format_float")) {
      EGLint colorComponentType;
      eglGetConfigAttrib(edpy, config, EGL_COLOR_COMPONENT_TYPE_EXT,
                         &colorComponentType);
      if (colorComponentType & EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT)
         attribs->floatComponents = GL_TRUE;
   }
   eglGetConfigAttrib(edpy, config, EGL_BIND_TO_TEXTURE_RGB,
                      &attribs->bindToTextureRGB);
   eglGetConfigAttrib(edpy, config, EGL_BIND_TO_TEXTURE_RGBA,
                      &attribs->bindToTextureRGBA);
   eglGetConfigAttrib(edpy, config, EGL_RENDERABLE_TYPE,
                      &attribs->renderableType);
   return GL_TRUE;
}


static void
print_config_attribs_verbose(const struct config_attribs *attribs)
{
#ifdef EGLX
   if (attribs->nativeRenderable)
      printf("Config ID: %x  Visual ID=%x  class=%s\n", attribs->id,
             attribs->vis_id, visual_class_name(attribs->klass));
   else
#endif
      printf("Config ID: %x\n", attribs->id);
   printf("    Surface Types=%s\n",
          config_surface_type(attribs->surfaceType));
   printf("    bufferSize=%d level=%d colorBufferType=%s\n",
          attribs->bufferSize, attribs->level,
          config_color_buffer_type_name(attribs->colorBufferType));
   printf("    rgba: redSize=%d greenSize=%d blueSize=%d alphaSize=%d float=%c\n",
          attribs->redSize, attribs->greenSize,
          attribs->blueSize, attribs->alphaSize,
          attribs->floatComponents ? 'Y' : 'N');
   printf("    depthSize=%d stencilSize=%d\n",
          attribs->depthSize, attribs->stencilSize);
   printf("    multiSample=%d  multiSampleBuffers=%d\n",
          attribs->numSamples, attribs->numMultisample);
   if (attribs->configCaveat == EGL_NONE)
      printf("    configCaveat=None\n");
   else if (attribs->configCaveat == EGL_SLOW_CONFIG)
      printf("    configCaveat=Slow\n");
   else if (attribs->configCaveat == EGL_NON_CONFORMANT_CONFIG)
      printf("    configCaveat=Nonconformant\n");
   if (attribs->transparentType == EGL_NONE) {
      printf("    Opaque.\n");
   }
   else if (attribs->transparentType == EGL_TRANSPARENT_RGB) {
      printf("    Transparent RGB: Red=%d Green=%d Blue=%d\n",
             attribs->transparentRedValue, attribs->transparentGreenValue,
             attribs->transparentBlueValue);
   }
   if (attribs->bindToTextureRGB || attribs->bindToTextureRGBA) {
      printf("    Bind to texture:");
      if (attribs->bindToTextureRGB)
         printf(" RGB");
      if (attribs->bindToTextureRGBA)
         printf(" RGBA");
      printf("\n");
   }
}


static void
print_config_attribs_short_header(void)
{
#ifdef EGLX
 printf("Cfg    Visual  tra buf lev buf colorbuffer   dep ste client APIs   ms  cav  surf\n");
 printf("ID    ID    cl ns  sz  el  typ r  g  b  a  F th  ncl GL GLES   VG ns b eat  typ\n");
 printf("                                                        1 2 3\n");
 printf("--------------------------------------------------------------------------------\n");
#else
 printf("Cfg   tra buf lev buf colorbuffer   dep ste client APIs   ms  cav  surf\n");
 printf("ID    ns  sz  el  typ r  g  b  a  F th  ncl GL GLES   VG ns b eat  typ\n");
 printf("                                               1 2 3\n");
 printf("-----------------------------------------------------------------------\n");
#endif
}


static void
print_config_attribs_short(const struct config_attribs *attribs)
{
   const char *caveat = caveat_string(attribs->configCaveat);

#ifdef EGLX
   printf("0x%03x 0x%03x %2s %c   %-3d %-3d %3s %-2d %-2d %-2d %-2d %c %-3d %-3d",
#else
   printf("0x%03x %c   %-3d %-3d %3s %-2d %-2d %-2d %-2d %c %-3d %-3d",
#endif
          attribs->id,
#ifdef EGLX
          attribs->vis_id,
          attribs->nativeRenderable ?
             visual_class_abbrev(attribs->klass) : "??",
#endif
          attribs->transparentType != EGL_NONE ? 'y' : '.',
          attribs->bufferSize,
          attribs->level,
          config_color_buffer_type_name(attribs->colorBufferType),
          attribs->redSize, attribs->greenSize,
          attribs->blueSize, attribs->alphaSize,
          attribs->floatComponents ? 'f' : '.',
          attribs->depthSize,
          attribs->stencilSize
          );

   printf(" %c  %c %c %c  %c  %-2d %-1d %s",
          attribs->renderableType & EGL_OPENGL_BIT ? 'y' : '.',
          attribs->renderableType & EGL_OPENGL_ES_BIT ? 'y' : '.',
          attribs->renderableType & EGL_OPENGL_ES2_BIT ? 'y' : '.',
          attribs->renderableType & EGL_OPENGL_ES3_BIT ? 'y' : '.',
          attribs->renderableType & EGL_OPENVG_BIT ? 'y' : '.',
          attribs->numSamples, attribs->numMultisample,
          caveat
          );

   printf(" ");
   if (attribs->surfaceType & EGL_PBUFFER_BIT) {
      if (attribs->bindToTextureRGBA)
         printf("A");
      else if (attribs->bindToTextureRGB)
         printf("R");
      else
         printf("P");
   } else
      printf(".");
   if (attribs->surfaceType & EGL_PIXMAP_BIT)
      printf("X");
   else
      printf(".");
   if (attribs->surfaceType & EGL_WINDOW_BIT)
      printf("W");
   else
      printf(".");
  printf("\n");
}


static void
print_config_info(EGLDisplay edpy, InfoMode mode)
{
   int numConfigs = 0;
   struct config_attribs attribs;
   EGLConfig *configs;
   int i;

   /* get list of all configs on this display */
   if (!eglGetConfigs(edpy, NULL, 0, &numConfigs) || numConfigs < 1)
      return;
   configs = (EGLConfig *)malloc(sizeof(EGLConfig) * numConfigs);
   if (!configs) {
      fprintf(stderr, "Error: malloc() failed\n");
      exit(1);
   }
   if (!eglGetConfigs(edpy, configs, numConfigs, &numConfigs)) {
      free(configs);
      return;
   }

   printf("%d EGLConfigs:\n", numConfigs);
   if (mode == Normal)
      print_config_attribs_short_header();

   for (i = 0; i < numConfigs; i++) {
      get_config_attribs(edpy, configs[i], &attribs);

      if (mode == Verbose)
         print_config_attribs_verbose(&attribs);
      else if (mode == Normal)
         print_config_attribs_short(&attribs);
   }
   printf("\n");

   free(configs);
}


static void
usage(void)
{
#ifdef EGLX
   printf("Usage: eglinfo [-display <display>] [-v] [-h] [-l] [-s]\n");
   printf("\t-display <display>: Print EGL configs for the specified X server.\n");
#else
   printf("Usage: eglinfo <device> [-v] [-h] [-l] [-s]\n");
   printf("\t<device>: Print EGL configs for the specified EGL device.  <device> can\n");
   printf("\t          be a DRI device path or an EGL device ID (use -e for a list.)\n");
   printf("\t-e: List all valid EGL devices\n");
#endif
   printf("\t-B: brief output, print only the basics.\n");
   printf("\t-v: Print config info in verbose form.\n");
   printf("\t-h: This information.\n");
   printf("\t-l: Print interesting OpenGL limits.\n");
   printf("\t-s: Print a single extension per line.\n");
}


int
main(int argc, char *argv[])
{
   int major, minor, i, validDevices = 0;
#ifdef EGLX
   Display *dpy;
#else
   const char *exts, *devStr = NULL;
   int numDevices, list = 0;
   EGLDeviceEXT *devices = NULL;
   PFNEGLQUERYDEVICESEXTPROC _eglQueryDevicesEXT;
   PFNEGLQUERYDEVICESTRINGEXTPROC _eglQueryDeviceStringEXT;
   PFNEGLGETPLATFORMDISPLAYEXTPROC _eglGetPlatformDisplayEXT;
#endif
   EGLDisplay edpy;
   struct options opts;
   GLboolean coreWorked;

#ifndef EGLX
   if (argc < 2) {
      usage();
      exit(0);
   }
#endif

   opts.mode = Normal;
   opts.limits = GL_FALSE;
   opts.singleLine = GL_FALSE;
   opts.displayName = NULL;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-v") == 0) {
         opts.mode = Verbose;
      }
      else if (strcmp(argv[i], "-B") == 0) {
         opts.mode = Brief;
      }
      else if (strcmp(argv[i], "-l") == 0) {
         opts.limits = GL_TRUE;
      }
      else if (strcmp(argv[i], "-h") == 0) {
         usage();
         exit(0);
      }
      else if (strcmp(argv[i], "-s") == 0) {
         opts.singleLine = GL_TRUE;
      }
#ifdef EGLX
      else if (strcmp(argv[i], "-display") == 0 && i + 1 < argc) {
         opts.displayName = argv[i + 1];
         i++;
      }
#else
      else if (strcmp(argv[i], "-e") == 0) {
         list = 1;
      }
      else {
         opts.displayName = argv[i];
      }
#endif
   }

#ifdef EGLX
   dpy = XOpenDisplay(opts.displayName);
   if (!dpy) {
      fprintf(stderr, "Error: unable to open display %s\n",
              XDisplayName(opts.displayName));
      return -1;
   }
   edpy = eglGetDisplay(dpy);
   if (!edpy) {
      fprintf(stderr, "Error: unable to open EGL display\n");
      return -1;
   }
   opts.displayName = DisplayString(dpy);
#else
   if (!opts.displayName && !list) {
      usage();
      exit(0);
   }

   exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
   if (!exts) {
      fprintf(stderr, "Error: could not query EGL extensions\n");
      return -1;
   }
   if (!strstr(exts, "EGL_EXT_platform_device")) {
      fprintf(stderr, "Error: EGL_EXT_platform_device extension not available\n");
      return -1;
   }
   _eglQueryDevicesEXT =
      (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
   if (!_eglQueryDevicesEXT) {
      fprintf(stderr, "Error: eglQueryDevicesEXT() could not be loaded\n");
      return -1;
   }
   if (!_eglQueryDevicesEXT(0, NULL, &numDevices) || numDevices < 1) {
      fprintf(stderr, "Error: no EGL devices found\n");
      return -1;
   }
   devices = (EGLDeviceEXT *)malloc(sizeof(EGLDeviceEXT) * numDevices);
   if (!devices) {
      fprintf(stderr, "Error: malloc() failed\n");
      return -1;
   }
   if (!_eglQueryDevicesEXT(numDevices, devices, &numDevices) ||
       numDevices < 1) {
      fprintf(stderr, "Error: could not query EGL devices\n");
      free(devices);
      return -1;
   }
   _eglQueryDeviceStringEXT =
      (PFNEGLQUERYDEVICESTRINGEXTPROC)eglGetProcAddress("eglQueryDeviceStringEXT");
   if (!_eglQueryDeviceStringEXT) {
      fprintf(stderr, "Error: eglQueryDeviceStringEXT() could not be loaded");
      return -1;
   }
   _eglGetPlatformDisplayEXT =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
   if (!_eglGetPlatformDisplayEXT) {
      fprintf(stderr, "Error: eglGetPlatformDisplayEXT() could not be loaded\n");
      return -1;
   }
   for (i = 0; i < numDevices; i++) {
      edpy = _eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[i],
                                       NULL);
      if (!edpy || !eglInitialize(edpy, &major, &minor))
         continue;
      eglTerminate(edpy);
      devStr = _eglQueryDeviceStringEXT(devices[i], EGL_DRM_DEVICE_FILE_EXT);
      if (opts.displayName && !list) {
         if (!strcasecmp(opts.displayName, "egl"))
            break;
         if (!strncasecmp(opts.displayName, "egl", 3)) {
            char temps[80];

            snprintf(temps, 80, "egl%d", validDevices);
            if (!strcasecmp(opts.displayName, temps))
               break;
         }
         if (devStr && !strcmp(devStr, opts.displayName))
            break;
      }
      if (list) {
         printf("EGL device ID: egl%d", validDevices);
         if (validDevices == 0)
            printf(" or egl");
         if (devStr)
            printf(", DRI device path: %s", devStr);
         printf("\n");
      }
      validDevices++;
   }
   if (list) {
      free(devices);
      return 0;
   }
   if (i == numDevices) {
      fprintf(stderr, "Error: invalid EGL device\n");
      free(devices);
      return -1;
   }
   edpy = _eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[i], NULL);
   if (!edpy) {
      fprintf(stderr, "Error: unable to open EGL display\n");
      free(devices);
      return -1;
   }
   free(devices);
#endif
   if (!eglInitialize(edpy, &major, &minor)) {
      fprintf(stderr, "Error: could not initialize EGL\n");
      return -1;
   }

#ifndef EGLX
   opts.displayName = (char *)devStr;
#endif
   coreWorked = print_display_info(edpy, &opts, validDevices, GL_TRUE,
                                   opts.limits, GL_FALSE);
   print_display_info(edpy, &opts, validDevices, GL_FALSE, opts.limits,
                      coreWorked);

   printf("\n");

   if (opts.mode != Brief)
      print_config_info(edpy, opts.mode);

   eglTerminate(edpy);

   return 0;
}
