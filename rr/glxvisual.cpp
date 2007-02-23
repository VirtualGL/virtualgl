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

#include "glxvisual.h"
#include <stdio.h>
#include <stdlib.h>

// This class attempts to manage visual properties across Windows and Pbuffers
// and across GLX 1.0 and 1.3 in a sane manner

#include "rrerror.h"
#include "fakerconfig.h"
#include "rrmutex.h"
#include "faker-sym.h"

extern Display *_localdpy;
#ifdef USEGLP
extern GLPDevice _localdev;
#endif
extern FakerConfig fconfig;

struct _visattrib
{
	double gamma;
	VisualID visualid;
	int depth, c_class;
	int level, stereo, db, gl, trans;
	int tindex, tr, tg, tb, ta;
};

static Display *_vadpy=NULL;
static int _vascreen=-1, _vaentries=0;
static rrcs _vamutex;
static struct _visattrib *_va;

#ifdef sparc
#include <X11/Xmu/XmuSolaris.h>
#else
#include "gamma.c"
#endif

static void buildVisAttribTable(Display *dpy, int screen)
{
	int clientglx=0, maj_opcode=-1, first_event=-1, first_error=-1, nv=0;
	XVisualInfo *visuals=NULL, vtemp;
	Atom atom=0;
	int len=10000;

	try {

	rrcs::safelock l(_vamutex);

	if(dpy==_vadpy && screen==_vascreen) return;
	if(_XQueryExtension(dpy, "GLX", &maj_opcode, &first_event, &first_error)
		&& maj_opcode>=0 && first_event>=0 && first_error>=0)
		clientglx=1;
	if(!(visuals=XGetVisualInfo(dpy, VisualNoMask, &vtemp, &nv)) || nv==0)
		_throw("No visuals found on display");

	if(_va) {delete [] _va;  _va=NULL;}
	if(!(_va=new struct _visattrib[nv])) _throw("Memory allocation failure");
	_vaentries=nv;
	memset(_va, 0, sizeof(struct _visattrib)*nv);

	for(int i=0; i<nv; i++)
	{
		_va[i].visualid=visuals[i].visualid;
		_va[i].depth=visuals[i].depth;
		_va[i].c_class=visuals[i].c_class;
	}

	if((atom=XInternAtom(dpy, "SERVER_OVERLAY_VISUALS", True))!=None)
	{
		struct overlay_info
		{
			unsigned long visualid;
			long transtype, transpixel, level;
		} *olprop=NULL;
		unsigned char *prop=NULL;
		unsigned long nop=0, bytesleft=0;
		int actualformat=0;
		Atom actualtype=0;

		do
		{
			nop=0;  actualformat=0;  actualtype=0;
			if(XGetWindowProperty(dpy, RootWindow(dpy, screen), atom, 0, len, False,
				atom, &actualtype, &actualformat, &nop, &bytesleft,
				(unsigned char **)&olprop)!=Success || nop<4 || actualformat!=32
				|| actualtype!=atom) goto done;
			len+=(bytesleft+3)/4;
			if(bytesleft && prop) {XFree(prop);  prop=NULL;}
		} while(bytesleft);
		
		for(unsigned long i=0; i<nop/4; i++)
		{
			for(int j=0; j<nv; j++)
			{
				if(olprop[i].visualid==_va[j].visualid)
				{
					_va[j].trans=1;
					if(olprop[i].transtype==1) // Transparent pixel
						_va[j].tindex=olprop[i].transpixel;
					else if(olprop[i].transtype==2) // Transparent mask
					{
						// Is this right??
						_va[j].tr=olprop[i].transpixel&0xFF;
						_va[j].tg=olprop[i].transpixel&0x00FF;
						_va[j].tb=olprop[i].transpixel&0x0000FF;
						_va[j].ta=olprop[i].transpixel&0x000000FF;
					}
					_va[j].level=olprop[i].level;
				}
			}
		}

		done:
		if(prop) {XFree(prop);  prop=NULL;}
	}

	for(int i=0; i<nv; i++)
	{
		XSolarisGetVisualGamma(dpy, screen, visuals[i].visual, &_va[i].gamma);
		if(clientglx)
		{
			_glXGetConfig(dpy, &visuals[i], GLX_DOUBLEBUFFER, &_va[i].db);
			_glXGetConfig(dpy, &visuals[i], GLX_USE_GL, &_va[i].gl);
			_glXGetConfig(dpy, &visuals[i], GLX_STEREO, &_va[i].stereo);
		}
	}

	_vadpy=dpy;  _vascreen=screen;

	} catch(...) {
		if(visuals) XFree(visuals);  if(_va) {delete [] _va;  _va=NULL;}
		throw;
	}
}

