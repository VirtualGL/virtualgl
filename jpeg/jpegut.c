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
#include "rrtimer.h"

#define _catch(f) {if((f)==-1) {printf("HPJPEG: %s\n", hpjGetErrorStr());  goto finally;}}

void initbuf(unsigned char *buf, int w, int h, int ps, int flags)
{
	int roffset=(flags&HPJ_BGR)?2:0, goffset=1, boffset=(flags&HPJ_BGR)?0:2, i,
		_i, j;
	if(flags&HPJ_ALPHAFIRST) {roffset++;  goffset++;  boffset++;}
	memset(buf, 0, w*h*ps);
	for(_i=0; _i<16; _i++)
	{
		if(flags&HPJ_BOTTOMUP) i=h-_i-1;  else i=_i;
		for(j=0; j<w; j++)
		{
			buf[(w*i+j)*ps+roffset]=255;
			if(((_i/8)+(j/8))%2==0)
			{
				buf[(w*i+j)*ps+goffset]=255;
				buf[(w*i+j)*ps+boffset]=255;
			}
		}
	}
	for(_i=16; _i<h; _i++)
	{
		if(flags&HPJ_BOTTOMUP) i=h-_i-1;  else i=_i;
		for(j=0; j<w; j++)
		{
			if(((_i/8)+(j/8))%2!=0)
			{
				buf[(w*i+j)*ps+roffset]=255;
				buf[(w*i+j)*ps+goffset]=255;
			}
		}
	}
}

int checkbuf(unsigned char *buf, int w, int h, int ps, int flags)
{
	int roffset=(flags&HPJ_BGR)?2:0, goffset=1, boffset=(flags&HPJ_BGR)?0:2, i,
		_i, j;
	if(flags&HPJ_ALPHAFIRST) {roffset++;  goffset++;  boffset++;}
	for(_i=0; _i<16; _i++)
	{
		if(flags&HPJ_BOTTOMUP) i=h-_i-1;  else i=_i;
		for(j=0; j<w; j++)
		{
			if(buf[(w*i+j)*ps+roffset]<253) return 0;
			if(((_i/8)+(j/8))%2==0)
			{
				if(buf[(w*i+j)*ps+goffset]<253) return 0;
				if(buf[(w*i+j)*ps+boffset]<253) return 0;
			}
			else
			{
				if(buf[(w*i+j)*ps+goffset]>2) return 0;
				if(buf[(w*i+j)*ps+boffset]>2) return 0;
			}
		}
	}
	for(_i=16; _i<h; _i++)
	{
		if(flags&HPJ_BOTTOMUP) i=h-_i-1;  else i=_i;
		for(j=0; j<w; j++)
		{
			if(buf[(w*i+j)*ps+boffset]>2) return 0;
			if(((_i/8)+(j/8))%2==0)
			{
				if(buf[(w*i+j)*ps+roffset]>2) return 0;
				if(buf[(w*i+j)*ps+goffset]>2) return 0;
			}
			else
			{
				if(buf[(w*i+j)*ps+roffset]<253) return 0;
				if(buf[(w*i+j)*ps+goffset]<253) return 0;
			}
		}
	}
	return 1;
}

void writejpeg(unsigned char *jpegbuf, unsigned long jpgbufsize, char *filename)
{
	FILE *outfile=NULL;
	if((outfile=fopen(filename, "wb"))==NULL)
	{
		printf("ERROR: Could not open %s for writing.\n", filename);
		goto finally;
	}
	if(fwrite(jpegbuf, jpgbufsize, 1, outfile)!=1)
	{
		printf("ERROR: Could not write to %s.\n", filename);
		goto finally;
	}

	finally:
	if(outfile) fclose(outfile);
}

