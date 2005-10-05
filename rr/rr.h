/* Copyright (C)2004 Landmark Graphics
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

#ifndef __RR_H
#define __RR_H

#if defined(_MSC_VER) && defined(_WIN32) && defined(DLLDEFINE)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#if defined(_MSC_VER) && defined(_WIN32)
#define DLLCALL __stdcall
#else
#define DLLCALL
#endif

/* Header contained in all image structures */
#pragma pack(1)
typedef struct _rrframeheader
{
	unsigned int size;       /* If this frame is compressed, the size (in bytes)
	                            of the compressed image that represents it */
	unsigned int winid;      /* The ID of the window on the client into which
	                            this frame should be drawn (usually this is the
	                            X11 Window handle) */
	unsigned short framew;   /* The width of the entire frame (in pixels) */
	unsigned short frameh;   /* The height of the entire frame (in pixels) */
	unsigned short width;    /* The width of this tile (in pixels) */
	unsigned short height;   /* The height of this tile (in pixels) */
	unsigned short x;        /* The X offset of this tile within the frame */
	unsigned short y;        /* The Y offset of this tile within the frame */
	unsigned char qual;      /* Quality of destination JPEG (1-100) */
	unsigned char subsamp;   /* YUV subsampling of destination JPEG
	                            (RR_411, RR_422, or RR_444) */
	unsigned char flags;     /* See enum below */
	unsigned char dpynum;    /* Display number on the client machine that
	                            contains the window into which this frame will be
	                            drawn */
} rrframeheader;
#pragma pack()

/* Header flags */
enum {
	RR_EOF=1, /* this tile is an End-of-Frame marker and contains no real
               image data */
	RR_LEFT,  /* this tile goes to the left buffer of a stereo frame */
	RR_RIGHT  /* this tile goes to the right buffer of a stereo frame */
};

typedef struct _RRFrame
{
	rrframeheader h;
	int compressed;
	unsigned char *bits;
	void *_opaque;
} RRFrame;

/* Compression types */
#define RR_COMPRESSOPT  2
enum {RRCOMP_DEFAULT=-1, RRCOMP_NONE=0, RRCOMP_JPEG};

/* Subsampling options */
#define RR_SUBSAMPOPT   3
enum {RR_444=0, RR_422, RR_411};

/* Pixel format options */
#define RR_FORMATOPT    6
enum {RR_RGB, RR_RGBA, RR_BGR, RR_BGRA, RR_ABGR, RR_ARGB};

/* Other */
#define RR_DEFAULTPORT        4242
#ifdef USESSL
#define RR_DEFAULTSSLPORT     4243
#else
#define RR_DEFAULTSSLPORT     RR_DEFAULTPORT
#endif
#define RR_DEFAULTTILESIZE    256
typedef void * RRDisplay;

/* Return codes */
#define RR_SUCCESS 0
#define RR_ERROR   -1

/* Global config structure (populated by RRGetConfig() */
typedef struct _rrconfig
{
	char *client;
	char *server;
	int qual;
	int subsamp;
	unsigned short port;
	int spoil;
	int ssl;
	int numprocs;
} rrconfig;

/* Maximum CPUs that be can be used for parallel image compression */
/* (the algorithms don't scale beyond 3) */
#define MAXPROCS 4

