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

#ifndef __BMP_H__
#define __BMP_H__

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
 #include <io.h>
#else
 #include <unistd.h>
#endif
#include "rrutil.h"

#ifndef BI_BITFIELDS
#define BI_BITFIELDS 3L
#endif
#ifndef BI_RGB
#define BI_RGB 0L
#endif

#define BMPHDRSIZE 54
typedef struct _bmphdr
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
} bmphdr;

// This will load a Windows bitmap from a file and return a buffer containing
// the bitmap data in top-down, BGR(A) format.  If pad=1, each line in the output
// data is padded to the nearest 32-bit boundary.  The width, height, and depth
// (in bytes per pixel) are returned in w, h, and d.

#define readme(fd, addr, size) \
	if((bytesread=read(fd, addr, (size)))==-1) {errstr=strerror(errno);  goto finally;} \
	if(bytesread!=(size)) {errstr="Read error";  goto finally;}

static const char *loadbmp(char *filename, unsigned char **buf, int *w, int *h, int *d, int pad)
{
	int fd=-1, bytesread, width, height, depth, pitch, topdown=0, i, index, opitch;
	unsigned char *tempbuf=NULL;  const char *errstr=NULL;
	bmphdr bh;  int flags=O_RDONLY;
	#ifdef _WIN32
	flags|=O_BINARY;
	#endif
	if(!filename || !buf || !w || !h || !d)
	{
		errstr="NULL argument to loadbmp()";  goto finally;
	}
	if((fd=open(filename, flags))==-1) {errstr=strerror(errno);  goto finally;}

	readme(fd, &bh.bfType, sizeof(unsigned short));
	readme(fd, &bh.bfSize, sizeof(unsigned int));
	readme(fd, &bh.bfReserved1, sizeof(unsigned short));
	readme(fd, &bh.bfReserved2, sizeof(unsigned short));
	readme(fd, &bh.bfOffBits, sizeof(unsigned int));
	readme(fd, &bh.biSize, sizeof(unsigned int));
	readme(fd, &bh.biWidth, sizeof(int));
	readme(fd, &bh.biHeight, sizeof(int));
	readme(fd, &bh.biPlanes, sizeof(unsigned short));
	readme(fd, &bh.biBitCount, sizeof(unsigned short));
	readme(fd, &bh.biCompression, sizeof(unsigned int));
	readme(fd, &bh.biSizeImage, sizeof(unsigned int));
	readme(fd, &bh.biXPelsPerMeter, sizeof(int));
	readme(fd, &bh.biYPelsPerMeter, sizeof(int));
	readme(fd, &bh.biClrUsed, sizeof(unsigned int));
	readme(fd, &bh.biClrImportant, sizeof(unsigned int));

	if(!littleendian())
	{
		bh.bfType=byteswap16(bh.bfType);
		bh.bfSize=byteswap(bh.bfSize);
		bh.bfOffBits=byteswap(bh.bfOffBits);
		bh.biSize=byteswap(bh.biSize);
		bh.biWidth=byteswap(bh.biWidth);
		bh.biHeight=byteswap(bh.biHeight);
		bh.biPlanes=byteswap16(bh.biPlanes);
		bh.biBitCount=byteswap16(bh.biBitCount);
		bh.biCompression=byteswap(bh.biCompression);
		bh.biSizeImage=byteswap(bh.biSizeImage);
		bh.biXPelsPerMeter=byteswap(bh.biXPelsPerMeter);
		bh.biYPelsPerMeter=byteswap(bh.biYPelsPerMeter);
		bh.biClrUsed=byteswap(bh.biClrUsed);
		bh.biClrImportant=byteswap(bh.biClrImportant);
	}
	if(bh.bfType!=0x4d42 || bh.bfOffBits<BMPHDRSIZE
	|| bh.biWidth<1 || bh.biHeight==0)
	{
		errstr="Corrupt bitmap header";  goto finally;
	}
	if((bh.biBitCount!=24 && bh.biBitCount!=32) || bh.biCompression!=BI_RGB)
	{
		errstr="Only uncompessed RGB bitmaps are supported";  goto finally;
	}
	width=bh.biWidth;  height=bh.biHeight;  depth=bh.biBitCount/8;
	if(height<0) {height=-height;  topdown=1;}
	*w=width;  *h=height;  *d=depth;
	pitch=((width*depth)+3)&(~3);
	opitch=pad?pitch:width*depth;
	if(pitch*height+bh.bfOffBits!=bh.bfSize)
	{
		errstr="Corrupt bitmap header";  goto finally;
	}
	if((tempbuf=(unsigned char *)malloc(pitch*height))==NULL
	|| (*buf=(unsigned char *)malloc(opitch*height))==NULL)
			{errstr="Memory allocation error";  goto finally;}
	if(lseek(fd, (long)bh.bfOffBits, SEEK_SET)!=(long)bh.bfOffBits)
	{
		errstr=strerror(errno);  goto finally;
	}
	if((bytesread=read(fd, tempbuf, pitch*height))==-1)
	{
		errstr=strerror(errno);  goto finally;
	}
	if(bytesread!=pitch*height) {errstr="Read error";  goto finally;}
	for(i=0; i<height; i++)
	{
		if(!topdown) index=height-1-i;  else index=i;
		memcpy(&(*buf)[i*opitch], &tempbuf[index*pitch], opitch);
	}

	finally:
	if(tempbuf) free(tempbuf);
	if(fd!=-1) close(fd);
	return errstr;
}