void gentestjpeg(hpjhandle hnd, unsigned char *jpegbuf, unsigned long *size,
	int w, int h, int ps, char *basefilename, int subsamp, int qual, int flags)
{
	char tempstr[1024];  unsigned char *bmpbuf=NULL;
	const char *pixformat;  double t;

	if(flags&HPJ_BGR)
	{
		if(ps==3) pixformat="BGR";
		else {if(flags&HPJ_ALPHAFIRST) pixformat="ABGR";  else pixformat="BGRA";}
	}
	else
	{
		if(ps==3) pixformat="RGB";
		else {if(flags&HPJ_ALPHAFIRST) pixformat="ARGB";  else pixformat="RGBA";}
	}
	printf("%s %s -> %s Q%d ... ", pixformat, (flags&HPJ_BOTTOMUP)?"Bottom-Up":"Top-Down ",
		subsamp==HPJ_444?"4:4:4":subsamp==HPJ_422?"4:2:2":"4:1:1", qual);

	if((bmpbuf=(unsigned char *)malloc(w*h*ps+1))==NULL)
	{
		printf("ERROR: Could not allocate buffer\n");  goto finally;
	}
	initbuf(bmpbuf, w, h, ps, flags);
	memset(jpegbuf, 0, HPJBUFSIZE(w, h));

	t=rrtime();
	_catch(hpjCompress(hnd, bmpbuf, w, 0, h, ps, jpegbuf, size, subsamp, qual, flags));
	t=rrtime()-t;

	sprintf(tempstr, "%s_enc_%s_%s_%sQ%d.jpg", basefilename, pixformat, (flags&HPJ_BOTTOMUP)?
		"BU":"TD", subsamp==HPJ_444?"444":subsamp==HPJ_422?"422":"411", qual);
	writejpeg(jpegbuf, *size, tempstr);
	printf("Done.  %f ms\n  Result in %s\n", t*1000., tempstr);

	finally:
	if(bmpbuf) free(bmpbuf);
}

void gentestbmp(hpjhandle hnd, unsigned char *jpegbuf, unsigned long jpegsize,
	int w, int h, int ps, char *basefilename, int subsamp, int qual, int flags)
{
	unsigned char *bmpbuf=NULL;
	const char *pixformat;  int _w=0, _h=0;  double t;

	if(flags&HPJ_BGR)
	{
		if(ps==3) pixformat="BGR";
		else {if(flags&HPJ_ALPHAFIRST) pixformat="ABGR";  else pixformat="BGRA";}
	}
	else
	{
		if(ps==3) pixformat="RGB";
		else {if(flags&HPJ_ALPHAFIRST) pixformat="ARGB";  else pixformat="RGBA";}
	}
	printf("JPEG -> %s %s ... ", pixformat, (flags&HPJ_BOTTOMUP)?"Bottom-Up":"Top-Down ");

	_catch(hpjDecompressHeader(hnd, jpegbuf, jpegsize, &_w, &_h));
	if(_w!=w || _h!=h)
	{
		printf("Incorrect JPEG header\n");  goto finally;
	}

	if((bmpbuf=(unsigned char *)malloc(w*h*ps+1))==NULL)
	{
		printf("ERROR: Could not allocate buffer\n");  goto finally;
	}
	memset(bmpbuf, 0, w*ps*h);

	t=rrtime();
	_catch(hpjDecompress(hnd, jpegbuf, jpegsize, bmpbuf, w, w*ps, h, ps, flags));
	t=rrtime()-t;

	if(checkbuf(bmpbuf, w, h, ps, flags)) printf("Passed.");
	else printf("FAILED!");

	printf("  %f ms\n\n", t*1000.);

	finally:
	if(bmpbuf) free(bmpbuf);
}

