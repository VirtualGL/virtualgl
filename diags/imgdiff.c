/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005 Sun Microsystems, Inc.
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
#include "rrutil.h"
#include "bmp.h"

#ifndef min
 #define min(a,b) ((a)<(b)?(a):(b))
#endif

unsigned char rct[256], gct[256], bct[256];

int main(int argc, char **argv)
{
	unsigned char *bmp1=NULL, *bmp2=NULL, *bmpdiff=NULL;  int useppm=0;
	unsigned char diff, diffmax, max[4]={0, 0, 0, 0}, min[4]={255, 255, 255, 255}, tmax, tmin;
	double avg[4]={0., 0., 0., 0.}, ssq[4]={0., 0., 0., 0.}, tavg=0., tssq=0.;
	int w1, h1, d1=3, w2, h2, d2=3, w, h, i, j, k, mag=0;
	char *temp;

	if(argc<3)
	{
		printf("USAGE: %s <bitmap 1> <bitmap 2> [-mag]\n", argv[0]);
		printf("\n-mag = show magnitude of differences using an artificial\n");
		printf("       color scale\n");
		exit(1);
	}
	if(argc>3 && !strcmp(argv[3], "-mag")) mag=1;

	if((temp=strrchr(argv[1], '.'))!=NULL && !stricmp(temp, ".ppm")) useppm=1;

	if(mag)
	{
		for(i=0; i<64; i++)
		{
			rct[i]=0;  gct[i]=i*4;  bct[i]=0;
		}
		for(i=0; i<64; i++)
		{
			rct[i+64]=i*4;  gct[i+64]=255;  bct[i+64]=0;
		}
		for(i=0; i<64; i++)
		{
			rct[i+128]=255;  gct[i+128]=255-i*4;  bct[i+128]=0;
		}
		for(i=0; i<64; i++)
		{
			rct[i+192]=255;  gct[i+192]=i*4;  bct[i+192]=i*4;
		}
	}

	if(loadbmp(argv[1], &bmp1, &w1, &h1, BMP_BGR, 1, 0)==-1)
	{
		puts(bmpgeterr());  exit(1);
	}
	if(loadbmp(argv[2], &bmp2, &w2, &h2, BMP_BGR, 1, 0)==-1)
	{
		puts(bmpgeterr());  exit(1);
	}
	w=min(w1, w2);  h=min(h1, h2);

	if((bmpdiff=(unsigned char *)malloc(w*h*d1))==NULL)
	{
		puts("Could not allocate memory");  exit(1);
	}

	for(i=0; i<h; i++)
		for(j=0; j<w; j++)
		{
			diffmax=0;
			for(k=0; k<d1; k++)
			{
				diff=(unsigned char)abs((int)bmp2[(w2*i+j)*d2+k]-(int)bmp1[(w1*i+j)*d1+k]);
				if(diff>diffmax) diffmax=diff;
				if(diff>max[k]) max[k]=diff;  if(diff<min[k]) min[k]=diff;
				avg[k]+=diff;  ssq[k]+=diff*diff;
				if(!mag) bmpdiff[(w*i+j)*d1+k]=diff;
			}
			if(mag)
			{
				bmpdiff[(w*i+j)*d1]=bct[diffmax];
				bmpdiff[(w*i+j)*d1+1]=gct[diffmax];
				bmpdiff[(w*i+j)*d1+2]=rct[diffmax];
			}
		}
	tmax=max[0];  tmin=min[0];
	for(k=0; k<d1; k++)
	{
		if(min[k]<tmin) tmin=min[k];
		if(max[k]>tmax) tmax=max[k];
		tavg+=avg[k];
		avg[k]/=((double)h*(double)w);
		tssq+=ssq[k];
		ssq[k]/=((double)h*(double)w);
		ssq[k]=sqrt(ssq[k]);
	}
	tavg/=((double)h*(double)w*(double)d1);
	tssq/=((double)h*(double)w*(double)d1);
	tssq=sqrt(tssq);

	if(savebmp(useppm?"diff.ppm":"diff.bmp", bmpdiff, w, h, BMP_BGR, 0, 0)==-1)
	{
		puts(bmpgeterr());  exit(1);
	}
	free(bmpdiff);
	for(k=0; k<d1; k++)
		printf("%s: min err.= %d max err.= %d avg err.= %f rms= %f PSNR= %f\n",
		k==0?"B":(k==1?"G":(k==2?"R":"A")), min[k], max[k], avg[k], ssq[k], 20.*log10(255./ssq[k]));
	printf("T: min err.= %d max err.= %d avg err.= %f rms= %f PSNR= %f\n",
		tmin, tmax, tavg, tssq, 20.*log10(255./tssq));
	return 0;
}