// This will save a top-down, BGR(A) buffer as a Windows bitmap.  If pad=1, each
// line in the input data should be padded to the nearest 32-bit boundary.
// d represents the image depth in bytes per pixel.

#define writeme(fd, addr, size) \
	if((byteswritten=write(fd, addr, (size)))==-1) {errstr=strerror(errno);  goto finally;} \
	if(byteswritten!=(size)) {errstr="Write error";  goto finally;}

static const char *savebmp(char *filename, unsigned char *buf, int w, int h, int d, int pad)
{
	int fd=-1, byteswritten, pitch, i, opitch;  int flags=O_RDWR|O_CREAT;
	unsigned char *tempbuf=NULL;  const char *errstr=NULL;
	bmphdr bh;  int mode;
	#ifdef _WIN32
	flags|=O_BINARY;  mode=_S_IREAD|_S_IWRITE;
	#else
	mode=S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	#endif
	if(!filename || !buf || w<1 || h<1)
	{
		errstr="bad argument to savebmp()";  goto finally;
	}
	if(d!=3 && d!=4)
	{
		errstr="Only RGB bitmaps are supported";  goto finally;
	}
	if((fd=open(filename, flags, mode))==-1) {errstr=strerror(errno);  goto finally;}
	pitch=((w*d)+3)&(~3);
	opitch=pad?pitch:w*d;

	bh.bfType=0x4d42;
	bh.bfSize=BMPHDRSIZE+pitch*h;
	bh.bfReserved1=0;  bh.bfReserved2=0;
	bh.bfOffBits=BMPHDRSIZE;
	bh.biSize=40;
	bh.biWidth=w;  bh.biHeight=h;
	bh.biPlanes=0;  bh.biBitCount=d*8;
	bh.biCompression=BI_RGB;  bh.biSizeImage=0;
	bh.biXPelsPerMeter=0;  bh.biYPelsPerMeter=0;
	bh.biClrUsed=0;  bh.biClrImportant=0;

	if(!littleendian())
	{
		bh.bfType=byteswap16(bh.bfType);
		bh.bfSize=byteswap(bh.bfSize);
		bh.bfOffBits=byteswap(bh.bfOffBits);
		bh.biSize=byteswap(bh.biSize);
		bh.biWidth=byteswap(bh.biWidth);
		bh.biHeight=byteswap(bh.biHeight);
		bh.biPlanes=byteswap16(bh.biPlanes);
		bh.biBitCount=byteswap16(bh.biBitCount);
		bh.biCompression=byteswap(bh.biCompression);
		bh.biSizeImage=byteswap(bh.biSizeImage);
		bh.biXPelsPerMeter=byteswap(bh.biXPelsPerMeter);
		bh.biYPelsPerMeter=byteswap(bh.biYPelsPerMeter);
		bh.biClrUsed=byteswap(bh.biClrUsed);
		bh.biClrImportant=byteswap(bh.biClrImportant);
	}

	writeme(fd, &bh.bfType, sizeof(unsigned short));
	writeme(fd, &bh.bfSize, sizeof(unsigned int));
	writeme(fd, &bh.bfReserved1, sizeof(unsigned short));
	writeme(fd, &bh.bfReserved2, sizeof(unsigned short));
	writeme(fd, &bh.bfOffBits, sizeof(unsigned int));
	writeme(fd, &bh.biSize, sizeof(unsigned int));
	writeme(fd, &bh.biWidth, sizeof(int));
	writeme(fd, &bh.biHeight, sizeof(int));
	writeme(fd, &bh.biPlanes, sizeof(unsigned short));
	writeme(fd, &bh.biBitCount, sizeof(unsigned short));
	writeme(fd, &bh.biCompression, sizeof(unsigned int));
	writeme(fd, &bh.biSizeImage, sizeof(unsigned int));
	writeme(fd, &bh.biXPelsPerMeter, sizeof(int));
	writeme(fd, &bh.biYPelsPerMeter, sizeof(int));
	writeme(fd, &bh.biClrUsed, sizeof(unsigned int));
	writeme(fd, &bh.biClrImportant, sizeof(unsigned int));

	if((tempbuf=(unsigned char *)malloc(pitch*h))==NULL)
	{
		errstr="Memory allocation error";  goto finally;
	}
	for(i=0; i<h; i++) memcpy(&tempbuf[i*pitch], &buf[(h-i-1)*opitch], opitch);
	if((byteswritten=write(fd, tempbuf, pitch*h))!=pitch*h)
	{
		errstr=strerror(errno);  goto finally;
	}

	finally:
	if(tempbuf) free(tempbuf);
	if(fd!=-1) close(fd);
	return errstr;
}

#endif
