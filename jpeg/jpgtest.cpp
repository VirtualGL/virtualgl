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
#include <math.h>
#include "bmp.h"
#include "rrutil.h"
#include "rrtimer.h"
#include "hpjpeg.h"

#define _catch(f) {if((f)==-1) {printf("Error in %s:\n%s\n", #f, hpjGetErrorStr());  exit(1);}}

int forcemmx=0, forcesse=0, forcesse2=0;
const int _ps[BMPPIXELFORMATS]={3, 4, 3, 4, 4, 4};
const int _flags[BMPPIXELFORMATS]={0, 0, HPJ_BGR, HPJ_BGR,
	HPJ_BGR|HPJ_ALPHAFIRST, HPJ_ALPHAFIRST};
const char *_pfname[]={"RGB", "RGBA", "BGR", "BGRA", "ABGR", "ARGB"};

void printsigfig(double val, int figs)
{
	char format[80];
	double _l=log10(val);  int l;
	if(_l<0.)
	{
		l=(int)fabs(_l);
		sprintf(format, "%%%d.%df", figs+l+2, figs+l);
	}
	else
	{
		l=(int)_l+1;
		if(figs<=l) sprintf(format, "%%.0f");
		else sprintf(format, "%%%d.%df", figs+1, figs-l);
	}	
	printf(format, val);
}

