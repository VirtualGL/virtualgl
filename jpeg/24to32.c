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
#include "bmp.h"

int main(int argc, char **argv)
{
	unsigned char *inbuf, *outbuf;  const char *error=NULL;
	int w, h, d, i, j;
	if(argc<3)
	{
		printf("USAGE: %s <input filename> <output filename>\n", argv[0]);
		exit(1);
	}
	if((error=loadbmp(argv[1], &inbuf, &w, &h, &d, 0))!=NULL)
	{
		printf("ERROR loading bitmap: %s\n", error);  exit(1);
	}
	else if(d==4)
	{
		printf("Bitmap is already 32-bit\n");  exit(0);
	}
	if((outbuf=(unsigned char *)malloc(w*4*h))==NULL)
	{
		printf("malloc() error.\n");  exit(1);
	}
	for(i=0; i<h; i++)
	{
		for(j=0; j<w; j++)
		{
			outbuf[(i*w+j)*4]=inbuf[(i*w+j)*3];
			outbuf[(i*w+j)*4+1]=inbuf[(i*w+j)*3+1];
			outbuf[(i*w+j)*4+2]=inbuf[(i*w+j)*3+2];
		}
	}
	if((error=savebmp(argv[2], outbuf, w, h, 4, 0))!=NULL)
	{
		printf("ERROR saving bitmap: %s\n", error);  exit(1);
	}
	free(outbuf);  free(inbuf);
	return 0;
}
