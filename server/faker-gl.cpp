/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011-2012, 2014 D. R. Commander
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

// Interposed OpenGL functions

#include <math.h>
#include "ContextHash.h"
#include "WindowHash.h"
#include "faker.h"

using namespace vglserver;


#define ROUND(f) ((f)>=0? (long)((f)+0.5) : (long)((f)-0.5))


static void doGLReadback(bool spoilLast, bool sync)
{
	VirtualWin *vw;
	GLXDrawable drawable;

	if(ctxhash.overlayCurrent()) return;
	drawable=_glXGetCurrentDrawable();
	if(!drawable) return;

	if(winhash.find(drawable, vw))
	{
		if(drawingToFront() || vw->dirty)
		{
				opentrace(doGLReadback);  prargx(vw->getGLXDrawable());
				prargi(sync);  prargi(spoilLast);  starttrace();

			vw->readback(GL_FRONT, spoilLast, sync);

				stoptrace();  closetrace();
		}
	}
}


extern "C" {

// VirtualGL reads back and sends the front buffer if something has been
// rendered to it since the last readback and one of the following functions is
// called to signal the end of a frame.

void glFinish(void)
{
	TRY();

		if(fconfig.trace) vglout.print("[VGL] glFinish()\n");

	_glFinish();
	fconfig.flushdelay=0.;
	doGLReadback(false, fconfig.sync);

	CATCH();
}


void glFlush(void)
{
	static double lastTime=-1.;  double thisTime;

	TRY();

		if(fconfig.trace) vglout.print("[VGL] glFlush()\n");

	_glFlush();
	if(lastTime<0.) lastTime=getTime();
	else
	{
		thisTime=getTime()-lastTime;
		if(thisTime-lastTime<0.01) fconfig.flushdelay=0.01;
		else fconfig.flushdelay=0.;
	}

	// See the notes regarding VGL_SPOILLAST and VGL_GLFLUSHTRIGGER in the
	// VirtualGL User's Guide.
	if(fconfig.glflushtrigger) doGLReadback(fconfig.spoillast, fconfig.sync);

	CATCH();
}


void glXWaitGL(void)
{
	TRY();

		if(fconfig.trace) vglout.print("[VGL] glXWaitGL()\n");

	if(ctxhash.overlayCurrent()) { _glXWaitGL();  return; }

	_glFinish();  // glXWaitGL() on some systems calls glFinish(), so we do this
	              // to avoid 2 readbacks
	fconfig.flushdelay=0.;
	doGLReadback(false, fconfig.sync);

	CATCH();
}


// If the application is rendering to the front buffer and switches the draw
// buffer before calling glFlush()/glFinish()/glXWaitGL(), we set a lazy
// readback trigger to indicate that the front buffer needs to be read back
// upon the next call to glFlush()/glFinish()/glXWaitGL().

void glDrawBuffer(GLenum mode)
{
	TRY();

	if(ctxhash.overlayCurrent()) { _glDrawBuffer(mode);  return; }

		opentrace(glDrawBuffer);  prargx(mode);  starttrace();

	VirtualWin *vw=NULL;  int before=-1, after=-1, rbefore=-1, rafter=-1;
	GLXDrawable drawable=_glXGetCurrentDrawable();

	if(drawable && winhash.find(drawable, vw))
	{
		before=drawingToFront();
		rbefore=drawingToRight();
		_glDrawBuffer(mode);
		after=drawingToFront();
		rafter=drawingToRight();
		if(before && !after) vw->dirty=true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty=true;
	}
	else _glDrawBuffer(mode);

		stoptrace();  if(drawable && vw) {
			prargi(vw->dirty);  prargi(vw->rdirty);  prargx(vw->getGLXDrawable());
		}  closetrace();

	CATCH();
}


// glPopAttrib() can change the draw buffer state as well :|

void glPopAttrib(void)
{
	TRY();

	if(ctxhash.overlayCurrent()) { _glPopAttrib();  return; }

		opentrace(glPopAttrib);  starttrace();

	VirtualWin *vw=NULL;  int before=-1, after=-1, rbefore=-1, rafter=-1;
	GLXDrawable drawable=_glXGetCurrentDrawable();

	if(drawable && winhash.find(drawable, vw))
	{
		before=drawingToFront();
		rbefore=drawingToRight();
		_glPopAttrib();
		after=drawingToFront();
		rafter=drawingToRight();
		if(before && !after) vw->dirty=true;
		if(rbefore && !rafter && vw->isStereo()) vw->rdirty=true;
	}
	else _glPopAttrib();

		stoptrace();  if(drawable && vw) {
			prargi(vw->dirty);  prargi(vw->rdirty);  prargx(vw->getGLXDrawable());
		}  closetrace();

	CATCH();
}


// Sometimes XNextEvent() is called from a thread other than the
// rendering thread, so we wait until glViewport() is called and
// take that opportunity to resize the off-screen drawable.

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRY();

	if(ctxhash.overlayCurrent()) { _glViewport(x, y, width, height);  return; }

		opentrace(glViewport);  prargi(x);  prargi(y);  prargi(width);
		prargi(height);  starttrace();

	GLXContext ctx=glXGetCurrentContext();
	GLXDrawable draw=_glXGetCurrentDrawable();
	GLXDrawable read=_glXGetCurrentReadDrawable();
	Display *dpy=_glXGetCurrentDisplay();
	GLXDrawable newRead=0, newDraw=0;

	if(dpy && (draw || read) && ctx)
	{
		newRead=read, newDraw=draw;
		VirtualWin *drawVW=NULL, *readVW=NULL;
		winhash.find(draw, drawVW);
		winhash.find(read, readVW);
		if(drawVW) drawVW->checkResize();
		if(readVW && readVW!=drawVW) readVW->checkResize();
		if(drawVW) newDraw=drawVW->updateGLXDrawable();
		if(readVW) newRead=readVW->updateGLXDrawable();
		if(newRead!=read || newDraw!=draw)
		{
			_glXMakeContextCurrent(dpy, newDraw, newRead, ctx);
			if(drawVW) { drawVW->clear();  drawVW->cleanup(); }
			if(readVW) readVW->cleanup();
		}
	}
	_glViewport(x, y, width, height);

		stoptrace();  if(draw!=newDraw) { prargx(draw);  prargx(newDraw); }
		if(read!=newRead) { prargx(read);  prargx(newRead); }  closetrace();

	CATCH();
}


