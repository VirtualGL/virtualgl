/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#define RR_MAJOR_VERSION 2
#define RR_MINOR_VERSION 1

// Argh!
#if !defined(__SUNPRO_CC) && !defined(__SUNPRO_C)
#pragma pack(1)
#endif

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
#define sizeof_rrframeheader 26

typedef struct _rrversion
{
	char id[3];
	unsigned char major;
	unsigned char minor;
} rrversion;
#define sizeof_rrversion 5

// Header from version 1 of the VirtualGL protocol (used to communicate with
// older clients
typedef struct _rrframeheader_v1
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
} rrframeheader_v1;
#define sizeof_rrframeheader_v1 24

#if !defined(__SUNPRO_CC) && !defined(__SUNPRO_C)
#pragma pack()
#endif

/* Header flags */
enum {
	RR_EOF=1, /* this tile is an End-of-Frame marker and contains no real
               image data */
	RR_LEFT,  /* this tile goes to the left buffer of a stereo frame */
	RR_RIGHT  /* this tile goes to the right buffer of a stereo frame */
};

/* Transport types */
#define RR_TRANSPORTOPT 3
enum rrtrans {RRTRANS_X11=0, RRTRANS_VGL, RRTRANS_SR};

/* Compression types */
#define RR_COMPRESSOPT  5
enum rrcomp {RRCOMP_PROXY=0, RRCOMP_JPEG, RRCOMP_RGB, RRCOMP_SR, RRCOMP_SRLOSSLESS};

static const rrtrans _Trans[RR_COMPRESSOPT]=
{
	RRTRANS_X11, RRTRANS_VGL, RRTRANS_VGL, RRTRANS_SR, RRTRANS_SR
};

static const int _Minsubsamp[RR_COMPRESSOPT]=
{
	-1, 0, -1, 1, -1
};

static const int _Defsubsamp[RR_COMPRESSOPT]=
{
	1, 1, 1, 16, 16
};

static const int _Maxsubsamp[RR_COMPRESSOPT]=
{
	-1, 4, -1, 16, -1
};

/* Stereo options */
#define RR_STEREOOPT    3
enum rrstereo {RRSTEREO_NONE=0, RRSTEREO_QUADBUF, RRSTEREO_REDCYAN};

/* Other */
#define RR_DEFAULTPORT        4242
#ifdef USESSL
#define RR_DEFAULTSSLPORT     4243
#else
#define RR_DEFAULTSSLPORT     RR_DEFAULTPORT
#endif
#define RR_DEFAULTTILESIZE    256

/* Maximum CPUs that be can be used for parallel image compression */
/* (the algorithms don't scale beyond 3) */
#define MAXPROCS 4

#endif
