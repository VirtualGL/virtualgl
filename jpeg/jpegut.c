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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hpjpeg.h"
#include "bmp.h"

#define _catch(f) {if((f)==-1) {printf("Error in %s:\n%s\n", #f, hpjGetErrorStr());  exit(1);}}

void writejpeg(unsigned char *jpegbuf, unsigned long jpgbufsize, char *filename)
{
	FILE *outfile;
	if((outfile=fopen(filename, "wb"))==NULL)
	{
		printf("ERROR: Could not open %s for writing.\n", filename);
		exit(1);
	}
	if(fwrite(jpegbuf, jpgbufsize, 1, outfile)!=1)
	{
		printf("ERROR: Could not write to %s.\n", filename);
		exit(1);
	}
	fclose(outfile);
}

void gentestjpeg(hpjhandle hnd, unsigned char *bmpbuf, unsigned char *jpegbuf,
	unsigned long *size, int w, int h, int ps, char *basefilename, int subsamp,
	int qual, int flags)
{
	char tempstr[1024];

	printf("%d-bit %s %s -> %s Q%d ... ", ps*8, (flags&HPJ_BGR)?"BGR":"RGB", (flags&HPJ_BOTTOMUP)?
		"Bottom-Up":"Top-Down ", subsamp==HPJ_444?"4:4:4":subsamp==HPJ_422?"4:2:2":"4:1:1", qual);

	_catch(hpjCompress(hnd, bmpbuf, w, HPJPAD(w*ps), h, ps,
		jpegbuf, size, subsamp, qual, flags));
	sprintf(tempstr, "%s_enc_%s_%s_%sQ%d.jpg", basefilename, (flags&HPJ_BGR)?"BGR":"RGB", (flags&HPJ_BOTTOMUP)?
		"BU":"TD", subsamp==HPJ_444?"444":subsamp==HPJ_422?"422":"411", qual);
	writejpeg(jpegbuf, *size, tempstr);
	printf("DONE\n");
}

void gentestbmp(hpjhandle hnd, unsigned char *jpegbuf, unsigned long jpegsize,
	unsigned char *bmpbuf, int w, int h, int ps, char *basefilename, int subsamp,
	int qual, int flags)
{
	char tempstr[1024];
	const char *error;
	printf("JPEG -> %d-bit %s %s ... ", ps*8, (flags&HPJ_BGR)?"BGR":"RGB", (flags&HPJ_BOTTOMUP)?
		"Bottom-Up":"Top-Down ");

	memset(bmpbuf, 0, HPJPAD(w*ps)*h);
	_catch(hpjDecompress(hnd, jpegbuf, jpegsize, bmpbuf, w, HPJPAD(w*ps), h, ps, flags));
	sprintf(tempstr, "%s_dec_%s_%s_%sQ%d.bmp", basefilename, (flags&HPJ_BGR)?"BGR":"RGB",
		(flags&HPJ_BOTTOMUP)? "BU":"TD", subsamp==HPJ_444?"444":subsamp==HPJ_422?"422":"411",
		qual);
	if((error=savebmp(tempstr, bmpbuf, w, h, ps, 1))!=NULL)
	{
		printf("ERROR saving bitmap: %s\n", error);  exit(1);
	}
	printf("DONE\n");
}

