/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011-2012 D. R. Commander
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


#define __round(f) ((f)>=0?(long)((f)+0.5):(long)((f)-0.5))


static void _doGLreadback(bool spoillast, bool sync)
{
	pbwin *pbw;
	GLXDrawable drawable;
	if(ctxh.overlaycurrent()) return;
	drawable=_glXGetCurrentDrawable();
	if(!drawable) return;
	if(winh.findpb(drawable, pbw))
	{
		if(_drawingtofront() || pbw->_dirty)
		{
				opentrace(_doGLreadback);  prargx(pbw->getglxdrawable());
				prargi(sync); prargi(spoillast);  starttrace();

			pbw->readback(GL_FRONT, spoillast, sync);

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

		if(fconfig.trace) rrout.print("[VGL] glFinish()\n");

	_glFinish();
	fconfig.flushdelay=0.;
	_doGLreadback(false, fconfig.sync);
	CATCH();
}


void glFlush(void)
{
	static double lasttime=-1.;  double thistime;
	TRY();

		if(fconfig.trace) rrout.print("[VGL] glFlush()\n");

	_glFlush();
	if(lasttime<0.) lasttime=rrtime();
	else
	{
		thistime=rrtime()-lasttime;
		if(thistime-lasttime<0.01) fconfig.flushdelay=0.01;
		else fconfig.flushdelay=0.;
	}
	// See the notes regarding VGL_SPOILLAST and VGL_GLFLUSHTRIGGER in the
	// VirtualGL User's Guide.
	if(fconfig.glflushtrigger) _doGLreadback(fconfig.spoillast, false);
	CATCH();
}


void glXWaitGL(void)
{
	TRY();

		if(fconfig.trace) rrout.print("[VGL] glXWaitGL()\n");

	if(ctxh.overlaycurrent()) {_glXWaitGL();  return;}

	_glFinish();  // glXWaitGL() on some systems calls glFinish(), so we do this
	              // to avoid 2 readbacks
	fconfig.flushdelay=0.;
	_doGLreadback(false, fconfig.sync);
	CATCH();
}


// If the application is rendering to the front buffer and switches the draw
// buffer before calling glFlush()/glFinish()/glXWaitGL(), we set a lazy
// readback trigger to indicate that the front buffer needs to be read back
// upon the next call to glFlush()/glFinish()/glXWaitGL().

void glDrawBuffer(GLenum mode)
{
	TRY();

	if(ctxh.overlaycurrent()) {_glDrawBuffer(mode);  return;}

		opentrace(glDrawBuffer);  prargx(mode);  starttrace();

	pbwin *pbw=NULL;  int before=-1, after=-1, rbefore=-1, rafter=-1;
	GLXDrawable drawable=_glXGetCurrentDrawable();
	if(drawable && winh.findpb(drawable, pbw))
	{
		before=_drawingtofront();
		rbefore=_drawingtoright();
		_glDrawBuffer(mode);
		after=_drawingtofront();
		rafter=_drawingtoright();
		if(before && !after) pbw->_dirty=true;
		if(rbefore && !rafter && pbw->stereo()) pbw->_rdirty=true;
	}
	else _glDrawBuffer(mode);

		stoptrace();  if(drawable && pbw) {prargi(pbw->_dirty);
		prargi(pbw->_rdirty);  prargx(pbw->getglxdrawable());}  closetrace();

	CATCH();
}


// glPopAttrib() can change the draw buffer state as well :|

void glPopAttrib(void)
{
	TRY();

	if(ctxh.overlaycurrent()) {_glPopAttrib();  return;}

		opentrace(glPopAttrib);  starttrace();

	pbwin *pbw=NULL;  int before=-1, after=-1, rbefore=-1, rafter=-1;
	GLXDrawable drawable=_glXGetCurrentDrawable();
	if(drawable && winh.findpb(drawable, pbw))
	{
		before=_drawingtofront();
		rbefore=_drawingtoright();
		_glPopAttrib();
		after=_drawingtofront();
		rafter=_drawingtoright();
		if(before && !after) pbw->_dirty=true;
		if(rbefore && !rafter && pbw->stereo()) pbw->_rdirty=true;
	}
	else _glPopAttrib();

		stoptrace();  if(drawable && pbw) {prargi(pbw->_dirty);
		prargi(pbw->_rdirty);  prargx(pbw->getglxdrawable());}  closetrace();

	CATCH();
}


// Sometimes XNextEvent() is called from a thread other than the
// rendering thread, so we wait until glViewport() is called and
// take that opportunity to resize the Pbuffer.

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRY();

	if(ctxh.overlaycurrent()) {_glViewport(x, y, width, height);  return;}

		opentrace(glViewport);  prargi(x);  prargi(y);  prargi(width);
		prargi(height);  starttrace();

	GLXContext ctx=glXGetCurrentContext();
	GLXDrawable draw=_glXGetCurrentDrawable();
	GLXDrawable read=_glXGetCurrentReadDrawable();
	Display *dpy=_glXGetCurrentDisplay();
	GLXDrawable newread=0, newdraw=0;
	if(dpy && (draw || read) && ctx)
	{
		newread=read, newdraw=draw;
		pbwin *drawpbw=NULL, *readpbw=NULL;
		winh.findpb(draw, drawpbw);
		winh.findpb(read, readpbw);
		if(drawpbw) drawpbw->checkresize();
		if(readpbw && readpbw!=drawpbw) readpbw->checkresize();
		if(drawpbw) newdraw=drawpbw->updatedrawable();
		if(readpbw) newread=readpbw->updatedrawable();
		if(newread!=read || newdraw!=draw)
		{
			_glXMakeContextCurrent(dpy, newdraw, newread, ctx);
			if(drawpbw) {drawpbw->clear();  drawpbw->cleanup();}
			if(readpbw) readpbw->cleanup();
		}
	}
	_glViewport(x, y, width, height);

		stoptrace();  if(draw!=newdraw) {prargx(draw);  prargx(newdraw);}
		if(read!=newread) {prargx(read);  prargx(newread);}  closetrace();

	CATCH();
}


// The following functions are interposed in order to make color index
// rendering work, since most platforms don't support color index Pbuffers.

void glClearIndex(GLfloat c)
{
	if(ctxh.overlaycurrent()) _glClearIndex(c);
	else glClearColor(c/255., 0., 0., 0.);
}


#define _dpixelconvert(ctype, gltype, size)  \
	if(type==gltype) {  \
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
	if(format==GL_COLOR_INDEX && !ctxh.overlaycurrent() && type!=GL_BITMAP)
	{
		format=GL_RED;
		if(type==GL_BYTE || type==GL_UNSIGNED_BYTE) type=GL_UNSIGNED_BYTE;
		else
		{
			int rowlen=-1, align=-1;  GLubyte *buf=NULL;
			_glGetIntegerv(GL_PACK_ALIGNMENT, &align);
			_glGetIntegerv(GL_PACK_ROW_LENGTH, &rowlen);
			newcheck(buf=new unsigned char[width*height])
			if(type==GL_SHORT) type=GL_UNSIGNED_SHORT;
			if(type==GL_INT) type=GL_UNSIGNED_INT;
			_dpixelconvert(unsigned short, GL_UNSIGNED_SHORT, 2)
			_dpixelconvert(unsigned int, GL_UNSIGNED_INT, 4)
			_dpixelconvert(float, GL_FLOAT, 4)
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
	if(!ctxh.overlaycurrent())
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_COLOR, c);
			if(params) *params=(GLdouble)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, c);
			if(params) *params=(GLdouble)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			_glGetDoublev(GL_RED_SCALE, params);
			if(params) *params=(GLdouble)__round(log(*params)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			_glGetDoublev(GL_RED_BIAS, params);
			if(params) *params=(GLdouble)__round((*params)*255.);
			return;
		}
	}
	_glGetDoublev(pname, params);
}