void dotest(unsigned char *srcbuf, int w, int h, BMPPIXELFORMAT pf, int bu,
	int jpegsub, int qual, char *filename, int dostrip, int useppm, int quiet)
{
	char tempstr[1024];
	FILE *outfile;  hpjhandle hnd;
	unsigned char *jpegbuf, *rgbbuf;
	rrtimer timer; double elapsed;
	int jpgbufsize=0, i, j, striph, ITER=5, temph;
	unsigned long *compstripsize;
	int flags=(forcemmx?HPJ_FORCEMMX:0)|(forcesse?HPJ_FORCESSE:0)|(forcesse2?HPJ_FORCESSE2:0);
	int ps=_ps[pf];
	int pitch=w*ps;

	flags |= _flags[pf];
	if(bu) flags |= HPJ_BOTTOMUP;

	if((jpegbuf=(unsigned char *)malloc(HPJBUFSIZE(w, h))) == NULL
	|| (rgbbuf=(unsigned char *)malloc(pitch*h)) == NULL
	|| (compstripsize=(unsigned long *)malloc(sizeof(unsigned long)*h)) == NULL)
	{
		puts("ERROR: Could not allocate image buffers.");
		exit(1);
	}

	if(!quiet) printf("\n>>>>>  %s (%s) <--> JPEG %s Q%d  <<<<<\n", _pfname[pf],
		bu?"Bottom-up":"Top-down",
		jpegsub==HPJ_411?"4:1:1":jpegsub==HPJ_422?"4:2:2":"4:4:4", qual);
	if(dostrip) striph=8;  else striph=h;
	do
	{
		striph*=2;  if(striph>h) striph=h;
		if(quiet) printf("%s\t%s\t%s\t%d\t",  _pfname[pf], bu?"BU":"TD",
			jpegsub==HPJ_411?"4:1:1":jpegsub==HPJ_422?"4:2:2":"4:4:4", qual);
		for(i=0; i<h; i++) memcpy(&rgbbuf[pitch*i], &srcbuf[w*ps*i], w*ps);
		if((hnd=hpjInitCompress())==NULL)
			{printf("Error in hpjInitCompress():\n%s\n", hpjGetErrorStr());  exit(1);}
		_catch(hpjCompress(hnd, rgbbuf, w, pitch, striph, ps,
			jpegbuf, &compstripsize[0], jpegsub, qual, flags));
		ITER=0;
		timer.start();
		do
		{
			jpgbufsize=0;
			j=0;
			do
			{
				if(h-j>striph && h-j<striph+HPJ_MINHEIGHT) temph=h-j;  else temph=min(striph, h-j);
				_catch(hpjCompress(hnd, &rgbbuf[pitch*j], w, pitch, temph, ps,
					&jpegbuf[w*3*j], &compstripsize[j], jpegsub, qual, flags));
				jpgbufsize+=compstripsize[j];
				j+=temph;
			} while(j<h);
			ITER++;
		} while((elapsed=timer.elapsed())<2.);
		_catch(hpjDestroy(hnd));
		if(quiet)
		{
			if(striph==h) printf("Full\t");  else printf("%d\t", striph);
			printsigfig((double)(w*h)/1000000.*(double)ITER/elapsed, 4);
			printf("\t");
			printsigfig((double)(w*h*ps)/(double)jpgbufsize, 4);
			printf("\t");
		}
		else
		{
			if(striph==h) printf("\nFull image\n");  else printf("\nStrip size: %d x %d\n", w, striph);
			printf("C--> Frame rate:           %f fps\n", (double)ITER/elapsed);
			printf("     Output image size:    %d bytes\n", jpgbufsize);
			printf("     Compression ratio:    %f:1\n", (double)(w*h*ps)/(double)jpgbufsize);
			printf("     Source throughput:    %f Megapixels/sec\n", (double)(w*h)/1000000.*(double)ITER/elapsed);
			printf("     Output bit stream:    %f Megabits/sec\n", (double)jpgbufsize*8./1000000.*(double)ITER/elapsed);
		}
		if(striph==h)
		{
			sprintf(tempstr, "%s_%dQ%d.jpg", filename, jpegsub==HPJ_444?444:
				jpegsub==HPJ_422?422:411, qual);
			if((outfile=fopen(tempstr, "wb"))==NULL)
			{
				puts("ERROR: Could not open reference image");
				exit(1);
			}
			if(fwrite(jpegbuf, jpgbufsize, 1, outfile)!=1)
			{
				puts("ERROR: Could not write reference image");
				exit(1);
			}
			fclose(outfile);
			if(!quiet) printf("Reference image written to %s\n", tempstr);
		}
		memset(rgbbuf, 127, pitch*h);  // Grey image means decompressor did nothing
		if((hnd=hpjInitDecompress())==NULL)
			{printf("Error in hpjInitDecompress():\n%s\n", hpjGetErrorStr());  exit(1);}
		_catch(hpjDecompress(hnd, jpegbuf, jpgbufsize, rgbbuf, w, pitch,
			striph, ps, flags));
		ITER=0;
		timer.start();
		do
		{
			j=0;
			do
			{
				if(h-j>striph && h-j<striph+HPJ_MINHEIGHT) temph=h-j;  else temph=min(striph, h-j);
				_catch(hpjDecompress(hnd, &jpegbuf[w*3*j], compstripsize[j],
					&rgbbuf[pitch*j], w, pitch, temph, ps, flags));
				j+=temph;
			} while(j<h);
			ITER++;
		}	while((elapsed=timer.elapsed())<2.);
		_catch(hpjDestroy(hnd));
		if(quiet)
		{
			printsigfig((double)(w*h)/1000000.*(double)ITER/elapsed, 4);
			printf("\n");
		}
		else
		{
			printf("D--> Frame rate:           %f fps\n", (double)ITER/elapsed);
			printf("     Dest. throughput:     %f Megapixels/sec\n", (double)(w*h)/1000000.*(double)ITER/elapsed);
		}
		if(striph==h) sprintf(tempstr, "%s_%dQ%d_full.%s", filename,
				jpegsub==HPJ_444?444:jpegsub==HPJ_422?422:411, qual, useppm?"ppm":"bmp");
		else sprintf(tempstr, "%s_%dQ%d_%d.%s", filename, jpegsub==HPJ_444?444:
				jpegsub==HPJ_422?422:411, qual, striph, useppm?"ppm":"bmp");
		if(savebmp(tempstr, rgbbuf, w, h, pf, pitch, bu)==-1)
		{
			printf("ERROR saving bitmap: %s\n", bmpgeterr());  exit(1);
			free(jpegbuf);  free(rgbbuf);  free(compstripsize);
		}
		sprintf(strrchr(tempstr, '.'), "-err.%s", useppm?"ppm":"bmp");
		if(!quiet) printf("Computing compression error and saving to %s.\n", tempstr);
		for(j=0; j<h; j++) for(i=0; i<w*ps; i++)
			rgbbuf[pitch*j+i]=abs(rgbbuf[pitch*j+i]-srcbuf[w*ps*j+i]);
		if(savebmp(tempstr, rgbbuf, w, h, pf, pitch, bu)==-1)
		{
			printf("ERROR saving bitmap: %s\n", bmpgeterr());  exit(1);
			free(jpegbuf);  free(rgbbuf);  free(compstripsize);
		}
	} while(striph<h);

	free(jpegbuf);  free(rgbbuf);  free(compstripsize);
}