void dotest(unsigned char *srcbuf, int w, int h, int ps, char *basefilename)
{
	hpjhandle hnd, dhnd;  unsigned char *jpegbuf=NULL, *rgbbuf=NULL;  int i;
	unsigned long size;

	if((jpegbuf=(unsigned char *)malloc(w*h*3)) == NULL
	|| (rgbbuf=(unsigned char *)malloc(HPJPAD(w*ps)*h)) == NULL)
	{
		puts("ERROR: Could not allocate output buffers.");
		exit(1);
	}

	if((hnd=hpjInitCompress())==NULL)
		{printf("Error in hpjInitCompress():\n%s\n", hpjGetErrorStr());  exit(1);}
	if((dhnd=hpjInitDecompress())==NULL)
		{printf("Error in hpjInitDecompress():\n%s\n", hpjGetErrorStr());  exit(1);}
	for(i=0; i<NUMSUBOPT; i++)
	{
		gentestjpeg(hnd, srcbuf, jpegbuf, &size, w, h, ps, basefilename, i, 100, 0);
		gentestbmp(dhnd, jpegbuf, size, rgbbuf, w, h, ps, basefilename, i, 100, 0);
		gentestjpeg(hnd, srcbuf, jpegbuf, &size, w, h, ps, basefilename, i, 50, 0);

		gentestjpeg(hnd, srcbuf, jpegbuf, &size, w, h, ps, basefilename, i, 100, HPJ_BGR);
		gentestbmp(dhnd, jpegbuf, size, rgbbuf, w, h, ps, basefilename, i, 100, HPJ_BGR);
		gentestjpeg(hnd, srcbuf, jpegbuf, &size, w, h, ps, basefilename, i, 50, HPJ_BGR);

		gentestjpeg(hnd, srcbuf, jpegbuf, &size, w, h, ps, basefilename, i, 100, HPJ_BOTTOMUP);
		gentestbmp(dhnd, jpegbuf, size, rgbbuf, w, h, ps, basefilename, i, 100, HPJ_BOTTOMUP);
		gentestjpeg(hnd, srcbuf, jpegbuf, &size, w, h, ps, basefilename, i, 50, HPJ_BOTTOMUP);

		gentestjpeg(hnd, srcbuf, jpegbuf, &size, w, h, ps, basefilename, i, 100, HPJ_BGR|HPJ_BOTTOMUP);
		gentestbmp(dhnd, jpegbuf, size, rgbbuf, w, h, ps, basefilename, i, 100, HPJ_BGR|HPJ_BOTTOMUP);
		gentestjpeg(hnd, srcbuf, jpegbuf, &size, w, h, ps, basefilename, i, 50, HPJ_BGR|HPJ_BOTTOMUP);
	}

	hpjDestroy(hnd);
	hpjDestroy(dhnd);

	if(rgbbuf) free(rgbbuf);
	if(jpegbuf) free(jpegbuf);
}

#define MAXLENGTH 2048

void dotest1(void)
{
	int i, j, i2;  unsigned char *bmpbuf, *jpgbuf;
	hpjhandle hnd;  unsigned long size;
	if((hnd=hpjInitCompress())==NULL)
		{printf("Error in hpjInitCompress():\n%s\n", hpjGetErrorStr());  exit(1);}
	printf("Buffer size regression test\n");
	for(j=1; j<16; j++)
	{
		for(i=1; i<MAXLENGTH; i++)
		{
			if(i%100==0) printf("%.4d x %.4d\b\b\b\b\b\b\b\b\b\b\b", i, j);
			if((bmpbuf=(unsigned char *)malloc(i*j*4))==NULL			
			|| (jpgbuf=(unsigned char *)malloc(HPJBUFSIZE(i, j)))==NULL)
			{
				printf("Memory allocation failure\n");  exit(1);
			}
			for(i2=0; i2<i*j; i2++)
			{
				if(i2%2==0) memset(&bmpbuf[i2*4], 0xFF, 4);
				else memset(&bmpbuf[i2*4], 0, 4);
			}
			_catch(hpjCompress(hnd, bmpbuf, i, i*4, j, 4,
				jpgbuf, &size, HPJ_444, 100, HPJ_BGR));
			free(bmpbuf);  free(jpgbuf);

			if((bmpbuf=(unsigned char *)malloc(j*i*4))==NULL			
			|| (jpgbuf=(unsigned char *)malloc(HPJBUFSIZE(j, i)))==NULL)
			{
				printf("Memory allocation failure\n");  exit(1);
			}
			for(i2=0; i2<j*i; i2++)
			{
				if(i2%2==0) memset(&bmpbuf[i2*4], 0xFF, 4);
				else memset(&bmpbuf[i2*4], 0, 4);
			}
			_catch(hpjCompress(hnd, bmpbuf, j, j*4, i, 4,
				jpgbuf, &size, HPJ_444, 100, HPJ_BGR));
			free(bmpbuf);  free(jpgbuf);
		}
	}
	printf("Done.      \n");
	hpjDestroy(hnd);
}

int main(int argc, char *argv[])
{
	unsigned char *bmpbuf=NULL;  int w, h, psize;
	const char *error=NULL;  char *temp;

	printf("\n");

	if(argc<2)
	{
		printf("USAGE: %s <Inputfile (BMP)>\n", argv[0]);
		printf("       %s -bufsize\n", argv[0]);
		printf("-bufsize = runs buffer size regression test for small JPEGs\n");
		exit(1);
	}

	if(!stricmp(argv[1], "-bufsize")) {dotest1();  exit(0);}

	if((error=loadbmp(argv[1], &bmpbuf, &w, &h, &psize, 1))!=NULL)
	{
		printf("ERROR loading bitmap: %s\n", error);  exit(1);
	}
	temp=strrchr(argv[1], '.');
	if(temp!=NULL) *temp='\0';

	dotest(bmpbuf, w, h, psize, argv[1]);

	if(bmpbuf) free(bmpbuf);
	return 0;
}

