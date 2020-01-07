/* Copyright (C)2014, 2017-2019 D. R. Commander
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bmp.h"
#include "vglutil.h"
#include "md5.h"


#define THROW(m) \
{ \
	printf("\n   ERROR: %s\n", m);  retval = -1;  goto bailout; \
}

#define THROW_MD5(filename, md5sum, ref) \
{ \
	printf("\n   %s has an MD5 sum of %s.\n   Should be %s.\n", filename, \
		md5sum, ref); \
	retval = -1;  goto bailout; \
}


static void initBuf(unsigned char *buf, int width, int pitch, int height,
	PF *pf, int orientation)
{
	int i, j;

	for(j = 0; j < height; j++)
	{
		int row = orientation == BMPORN_TOPDOWN ? j : height - j - 1;
		for(i = 0; i < width; i++)
		{
			int r, g, b;
			memset(&buf[row * pitch + i * pf->size], 0, pf->size);
			r = (i * 256 / width) % 256;  g = (j * 256 / height) % 256;
			b = (j * 256 / height + i * 256 / width) % 256;
			if(pf->bpc == 10)
			{
				r <<= 2;  g <<= 2;  b <<= 2;
			}
			pf->setRGB(&buf[row * pitch + i * pf->size], r, g, b);
		}
	}
}


static int cmpBuf(unsigned char *buf, int width, int pitch, int height, PF *pf,
	int orientation)
{
	int i, j, retval = 1;

	for(j = 0; j < height; j++)
	{
		int row = orientation == BMPORN_TOPDOWN ? j : height - j - 1;
		for(i = 0; i < width; i++)
		{
			int r, g, b;
			pf->getRGB(&buf[row * pitch + i * pf->size], &r, &g, &b);
			if(pf->bpc == 10)
			{
				r >>= 2;  g >>= 2;  b >>= 2;
			}
			if(r != (i * 256 / width) % 256 || g != (j * 256 / height) % 256
				|| b != (j * 256 / height + i * 256 / width) % 256)
				retval = 0;
		}
	}
	return retval;
}


static int doTest(const char *ext, int width, int align, int height, PF *pf,
	enum BMPORN orientation)
{
	char filename[80], *md5sum, md5buf[65];
	int pitch = BMPPAD(width * pf->size, align), loadWidth = 0, loadHeight = 0,
		retval = 0;
	unsigned char *buf = NULL;
	char *md5ref = !stricmp(ext, "ppm") ? "c0c9f772b464d1896326883a5c79c545" :
		"b03eec1eaaad38fed9cab5082bf37e52";

	if((buf = (unsigned char *)malloc(pitch * height)) == NULL)
		THROW("Could not allocate memory");
	initBuf(buf, width, pitch, height, pf, orientation);

	snprintf(filename, 80, "bmptest_%s_%d_%s.%s", pf->name, align,
		orientation == BMPORN_TOPDOWN ? "td" : "bu", ext);
	if(bmp_save(filename, buf, width, pitch, height, pf->id, orientation) == -1)
		THROW(bmp_geterr());
	md5sum = MD5File(filename, md5buf);
	if(stricmp(md5sum, md5ref))
		THROW_MD5(filename, md5sum, md5ref);

	free(buf);  buf = NULL;
	if(bmp_load(filename, &buf, &loadWidth, align, &loadHeight, pf->id,
		orientation) == -1)
		THROW(bmp_geterr());
	if(width != loadWidth || height != loadHeight)
	{
		printf("\n   Image dimensions of %s are bogus\n", filename);
		retval = -1;  goto bailout;
	}
	if(!cmpBuf(buf, width, pitch, height, pf, orientation))
	{
		printf("\n   Pixel data in %s is bogus\n", filename);
		retval = -1;  goto bailout;
	}
	unlink(filename);

	bailout:
	free(buf);
	return retval;
}


int main(void)
{
	int align, width = 35, height = 39, format;

	for(align = 1; align <= 8; align *= 2)
	{
		for(format = 0; format < PIXELFORMATS - 1; format++)
		{
			PF *pf = pf_get(format);

			printf("%s Top-Down BMP (row alignment = %d bytes)  ...  ", pf->name,
				align);
			if(doTest("bmp", width, align, height, pf, BMPORN_TOPDOWN) == -1)
				return -1;
			printf("OK.\n");

			printf("%s Top-Down PPM (row alignment = %d bytes)  ...  ", pf->name,
				align);
			if(doTest("ppm", width, align, height, pf, BMPORN_TOPDOWN) == -1)
				return -1;
			printf("OK.\n");

			printf("%s Bottom-Up BMP (row alignment = %d bytes)  ...  ", pf->name,
				align);
			if(doTest("bmp", width, align, height, pf, BMPORN_BOTTOMUP) == -1)
				return -1;
			printf("OK.\n");

			printf("%s Bottom-Up PPM (row alignment = %d bytes)  ...  ", pf->name,
				align);
			if(doTest("ppm", width, align, height, pf, BMPORN_BOTTOMUP) == -1)
				return -1;
			printf("OK.\n");
		}
	}

	return 0;
}