void glGetFloatv(GLenum pname, GLfloat *params)
{
	if(!ctxh.overlaycurrent())
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_COLOR, c);
			if(params) *params=(GLfloat)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, c);
			if(params) *params=(GLfloat)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			GLdouble d;
			_glGetDoublev(GL_RED_SCALE, &d);
			if(params) *params=(GLfloat)__round(log(d)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			GLdouble d;
			_glGetDoublev(GL_RED_BIAS, &d);
			if(params) *params=(GLfloat)__round(d*255.);
			return;
		}
	}
	_glGetFloatv(pname, params);
}


void glGetIntegerv(GLenum pname, GLint *params)
{
	if(!ctxh.overlaycurrent())
	{
		if(pname==GL_CURRENT_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_COLOR, c);
			if(params) *params=(GLint)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_CURRENT_RASTER_INDEX)
		{
			GLdouble c[4];
			_glGetDoublev(GL_CURRENT_RASTER_COLOR, c);
			if(params) *params=(GLint)__round(c[0]*255.);
			return;
		}
		else if(pname==GL_INDEX_SHIFT)
		{
			double d;
			_glGetDoublev(GL_RED_SCALE, &d);
			if(params) *params=(GLint)__round(log(d)/log(2.));
			return;
		}
		else if(pname==GL_INDEX_OFFSET)
		{
			double d;
			_glGetDoublev(GL_RED_BIAS, &d);
			if(params) *params=(GLint)__round(d*255.);
			return;
		}
	}
	_glGetIntegerv(pname, params);
}