GLXFBConfig *__vglConfigsFromVisAttribs(const int attribs[],
	int &depth, int &c_class, int &level, int &stereo, int &trans,
	int &nelements, bool glx13)
{
	int glxattribs[257], j=0;
	int doublebuffer=0, buffersize=-1, redsize=-1, greensize=-1,
		bluesize=-1;

	depth=glx13? 24:8;  c_class=glx13? TrueColor:PseudoColor;

	for(int i=0; attribs[i]!=None && i<=254; i++)
	{
		if(attribs[i]==GLX_DOUBLEBUFFER)
		{
			if(glx13) {doublebuffer=attribs[i+1];  i++;}
			else doublebuffer=1;
		}
		else if(attribs[i]==GLX_RGBA)
		{
			depth=24;  c_class=TrueColor;
		}
		else if(attribs[i]==GLX_RENDER_TYPE)
		{
			if(attribs[i+1]&GLX_COLOR_INDEX_BIT)
				{depth=8;  c_class=PseudoColor;}
			i++;
		}
		else if(attribs[i]==GLX_BUFFER_SIZE)
		{
			buffersize=attribs[i+1];  i++;
		}
		else if(attribs[i]==GLX_LEVEL)
		{
			level=attribs[i+1];  i++;
		}
		else if(attribs[i]==GLX_STEREO)
		{
			if(glx13) {stereo=attribs[i+1];  i++;}
			else stereo=1;
		}
		else if(attribs[i]==GLX_RED_SIZE)
		{
			redsize=attribs[i+1];  i++;
		}
		else if(attribs[i]==GLX_GREEN_SIZE)
		{
			greensize=attribs[i+1];  i++;
		}
		else if(attribs[i]==GLX_BLUE_SIZE)
		{
			bluesize=attribs[i+1];  i++;
		}
		else if(attribs[i]==GLX_TRANSPARENT_TYPE)
		{
			if(attribs[i+1]==GLX_TRANSPARENT_INDEX
				|| attribs[i+1]==GLX_TRANSPARENT_RGB)
				trans=1;
			i++;
		}
		else if(attribs[i]==GLX_DRAWABLE_TYPE) i++;
		else if(attribs[i]==GLX_X_VISUAL_TYPE) i++;
		else if(attribs[i]==GLX_VISUAL_ID) i++;
		else if(attribs[i]==GLX_X_RENDERABLE) i++;
		else if(attribs[i]==GLX_TRANSPARENT_INDEX_VALUE) i++;
		else if(attribs[i]==GLX_TRANSPARENT_RED_VALUE) i++;
		else if(attribs[i]==GLX_TRANSPARENT_GREEN_VALUE) i++;
		else if(attribs[i]==GLX_TRANSPARENT_BLUE_VALUE) i++;
		else if(attribs[i]==GLX_TRANSPARENT_ALPHA_VALUE) i++;
		else if(attribs[i]!=GLX_USE_GL)
		{
			glxattribs[j++]=attribs[i];  glxattribs[j++]=attribs[i+1];
			i++;
		}
	}
	glxattribs[j++]=GLX_DOUBLEBUFFER;  glxattribs[j++]=doublebuffer;
	glxattribs[j++]=GLX_RENDER_TYPE;  glxattribs[j++]=GLX_RGBA_BIT;
	if(buffersize>=0 && redsize<0 && greensize<0 && bluesize<0)
	{
		glxattribs[j++]=GLX_RED_SIZE;  glxattribs[j++]=buffersize;
		glxattribs[j++]=GLX_GREEN_SIZE;  glxattribs[j++]=buffersize;
		glxattribs[j++]=GLX_BLUE_SIZE;  glxattribs[j++]=buffersize;
	}
	else
	{
		if(redsize>=0) {glxattribs[j++]=GLX_RED_SIZE;  glxattribs[j++]=redsize;}
		if(greensize>=0) {glxattribs[j++]=GLX_GREEN_SIZE;  glxattribs[j++]=greensize;}
		if(bluesize>=0) {glxattribs[j++]=GLX_BLUE_SIZE;  glxattribs[j++]=bluesize;}
	}

	if(stereo)
	{
		glxattribs[j++]=GLX_STEREO;  glxattribs[j++]=stereo;
	}
	#ifdef USEGLP
	if(!fconfig.glp) {
	#endif
	if(!fconfig.usewindow)
	{
		glxattribs[j++]=GLX_DRAWABLE_TYPE;  glxattribs[j++]=GLX_PBUFFER_BIT;
	}
	#ifdef USEGLP
	}
	#endif
	glxattribs[j]=None;
	#ifdef USEGLP
	if(fconfig.glp)
		return glPChooseFBConfig(_localdev, glxattribs, &nelements);
	else
	#endif
		return _glXChooseFBConfig(_localdpy, DefaultScreen(_localdpy), glxattribs, &nelements);
}

