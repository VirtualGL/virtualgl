/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014, 2017 D. R. Commander
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

/* This provides rudimentary facilities for loading and saving true color
   BMP and PPM files */

#ifndef __BMP_H__
#define __BMP_H__

#include "pf.h"


#define BMP_NUMORN  2
enum BMPORN { BMPORN_TOPDOWN = 0, BMPORN_BOTTOMUP };

#define BMPPAD(width, align)  (((width) + ((align) - 1)) & (~((align) - 1)))


#ifdef __cplusplus
extern "C" {
#endif

/* This will load a Windows bitmap from a file and return a buffer with the
   specified pixel format, scanline alignment, and orientation.  The width and
   height are returned in *width and *height.  Use free() to free the
   buffer once you are finished with it. */

int bmp_load(char *filename, unsigned char **buf, int *width, int align,
	int *height, int format, enum BMPORN orientation);


/* This will save a buffer with the specified pixel format, pitch, orientation,
   width, and height as a 24-bit Windows bitmap or PPM (the filename determines
   which format to use.) */

int bmp_save(char *filename, unsigned char *buf, int width, int pitch,
	int height, int format, enum BMPORN orientation);


const char *bmp_geterr(void);

#ifdef __cplusplus
}
#endif

#endif  /* __BMP_H__ */
