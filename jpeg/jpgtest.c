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
#include "bmp.h"
#include "hputil.h"
#include "hpjpeg.h"

#define _catch(f) {if((f)==-1) {printf("Error in %s:\n%s\n", #f, hpjGetErrorStr());  exit(1);}}

int forcemmx=0, forcesse=0, forcesse2=0;

void dotest(unsigned char *srcbuf, int w, int h, int ps, int jpegsub, int qual,
	char *descr, char *basefilename, int dostrip)
{
	char tempstr[1024];  const char *error=NULL;
	FILE *outfile;  hpjhandle hnd;
	unsigned char *jpegbuf, *rgbbuf;
	double start, elapsed;
	int jpgbufsize=0, i, j, striph, ITER=5, temph;
	unsigned long *compstripsize; 
	static int testnum=1;
	int pitch=((w*ps)+3)&(~3);

	if((jpegbuf=(unsigned char *)malloc(w*h*3)) == NULL
	|| (rgbbuf=(unsigned char *)malloc(pitch*h)) == NULL
	|| (compstripsize=(unsigned long *)malloc(sizeof(unsigned long)*h)) == NULL)
	{
		puts("ERROR: Could not allocate image buffers.");
		exit(1);
	}

	printf("\n>>>>>  %s  <<<<<\n", descr);
	if(dostrip) striph=8;  else striph=h;
	do
	{
		striph*=2;  if(striph>h) striph=h;
		memcpy(rgbbuf, srcbuf, pitch*h);
		if((hnd=hpjInitCompress())==NULL)
			{printf("Error in hpjInitCompress():\n%s\n", hpjGetErrorStr());  exit(1);}
		_catch(hpjCompress(hnd, rgbbuf, w, HPJPAD(w*ps), striph, ps,
			jpegbuf, &compstripsize[0], jpegsub, qual, HPJ_BGR
			|(forcemmx?HPJ_FORCEMMX:0)|(forcesse?HPJ_FORCESSE:0)|(forcesse2?HPJ_FORCESSE2:0)));
		ITER=5;
		do
		{
			ITER*=2;
			start=hptime();
			for(i=0; i<ITER; i++)
			{
				jpgbufsize=0;
				j=0;
				do
				{
					if(h-j>striph && h-j<striph+HPJ_MINHEIGHT) temph=h-j;  else temph=min(striph, h-j);
					_catch(hpjCompress(hnd, &rgbbuf[pitch*j], w, HPJPAD(w*ps), temph, ps,
						&jpegbuf[w*3*j], &compstripsize[j], jpegsub, qual, HPJ_BGR
						|(forcemmx?HPJ_FORCEMMX:0)|(forcesse?HPJ_FORCESSE:0)|(forcesse2?HPJ_FORCESSE2:0)));
					jpgbufsize+=compstripsize[j];
					j+=temph;
				} while(j<h);
			}
			elapsed=hptime()-start;
		} while(elapsed<2.);
		_catch(hpjDestroy(hnd));
		if(striph==h) printf("\nFull image\n");  else printf("\nStrip size: %d x %d\n", w, striph);
		printf("C--> Frame rate:           %f fps\n", (double)ITER/elapsed);
		printf("     Output image size:    %d bytes\n", jpgbufsize);
		printf("     Compression ratio:    %f:1\n", (double)(w*h*ps)/(double)jpgbufsize);
		printf("     Source throughput:    %f Megapixels/sec\n", (double)(w*h)/1000000.*(double)ITER/elapsed);
		printf("     Output bit stream:    %f Megabits/sec\n", (double)jpgbufsize*8./1000000.*(double)ITER/elapsed);
		if(striph==h)
		{
			sprintf(tempstr, "%s_%d.jpg", basefilename, testnum);
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
			printf("Reference image written to %s\n", tempstr);
		}
		if((hnd=hpjInitDecompress())==NULL)
			{printf("Error in hpjInitDecompress():\n%s\n", hpjGetErrorStr());  exit(1);}
		_catch(hpjDecompress(hnd, jpegbuf, jpgbufsize, rgbbuf, w, HPJPAD(w*ps), striph, ps, HPJ_BGR
			|(forcemmx?HPJ_FORCEMMX:0)|(forcesse?HPJ_FORCESSE:0)|(forcesse2?HPJ_FORCESSE2:0)));
		ITER=5;
		do
		{
			ITER*=2;
			start=hptime();
			for(i=0; i<ITER; i++)
			{
				j=0;
				do
				{
					if(h-j>striph && h-j<striph+HPJ_MINHEIGHT) temph=h-j;  else temph=min(striph, h-j);
					_catch(hpjDecompress(hnd, &jpegbuf[w*3*j], compstripsize[j],
						&rgbbuf[pitch*j], w, HPJPAD(w*ps), temph, ps, HPJ_BGR
						|(forcemmx?HPJ_FORCEMMX:0)|(forcesse?HPJ_FORCESSE:0)|(forcesse2?HPJ_FORCESSE2:0)));
					j+=temph;
				} while(j<h);
			}
			elapsed=hptime()-start;
		} while(elapsed<2.);
		_catch(hpjDestroy(hnd));
		printf("D--> Frame rate:           %f fps\n", (double)ITER/elapsed);
		printf("     Dest. throughput:     %f Megapixels/sec\n", (double)(w*h)/1000000.*(double)ITER/elapsed);
		if(striph==h) sprintf(tempstr, "%s_%d_full.bmp", basefilename, testnum);
		else sprintf(tempstr, "%s_%d_%d.bmp", basefilename, testnum, striph);
		if((error=savebmp(tempstr, rgbbuf, w, h, ps, 1))!=NULL)
		{
			printf("ERROR saving bitmap: %s\n", error);  exit(1);
			free(jpegbuf);  free(rgbbuf);  free(compstripsize);
		}
		sprintf(strrchr(tempstr, '.'), "-err.bmp");
		printf("Computing compression error and saving to %s.\n", tempstr);
		for(j=0; j<h; j++) for(i=0; i<pitch; i++)
			rgbbuf[pitch*j+i]=abs(rgbbuf[pitch*j+i]-srcbuf[pitch*j+i]);
		if((error=savebmp(tempstr, rgbbuf, w, h, ps, 1))!=NULL)
		{
			printf("ERROR saving bitmap: %s\n", error);  exit(1);
			free(jpegbuf);  free(rgbbuf);  free(compstripsize);
		}
	} while(striph<h);
	testnum++;

	free(jpegbuf);  free(rgbbuf);  free(compstripsize);
}


