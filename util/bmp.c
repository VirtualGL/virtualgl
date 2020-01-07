/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2014, 2017-2019 D. R. Commander
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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif
#include "vglutil.h"
#include "bmp.h"


#ifndef BI_RGB
#define BI_RGB  0L
#endif


#define BMPHDRSIZE  54
typedef struct
{
	unsigned short bfType;
	unsigned int bfSize;
	unsigned short bfReserved1, bfReserved2;
	unsigned int bfOffBits;

	unsigned int biSize;
	int biWidth, biHeight;
	unsigned short biPlanes, biBitCount;
	unsigned int biCompression, biSizeImage;
	int biXPelsPerMeter, biYPelsPerMeter;
	unsigned int biClrUsed, biClrImportant;
} BitmapHeader;


static const char *errorStr = "No error";


#define THROW(m) \
{ \
	errorStr = m;  ret = -1;  goto finally; \
}
#define TRY_UNIX(f) \
{ \
	if((f) == -1) THROW(strerror(errno)); \
}
#define CATCH(f) \
{ \
	if((f) == -1) \
	{ \
		ret = -1;  goto finally; \
	} \
}


#define READ(fd, addr, size) \
	if((bytesRead = read(fd, addr, (size))) == -1) THROW(strerror(errno)); \
	if(bytesRead != (size)) THROW("Read error");

#define WRITE(fd, addr, size) \
	if((bytesWritten = write(fd, addr, (size))) == -1) THROW(strerror(errno)); \
	if(bytesWritten != (size)) THROW("Write error");


static void pixelConvert(unsigned char *srcBuf, int width, int srcPitch,
	int height, PF *srcpf, unsigned char *dstBuf, int dstPitch, PF *dstpf,
	int flip)
{
	if(flip)
	{
		srcBuf = &srcBuf[srcPitch * (height - 1)];
		srcPitch = -srcPitch;
	}
	srcpf->convert(srcBuf, width, srcPitch, height, dstBuf, dstPitch, dstpf);
	while(height--)
	{
		if(width * dstpf->size != dstPitch)
			memset(&dstBuf[width * dstpf->size], 0, dstPitch - width * dstpf->size);
		dstBuf += dstPitch;
	}
}


static int ppm_load(int *fd, unsigned char **buf, int *width, int align,
	int *height, PF *pf, enum BMPORN orientation, int ascii)
{
	FILE *file = NULL;  int ret = 0, scaleFactor, dstPitch;
	unsigned char *tempbuf = NULL;  char temps[255], temps2[255];
	int numRead = 0, totalRead = 0, pixel[3], i, j;

	if((file = fdopen(*fd, "r")) == NULL) THROW(strerror(errno));

	do
	{
		if(!fgets(temps, 255, file)) THROW("Read error");
		if(strlen(temps) == 0 || temps[0] == '\n') continue;
		if(sscanf(temps, "%s", temps2) == 1 && temps2[1] == '#') continue;
		switch(totalRead)
		{
			case 0:
				if((numRead = sscanf(temps, "%d %d %d", width, height,
					&scaleFactor)) == EOF)
					THROW("Read error");
				break;
			case 1:
				if((numRead = sscanf(temps, "%d %d", height, &scaleFactor)) == EOF)
					THROW("Read error");
				break;
			case 2:
				if((numRead = sscanf(temps, "%d", &scaleFactor)) == EOF)
					THROW("Read error");
				break;
		}
		totalRead += numRead;
	} while(totalRead < 3);
	if((*width) < 1 || (*height) < 1 || scaleFactor < 1)
		THROW("Corrupt PPM header");

	dstPitch = (((*width) * pf->size) + (align - 1)) & (~(align - 1));
	if((*buf = (unsigned char *)malloc(dstPitch * (*height))) == NULL)
		THROW("Memory allocation error");
	if(ascii)
	{
		for(j = 0; j < *height; j++)
		{
			for(i = 0; i < *width; i++)
			{
				if(fscanf(file, "%d%d%d", &pixel[0], &pixel[1], &pixel[2]) != 3)
					THROW("Read error");
				pf->setRGB(&(*buf)[j * dstPitch + i * pf->size],
					pixel[0] * 255 / scaleFactor, pixel[1] * 255 / scaleFactor,
					pixel[2] * 255 / scaleFactor);
			}
		}
	}
	else
	{
		if(scaleFactor != 255)
			THROW("Binary PPMs must have 8-bit components");
		if((tempbuf = (unsigned char *)malloc((*width) * (*height) * 3)) == NULL)
			THROW("Memory allocation error");
		if(fread(tempbuf, (*width) * (*height) * 3, 1, file) != 1)
			THROW("Read error");
		pixelConvert(tempbuf, *width, (*width) * 3, *height, pf_get(PF_RGB), *buf,
			dstPitch, pf, orientation == BMPORN_BOTTOMUP);
	}

	finally:
	if(file) { fclose(file);  *fd = -1; }
	free(tempbuf);
	return ret;
}


