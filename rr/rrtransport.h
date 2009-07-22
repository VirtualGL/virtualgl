/* Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2009 D. R. Commander
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

#ifndef __RRTRANSPORT_H
#define __RRTRANSPORT_H

#include <X11/Xlib.h>
#include "rr.h"

// Pixel formats
#define RRTRANS_FORMATOPT 6
enum {RRTRANS_RGB, RRTRANS_RGBA, RRTRANS_BGR, RRTRANS_BGRA, RRTRANS_ABGR,
	RRTRANS_ARGB};

static const int rrtrans_ps[RRTRANS_FORMATOPT]={3, 4, 3, 4, 4, 4};
static const int rrtrans_bgr[RRTRANS_FORMATOPT]={0, 0, 1, 1, 1, 0};
static const int rrtrans_afirst[RRTRANS_FORMATOPT]={0, 0, 0, 0, 1, 1};

#if !defined(__SUNPRO_CC) && !defined(__SUNPRO_C)
#pragma pack(1)
#endif

typedef struct _RRFrame
{
	/* A pointer to the pixels in the framebuffer allocated by the transport
		plugin.  If this is a stereo frame, this points to the pixels in the left
		eye buffer.  Pixels returned from VirtualGL are always returned in
    bottom-up order. */
	unsigned char *bits;

	/* If this is a stereo frame, this points to the pixels in the right eye
		buffer */
	unsigned char *rbits;

	/* The format of the pixels in the allocated framebuffer (see enum above) */
	int format;

	/* The width and height of the allocate framebuffer, in pixels */
	int w, h;

	/* The number of bytes in each pixel row of the allocated framebuffer */
	int pitch;

	/* The window ID into which this frame should be drawn.  This is set by
		VirtualGL prior to calling RRTransSendFrame() */
	unsigned int winid;

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
	RRTransInit (FakerConfig *fconfig);
/*
   Initialize an instance of the transport plugin

   fconfig (IN) = pointer to VirtualGL's faker configuration structure, which
                  can be read or modified by the plugin

   RETURN VALUE:
   If successful, a non-NULL instance handle is returned.  This handle can then
   be passed to the functions below.  If the plugin fails to initialize
   properly, then RRTransInit() will return NULL.  RRTransGetError() can
   be called to determine the reason for the failure.
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
   port (IN) = the receiver port to which to connect.  This is typically
               the TCP port on which the receiver is listening, but this
               parameter may be used for other purposes as well.

   RETURN VALUE:
   This function returns 0 on success or -1 on failure.  RRTransGetError() can
   be called to determine the cause of the failure.
*/

RRFrame *
	RRTransGetFrame (void *handle, int width, int height, int stereo, int spoil);
/*
   Retrieve a frame buffer of the requested dimensions from the transport
   plugin's buffer pool

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())
   width, height (IN) = dimensions of frame buffer to be allocated by the
                        transport plugin
   stereo (IN) = 1 to request a stereo frame or 0 to request a mono frame
   spoil (IN) = 1 to allow this frame to be discarded if the receiver is busy
                or 0 to force this frame to be sent

   RETURN VALUE:
   If a buffer is successfully allocated, then a non-NULL pointer to an RRFrame
   structure is returned (see above for a description of this structure.)
   Otherwise, NULL is returned and the reason for the failure can be queried
   with RRTransGetError().

   DEADLOCK WARNING:
   This function may block until a frame is available in the pool.  Frames
   obtained via. RRTransGetFrame() should be sent as soon as possible to avoid
   the possibility of a deadlock.
*/

int
	RRTransFrameReady(void *handle);
/*
   Returns 1 if the plugin is ready to deliver a new frame immediately or
   0 if it is currently processing a frame and a new frame would have to be
   queued for transmission.  This function can be used to implement the "spoil
   last frame" spoiling algorithm within the plugin.

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())
*/

int
	RRTransSendFrame (void *handle, RRFrame *frame);
/*
   Send the contents of a frame buffer to the receiver

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRTransInit())
   frame (IN) = pointer to an RRFrame structure obtained in a previous call to
                RRTransGetFrame()

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

#endif

#endif