int main(int argc, char *argv[])
{
	unsigned char *bmpbuf=NULL;  int w, h, i, useppm=0;
	int qual, dostrip=0, quiet=0, hiqual=-1;  char *temp;
	BMPPIXELFORMAT pf=BMP_BGR;
	int bu=0;

	printf("\n");

	if(argc<3)
	{
		printf("USAGE: %s <Inputfile (BMP|PPM)> <%% Quality>\n\n", argv[0]);
		printf("       [-strip]\n");
		printf("       Test performance of the codec when the image is encoded\n");
		printf("       as separate strips of varying sizes.\n\n");
		printf("       [-forcemmx] [-forcesse] [-forcesse2]\n");
		printf("       Force MMX, SSE, or SSE2 code paths in Intel codec\n\n");
		printf("       [-rgb | -bgr | -rgba | -bgra | -abgr | -argb]\n");
		printf("       Test the specified color conversion path in the codec (default: BGR)\n\n");
		printf("       [-quiet]\n");
		printf("       Output in tabular rather than verbose format\n\n");
		printf("       NOTE: If the quality is specified as a range, i.e. 90-100, a separate\n");
		printf("       test will be performed for all quality values in the range.\n");
		exit(1);
	}
	if((qual=atoi(argv[2]))<1 || qual>100)
	{
		puts("ERROR: Quality must be between 1 and 100.");
		exit(1);
	}
	if((temp=strchr(argv[2], '-'))!=NULL && strlen(temp)>1
		&& sscanf(&temp[1], "%d", &hiqual)==1 && hiqual>qual && hiqual>=1
		&& hiqual<=100) {}
	else hiqual=qual;

	if(argc>3)
	{
		for(i=3; i<argc; i++)
		{
			if(!stricmp(argv[i], "-strip")) dostrip=1;
			if(!stricmp(argv[i], "-forcesse2"))
			{
				printf("Using SSE2 code in Intel compressor\n");
				forcesse2=1;
			}
			if(!stricmp(argv[i], "-forcesse"))
			{
				printf("Using SSE code in Intel compressor\n");
				forcesse=1;
			}
			if(!stricmp(argv[i], "-forcemmx"))
			{
				printf("Using MMX code in Intel compressor\n");
				forcemmx=1;
			}
			if(!stricmp(argv[i], "-rgb")) pf=BMP_RGB;
			if(!stricmp(argv[i], "-rgba")) pf=BMP_RGBA;
			if(!stricmp(argv[i], "-bgr")) pf=BMP_BGR;
			if(!stricmp(argv[i], "-bgra")) pf=BMP_BGRA;
			if(!stricmp(argv[i], "-abgr")) pf=BMP_ABGR;
			if(!stricmp(argv[i], "-argb")) pf=BMP_ARGB;
			if(!stricmp(argv[i], "-bottomup")) bu=1;
			if(!stricmp(argv[i], "-quiet")) quiet=1;
		}
	}

	if(loadbmp(argv[1], &bmpbuf, &w, &h, pf, 1, bu)==-1)
	{
		printf("ERROR loading bitmap: %s\n", bmpgeterr());  exit(1);
	}

	temp=strrchr(argv[1], '.');
	if(temp!=NULL)
	{
		if(!stricmp(temp, ".ppm")) useppm=1;
		*temp='\0';
	}

	if(quiet)
	{
		printf("All performance values in Mpixels/sec\n\n");
		printf("Bitmap\tBitmap\tJPEG\tJPEG\tStrip\tCompr\tCompr\tDecomp\n");
		printf("Format\tOrder\tFormat\tQual\tSize\tPerf\tRatio\tPerf\n\n");
	}

	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, pf, bu, HPJ_411, i, argv[1], dostrip, useppm, quiet);
	if(quiet) printf("\n");
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, pf, bu, HPJ_422, i, argv[1], dostrip, useppm, quiet);
	if(quiet) printf("\n");
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, pf, bu, HPJ_444, i, argv[1], dostrip, useppm, quiet);

	if(bmpbuf) free(bmpbuf);
	return 0;
}