int bmp_load(char *filename, unsigned char **buf, int *width, int align,
	int *height, int format, enum BMPORN orientation)
{
	int fd = -1, bytesRead, srcPitch, srcPixelSize, dstPitch, ret = 0;
	enum BMPORN srcOrientation = BMPORN_BOTTOMUP;
	unsigned char *tempbuf = NULL;
	BitmapHeader bh;  int flags = O_RDONLY;
	PF *pf = pf_get(format);

	#ifdef _WIN32
	flags |= O_BINARY;
	#endif
	if(!filename || !buf || !width || !height || pf->bpc < 8 || align < 1)
		THROW("Invalid argument to bmp_load()");
	if((align & (align - 1)) != 0)
		THROW("Alignment must be a power of 2");
	TRY_UNIX(fd = open(filename, flags));

	READ(fd, &bh.bfType, sizeof(unsigned short));
	if(!LittleEndian()) bh.bfType = BYTESWAP16(bh.bfType);

	if(bh.bfType == 0x3650)
	{
		CATCH(ppm_load(&fd, buf, width, align, height, pf, orientation, 0));
		goto finally;
	}
	if(bh.bfType == 0x3350)
	{
		CATCH(ppm_load(&fd, buf, width, align, height, pf, orientation, 1));
		goto finally;
	}

	READ(fd, &bh.bfSize, sizeof(unsigned int));
	READ(fd, &bh.bfReserved1, sizeof(unsigned short));
	READ(fd, &bh.bfReserved2, sizeof(unsigned short));
	READ(fd, &bh.bfOffBits, sizeof(unsigned int));
	READ(fd, &bh.biSize, sizeof(unsigned int));
	READ(fd, &bh.biWidth, sizeof(int));
	READ(fd, &bh.biHeight, sizeof(int));
	READ(fd, &bh.biPlanes, sizeof(unsigned short));
	READ(fd, &bh.biBitCount, sizeof(unsigned short));
	READ(fd, &bh.biCompression, sizeof(unsigned int));
	READ(fd, &bh.biSizeImage, sizeof(unsigned int));
	READ(fd, &bh.biXPelsPerMeter, sizeof(int));
	READ(fd, &bh.biYPelsPerMeter, sizeof(int));
	READ(fd, &bh.biClrUsed, sizeof(unsigned int));
	READ(fd, &bh.biClrImportant, sizeof(unsigned int));

	if(!LittleEndian())
	{
		bh.bfSize = BYTESWAP(bh.bfSize);
		bh.bfOffBits = BYTESWAP(bh.bfOffBits);
		bh.biSize = BYTESWAP(bh.biSize);
		bh.biWidth = BYTESWAP(bh.biWidth);
		bh.biHeight = BYTESWAP(bh.biHeight);
		bh.biPlanes = BYTESWAP16(bh.biPlanes);
		bh.biBitCount = BYTESWAP16(bh.biBitCount);
		bh.biCompression = BYTESWAP(bh.biCompression);
		bh.biSizeImage = BYTESWAP(bh.biSizeImage);
		bh.biXPelsPerMeter = BYTESWAP(bh.biXPelsPerMeter);
		bh.biYPelsPerMeter = BYTESWAP(bh.biYPelsPerMeter);
		bh.biClrUsed = BYTESWAP(bh.biClrUsed);
		bh.biClrImportant = BYTESWAP(bh.biClrImportant);
	}

	if(bh.bfType != 0x4d42 || bh.bfOffBits < BMPHDRSIZE || bh.biWidth < 1
		|| bh.biHeight == 0)
		THROW("Corrupt bitmap header");
	if((bh.biBitCount != 24 && bh.biBitCount != 32)
		|| bh.biCompression != BI_RGB)
		THROW("Only uncompessed RGB bitmaps are supported");

	*width = bh.biWidth;  *height = bh.biHeight;
	srcPixelSize = bh.biBitCount / 8;
	if(*height < 0) { *height = -(*height);  srcOrientation = BMPORN_TOPDOWN; }
	srcPitch = (((*width) * srcPixelSize) + 3) & (~3);
	dstPitch = (((*width) * pf->size) + (align - 1)) & (~(align - 1));

	if(srcPitch * (*height) + bh.bfOffBits != bh.bfSize)
		THROW("Corrupt bitmap header");
	if((tempbuf = (unsigned char *)malloc(srcPitch * (*height))) == NULL
		|| (*buf = (unsigned char *)malloc(dstPitch * (*height))) == NULL)
		THROW("Memory allocation error");
	if(lseek(fd, (long)bh.bfOffBits, SEEK_SET) != (long)bh.bfOffBits)
		THROW(strerror(errno));
	TRY_UNIX(bytesRead = read(fd, tempbuf, srcPitch * (*height)));
	if(bytesRead != srcPitch * (*height)) THROW("Read error");

	pixelConvert(tempbuf, *width, srcPitch, *height, pf_get(PF_BGR), *buf,
		dstPitch, pf, srcOrientation != orientation);

	finally:
	free(tempbuf);
	if(fd != -1) close(fd);
	return ret;
}