int __vglClientVisualAttrib(Display *dpy, int screen, VisualID vid, int attribute)
{
	buildVisAttribTable(dpy, screen);
	for(int i=0; i<_vaentries; i++)
	{
		if(_va[i].visualid==vid)
		{
			if(attribute==GLX_LEVEL) return _va[i].level;
			if(attribute==GLX_TRANSPARENT_TYPE)
			{
				if(_va[i].trans)
				{
					if(_va[i].c_class==TrueColor) return GLX_TRANSPARENT_RGB;
					else return GLX_TRANSPARENT_INDEX;
				}
				else return GLX_NONE;
			}
			if(attribute==GLX_TRANSPARENT_INDEX_VALUE)
			{
				if(fconfig.transpixel>=0) return fconfig.transpixel;
				else return _va[i].tindex;
			}
			if(attribute==GLX_TRANSPARENT_RED_VALUE) return _va[i].tr;
			if(attribute==GLX_TRANSPARENT_GREEN_VALUE) return _va[i].tg;
			if(attribute==GLX_TRANSPARENT_BLUE_VALUE) return _va[i].tb;
			if(attribute==GLX_TRANSPARENT_ALPHA_VALUE) return _va[i].ta;
			if(attribute==GLX_STEREO)
			{
				return (_va[i].stereo && _va[i].gl && _va[i].db);
			}
		}
	}		
	return 0;
}

int __vglServerVisualAttrib(GLXFBConfig c, int attribute)
{
	int value=0;
	#ifdef USEGLP
	if(fconfig.glp)
		glPGetFBConfigAttrib(c, attribute, &value);
	else
	#endif
		_glXGetFBConfigAttrib(_localdpy, c, attribute, &value);
	return value;
}

int __vglVisualDepth(Display *dpy, int screen, VisualID vid)
{
	buildVisAttribTable(dpy, screen);
	for(int i=0; i<_vaentries; i++)
	{
		if(_va[i].visualid==vid) return _va[i].depth;
	}		
	return 24;
}

int __vglVisualClass(Display *dpy, int screen, VisualID vid)
{
	buildVisAttribTable(dpy, screen);
	for(int i=0; i<_vaentries; i++)
	{
		if(_va[i].visualid==vid) return _va[i].c_class;
	}		
	return TrueColor;
}

double __vglVisualGamma(Display *dpy, int screen, VisualID vid)
{
	buildVisAttribTable(dpy, screen);
	for(int i=0; i<_vaentries; i++)
	{
		if(_va[i].visualid==vid) return _va[i].gamma;
	}		
	return 2.22;
}

VisualID __vglMatchVisual(Display *dpy, int screen,
	int depth, int c_class, int level, int stereo, int trans)
{
	int i;
	if(!dpy) return 0;

	buildVisAttribTable(dpy, screen);

	// Try to find an exact match
	for(i=0; i<_vaentries; i++)
	{
		int match=1;
		if(_va[i].c_class!=c_class) match=0;
		if(_va[i].depth!=depth) match=0;
		if(fconfig.usesungamma && _va[i].gamma!=1.0) match=0;
		if(!fconfig.usesungamma && _va[i].gamma==1.0) match=0;
		if(stereo!=_va[i].stereo) match=0;
		if(stereo && !_va[i].db) match=0;
		if(stereo && !_va[i].gl) match=0;
		if(stereo && _va[i].c_class!=TrueColor) match=0;
		if(level!=_va[i].level) match=0;
		if(trans && !_va[i].trans) match=0;
		if(match) return _va[i].visualid;
	}

	// Try again, without gamma restriction
	for(i=0; i<_vaentries; i++)
	{
		int match=1;
		if(_va[i].c_class!=c_class) match=0;
		if(_va[i].depth!=depth) match=0;
		if(stereo!=_va[i].stereo) match=0;
		if(stereo && !_va[i].db) match=0;
		if(stereo && !_va[i].gl) match=0;
		if(stereo && _va[i].c_class!=TrueColor) match=0;
		if(level!=_va[i].level) match=0;
		if(trans && !_va[i].trans) match=0;
		if(match) return _va[i].visualid;
	}

	return 0;
}

XVisualInfo *__vglVisualFromVisualID(Display *dpy, VisualID vid)
{
	XVisualInfo vtemp;  int n=0;
	vtemp.visualid=vid;
	return XGetVisualInfo(dpy, VisualIDMask, &vtemp, &n);
}

int __vglConfigDepth(GLXFBConfig c)
{
	int depth, render_type, r, g, b;
	errifnot(c);
	render_type=__vglServerVisualAttrib(c, GLX_RENDER_TYPE);
	if(render_type==GLX_RGBA_BIT)
	{
		r=__vglServerVisualAttrib(c, GLX_RED_SIZE);
		g=__vglServerVisualAttrib(c, GLX_GREEN_SIZE);
		b=__vglServerVisualAttrib(c, GLX_BLUE_SIZE);
		depth=r+g+b;
		if(depth<8) depth=1;  // Monochrome
	}
	else
	{
		depth=__vglServerVisualAttrib(c, GLX_BUFFER_SIZE);
	}
	return depth;
}

int __vglConfigClass(GLXFBConfig c)
{
	int rendertype;
	errifnot(c);
	rendertype=__vglServerVisualAttrib(c, GLX_RENDER_TYPE);
	if(rendertype==GLX_RGBA_BIT) return TrueColor;
	else return PseudoColor;
}
