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

#define BMPPIXELFORMATS 6
enum BMPPIXELFORMAT {BMP_RGB=0, BMP_RGBA, BMP_BGR, BMP_BGRA, BMP_ABGR, BMP_ARGB};

// This will load a Windows bitmap from a file and return a buffer with the
// specified pixel format, scanline alignment, and orientation.  The width and
// height are returned in w and h.

#define readme(fd, addr, size) \
	if((bytesread=read(fd, addr, (size)))==-1) {errstr=strerror(errno);  goto finally;} \
	if(bytesread!=(size)) {errstr="Read error";  goto finally;}

static const char *loadbmp(char *filename, unsigned char **buf, int *w, int *h,
	BMPPIXELFORMAT f, int align, int dstbottomup)
{
	int fd=-1, bytesread, width, height, srcpitch, srcbottomup=1,
		i, j, srcps, dstpitch;
	unsigned char *tempbuf=NULL, *srcptr, *srcptr0, *dstptr, *dstptr0;
	const char *errstr=NULL;
	bmphdr bh;  int flags=O_RDONLY;
	const int ps[BMPPIXELFORMATS]={3, 4, 3, 4, 4, 4};
	const int roffset[BMPPIXELFORMATS]={0, 0, 2, 2, 3, 1};
	const int goffset[BMPPIXELFORMATS]={1, 1, 1, 1, 2, 2};
	const int boffset[BMPPIXELFORMATS]={2, 2, 0, 0, 1, 3};

	dstbottomup=dstbottomup? 1:0;
	#ifdef _WIN32
	flags|=O_BINARY;
	#endif
	if(!filename || !buf || !w || !h || f<0 || f>BMPPIXELFORMATS-1 || align<1)
	{
		errstr="invalid argument to loadbmp()";  goto finally;
	}
	if(align&(align-1)!=0)
	{
		errstr="Alignment must be a power of 2";  goto finally;
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
	width=bh.biWidth;  height=bh.biHeight;  srcps=bh.biBitCount/8;
	if(height<0) {height=-height;  srcbottomup=0;}
	*w=width;  *h=height;
	srcpitch=((width*srcps)+3)&(~3);
	dstpitch=((width*ps[f])+(align-1))&(~(align-1));

	if(srcpitch*height+bh.bfOffBits!=bh.bfSize)
	{
		errstr="Corrupt bitmap header";  goto finally;
	}
	if((tempbuf=(unsigned char *)malloc(srcpitch*height))==NULL
	|| (*buf=(unsigned char *)malloc(dstpitch*height))==NULL)
			{errstr="Memory allocation error";  goto finally;}
	if(lseek(fd, (long)bh.bfOffBits, SEEK_SET)!=(long)bh.bfOffBits)
	{
		errstr=strerror(errno);  goto finally;
	}
	if((bytesread=read(fd, tempbuf, srcpitch*height))==-1)
	{
		errstr=strerror(errno);  goto finally;
	}
	if(bytesread!=srcpitch*height) {errstr="Read error";  goto finally;}

	srcptr=(srcbottomup!=dstbottomup)? &tempbuf[srcpitch*(height-1)]:tempbuf;
	for(j=0, dstptr=*buf; j<height; j++,
		srcptr+=(srcbottomup!=dstbottomup)? -srcpitch:srcpitch, dstptr+=dstpitch)
	{
		for(i=0, srcptr0=srcptr, dstptr0=dstptr; i<width; i++,
			srcptr0+=srcps, dstptr0+=ps[f])
		{
			dstptr0[boffset[f]]=srcptr0[0];
			dstptr0[goffset[f]]=srcptr0[1];
			dstptr0[roffset[f]]=srcptr0[2];
		}
	}

	finally:
	if(tempbuf) free(tempbuf);
	if(fd!=-1) close(fd);
	return errstr;
}

// This will save a buffer with the specified pixel format, pitch, orientation,
// width, and height as a 24-bit Windows bitmap.

#define writeme(fd, addr, size) \
	if((byteswritten=write(fd, addr, (size)))==-1) {errstr=strerror(errno);  goto finally;} \
	if(byteswritten!=(size)) {errstr="Write error";  goto finally;}

static const char *savebmp(char *filename, unsigned char *buf, int w, int h,
	BMPPIXELFORMAT f, int srcpitch, int srcbottomup)
{
	int fd=-1, byteswritten, dstpitch, i, j;  int flags=O_RDWR|O_CREAT|O_TRUNC;
	unsigned char *tempbuf=NULL, *srcptr, *srcptr0, *dstptr, *dstptr0;
	const char *errstr=NULL;
	bmphdr bh;  int mode;
	const int ps[BMPPIXELFORMATS]={3, 4, 3, 4, 4, 4};
	const int roffset[BMPPIXELFORMATS]={0, 0, 2, 2, 3, 1};
	const int goffset[BMPPIXELFORMATS]={1, 1, 1, 1, 2, 2};
	const int boffset[BMPPIXELFORMATS]={2, 2, 0, 0, 1, 3};

	#ifdef _WIN32
	flags|=O_BINARY;  mode=_S_IREAD|_S_IWRITE;
	#else
	mode=S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	#endif
	if(!filename || !buf || w<1 || h<1 || f<0 || f>BMPPIXELFORMATS-1 || srcpitch<1)
	{
		errstr="bad argument to savebmp()";  goto finally;
	}
	if((fd=open(filename, flags, mode))==-1) {errstr=strerror(errno);  goto finally;}
	dstpitch=((w*3)+3)&(~3);

	bh.bfType=0x4d42;
	bh.bfSize=BMPHDRSIZE+dstpitch*h;
	bh.bfReserved1=0;  bh.bfReserved2=0;
	bh.bfOffBits=BMPHDRSIZE;
	bh.biSize=40;
	bh.biWidth=w;  bh.biHeight=h;
	bh.biPlanes=0;  bh.biBitCount=24;
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

	if((tempbuf=(unsigned char *)malloc(dstpitch*h))==NULL)
	{
		errstr="Memory allocation error";  goto finally;
	}

	srcptr=srcbottomup? buf:&buf[srcpitch*(h-1)];
	for(j=0, dstptr=tempbuf; j<h; j++, srcptr+=srcbottomup? srcpitch:-srcpitch,
		dstptr+=dstpitch)
	{
		for(i=0, srcptr0=srcptr, dstptr0=dstptr; i<w; i++,
			srcptr0+=ps[f], dstptr0+=3)
		{
			dstptr0[0]=srcptr0[boffset[f]];
			dstptr0[1]=srcptr0[goffset[f]];
			dstptr0[2]=srcptr0[roffset[f]];
		}
	}

	if((byteswritten=write(fd, tempbuf, dstpitch*h))!=dstpitch*h)
	{
		errstr=strerror(errno);  goto finally;
	}

	finally:
	if(tempbuf) free(tempbuf);
	if(fd!=-1) close(fd);
	return errstr;
}

#endif
