/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2014, 2019 D. R. Commander
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

/* FBXV -- An FBX-like API to abstract interaction with X Video */

#ifndef __FBXV_H__
#define __FBXV_H__

#define USESHM

#include <stdio.h>
#include <X11/Xlib.h>
#ifdef USESHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#include <X11/Xutil.h>


#define YUY2_PACKED  0x32595559
#define YV12_PLANAR  0x32315659
#define UYVY_PACKED  0x59565955
#define I420_PLANAR  0x30323449


typedef struct _fbxv_struct
{
	Display *dpy;  Window win;
	int shm, reqwidth, reqheight, port, doexpose;
	#ifdef USESHM
	XShmSegmentInfo shminfo;  int xattach;
	#endif
	GC xgc;
	XvImage *xvi;
} fbxv_struct;


#ifdef __cplusplus
extern "C" {
#endif

/*
   All of these methods return -1 on failure or 0 on success.
*/


/*
  fb = Address of fbxv_struct (must be pre-allocated by user)
  Display *dpy = X11 display handle
  Window win = X11 drawable into which the video should be drawn
  width = Width of image (in pixels) that you wish to create.  0 = use width
          of window
  height = Height of image (in pixels) that you wish to create.  0 = use height
           of window
  format = GUID of desired X Video image format.  If the X Video extension does
           not support this format, then fbxv_init() returns an error.
  useShm = Use MIT-SHM extension, if available

  NOTES:
  -- fbxv_init() is idempotent.  If you call it multiple times, it will
     re-initialize the buffer only when it is necessary to do so (such as when
     the window size has changed.)
  -- If this function returns successfully, fb->xvi will point to a valid
     XvImage structure that can be populated with image data.  Double-check
     fb->xvi->width and fb->xvi->height, as these may differ from the requested
     width and height.
*/
int fbxv_init(fbxv_struct *fb, Display *dpy, Window win, int width, int height,
	int format, int useShm);


/*
  This routine draws an X Video image stored in fb to the framebuffer

  fb = Address of fbxv_struct previously initialized by a call to fbxv_init()
       fb->xvi->data should contain the image you wish to draw
  srcX = left offset of the region you wish to draw (relative to the X Video
         image)
  srcY = top offset of the region you wish to draw (relative to the X Video
         image)
  srcWidth, srcHeight = width and height of the portion of the source image you
                        wish to draw (0 = whole image)
  dstX = left offset of the window region into which the image should be
         drawn (relative to the window's client area)
  dstY = top offset of the window region into which the image should be
         drawn (relative to the window's client area)
  dstWidth, dstHeight = width and height of the window region into which the
                        image should be drawn
*/
int fbxv_write(fbxv_struct *fb, int srcX, int srcY, int srcWidth,
	int srcHeight, int dstX, int dstY, int dstWidth, int dstHeight);


/*
  Frees the X Video image associated with fb (if any), then frees the memory
  used by fb.

  NOTE: this routine is idempotent.  It only frees stuff that needs freeing.
*/
int fbxv_term(fbxv_struct *fb);


/*
  This returns a string containing the reason why the last command failed
*/
char *fbxv_geterrmsg(void);


/*
  This returns the line (within fbxv.c) of the last failure
*/
int fbxv_geterrline(void);


/*
  By default, FBXV will not print warning messages (such as messages related to
  its automatic selection of a particular drawing method.)  These messages are
  sometimes useful when diagnosing performance issues.  Passing a stream
  pointer (such as stdout, stderr, or a pointer returned from a previous call
  to fopen()) to this function will enable warning messages and will cause them
  to be printed to the specified stream.  Passing an argument of NULL to this
  function will disable warnings.
*/
void fbxv_printwarnings(FILE *stream);


#ifdef __cplusplus
}
#endif

#endif  /* __FBXV_H__ */