#ifdef __cplusplus
extern "C" {
#endif


DLLEXPORT RRDisplay DLLCALL
	RROpenDisplay (char *displayname);
/*
   Open a display connection to a VirtualGL client.  The TCP port, SSL,
   and multi-thread options are read from VGL's global configuration,
   which can be set either through environment variables or through
   RRSetConfig().

   PARAMETERS:
   displayname (IN) = X display name (e.g. "mymachine:0.0") on which the client
                      is running.  Setting this to NULL will allow you to
                      compress frames offline for later transmission.

   RETURN VALUE:
   If successful, a valid display connection handle is returned.  Otherwise,
   NULL is returned and the reason for the failure can be queried with
   RRErrorString() and RRErrorLocation().
*/


DLLEXPORT int DLLCALL
	RRCloseDisplay (RRDisplay display);
/*
   Close a display connection

   PARAMETERS:
   display (IN) = display connection handle returned from a previous call to
                  RROpenDisplay()

   RETURN VALUE:
   If successful, RR_SUCCESS is returned.  Otherwise, RR_ERROR is returned and
   the reason for the failure can be queried with RRErrorString() and
   RRErrorLocation().
*/


DLLEXPORT int DLLCALL
	RRGetFrame (RRDisplay display, int width, int height, int pixelformat,
		int bottomup, RRFrame *frame);
/*
   Retrieve a frame buffer of the requested dimensions and pixel format from
   the VirtualGL buffer pool.  Pixels in the frame are packed with 1-byte row
   alignment.

   PARAMETERS:
   display (IN) = display connection handle returned from a previous call to
                  RROpenDisplay()
   width (IN) = desired width of the frame buffer
   height (IN) = desired height of the frame buffer
   pixelformat (IN) = desired pixel format of the frame buffer.  This should be
                      one of the supported pixel format enumerators (see above)
   bottomup (IN) = 1 if this frame will have bottom-up pixel ordering or
                   0 otherwise
   frame (OUT) = address of a pre-allocated RRFrame structure which will
                 receive information about the frame buffer.

   This function initializes the following RRFrame structure members:
   frame->bits = pointer to the pixels in the frame buffer
   frame->h.framew = frame->h.width = width of the frame (in pixels)
   frame->h.frameh = frame->h.height = height of the frame (in pixels)
   frame->h.x = frame->h.y = 0

   RETURN VALUE:
   If successful, RR_SUCCESS is returned.  Otherwise, RR_ERROR is returned and
   the reason for the failure can be queried with RRErrorString() and
   RRErrorLocation().

   DEADLOCK WARNING:
   This function will block until a frame is available in the pool.  Frames
   obtained via. RRGetFrame() should be released as soon as possible to avoid a
   deadlock.  They can be released explicitly via. RRReleaseFrame() or
   implicitly via. RRSendFrame() or RRCompressFrame().
*/


DLLEXPORT int DLLCALL
	RRCompressFrame (RRDisplay display, RRFrame *frame);
/*
   This function compresses an uncompressed frame, releases the frame from
   the pool, and returns a compressed frame that can be stored for later
   transmission.  The compressed frame should be freed with RRReleaseFrame()
   when you have finished storing it.

   PARAMETERS:
   display (IN) = display connection handle returned from a previous call to
                  RROpenDisplay()
   frame (IN) = RRFrame structure populated previously with a call to
                RRGetFrame().  The following structure members must be set
                prior to calling RRCompressFrame():

                frame->bits = pointer to the uncompressed pixels
                              (if compressing a tile, this should point to
                              the first pixel in the tile)
                frame->h.width = width of the region to compress
                frame->h.height = height of the region to compress
                frame->h.qual = compression quality
                frame->h.subsamp = compression YUV subsampling factor

   RETURN VALUE:
   If successful, RR_SUCCESS is returned.  Otherwise, RR_ERROR is returned and
   the reason for the failure can be queried with RRErrorString() and
   RRErrorLocation().

   NOTE:
   Unlike uncompressed frames, compressed frames do not come from a pool.
   There is no danger of deadlock by leaving them unreleased, but they will
   take up a significant amount of memory.

   PERFORMANCE WARNING:
   Compressing a frame in this manner currently bypasses the multi-threaded
   compressor and the inter-frame differencing engine, so what you get back
   will be the whole frame compressed as a single image.  This is far from the
   most efficient approach and should be only used to compress frames for
   offline storage.
*/


DLLEXPORT int DLLCALL
	RRReleaseFrame (RRDisplay display, RRFrame *frame);
/*
   If the frame is compressed, this function frees the structure and image data
   associated with it.  If the frame is uncompressed, this function returns
   the frame to the pool.  In either case, the frame cannot be used again after
   it is released.

   PARAMETERS:
   display (IN) = display connection handle returned from a previous call to
                  RROpenDisplay()
   frame (IN) = RRFrame structure previously initialized by RRGetFrame() and/or
                RRCompressFrame().

   RETURN VALUE:
   If successful, RR_SUCCESS is returned.  Otherwise, RR_ERROR is returned and
   the reason for the failure can be queried with RRErrorString() and
   RRErrorLocation().
*/


DLLEXPORT int DLLCALL
	RRFrameReady (RRDisplay display);
/*
   Poll the send queue to see whether a new frame would stall

   PARAMETERS:
   display (IN) = display connection handle returned from a previous call to
                  RROpenDisplay()

   RETURN VALUE:
   1 if the queue is empty and a new frame can be compressed and sent
   immediately.  0 if the queue is not empty and the current frame would stall
   by at least one frame interval before being compressed and sent.  On
   failure, RR_ERROR is returned and the reason for the failure can be queried
   with RRErrorString() and RRErrorLocation().
*/   


DLLEXPORT int DLLCALL
	RRSendFrame (RRDisplay display, RRFrame *frame);
/*
   If the frame is uncompressed, this function adds it to the outgoing queue
   where it will be asynchronously compressed and sent by VirtualGL.  If the
   frame is compressed, this function synchronously sends it to the client.  In
   either case, the frame is released and cannot be used again.

   PARAMETERS:
   display (IN) = display connection handle returned from a previous call to
                  RROpenDisplay()
   frame (IN) = RRFrame structure previously initialized by RRGetFrame() and/or
                RRCompressFrame().

   RETURN VALUE:
   If successful, RR_SUCCESS is returned.  Otherwise, RR_ERROR is returned and
   the reason for the failure can be queried with RRErrorString() and
   RRErrorLocation().
*/


DLLEXPORT int DLLCALL
	RRSendRawFrame(RRDisplay display, rrframeheader *h, unsigned char *bits);
/*
   Synchronously send raw frame data to the client.  This data is usually
   obtained from a previously stored frame.

   PARAMETERS:
   display (IN) = display connection handle returned from a previous call to
                  RROpenDisplay()
   rrframeheader (IN) = header structure stored with the frame.  The following
                        members should be populated prior to calling this
                        function.

                        h->size = size of compressed data (in bytes)
                        h->framew, h->frameh = frame dimensions
                        h->width, h->height = tile dimensions
                        h->x, h->y = tile offset (0 if the tile
                                     occupies the entire frame)

   bits (IN) = pointer to compressed data buffer.  It should contain h->size
               bytes of data

   RETURN VALUE:
   If successful, RR_SUCCESS is returned.  Otherwise, RR_ERROR is returned and
   the reason for the failure can be queried with RRErrorString() and
   RRErrorLocation().
*/


DLLEXPORT int DLLCALL
	RRGetConfig(rrconfig *fc);
/*
   This function populates a structure with parameters from the VirtualGL
   global configuration, which is read from the environment during
   initialization.  The values returned in this structure can be modified
   and then passed back into RRSetConfig() to alter the global configuration,
   or they can be used as parameters to RRSendFrame(), etc.

   PARAMETERS:
   fc (IN) = address of previously allocated rrconfig structure.  This will
             be populated with values from the global configuration when this
             function successfully returns.

   RETURN VALUE:
   If successful, RR_SUCCESS is returned.  Otherwise, RR_ERROR is returned and
   the reason for the failure can be queried with RRErrorString() and
   RRErrorLocation().
*/


DLLEXPORT int DLLCALL
	RRSetConfig(rrconfig *fc);
/*
   This function alters the VirtualGL global configuration with the
   corresponding parameters in fc.  Parameters which are out of range or NULL
   will be ignored.

   PARAMETERS:
   fc (IN) = address of previously allocated rrconfig structure.

   RETURN VALUE:
   If successful, RR_SUCCESS is returned.  Otherwise, RR_ERROR is returned and
   the reason for the failure can be queried with RRErrorString() and
   RRErrorLocation().
*/

DLLEXPORT char * DLLCALL
	RRErrorString(void);
/*
   Returns a string describing the nature of the most recent RRlib failure
*/


DLLEXPORT const char * DLLCALL
	RRErrorLocation(void);
/*
   Returns a string describing the location of the most recent RRlib failure
*/


#ifdef __cplusplus
}
#endif

#endif