#define EMULATE_CI  \
	(ctxhash.colorIndexCurrent() && !ctxhash.overlayCurrent())


// The following functions are interposed in order to make color index
// rendering work, since most platforms don't support color index Pbuffers.

void glClearIndex(GLfloat c)
{
	if(EMULATE_CI) glClearColor(c/255., 0., 0., 0.);
	else _glClearIndex(c);
}


#define DRAW_PIXEL_CONVERT(ctype, glType, size)  \
	if(type==glType) {  \
		unsigned char *p=(unsigned char *)pixels;  \
		unsigned char *b=buf;  \
		int w=(rowlen>0? rowlen:width)*size;  \
		if(size<align) w=(w+align-1)&(~(align-1));  \
		for(int i=0; i<height; i++, p+=w, b+=width) {  \
			ctype *p2=(ctype *)p;  \
			for(int j=0; j<width; j++) b[j]=(unsigned char)p2[j];  \
		}}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
	const GLvoid *pixels)
{
	TRY();

	if(format==GL_COLOR_INDEX && !ctxhash.overlayCurrent() && type!=GL_BITMAP)
	{
		format=GL_RED;
		if(type==GL_BYTE || type==GL_UNSIGNED_BYTE) type=GL_UNSIGNED_BYTE;
		else
		{
			int rowlen=-1, align=-1;  GLubyte *buf=NULL;

			_glGetIntegerv(GL_PACK_ALIGNMENT, &align);
			_glGetIntegerv(GL_PACK_ROW_LENGTH, &rowlen);
			_newcheck(buf=new unsigned char[width*height])
			if(type==GL_SHORT) type=GL_UNSIGNED_SHORT;
			if(type==GL_INT) type=GL_UNSIGNED_INT;
			DRAW_PIXEL_CONVERT(unsigned short, GL_UNSIGNED_SHORT, 2)
			DRAW_PIXEL_CONVERT(unsigned int, GL_UNSIGNED_INT, 4)
			DRAW_PIXEL_CONVERT(float, GL_FLOAT, 4)
			glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 1);
			_glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, buf);
			glPopClientAttrib();
			delete [] buf;
			return;
		}
	}
	_glDrawPixels(width, height, format, type, pixels);

	CATCH();
}


