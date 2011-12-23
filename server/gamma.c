/* Copyright (C)2005 Sun Microsystems, Inc.
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>


#define DEFAULT_GAMMA 2.22


static unsigned short nextitem(unsigned char **ptr, unsigned long *n)
{
	unsigned short value=*(unsigned short *)(*ptr);
	(*ptr)+=2;  (*n)--;  return value;
}


/*
   This emulates the XSolarisGetVisualGamma() function so that a non-Solaris
   X client can select a corrected visual on a Solaris X server.

   Information about the XDCCC_LINEAR_RGB_CORRECTION property was obtained
   from the Sun X Server Device Developer's Guide
   http://docs.sun.com/app/docs/doc/802-5378/6i951dj4r?a=view
   and from the X11 Inter-Client Communication Conventions Manual:
   http://tronche.com/gui/x/icccm/sec-7.html

   Other hints were obtained from xcmsdb.c, part of the X.org source tree.
*/

Status _XSolarisGetVisualGamma(Display *dpy, int screen, Visual *visual,
	double *gamma)
{
	Atom atom=0, actualtype=0;
	long len=10000;
	unsigned long n=0, bytesleft=0;
	int actualformat=0;
	unsigned char *prop=NULL, *ptr=NULL;
	VisualID id=0;
	int type, tablelen, count, j, k;
	unsigned short e1, e2;

	if(!dpy || !visual || !gamma) return BadMatch;
	*gamma=DEFAULT_GAMMA;

	if(!(atom=XInternAtom(dpy, "XDCCC_LINEAR_RGB_CORRECTION", True)))
		goto done;

	do
	{
		n=0;  actualformat=0;  actualtype=0;
		XGetWindowProperty(dpy, RootWindow(dpy, screen), atom, 0, len, False,
			XA_INTEGER, &actualtype, &actualformat, &n, &bytesleft, &prop);
		if(n<1 || actualformat!=16 || actualtype!=XA_INTEGER) goto done;
		len+=(bytesleft+3)/4;
		if(bytesleft && prop) {XFree(prop);  prop=NULL;}
	} while(bytesleft);

	ptr=prop;
	while(n>0)
	{
		if(n<7) goto done;
			/* 2 for visual ID + 1 for type + 1 for count + 3 for minimum 1-entry table */
		id=nextitem(&ptr, &n)<<16 | nextitem(&ptr, &n);
		type=nextitem(&ptr, &n);
		if(id==visual->visualid && type!=0) goto done;
			/* Ignore Type 1 tables for now */
		count=nextitem(&ptr, &n);
		if(count) for(j=0; j<count; j++)
		{
			tablelen=nextitem(&ptr, &n)+1;
			if(id==visual->visualid && tablelen!=2) goto done;
			if(tablelen) for(k=0; k<tablelen; k++)
			{
				e1=nextitem(&ptr, &n);  e2=nextitem(&ptr, &n);
				if(id==visual->visualid)
				{
					if(k==0 && (e1!=0 || e2!=0)) goto done;
					if(k==1)
					{
						if(e1!=0xffff || e2!=0xffff) goto done;
						else
						{
							*gamma=1.0;  goto done;
						}
					}
				}
			}
		}
	}			

	done:
	if(prop) {XFree(prop);  prop=NULL;}
	return Success;
}
