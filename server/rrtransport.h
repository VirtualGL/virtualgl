/* Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2009-2011 D. R. Commander
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

#ifndef __RRTRANSPORT_H__
#define __RRTRANSPORT_H__

#include <X11/Xlib.h>
#include "rr.h"


// Pixel formats
#define RRTRANS_FORMATOPT 7
enum {RRTRANS_RGB, RRTRANS_RGBA, RRTRANS_BGR, RRTRANS_BGRA, RRTRANS_ABGR,
      RRTRANS_ARGB, RRTRANS_INDEX};

static const int rrtrans_ps[RRTRANS_FORMATOPT]={3, 4, 3, 4, 4, 4, 1};
static const int rrtrans_bgr[RRTRANS_FORMATOPT]={0, 0, 1, 1, 1, 0, 0};
static const int rrtrans_afirst[RRTRANS_FORMATOPT]={0, 0, 0, 0, 1, 1, 0};


#if !defined(__SUNPRO_CC) && !defined(__SUNPRO_C)
#pragma pack(1)
#endif

typedef struct _RRFrame
{
  /* A pointer to the pixels in the framebuffer allocated by the transport
     plugin.  If this is a stereo frame, then this pointer points to the pixels
     in the left eye buffer.  Pixels delivered from VirtualGL to the plugin are
     always delivered in bottom-up order. */
  unsigned char *bits;

  /* If this is a stereo frame, then this pointer points to the pixels in the
     right eye buffer */
  unsigned char *rbits;

  /* The format of the pixels in the allocated framebuffer (see enum above) */
  int format;

  /* The width and height of the allocated framebuffer, in pixels */
  int w, h;

  /* The number of bytes in each pixel row of the allocated framebuffer */
  int pitch;

  /* A pointer to a data structure used by the plugin to represent the
     framebuffer.  No user serviceable parts inside. */
  void *opaque;

} RRFrame;

#if !defined(__SUNPRO_CC) && !defined(__SUNPRO_C)
#pragma pack()
#endif


#ifndef RRTRANS_NOPROTOTYPES

#ifdef __cplusplus
extern "C" {
#endif

void *
   RRTransInit (Display *dpy, Window win, FakerConfig *fconfig);
/*
   Initialize an instance of the transport plugin

   dpy (IN) = a handle to the 2D X server display connection.  The plugin can
              use this handle to send the 3D pixels to the 2D X server using
              X11 functions, if it so desires.
   win (IN) = a handle to the X window into which the application intends the
              3D pixels to be rendered.  The plugin can use this handle to
              deliver the 3D pixels into the window using X11 functions, if it
              so desires.
   fconfig (IN) = pointer to VirtualGL's faker configuration structure, which
                  can be read or modified by the plugin

   RETURN VALUE:
   If successful, a non-NULL instance handle is returned.  This handle can then
   be passed to the functions below.  If the plugin fails to initialize
   properly, then RRTransInit() returns NULL, and RRTransGetError() can be
   called to determine the reason for the failure.
*/


int
   RRTransConnect (void *handle, char *receiver_name, int port);
/*
   Connect to a remote image receiver

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())
   receiver_name (IN) = character string containing the name of the receiver
                        to which to connect.  This is typically the TCP/IP
                        address or hostname of the client machine, but this
                        parameter may be used for other purposes as well.
                        VirtualGL sets this parameter to the value of the
                        VGL_CLIENT environment variable or, if VGL_CLIENT is
                        unset, to the display string of the 2D X server.
   port (IN) = the receiver port to which to connect.  This is typically
               the TCP port on which the receiver is listening, but this
               parameter may be used for other purposes as well.  VirtualGL
               sets this parameter to the value of the "port" variable in the
               faker configuration structure.  See the User's Guide for more
               information on how that variable is set.

   RETURN VALUE:
   This function returns 0 on success or -1 on failure.  RRTransGetError() can
   be called to determine the cause of the failure.
*/


RRFrame *
   RRTransGetFrame (void *handle, int width, int height, int format,
      int stereo);
/*
   Retrieve a frame buffer of the requested dimensions from the transport
   plugin's buffer pool.  If no frame buffers are available in the pool, then
   this function waits until one is available.

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())
   width, height (IN) = dimensions of frame buffer to be allocated by the
                        transport plugin
   format (IN) = the pixel format that VirtualGL believes will produce the
                 most optimal readback performance.  This will always
                 correspond to the "native" pixel format of the Pbuffer.  When
                 PBO readback mode is enabled, then the plugin must return a
                 frame with this pixel format, or VirtualGL will automatically
                 fall back to using synchronous readback mode for this frame.
   stereo (IN) = 1 to request a stereo frame or 0 to request a mono frame

   RETURN VALUE:
   If a buffer is successfully allocated, then a non-NULL pointer to an RRFrame
   structure is returned (see above for a description of this structure.)
   Otherwise, NULL is returned and the reason for the failure can be queried
   with RRTransGetError().
*/


int
   RRTransReady(void *handle);
/*
   Returns 1 if the plugin is ready to deliver a new frame immediately or
   0 if it is currently processing a frame and a new frame would have to be
   queued for transmission.  This function is used by VirtualGL to implement
   frame spoiling with asynchronous image transports.  The function always
   returns 1 if the image transport being implemented by the plugin is
   synchronous.

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())

   RETURN VALUE:
   This function returns -1 on failure.  RRTransGetError() can be called to
   determine the cause of the failure.
*/


int
   RRTransSynchronize(void *handle);
/*
   In asynchronous image transports, this function blocks until the queue is
   empty.  VirtualGL calls this function only if frame spoiling is disabled.
   It is used to synchronize the image transport thread with the OpenGL
   rendering thread while still allowing those two operations to occur in
   parallel.

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())

   RETURN VALUE:
   This function returns 0 on success or -1 on failure.  RRTransGetError() can
   be called to determine the cause of the failure.
*/


int
   RRTransSendFrame (void *handle, RRFrame *frame, int sync);
/*
   Send the contents of a frame buffer to the receiver (or queue it for
   transmission)

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())
   frame (IN) = pointer to an RRFrame structure obtained in a previous call to
                RRTransGetFrame()
   sync (IN) = if this parameter is set to 1, then this frame must be delivered
               synchronously to the client in order to maintain strict GLX
               conformance

   RETURN VALUE:
   This function returns 0 on success or -1 on failure.  RRTransGetError() can
   be called to determine the cause of the failure.
*/


int
   RRTransDestroy(void *handle);
/*
   Clean up an instance of the transport plugin

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())

   RETURN VALUE:
   This function returns 0 on success or -1 on failure.  RRTransGetError() can
   be called to determine the cause of the failure.
*/


const char *
   RRTransGetError(void);
/*
   Return an error string describing why the last command failed
*/


#ifdef __cplusplus
}
#endif

#endif // RRTRANS_NOPROTOTYPES

#endif // __RRTRANSPORT_H__