void glGetDoublev(GLenum pname, GLdouble *params)
{
	if(EMULATE_CI)
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble color[4];
			_glGetDoublev(GL_CURRENT_COLOR, color);
			if(params) *params=(GLdouble)ROUND(color[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble color[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, color);
			if(params) *params=(GLdouble)ROUND(color[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			_glGetDoublev(GL_RED_SCALE, params);
			if(params) *params=(GLdouble)ROUND(log(*params)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			_glGetDoublev(GL_RED_BIAS, params);
			if(params) *params=(GLdouble)ROUND((*params)*255.);
			return;
		}
	}
	_glGetDoublev(pname, params);
}


void glGetFloatv(GLenum pname, GLfloat *params)
{
	if(EMULATE_CI)
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble color[4];
			_glGetDoublev(GL_CURRENT_COLOR, color);
			if(params) *params=(GLfloat)ROUND(color[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble color[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, color);
			if(params) *params=(GLfloat)ROUND(color[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			GLdouble scale;
			_glGetDoublev(GL_RED_SCALE, &scale);
			if(params) *params=(GLfloat)ROUND(log(scale)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			GLdouble bias;
			_glGetDoublev(GL_RED_BIAS, &bias);
			if(params) *params=(GLfloat)ROUND(bias*255.);
			return;
		}
	}
	_glGetFloatv(pname, params);
}


void glGetIntegerv(GLenum pname, GLint *params)
{
	if(EMULATE_CI)
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble color[4];
			_glGetDoublev(GL_CURRENT_COLOR, color);
			if(params) *params=(GLint)ROUND(color[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble color[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, color);
			if(params) *params=(GLint)ROUND(color[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			double scale;
			_glGetDoublev(GL_RED_SCALE, &scale);
			if(params) *params=(GLint)ROUND(log(scale)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			double bias;
			_glGetDoublev(GL_RED_BIAS, &bias);
			if(params) *params=(GLint)ROUND(bias*255.);
			return;
		}
	}
	_glGetIntegerv(pname, params);
}


void glIndexd(GLdouble index)
{
	if(EMULATE_CI) glColor3d(index/255., 0.0, 0.0);
	else _glIndexd(index);
}

void glIndexf(GLfloat index)
{
	if(EMULATE_CI) glColor3f(index/255., 0., 0.);
	else _glIndexf(index);
}

void glIndexi(GLint index)
{
	if(EMULATE_CI) glColor3f((GLfloat)index/255., 0, 0);
	else _glIndexi(index);
}

void glIndexs(GLshort index)
{
	if(EMULATE_CI) glColor3f((GLfloat)index/255., 0, 0);
	else _glIndexs(index);
}

void glIndexub(GLubyte index)
{
	if(EMULATE_CI) glColor3f((GLfloat)index/255., 0, 0);
	else _glIndexub(index);
}

void glIndexdv(const GLdouble *index)
{
	if(EMULATE_CI)
	{
		GLdouble color[3]={index? (*index)/255.:0., 0., 0.};
		glColor3dv(index? color:NULL);
	}
	else _glIndexdv(index);
}

void glIndexfv(const GLfloat *index)
{
	if(EMULATE_CI)
	{
		GLfloat color[3]={index? (*index)/255.0f:0.0f, 0.0f, 0.0f};
		glColor3fv(index? color:NULL);  return;
	}
	else _glIndexfv(index);
}

void glIndexiv(const GLint *index)
{
	if(EMULATE_CI)
	{
		GLfloat color[3]={index? (GLfloat)(*index)/255.0f:0.0f, 0.0f, 0.0f};
		glColor3fv(index? color:NULL);  return;
	}
	else _glIndexiv(index);
}

void glIndexsv(const GLshort *index)
{
	if(EMULATE_CI)
	{
		GLfloat color[3]={index? (GLfloat)(*index)/255.0f:0.0f, 0.0f, 0.0f};
		glColor3fv(index? color:NULL);  return;
	}
	else _glIndexsv(index);
}

void glIndexubv(const GLubyte *index)
{
	if(EMULATE_CI)
	{
		GLfloat color[3]={index? (GLfloat)(*index)/255.0f:0.0f, 0.0f, 0.0f};
		glColor3fv(index? color:NULL);  return;
	}
	else _glIndexubv(index);
}


void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	GLfloat mat[]={1., 1., 1., 1.};

	if(pname==GL_COLOR_INDEXES && params)
	{
		mat[0]=params[0]/255.;
		_glMaterialfv(face, GL_AMBIENT, mat);
		mat[0]=params[1]/255.;
		_glMaterialfv(face, GL_DIFFUSE, mat);
		mat[0]=params[2]/255.;
		_glMaterialfv(face, GL_SPECULAR, mat);
	}
	else _glMaterialfv(face, pname, params);
}


void glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
	GLfloat mat[]={1., 1., 1., 1.};

	if(pname==GL_COLOR_INDEXES && params)
	{
		mat[0]=(GLfloat)params[0]/255.;
		_glMaterialfv(face, GL_AMBIENT, mat);
		mat[0]=(GLfloat)params[1]/255.;
		_glMaterialfv(face, GL_DIFFUSE, mat);
		mat[0]=(GLfloat)params[2]/255.;
		_glMaterialfv(face, GL_SPECULAR, mat);
	}
	else _glMaterialiv(face, pname, params);
}


void glPixelTransferf(GLenum pname, GLfloat param)
{
	if(EMULATE_CI)
	{
		if(pname==GL_INDEX_SHIFT)
		{
			_glPixelTransferf(GL_RED_SCALE, pow(2., (double)param));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			_glPixelTransferf(GL_RED_BIAS, param/255.);
			return;
		}
	}
	_glPixelTransferf(pname, param);
}


void glPixelTransferi(GLenum pname, GLint param)
{
	if(EMULATE_CI)
	{
		if(pname==GL_INDEX_SHIFT)
		{
			_glPixelTransferf(GL_RED_SCALE, pow(2., (double)param));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			_glPixelTransferf(GL_RED_BIAS, (GLfloat)param/255.);
			return;
		}
	}
	_glPixelTransferi(pname, param);
}


#define READ_PIXEL_CONVERT(ctype, glType, size)  \
	if(type==glType) {  \
		unsigned char *p=(unsigned char *)pixels;  \
		unsigned char *b=buf;  \
		int w=(rowlen>0? rowlen:width)*size;  \
		if(size<align) w=(w+align-1)&(~(align-1));  \
		for(int i=0; i<height; i++, p+=w, b+=width) {  \
			ctype *p2=(ctype *)p;  \
			for(int j=0; j<width; j++) p2[j]=(ctype)b[j];  \
		}}


void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
	GLenum format, GLenum type, GLvoid *pixels)
{
	TRY();

	if(format==GL_COLOR_INDEX && !ctxhash.overlayCurrent() && type!=GL_BITMAP)
	{
		format=GL_RED;
		if(type==GL_BYTE || type==GL_UNSIGNED_BYTE) type=GL_UNSIGNED_BYTE;
		else
		{
			int rowlen=-1, align=-1;  GLubyte *buf=NULL;
			_glGetIntegerv(GL_PACK_ALIGNMENT, &align);
			_glGetIntegerv(GL_PACK_ROW_LENGTH, &rowlen);
			_newcheck(buf=new unsigned char[width*height])
			if(type==GL_SHORT) type=GL_UNSIGNED_SHORT;
			if(type==GL_INT) type=GL_UNSIGNED_INT;
			glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 1);
			_glReadPixels(x, y, width, height, format, GL_UNSIGNED_BYTE, buf);
			glPopClientAttrib();
			READ_PIXEL_CONVERT(unsigned short, GL_UNSIGNED_SHORT, 2)
			READ_PIXEL_CONVERT(unsigned int, GL_UNSIGNED_INT, 4)
			READ_PIXEL_CONVERT(float, GL_FLOAT, 4)
			delete [] buf;
			return;
		}
	}
	_glReadPixels(x, y, width, height, format, type, pixels);

	CATCH();
}


} // extern "C"