void glIndexd(GLdouble c)
{
	if(ctxh.overlaycurrent()) _glIndexd(c);
	else glColor3d(c/255., 0.0, 0.0);
}

void glIndexf(GLfloat c)
{
	if(ctxh.overlaycurrent()) _glIndexf(c);
	else glColor3f(c/255., 0., 0.);
}

void glIndexi(GLint c)
{
	if(ctxh.overlaycurrent()) _glIndexi(c);
	else glColor3f((GLfloat)c/255., 0, 0);
}

void glIndexs(GLshort c)
{
	if(ctxh.overlaycurrent()) _glIndexs(c);
	else glColor3f((GLfloat)c/255., 0, 0);
}

void glIndexub(GLubyte c)
{
	if(ctxh.overlaycurrent()) _glIndexub(c);
	else glColor3f((GLfloat)c/255., 0, 0);
}

void glIndexdv(const GLdouble *c)
{
	if(ctxh.overlaycurrent()) _glIndexdv(c);
	else
	{
		GLdouble v[3]={c? (*c)/255.:0., 0., 0.};
		glColor3dv(c? v:NULL);
	}
}

void glIndexfv(const GLfloat *c)
{
	if(ctxh.overlaycurrent()) _glIndexfv(c);
	else
	{
		GLfloat v[3]={c? (*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
}

void glIndexiv(const GLint *c)
{
	if(ctxh.overlaycurrent()) _glIndexiv(c);
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
}

void glIndexsv(const GLshort *c)
{
	if(ctxh.overlaycurrent()) _glIndexsv(c);
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
}

void glIndexubv(const GLubyte *c)
{
	if(ctxh.overlaycurrent()) _glIndexubv(c);
	else
	{
		GLfloat v[3]={c? (GLfloat)(*c)/255.:0., 0., 0.};
		glColor3fv(c? v:NULL);  return;
	}
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
	if(!ctxh.overlaycurrent())
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
	if(!ctxh.overlaycurrent())
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


#define _rpixelconvert(ctype, gltype, size)  \
	if(type==gltype) {  \
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
	if(format==GL_COLOR_INDEX && !ctxh.overlaycurrent() && type!=GL_BITMAP)
	{
		format=GL_RED;
		if(type==GL_BYTE || type==GL_UNSIGNED_BYTE) type=GL_UNSIGNED_BYTE;
		else
		{
			int rowlen=-1, align=-1;  GLubyte *buf=NULL;
			_glGetIntegerv(GL_PACK_ALIGNMENT, &align);
			_glGetIntegerv(GL_PACK_ROW_LENGTH, &rowlen);
			newcheck(buf=new unsigned char[width*height])
			if(type==GL_SHORT) type=GL_UNSIGNED_SHORT;
			if(type==GL_INT) type=GL_UNSIGNED_INT;
			glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 1);
			_glReadPixels(x, y, width, height, format, GL_UNSIGNED_BYTE, buf);
			glPopClientAttrib();
			_rpixelconvert(unsigned short, GL_UNSIGNED_SHORT, 2)
			_rpixelconvert(unsigned int, GL_UNSIGNED_INT, 4)
			_rpixelconvert(float, GL_FLOAT, 4)
			delete [] buf;
			return;
		}
	}
	_glReadPixels(x, y, width, height, format, type, pixels);
	CATCH();
}


} // extern "C"
