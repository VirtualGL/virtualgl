/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005-2007 Sun Microsystems, Inc.
 * Copyright (C)2009-2013 D. R. Commander
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
  unsigned char qual;      /* Quality used when compressing the image (1-100) */
  unsigned char subsamp;   /* Chrominance subsampling used when compressing
                              the image. (1=4:4:4, 2=4:2:2, 4=4:2:0) */
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
  unsigned char qual;      /* Quality used when compressing the image (1-100) */
  unsigned char subsamp;   /* Chrominance subsampling used when compressing
                              the image. (1=4:4:4, 2=4:2:2, 4=4:2:0) */
  unsigned char flags;     /* See enum below */
  unsigned char dpynum;    /* Display number on the client machine that
                              contains the window into which this frame will be
                              drawn */
} rrframeheader_v1;
#define sizeof_rrframeheader_v1 24

/* Header flags */
enum {
  RR_EOF=1, /* this tile is an End-of-Frame marker and contains no real
               image data */
  RR_LEFT,  /* this tile goes to the left buffer of a stereo frame */
  RR_RIGHT  /* this tile goes to the right buffer of a stereo frame */
};

/* Transport types */
#define RR_TRANSPORTOPT 3
enum rrtrans {RRTRANS_X11=0, RRTRANS_VGL, RRTRANS_XV};

/* Compression types */
#define RR_COMPRESSOPT  5
enum rrcomp {RRCOMP_PROXY=0, RRCOMP_JPEG, RRCOMP_RGB, RRCOMP_XV, RRCOMP_YUV};

/* Readback types */
#define RR_READBACKOPT  3
enum rrread {RRREAD_NONE=0, RRREAD_SYNC, RRREAD_PBO};

static const enum rrtrans _Trans[RR_COMPRESSOPT]=
{
  RRTRANS_X11, RRTRANS_VGL, RRTRANS_VGL, RRTRANS_XV, RRTRANS_VGL
};

static const int _Minsubsamp[RR_COMPRESSOPT]=
{
  -1, 0, -1, 4, 4
};

static const int _Defsubsamp[RR_COMPRESSOPT]=
{
  1, 1, 1, 4, 4
};

static const int _Maxsubsamp[RR_COMPRESSOPT]=
{
  -1, 4, -1, 4, 4
};

/* Stereo options */
#define RR_STEREOOPT    9
enum rrstereo {RRSTEREO_LEYE=0, RRSTEREO_REYE, RRSTEREO_QUADBUF,
               RRSTEREO_REDCYAN, RRSTEREO_GREENMAGENTA, RRSTEREO_BLUEYELLOW,
               RRSTEREO_INTERLEAVED, RRSTEREO_TOPBOTTOM, RRSTEREO_SIDEBYSIDE};

/* 3D drawable options */
#define RR_DRAWABLEOPT  2
enum rrdrawable {RRDRAWABLE_PBUFFER=0, RRDRAWABLE_PIXMAP};

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

#define MAXSTR 256

/* Faker configuration */
typedef struct _FakerConfig
{
  char allowindirect;
  char autotest;
  char client[MAXSTR];
  int compress;
  char config[MAXSTR];
  char defaultfbconfig[MAXSTR];
  char drawable;
  double flushdelay;
  int forcealpha;
  double fps;
  double gamma;
  unsigned char gamma_lut[256];
  unsigned short gamma_lut16[65536];
  char glflushtrigger;
  char gllib[MAXSTR];
  char gui;
  unsigned int guikey;
  char guikeyseq[MAXSTR];
  unsigned int guimod;
  char interframe;
  char localdpystring[MAXSTR];
  char log[MAXSTR];
  char logo;
  int np;
  int port;
  char probeglx;
  int qual;
  char readback;
  double refreshrate;
  int samples;
  char spoil;
  char spoillast;
  char ssl;
  int stereo;
  int subsamp;
  char sync;
  int tilesize;
  char trace;
  int transpixel;
  char transport[MAXSTR];
  char transvalid[RR_TRANSPORTOPT];
  char trapx11;
  char vendor[MAXSTR];
  char verbose;
  char wm;
  char x11lib[MAXSTR];
} FakerConfig;

#if !defined(__SUNPRO_CC) && !defined(__SUNPRO_C)
#pragma pack()
#endif

#endif
