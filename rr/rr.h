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

#include <X11/Xlib.h>

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

// Header contained in all image structures
#pragma pack(1)
typedef struct _rrframeheader
{
	unsigned int size;       // For JPEG images, this contains the size (in bytes)
                           // of the compressed images.  For uncompressed images,
                           // it should be 0.
	unsigned int winid;      // Usually the X-Window ID, but can be used for other purposes
	unsigned short winw;     // The width of the source window
	unsigned short winh;     // The height of the source window
	unsigned short bmpw;     // The width of the source bitmap
	unsigned short bmph;     // The height of the source bitmap
	unsigned short bmpx;     // The X offset of the bitmap within the window
	unsigned short bmpy;     // The Y offset of the bitmap within the window
	unsigned char qual;      // Quality of destination JPEG (0-100)
	unsigned char subsamp;   // Subsampling of destination JPEG
                           // (RR_411, RR_422, or RR_444)
	unsigned char eof;       // 1 if this is the last (or only) packet in the frame
	unsigned char dpynum;    // Display number on the client machine
} rrframeheader;
#pragma pack()

// Bitmap structure used to directly feed compressor.  This is usually allocated
// by RRlib via. a call to RRGetBitmap
typedef struct _rrbmp
{
	rrframeheader h;
	int flags;              // See below
	int pixelsize;          // bytes per pixel
	int strip_height;       // Height of image strips to send (only image strips
                          // which differ from the previous frame are sent)
	unsigned char *bits;    // The pixel buffer
} rrbmp;

// Bitmap flags
#define RRBMP_BOTTOMUP 1  // Bottom-up bitmap (as opposed to top-down)
#define RRBMP_BGR      2  // BGR or BGRA pixel order
#define RRBMP_EOLPAD   4  // Each input scanline is padded to the next 32-bit boundary
#define RRBMP_FORCE    8  // Send this frame, even if identical to last.  Normally,
                          // only a refresh packet is sent for identical frames

// Subsampling options
#define RR_SUBSAMP  3
enum {RR_444=0, RR_422, RR_411};

// Other
#define RR_DEFAULTPORT	4242
typedef void * RRDisplay;

// Return codes
#define RR_SUCCESS 0
#define RR_ERROR   -1

// Error handling
typedef struct _RRError
{
	const char *file;
	int line;
	char *message;
} RRError;

#ifdef __cplusplus
extern "C" {
#endif

// Display Server API
DLLEXPORT RRDisplay DLLCALL
	RRInitDisplay(unsigned short port, int ssl);
DLLEXPORT int DLLCALL
	RRTerminateDisplay(RRDisplay rrdpy);

// Display Client API
DLLEXPORT RRDisplay DLLCALL
	RROpenDisplay(char *servername, unsigned short port, int ssl);
DLLEXPORT rrbmp* DLLCALL
	RRGetBitmap(RRDisplay rrdpy, int width, int height, int pixelsize);
DLLEXPORT int DLLCALL
	RRFrameReady(RRDisplay rrdpy);
DLLEXPORT int DLLCALL
	RRSendFrame(RRDisplay rrdpy, rrbmp *frame);
DLLEXPORT int DLLCALL
	RRCloseDisplay(RRDisplay rrdpy);

// Other functions
DLLEXPORT RRError DLLCALL
	RRGetError(void);

#ifdef __cplusplus
}
#endif

#endif