int main(int argc, char *argv[])
{
	unsigned char *bmpbuf=NULL;  int w, h, psize, i;
	int qual, dostrip=0;
	const char *error=NULL;  char *temp;

	printf("\n");

	if(argc<3)
	{
		printf("USAGE: %s <Inputfile (BMP)> <%% Quality> [-strip]", argv[0]);
		printf("\n       [-forcemmx] [-forcesse] [-forcesse2]\n");
		exit(1);
	}
	if((qual=atoi(argv[2]))<1 || qual>100)
	{
		puts("ERROR: Quality must be between 1 and 100.");
		exit(1);
	}
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
		}
	}
	hptimer_init();

	if((error=loadbmp(argv[1], &bmpbuf, &w, &h, &psize, 1))!=NULL)
	{
		printf("ERROR loading bitmap: %s\n", error);  exit(1);
	}
	temp=strrchr(argv[1], '.');
	if(temp!=NULL) *temp='\0';

	dotest(bmpbuf, w, h, psize, HPJ_411, qual, "RGB <-> YUV 4:1:1", argv[1], dostrip);
	dotest(bmpbuf, w, h, psize, HPJ_422, qual, "RGB <-> YUV 4:2:2", argv[1], dostrip);
	dotest(bmpbuf, w, h, psize, HPJ_444, qual, "RGB <-> YUV 4:4:4", argv[1], dostrip);

	if(bmpbuf) free(bmpbuf);
	return 0;
}
