/* Copyright (C)2005 Sun Microsystems, Inc.
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#ifndef __RRSUNRAY_H
#define __RRSUNRAY_H

#include <X11/Xlib.h>

// Pixel formats
#define RRSUNRAY_FORMATOPT 5
enum {RRSUNRAY_RGB, RRSUNRAY_RGBA, RRSUNRAY_BGR, RRSUNRAY_BGRA, RRSUNRAY_ABGR};

static const int rrsunray_ps[RRSUNRAY_FORMATOPT]={3, 4, 3, 4, 4};
static const int rrsunray_bgr[RRSUNRAY_FORMATOPT]={0, 0, 1, 1, 1};
static const int rrsunray_afirst[RRSUNRAY_FORMATOPT]={0, 0, 0, 0, 1};

#ifdef __cplusplus
extern "C" {
#endif

int
	RRSunRayQueryDisplay (Display *display);
/*
   This function is called by VirtualGL to determine whether the plugin
   library is installed, whether the necessary functions can be loaded from
   it, and whether the display in question is a Sun Ray display.

   RETURN VALUE:
   See below
*/

// RRSunRayQueryDisplay() return codes
enum {RRSUNRAY_NOT=0, RRSUNRAY_NO_ROUTE, RRSUNRAY_WITH_ROUTE};

void *
	RRSunRayInit (Display *display, Window win);
/*
   Initialize an instance of the VirtualGL Sun Ray compressor

   PARAMETERS:
   display (IN) - X display handle associated with this instance.  This should
                  point to a valid Sun Ray display
   win (IN) - Window handle associated with this instance.  This should be a
              valid window handle on the Sun Ray display
   
   RETURN VALUE:
   If successful, a non-NULL instance handle is returned.  This handle can then
   be passed to the functions below.  If the display is not a Sun Ray display,
   VirtualGL cannot find the Sun Ray plugin, or the plugin fails to initialize
   properly, then RRSunRayInit() will return NULL.  RRSunRayGetError() can
   be called to determine the reason for the failure.
*/

unsigned char *
	RRSunRayGetFrame (void *handle, int width, int height, int *pitch,
		int *format, int *bottomup);
/*
   Retrieve a frame buffer of the requested dimensions from the Sun Ray plugin's
   buffer pool.

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRSunRayInit())
   width, height (IN) = dimensions of frame buffer to be allocated by the
                        Sun Ray plugin
   pitch (OUT) = bytes per line of allocated frame buffer
   bottomup (OUT) = 1 if the allocated frame buffer uses bottom-up pixel
                    ordering or 0 otherwise
   format (OUT) = pixel format of allocated frame buffer (see enum)

   RETURN VALUE:
   If a buffer is successfully allocated, then a non-NULL pointer to the buffer
   is returned.  Otherwise, NULL is returned and the reason for the failure can
   be queried with RRSunRayGetError().

   DEADLOCK WARNING:
   This function may block until a frame is available in the pool.  Frames
   obtained via. RRSunRayGetFrame() should be sent as soon as possible to avoid
   the possibility of a deadlock.
*/

int
	RRSunRaySendFrame (void *handle, unsigned char *bitmap, int width,
		int height, int pitch, int format, int bottomup);
/*
   Send the contents of a frame buffer to the VirtualGL Sun Ray compressor.

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRSunRayInit())
   bitmap (IN) = pointer to the frame buffer
   width, height (IN) = frame buffer dimensions
   pitch (IN) = bytes per line of the frame buffer
   bottomup (IN) = 1 if the frame buffer uses bottom up pixel ordering or 0
                   otherwise
   format (IN) = pixel format of the frame buffer (see enum)

   RETURN VALUE:
   This function returns 0 on success or -1 on failure.  RRSunRayGetError() can
   be called to determine the cause of the failure.
*/

int
	RRSunRayDestroy(void *handle);
/*
   Clean up an instance of the Sun Ray plugin

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRSunRayInit())

   RETURN VALUE:
   This function returns 0 on success or -1 on failure.  RRSunRayGetError() can
   be called to determine the cause of the failure.
*/

const char *
	RRSunRayGetError(void *handle);
/*
   Return an error string describing why the last command failed

   PARAMETERS:
   handle (IN) = instance handle (returned from a previous call to
                 RRSunRayInit())
*/

#ifdef __cplusplus
}
#endif

#endif
