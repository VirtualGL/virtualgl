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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vglutil.h"
#include "bmp.h"


unsigned char redMap[256], greenMap[256], blueMap[256];


static void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s <image 1> <image 2> [-mag]\n", argv[0]);
	fprintf(stderr, "       (images must be in BMP or PPM format)\n\n");
	fprintf(stderr, "-mag = Show magnitude of differences using an artificial color scale\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	unsigned char *img1 = NULL, *img2 = NULL, *errImg = NULL;
	int usePPM = 0;
	unsigned char err, pixelErr, maxCompErr[4] = { 0, 0, 0, 0 },
		minCompErr[4] = { 255, 255, 255, 255 }, maxTotalErr, minTotalErr;
	double avgCompErr[4] = { 0., 0., 0., 0. }, avgTotalErr = 0.,
		compSumSquares[4] = { 0., 0., 0., 0. }, totalSumSquares = 0.,
		rms[4] = { 0., 0., 0., 0. }, totalRMS;
	int width1, height1, ps1 = 3, width2, height2, ps2 = 3, width, height, i, j,
		k, mag = 0;
	char *temp;

	if(argc < 3) usage(argv);
	if(argc > 3) for(i = 3; i < argc; i++)
	{
		if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
		else if(!stricmp(argv[i], "-mag")) mag = 1;
		else usage(argv);
	}

	if((temp = strrchr(argv[1], '.')) != NULL && !stricmp(temp, ".ppm"))
		usePPM = 1;

	if(mag)
	{
		for(i = 0; i < 64; i++)
		{
			redMap[i] = 0;  greenMap[i] = i * 4;  blueMap[i] = 0;
		}
		for(i = 0; i < 64; i++)
		{
			redMap[i + 64] = i * 4;  greenMap[i + 64] = 255;  blueMap[i + 64] = 0;
		}
		for(i = 0; i < 64; i++)
		{
			redMap[i + 128] = 255;  greenMap[i + 128] = 255 - i * 4;
			blueMap[i + 128] = 0;
		}
		for(i = 0; i < 64; i++)
		{
			redMap[i + 192] = 255;  greenMap[i + 192] = i * 4;
			blueMap[i + 192] = i * 4;
		}
	}

	if(bmp_load(argv[1], &img1, &width1, 1, &height1, PF_BGR,
		BMPORN_TOPDOWN) == -1)
	{
		puts(bmp_geterr());  exit(1);
	}
	if(bmp_load(argv[2], &img2, &width2, 1, &height2, PF_BGR,
		BMPORN_TOPDOWN) == -1)
	{
		puts(bmp_geterr());  exit(1);
	}
	width = min(width1, width2);  height = min(height1, height2);

	if((errImg = (unsigned char *)malloc(width * height * ps1)) == NULL)
	{
		puts("Could not allocate memory");  exit(1);
	}

	for(j = 0; j < height; j++)
		for(i = 0; i < width; i++)
		{
			pixelErr = 0;
			for(k = 0; k < ps1; k++)
			{
				err = (unsigned char)abs((int)img2[(width2 * j + i) * ps2 + k] -
					(int)img1[(width1 * j + i) * ps1 + k]);
				if(err > pixelErr) pixelErr = err;
				if(err > maxCompErr[k]) maxCompErr[k] = err;
				if(err < minCompErr[k]) minCompErr[k] = err;
				avgCompErr[k] += err;  compSumSquares[k] += err * err;
				if(!mag) errImg[(width * j + i) * ps1 + k] = err;
			}
			if(mag)
			{
				errImg[(width * j + i) * ps1] = blueMap[pixelErr];
				errImg[(width * j + i) * ps1 + 1] = greenMap[pixelErr];
				errImg[(width * j + i) * ps1 + 2] = redMap[pixelErr];
			}
		}
	maxTotalErr = maxCompErr[0];  minTotalErr = minCompErr[0];
	for(k = 0; k < ps1; k++)
	{
		if(minCompErr[k] < maxTotalErr) minTotalErr = minCompErr[k];
		if(maxCompErr[k] > maxTotalErr) maxTotalErr = maxCompErr[k];
		avgTotalErr += avgCompErr[k];
		avgCompErr[k] /= ((double)height * (double)width);
		totalSumSquares += compSumSquares[k];
		compSumSquares[k] /= ((double)height * (double)width);
		rms[k] = sqrt(compSumSquares[k]);
	}
	avgTotalErr /= ((double)height * (double)width * (double)ps1);
	totalSumSquares /= ((double)height * (double)width * (double)ps1);
	totalRMS = sqrt(totalSumSquares);

	if(bmp_save(usePPM ? "diff.ppm" : "diff.bmp", errImg, width, 0, height,
		PF_BGR, BMPORN_TOPDOWN) == -1)
	{
		puts(bmp_geterr());  exit(1);
	}
	free(errImg);

	for(k = 0; k < ps1; k++)
		printf("%s: min err.= %d max err.= %d avg err.= %f rms= %f PSNR= %f\n",
			(k == 0 ? "B" : (k == 1 ? "G" : (k == 2 ? "R" : "A"))), minCompErr[k],
			maxCompErr[k], avgCompErr[k], rms[k], 20. * log10(255. / rms[k]));
	printf("T: min err.= %d max err.= %d avg err.= %f rms= %f PSNR= %f\n",
		minTotalErr, maxTotalErr, avgTotalErr, totalRMS,
		20. * log10(255. / totalRMS));

	return 0;
}
