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
#include <string.h>
#ifdef _WIN32
 #include <io.h>
#else
 #include <unistd.h>
#endif

#ifndef BI_BITFIELDS
#define BI_BITFIELDS 3L
#endif
#ifndef BI_RGB
#define BI_RGB 0L
#endif

#pragma pack (1)
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
// the bitmap data in top-down, BGRA format.  If pad=1, each line in the output
// data is padded to the nearest 32-bit boundary

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
	if((bytesread=read(fd, &bh, sizeof(bmphdr)))==-1) {errstr=strerror(errno);  goto finally;}
	if(bytesread!=sizeof(bmphdr) || bh.bfType!=0x4d42 || bh.bfOffBits<sizeof(bmphdr)
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

// This will save a top-down, BGRA buffer as a Windows bitmap.  If pad=1, each
// line in the input data should be padded to the nearest 32-bit boundary

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
	bh.bfSize=sizeof(bmphdr)+pitch*h;
	bh.bfReserved1=0;  bh.bfReserved2=0;
	bh.bfOffBits=sizeof(bmphdr);
	bh.biSize=40;
	bh.biWidth=w;  bh.biHeight=h;
	bh.biPlanes=0;  bh.biBitCount=d*8;
	bh.biCompression=BI_RGB;  bh.biSizeImage=0;
	bh.biXPelsPerMeter=0;  bh.biYPelsPerMeter=0;
	bh.biClrUsed=0;  bh.biClrImportant=0;
	if((byteswritten=write(fd, &bh, sizeof(bmphdr)))!=sizeof(bmphdr)) {errstr=strerror(errno);  goto finally;}

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
