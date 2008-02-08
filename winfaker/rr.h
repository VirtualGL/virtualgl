/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005-2008 Sun Microsystems, Inc.
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

/* Header contained in all image structures */
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
	unsigned char subsamp;   /* Chrominance subsampling of destination JPEG
	                            1 (=4:4:4), 2 (=4:2:2), or 4 (=4:1:1 or 4:2:0) */
	unsigned char flags;     /* See enum below */
	unsigned char compress;  /* Compression algorithm (see enum below) */
	unsigned short dpynum;   /* Display number on the client machine that
	                            contains the window into which this frame will be
	                            drawn */
} rrframeheader;

/* Header flags */
enum {
	RR_EOF=1, /* this tile is an End-of-Frame marker and contains no real
               image data */
	RR_LEFT,  /* this tile goes to the left buffer of a stereo frame */
	RR_RIGHT  /* this tile goes to the right buffer of a stereo frame */
};

/* Stereo options */
#define RR_STEREOOPT    4
enum rrstereo {RRSTEREO_LEYE=0, RRSTEREO_REYE, RRSTEREO_QUADBUF, RRSTEREO_REDCYAN};

#endif