static int ppm_save(char *filename, unsigned char *buf, int width, int pitch,
	int height, PF *pf, enum BMPORN orientation)
{
	FILE *file = NULL;  int ret = 0;
	unsigned char *tempbuf = NULL;

	if((file = fopen(filename, "wb")) == NULL) THROW(strerror(errno));
	if(fprintf(file, "P6\n") < 1) THROW("Write error");
	if(fprintf(file, "%d %d\n", width, height) < 1) THROW("Write error");
	if(fprintf(file, "255\n") < 1) THROW("Write error");

	if((tempbuf = (unsigned char *)malloc(width * height * 3)) == NULL)
		THROW("Memory allocation error");

	pixelConvert(buf, width, pitch, height, pf, tempbuf, width * 3,
		pf_get(PF_RGB), orientation == BMPORN_BOTTOMUP);

	if((fwrite(tempbuf, width * height * 3, 1, file)) != 1)
		THROW("Write error");

	finally:
	free(tempbuf);
	if(file) fclose(file);
	return ret;
}


int bmp_save(char *filename, unsigned char *buf, int width, int pitch,
	int height, int format, enum BMPORN orientation)
{
	int fd = -1, bytesWritten, dstPitch, ret = 0;
	int flags = O_RDWR | O_CREAT | O_TRUNC;
	unsigned char *tempbuf = NULL;  char *temp;
	BitmapHeader bh;  int mode;
	PF *pf = pf_get(format);

	#ifdef _WIN32
	flags |= O_BINARY;  mode = _S_IREAD | _S_IWRITE;
	#else
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	#endif
	if(!filename || !buf || width < 1 || height < 1 || pf->bpc < 8 || pitch < 0)
		THROW("Invalid argument to bmp_save()");

	if(pitch == 0) pitch = width * pf->size;

	if((temp = strrchr(filename, '.')) != NULL)
	{
		if(!stricmp(temp, ".ppm"))
			return ppm_save(filename, buf, width, pitch, height, pf, orientation);
	}

	TRY_UNIX(fd = open(filename, flags, mode));
	dstPitch = ((width * 3) + 3) & (~3);

	bh.bfType = 0x4d42;
	bh.bfSize = BMPHDRSIZE + dstPitch * height;
	bh.bfReserved1 = 0;  bh.bfReserved2 = 0;
	bh.bfOffBits = BMPHDRSIZE;
	bh.biSize = 40;
	bh.biWidth = width;  bh.biHeight = height;
	bh.biPlanes = 0;  bh.biBitCount = 24;
	bh.biCompression = BI_RGB;  bh.biSizeImage = 0;
	bh.biXPelsPerMeter = 0;  bh.biYPelsPerMeter = 0;
	bh.biClrUsed = 0;  bh.biClrImportant = 0;

	if(!LittleEndian())
	{
		bh.bfType = BYTESWAP16(bh.bfType);
		bh.bfSize = BYTESWAP(bh.bfSize);
		bh.bfOffBits = BYTESWAP(bh.bfOffBits);
		bh.biSize = BYTESWAP(bh.biSize);
		bh.biWidth = BYTESWAP(bh.biWidth);
		bh.biHeight = BYTESWAP(bh.biHeight);
		bh.biPlanes = BYTESWAP16(bh.biPlanes);
		bh.biBitCount = BYTESWAP16(bh.biBitCount);
		bh.biCompression = BYTESWAP(bh.biCompression);
		bh.biSizeImage = BYTESWAP(bh.biSizeImage);
		bh.biXPelsPerMeter = BYTESWAP(bh.biXPelsPerMeter);
		bh.biYPelsPerMeter = BYTESWAP(bh.biYPelsPerMeter);
		bh.biClrUsed = BYTESWAP(bh.biClrUsed);
		bh.biClrImportant = BYTESWAP(bh.biClrImportant);
	}

	WRITE(fd, &bh.bfType, sizeof(unsigned short));
	WRITE(fd, &bh.bfSize, sizeof(unsigned int));
	WRITE(fd, &bh.bfReserved1, sizeof(unsigned short));
	WRITE(fd, &bh.bfReserved2, sizeof(unsigned short));
	WRITE(fd, &bh.bfOffBits, sizeof(unsigned int));
	WRITE(fd, &bh.biSize, sizeof(unsigned int));
	WRITE(fd, &bh.biWidth, sizeof(int));
	WRITE(fd, &bh.biHeight, sizeof(int));
	WRITE(fd, &bh.biPlanes, sizeof(unsigned short));
	WRITE(fd, &bh.biBitCount, sizeof(unsigned short));
	WRITE(fd, &bh.biCompression, sizeof(unsigned int));
	WRITE(fd, &bh.biSizeImage, sizeof(unsigned int));
	WRITE(fd, &bh.biXPelsPerMeter, sizeof(int));
	WRITE(fd, &bh.biYPelsPerMeter, sizeof(int));
	WRITE(fd, &bh.biClrUsed, sizeof(unsigned int));
	WRITE(fd, &bh.biClrImportant, sizeof(unsigned int));

	if((tempbuf = (unsigned char *)malloc(dstPitch * height)) == NULL)
		THROW("Memory allocation error");

	pixelConvert(buf, width, pitch, height, pf, tempbuf, dstPitch,
		pf_get(PF_BGR), orientation != BMPORN_BOTTOMUP);

	if((bytesWritten =
		write(fd, tempbuf, dstPitch * height)) != dstPitch * height)
		THROW(strerror(errno));

	finally:
	free(tempbuf);
	if(fd != -1) close(fd);
	return ret;
}


const char *bmp_geterr(void)
{
	return errorStr;
}