void dotest(int w, int h, int ps, char *basefilename)
{
	hpjhandle hnd=NULL, dhnd=NULL;  unsigned char *jpegbuf=NULL;
	unsigned long size;

	if((jpegbuf=(unsigned char *)malloc(HPJBUFSIZE(w, h))) == NULL)
	{
		puts("ERROR: Could not allocate buffer.");  goto finally;
	}

	if((hnd=hpjInitCompress())==NULL)
		{printf("Error in hpjInitCompress():\n%s\n", hpjGetErrorStr());  goto finally;}
	if((dhnd=hpjInitDecompress())==NULL)
		{printf("Error in hpjInitDecompress():\n%s\n", hpjGetErrorStr());  goto finally;}

	gentestjpeg(hnd, jpegbuf, &size, w, h, ps, basefilename, HPJ_444, 100, 0);
	gentestbmp(dhnd, jpegbuf, size, w, h, ps, basefilename, HPJ_444, 100, 0);

	gentestjpeg(hnd, jpegbuf, &size, w, h, ps, basefilename, HPJ_444, 100, HPJ_BGR);
	gentestbmp(dhnd, jpegbuf, size, w, h, ps, basefilename, HPJ_444, 100, HPJ_BGR);

	gentestjpeg(hnd, jpegbuf, &size, w, h, ps, basefilename, HPJ_444, 100, HPJ_BOTTOMUP);
	gentestbmp(dhnd, jpegbuf, size, w, h, ps, basefilename, HPJ_444, 100, HPJ_BOTTOMUP);

	gentestjpeg(hnd, jpegbuf, &size, w, h, ps, basefilename, HPJ_444, 100, HPJ_BGR|HPJ_BOTTOMUP);
	gentestbmp(dhnd, jpegbuf, size, w, h, ps, basefilename, HPJ_444, 100, HPJ_BGR|HPJ_BOTTOMUP);

	if(ps==4)
	{
		gentestjpeg(hnd, jpegbuf, &size, w, h, ps, basefilename, HPJ_444, 100, HPJ_ALPHAFIRST);
		gentestbmp(dhnd, jpegbuf, size, w, h, ps, basefilename, HPJ_444, 100, HPJ_ALPHAFIRST);

		gentestjpeg(hnd, jpegbuf, &size, w, h, ps, basefilename, HPJ_444, 100, HPJ_ALPHAFIRST|HPJ_BGR);
		gentestbmp(dhnd, jpegbuf, size, w, h, ps, basefilename, HPJ_444, 100, HPJ_ALPHAFIRST|HPJ_BGR);

		gentestjpeg(hnd, jpegbuf, &size, w, h, ps, basefilename, HPJ_444, 100, HPJ_ALPHAFIRST|HPJ_BOTTOMUP);
		gentestbmp(dhnd, jpegbuf, size, w, h, ps, basefilename, HPJ_444, 100, HPJ_ALPHAFIRST|HPJ_BOTTOMUP);

		gentestjpeg(hnd, jpegbuf, &size, w, h, ps, basefilename, HPJ_444, 100, HPJ_ALPHAFIRST|HPJ_BGR|HPJ_BOTTOMUP);
		gentestbmp(dhnd, jpegbuf, size, w, h, ps, basefilename, HPJ_444, 100, HPJ_ALPHAFIRST|HPJ_BGR|HPJ_BOTTOMUP);
	}

	finally:
	if(hnd) hpjDestroy(hnd);
	if(dhnd) hpjDestroy(dhnd);

	if(jpegbuf) free(jpegbuf);
}

#define MAXLENGTH 2048

void dotest1(void)
{
	int i, j, i2;  unsigned char *bmpbuf=NULL, *jpgbuf=NULL;
	hpjhandle hnd=NULL;  unsigned long size;
	if((hnd=hpjInitCompress())==NULL)
		{printf("Error in hpjInitCompress():\n%s\n", hpjGetErrorStr());  goto finally;}
	printf("Buffer size regression test\n");
	for(j=1; j<16; j++)
	{
		for(i=1; i<MAXLENGTH; i++)
		{
			if(i%100==0) printf("%.4d x %.4d\b\b\b\b\b\b\b\b\b\b\b", i, j);
			if((bmpbuf=(unsigned char *)malloc(i*j*4))==NULL
			|| (jpgbuf=(unsigned char *)malloc(HPJBUFSIZE(i, j)))==NULL)
			{
				printf("Memory allocation failure\n");  goto finally;
			}
			for(i2=0; i2<i*j; i2++)
			{
				if(i2%2==0) memset(&bmpbuf[i2*4], 0xFF, 4);
				else memset(&bmpbuf[i2*4], 0, 4);
			}
			_catch(hpjCompress(hnd, bmpbuf, i, i*4, j, 4,
				jpgbuf, &size, HPJ_444, 100, HPJ_BGR));
			free(bmpbuf);  bmpbuf=NULL;  free(jpgbuf);  jpgbuf=NULL;

			if((bmpbuf=(unsigned char *)malloc(j*i*4))==NULL
			|| (jpgbuf=(unsigned char *)malloc(HPJBUFSIZE(j, i)))==NULL)
			{
				printf("Memory allocation failure\n");  goto finally;
			}
			for(i2=0; i2<j*i; i2++)
			{
				if(i2%2==0) memset(&bmpbuf[i2*4], 0xFF, 4);
				else memset(&bmpbuf[i2*4], 0, 4);
			}
			_catch(hpjCompress(hnd, bmpbuf, j, j*4, i, 4,
				jpgbuf, &size, HPJ_444, 100, HPJ_BGR));
			free(bmpbuf);  bmpbuf=NULL;  free(jpgbuf);  jpgbuf=NULL;
		}
	}
	printf("Done.      \n");

	finally:
	if(bmpbuf) free(bmpbuf);  if(jpgbuf) free(jpgbuf);
	if(hnd) hpjDestroy(hnd);
}

int main(int argc, char *argv[])
{
	dotest(35, 35, 3, "test");
	dotest(35, 35, 4, "test");
	dotest1();

	return 0;
}

