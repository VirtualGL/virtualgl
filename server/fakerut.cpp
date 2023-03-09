// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2010-2015, 2017-2023 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include <stdio.h>
#include <stdlib.h>
#define GLX_GLXEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glu.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#ifdef FAKEXCB
	#include <xcb/xcb.h>
	#include <xcb/glx.h>
	#include <X11/Xlib-xcb.h>
#endif
#include <dlfcn.h>
#include <unistd.h>
#include "Error.h"
#include "Thread.h"
#include <X11/Xmd.h>
#include <GL/glxproto.h>
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif

using namespace util;


#define CHECK_GL_ERROR() \
{ \
	int e = glGetError(); \
	if(e != GL_NO_ERROR) THROW("OpenGL error"); \
	while(e != GL_NO_ERROR) e = glGetError(); \
}


#define CLEAR_BUFFER(buffer, r, g, b, a) \
{ \
	if(buffer > 0) \
	{ \
		if(buffer >= GL_FRONT_LEFT && buffer <= GL_BACK_RIGHT) \
		{ \
			GLenum tempBuf = buffer; \
			glDrawBuffers(1, &tempBuf); \
		} \
		else glDrawBuffer(buffer); \
	} \
	glClearColor(r, g, b, a); \
	glClear(GL_COLOR_BUFFER_BIT); \
}


#define VERIFY_BUF_COLOR(buf, colorShouldBe, tag) \
{ \
	if(buf > 0) glReadBuffer(buf); \
	unsigned int color = checkBufferColor(); \
	if(color != (colorShouldBe)) \
		PRERROR2(tag " is 0x%.6x, should be 0x%.6x", color, (colorShouldBe)); \
}


#define VERIFY_FBO(drawFBO, drawBuf0, drawBuf1, readFBO, readBuf) \
{ \
	GLint __drawFBO = -1; \
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &__drawFBO); \
	if(__drawFBO != (GLint)drawFBO) \
		PRERROR2("Draw FBO is %d, should be %d", __drawFBO, drawFBO); \
	GLint __readFBO = -1; \
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &__readFBO); \
	if(__readFBO != (GLint)readFBO) \
		PRERROR2("Read FBO is %d, should be %d", __readFBO, readFBO); \
	GLint __drawBuf0 = -1; \
	glGetIntegerv(GL_DRAW_BUFFER0, &__drawBuf0); \
	if(__drawBuf0 != (GLint)drawBuf0) \
		PRERROR2("Draw buffer 0 is 0x%.4x, should be 0x%.4x", __drawBuf0, \
			drawBuf0); \
	GLint __drawBuf1 = -1; \
	glGetIntegerv(GL_DRAW_BUFFER1, &__drawBuf1); \
	if(__drawBuf1 != (GLint)drawBuf1) \
		PRERROR2("Draw buffer 1 is 0x%.4x, should be 0x%.4x", __drawBuf1, \
			drawBuf1); \
	GLint __readBuf = -1; \
	glGetIntegerv(GL_READ_BUFFER, &__readBuf); \
	if(__readBuf != (GLint)readBuf) \
		PRERROR2("Read buffer is 0x%.4x, should be 0x%.4x", __readBuf, readBuf); \
}


#define GLX_EXTENSION_EXISTS(ext) \
	strstr(glXQueryExtensionsString(dpy, DefaultScreen(dpy)), #ext)


#if 0
void clickToContinue(Display *dpy)
{
	XEvent e;
	printf("Click mouse in window to continue ...\n");
	while(1)
	{
		if(XNextEvent(dpy, &e)) break;
		if(e.type == ButtonPress) break;
	}
}
#endif

// Same as THROW but without the line number
#define THROWNL(m)  throw(Error(__FUNCTION__, m, 0))

#define PRERROR1(m, a1) \
{ \
	char tempErrStr[256]; \
	snprintf(tempErrStr, 256, m, a1); \
	throw(Error(__FUNCTION__, tempErrStr, 0)); \
}

#define PRERROR2(m, a1, a2) \
{ \
	char tempErrStr[256]; \
	snprintf(tempErrStr, 256, m, a1, a2); \
	throw(Error(__FUNCTION__, tempErrStr, 0)); \
}

#define PRERROR3(m, a1, a2, a3) \
{ \
	char tempErrStr[256]; \
	snprintf(tempErrStr, 256, m, a1, a2, a3); \
	throw(Error(__FUNCTION__, tempErrStr, 0)); \
}

#define PRERROR4(m, a1, a2, a3, a4) \
{ \
	char tempErrStr[256]; \
	snprintf(tempErrStr, 256, m, a1, a2, a3, a4); \
	throw(Error(__FUNCTION__, tempErrStr, 0)); \
}

#define PRERROR5(m, a1, a2, a3, a4, a5) \
{ \
	char tempErrStr[256]; \
	snprintf(tempErrStr, 256, m, a1, a2, a3, a4, a5); \
	throw(Error(__FUNCTION__, tempErrStr, 0)); \
}


unsigned int checkBufferColor(void)
{
	int i, viewport[4], ps = 3;  unsigned int ret = 0;
	unsigned char *buf = NULL;

	try
	{
		viewport[0] = viewport[1] = viewport[2] = viewport[3] = 0;
		glGetIntegerv(GL_VIEWPORT, viewport);
		if(viewport[2] < 1 || viewport[3] < 1)
			THROW("Invalid viewport dimensions");

		if((buf = (unsigned char *)malloc(viewport[2] * viewport[3] * ps)) == NULL)
			THROW("Could not allocate buffer");
		memset(buf, 128, viewport[2] * viewport[3] * ps);

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, viewport[2], viewport[3], GL_RGB, GL_UNSIGNED_BYTE,
			buf);
		for(i = 3; i < viewport[2] * viewport[3] * ps; i += ps)
		{
			if(buf[i] != buf[0] || buf[i + 1] != buf[1] || buf[i + 2] != buf[2])
				THROW("Bogus data read back");
		}
		ret = buf[0] | (buf[1] << 8) | (buf[2] << 16);
		free(buf);
		return ret;
	}
	catch(...)
	{
		free(buf);
		throw;
	}
}


void checkWindowColor(Display *dpy, Window win, unsigned int color,
	bool right = false)
{
	int fakerColor;
	typedef int (*_vgl_getAutotestColorType)(Display *, Window, int);
	_vgl_getAutotestColorType _vgl_getAutotestColor;

	_vgl_getAutotestColor =
		(_vgl_getAutotestColorType)dlsym(RTLD_DEFAULT, "_vgl_getAutotestColor");
	if(!_vgl_getAutotestColor)
		THROWNL("Can't communicate w/ faker");
	fakerColor = _vgl_getAutotestColor(dpy, win, right);
	if(fakerColor < 0 || fakerColor > 0xffffff)
		THROWNL("Bogus data read back");
	if((unsigned int)fakerColor != color)
	{
		if(right)
			PRERROR2("R.buf is 0x%.6x, should be 0x%.6x", fakerColor, color)
		else
			PRERROR2("Color is 0x%.6x, should be 0x%.6x", fakerColor, color)
	}
}


void checkFrame(Display *dpy, Window win, int desiredReadbacks, int &lastFrame)
{
	int frame;
	typedef int (*_vgl_getAutotestFrameType)(Display *, Window);
	_vgl_getAutotestFrameType _vgl_getAutotestFrame;

	_vgl_getAutotestFrame =
		(_vgl_getAutotestFrameType)dlsym(RTLD_DEFAULT, "_vgl_getAutotestFrame");
	if(!_vgl_getAutotestFrame)
		THROWNL("Can't communicate w/ faker");
	frame = _vgl_getAutotestFrame(dpy, win);
	if(frame < 1)
		THROWNL("Can't communicate w/ faker");
	if(frame - lastFrame != desiredReadbacks && desiredReadbacks >= 0)
		PRERROR3("Expected %d readback%s, not %d", desiredReadbacks,
			desiredReadbacks == 1 ? "" : "s", frame - lastFrame);
	lastFrame = frame;
}


void checkCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx, int width, int height)
{
	if(glXGetCurrentDisplay() != dpy)
		THROW("glXGetCurrentDisplay() returned incorrect value");
	if(glXGetCurrentDrawable() != draw)
		THROW("glXGetCurrentDrawable() returned incorrect value");
	if(glXGetCurrentReadDrawable() != read)
		THROW("glXGetCurrentReadDrawable() returned incorrect value");
	if(glXGetCurrentContext() != ctx)
		THROW("glXGetCurrentContext() returned incorrect value");
	int viewport[4] = { 0, 0, 0, 0 };
	glGetIntegerv(GL_VIEWPORT, viewport);
	if(viewport[2] < 1 || viewport[3] < 1)
		THROW("Invalid viewport dimensions");
	if(viewport[2] != width || viewport[3] != height)
		PRERROR4("Viewport is %dx%d, should be %dx%d\n", viewport[2], viewport[3],
			width, height);
}


void checkBufferState(int oldDrawBuf, int oldReadBuf, Display *dpy,
	GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
	if(glXGetCurrentDisplay() != dpy)
		THROWNL("Current display changed");
	if(glXGetCurrentDrawable() != draw || glXGetCurrentReadDrawable() != read)
		THROWNL("Current drawable changed");
	if(glXGetCurrentContext() != ctx)
		THROWNL("Context changed");
	int drawBuf = -1;
	glGetIntegerv(GL_DRAW_BUFFER, &drawBuf);
	if(drawBuf != oldDrawBuf)
		THROWNL("Draw buffer changed");
	int readBuf = -1;
	glGetIntegerv(GL_READ_BUFFER, &readBuf);
	if(readBuf != oldReadBuf)
		THROWNL("Read buffer changed");
}


// Check whether double buffering works properly
bool doubleBufferTest(void)
{
	unsigned int bgColor = 0, fgColor = 0;
	int oldReadBuf = GL_BACK, oldDrawBuf = GL_BACK;

	glGetIntegerv(GL_READ_BUFFER, &oldReadBuf);
	glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuf);
	CLEAR_BUFFER(GL_FRONT_AND_BACK, 0., 0., 0., 0.);
	CLEAR_BUFFER(GL_BACK, 1., 1., 1., 0.);
	CLEAR_BUFFER(GL_FRONT, 1., 0., 1., 0.);
	glReadBuffer(GL_BACK);
	bgColor = checkBufferColor();
	glReadBuffer(GL_FRONT);
	fgColor = checkBufferColor();
	glReadBuffer(oldReadBuf);
	glDrawBuffer(oldDrawBuf);
	if(bgColor == 0xffffff && fgColor == 0xff00ff) return true;
	return false;
}


// Check whether stereo works properly
bool stereoTest(void)
{
	unsigned int rightColor = 0, leftColor = 0;
	int oldReadBuf = GL_BACK, oldDrawBuf = GL_BACK;

	glGetIntegerv(GL_READ_BUFFER, &oldReadBuf);
	glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuf);
	CLEAR_BUFFER(GL_FRONT_AND_BACK, 0., 0., 0., 0.);
	CLEAR_BUFFER(GL_RIGHT, 1., 1., 1., 0.);
	CLEAR_BUFFER(GL_LEFT, 1., 0., 1., 0.);
	glReadBuffer(GL_RIGHT);
	rightColor = checkBufferColor();
	glReadBuffer(GL_LEFT);
	leftColor = checkBufferColor();
	glReadBuffer(oldReadBuf);
	glDrawBuffer(oldDrawBuf);
	if(rightColor == 0xffffff && leftColor == 0xff00ff) return true;
	return false;
}


#define GLGET_TEST(param) \
{ \
	GLenum err; \
	GLint ival = -1; \
	glGetIntegerv(param, &ival); \
	if(glGetError() != GL_NO_ERROR) \
		THROW("OpenGL error in glGetIntegerv()\n"); \
	\
	GLboolean bval = -1; \
	glGetBooleanv(param, &bval); \
	if((err = glGetError()) != GL_NO_ERROR) \
		PRERROR1("OpenGL error 0x%.4x in glGetBooleanv()\n", err); \
	if(bval != (ival == 0 ? GL_FALSE : GL_TRUE)) \
		PRERROR3("glGetBooleanv(0x%.4x) %d != %d", param, bval, ival); \
	\
	GLdouble dval = -1; \
	glGetDoublev(param, &dval); \
	if((err = glGetError()) != GL_NO_ERROR) \
		PRERROR1("OpenGL error 0x%.4x in glGetDoublev()\n", err); \
	if(dval != (GLdouble)ival) \
		PRERROR3("glGetDoublev(0x%.4x) %f != %f", param, dval, (GLdouble)ival); \
	\
	GLfloat fval = -1; \
	glGetFloatv(param, &fval); \
	if((err = glGetError()) != GL_NO_ERROR) \
		PRERROR1("OpenGL error 0x%.4x in glGetFloatv()\n", err); \
	if(fval != (GLfloat)ival) \
		PRERROR3("glGetFloatv(0x%.4x) %f != %f\n", param, fval, (GLfloat)ival); \
	\
	GLint64 ival64 = -1; \
	glGetInteger64v(param, &ival64); \
	if((err = glGetError()) != GL_NO_ERROR) \
		PRERROR1("OpenGL error 0x%.4x in glGetInteger64v()\n", err); \
	if(ival64 != (GLint64)ival) \
		PRERROR3("glGetInteger64v(0x%.4x) %ld != %d\n", param, (long)ival64, \
			ival); \
}

void glGetTest(void)
{
	GLGET_TEST(GL_DOUBLEBUFFER);
	GLGET_TEST(GL_DRAW_BUFFER);
	GLGET_TEST(GL_DRAW_BUFFER0);
	GLGET_TEST(GL_DRAW_FRAMEBUFFER_BINDING);
	GLGET_TEST(GL_MAX_DRAW_BUFFERS);
	GLGET_TEST(GL_READ_BUFFER);
	GLGET_TEST(GL_READ_FRAMEBUFFER_BINDING);
	GLGET_TEST(GL_STEREO);
}


typedef struct
{
	GLfloat r, g, b;
	unsigned int bits;
} Color;

#define NC  6
static Color colors[NC] =
{
	{ 1., 0., 0., 0x0000ff },
	{ 0., 1., 0., 0x00ff00 },
	{ 0., 0., 1., 0xff0000 },
	{ 0., 1., 1., 0xffff00 },
	{ 1., 0., 1., 0xff00ff },
	{ 1., 1., 0., 0x00ffff }
};

class TestColor
{
	public:

		TestColor(int index_) : index(index_ % NC)
		{
		}

		GLfloat &r(int offset = 0) { return colors[(index + offset + NC) % NC].r; }
		GLfloat &g(int offset = 0) { return colors[(index + offset + NC) % NC].g; }
		GLfloat &b(int offset = 0) { return colors[(index + offset + NC) % NC].b; }
		void next(void) { index = (index + 1) % NC; }

		unsigned int &bits(int offset = 0)
		{
			return colors[(index + offset + NC) % NC].bits;
		}

		void clear(int buffer)
		{
			CLEAR_BUFFER(buffer, r(), g(), b(), 0.)
			next();
		}

	private:

		int index;
};


// This tests the faker's readback heuristics
int readbackTest(bool stereo, bool doNamedFB)
{
	TestColor clr(0), sclr(3);
	Display *dpy = NULL;  Window win0 = 0, win1 = 0;
	int dpyw, dpyh, lastFrame0 = 0, lastFrame1 = 0, retval = 1;
	int glxattribs[] = { GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None, None };
	int glxattribs13[] = { GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None, None, None };
	XVisualInfo *vis0 = NULL, *vis1 = NULL;
	GLXFBConfig config = 0, *configs = NULL;
	int n = 0;
	GLXContext ctx0 = 0, ctx1 = 0;
	XSetWindowAttributes swa;

	if(stereo)
	{
		glxattribs[8] = glxattribs13[12] = GLX_STEREO;
		glxattribs13[13] = 1;
	}

	printf("Readback heuristics test ");
	if(stereo) printf("(Stereo RGB)\n\n");
	else printf("(Mono RGB)\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
		dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

		if((vis0 = glXChooseVisual(dpy, DefaultScreen(dpy), glxattribs)) == NULL)
			THROW("Could not find a suitable visual");
		if((configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattribs13, &n))
			== NULL || n == 0)
			THROW("Could not find a suitable FB config");
		config = configs[0];
		XFree(configs);  configs = NULL;

		Window root = RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap = XCreateColormap(dpy, root, vis0->visual, AllocNone);
		swa.border_pixel = 0;
		swa.event_mask = 0;
		if((win0 = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0,
			vis0->depth, InputOutput, vis0->visual,
			CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
			THROW("Could not create window");
		if(!(vis1 = glXGetVisualFromFBConfig(dpy, config)))
			THROW("glXGetVisualFromFBConfig()");
		swa.colormap = XCreateColormap(dpy, root, vis1->visual, AllocNone);
		if((win1 = XCreateWindow(dpy, root, dpyw / 2, 0, dpyw / 2, dpyh / 2, 0,
			vis1->depth, InputOutput, vis1->visual,
			CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
			THROW("Could not create window");
		XFree(vis1);  vis1 = NULL;

		if((ctx0 = glXCreateContext(dpy, vis0, 0, True)) == NULL)
			THROW("Could not establish GLX context");
		XFree(vis0);  vis0 = NULL;
		if((ctx1 = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, NULL, True))
			== NULL)
			THROW("Could not establish GLX context");

		if(!glXMakeCurrent(dpy, win1, ctx0))
			THROW("Could not make context current");
		checkCurrent(dpy, win1, win1, ctx0, dpyw / 2, dpyh / 2);
		if(stereo && !stereoTest())
		{
			THROW("Stereo is not available or is not properly implemented");
		}
		if(!doubleBufferTest())
			THROW("Double buffering appears to be broken");
		glGetTest();
		glReadBuffer(GL_BACK);

		if(!glXMakeContextCurrent(dpy, win1, win0, ctx1))
			THROW("Could not make context current");
		checkCurrent(dpy, win1, win0, ctx1, dpyw / 2, dpyh / 2);

		XMapWindow(dpy, win0);
		XMapWindow(dpy, win1);

		// Faker should readback back buffer on a call to glXSwapBuffers()
		try
		{
			printf("glXSwapBuffers() [b]:          ");
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			glReadBuffer(GL_FRONT);
			// Intentionally leave a pending GL error (VirtualGL should clear the
			// error state prior to readback)
			char pixel[4];
			glReadPixels(0, 0, 1, 1, 0, GL_BYTE, pixel);
			glXSwapBuffers(dpy, win1);
			checkBufferState(stereo ? GL_FRONT_RIGHT : GL_FRONT, GL_FRONT, dpy, win1,
				win0, ctx1);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win1, sclr.bits(-2), true);
			// Make sure that glXSwapBuffers() actually swapped
			glDrawBuffer(GL_FRONT);
			glFinish();
			checkBufferState(GL_FRONT, GL_FRONT, dpy, win1, win0, ctx1);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win1, sclr.bits(-2), true);
			// Swapping buffers while the render mode != GL_RENDER will cause
			// VirtualGL < 2.5.2 to throw a GLXBadContextState error
			GLfloat fbBuffer[2];
			glFeedbackBuffer(1, GL_2D, fbBuffer);
			glRenderMode(GL_FEEDBACK);
			glXSwapBuffers(dpy, win1);
			glRenderMode(GL_RENDER);
			checkBufferState(GL_FRONT, GL_FRONT, dpy, win1, win0, ctx1);
			checkFrame(dpy, win1, 0, lastFrame1);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		// Faker should readback front buffer on glFlush(), glFinish(), and
		// glXWaitGL()
		try
		{
			printf("glFlush() [f]:                 ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glFlush();  glFlush();
			checkFrame(dpy, win1, 1, lastFrame1);
			glDrawBuffer(GL_FRONT);
			GLuint selectBuffer[2];
			glSelectBuffer(1, selectBuffer);
			glRenderMode(GL_SELECT);  glFlush();
			GLfloat fbBuffer[2];
			glFeedbackBuffer(1, GL_2D, fbBuffer);
			glRenderMode(GL_FEEDBACK);  glFlush();
			glRenderMode(GL_RENDER);  glFlush();
			checkBufferState(GL_FRONT, GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win1, sclr.bits(-2), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glFinish() [f]:                ");
			clr.clear(stereo ? GL_FRONT : GL_FRONT_LEFT);
			if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(stereo ? GL_BACK : GL_BACK_LEFT);
			if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkFrame(dpy, win1, 1, lastFrame1);
			glDrawBuffer(GL_FRONT);  glFinish();
			checkBufferState(GL_FRONT, GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win1, sclr.bits(-2), true);
			GLint major = -1, minor = -1;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);
			if(doNamedFB && (major > 4 || (major == 4 && minor >= 5)))
			{
				if(stereo)
				{
					PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC __glFramebufferDrawBuffersEXT =
						(PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC)glXGetProcAddress(
							(const GLubyte *)"glFramebufferDrawBuffersEXT");
					if(!__glFramebufferDrawBuffersEXT)
						THROW("glFramebufferDrawBuffersEXT() not available");
					const GLenum buf = GL_BACK;
					__glFramebufferDrawBuffersEXT(0, 1, &buf);

					PFNGLFRAMEBUFFERREADBUFFEREXTPROC __glFramebufferReadBufferEXT =
						(PFNGLFRAMEBUFFERREADBUFFEREXTPROC)glXGetProcAddress(
							(const GLubyte *)"glFramebufferReadBufferEXT");
					if(!__glFramebufferReadBufferEXT)
						THROW("glFramebufferReadBufferEXT() not available");
					__glFramebufferReadBufferEXT(0, GL_FRONT);
				}
				else
				{
					PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC __glNamedFramebufferDrawBuffers =
						(PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC)glXGetProcAddress(
							(const GLubyte *)"glNamedFramebufferDrawBuffers");
					if(!__glNamedFramebufferDrawBuffers)
						THROW("glNamedFramebufferDrawBuffers() not available");
					const GLenum buf = GL_BACK;
					__glNamedFramebufferDrawBuffers(0, 1, &buf);

					PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC __glNamedFramebufferReadBuffer =
						(PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC)glXGetProcAddress(
							(const GLubyte *)"glNamedFramebufferReadBuffer");
					if(!__glNamedFramebufferReadBuffer)
						THROW("glNamedFramebufferReadBuffer() not available");
					__glNamedFramebufferReadBuffer(0, GL_FRONT);
				}
				VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_FRONT);
				glFinish();
				checkFrame(dpy, win1, 1, lastFrame1);
				checkWindowColor(dpy, win1, clr.bits(-2));
				if(stereo)
					checkWindowColor(dpy, win1, sclr.bits(-2), true);
				glDrawBuffer(GL_FRONT);
			}
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glXWaitGL() [f]:               ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glXWaitGL();  glXWaitGL();
			checkFrame(dpy, win1, 1, lastFrame1);
			glDrawBuffer(GL_FRONT);  glXWaitGL();
			checkBufferState(GL_FRONT, GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win1, sclr.bits(-2), true);
			GLint major = -1, minor = -1;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);
			if(doNamedFB && (major > 4 || (major == 4 && minor >= 5)))
			{
				if(stereo)
				{
					PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC __glFramebufferDrawBufferEXT =
						(PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC)glXGetProcAddress(
							(const GLubyte *)"glFramebufferDrawBufferEXT");
					if(!__glFramebufferDrawBufferEXT)
						THROW("glFramebufferDrawBufferEXT() not available");
					__glFramebufferDrawBufferEXT(0, GL_BACK);
				}
				else
				{
					PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC __glNamedFramebufferDrawBuffer =
						(PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC)glXGetProcAddress(
							(const GLubyte *)"glNamedFramebufferDrawBuffer");
					if(!__glNamedFramebufferDrawBuffer)
						THROW("glNamedFramebufferDrawBuffer() not available");
					__glNamedFramebufferDrawBuffer(0, GL_BACK);
				}
				VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_BACK);
				glXWaitGL();
				checkFrame(dpy, win1, 1, lastFrame1);
				checkWindowColor(dpy, win1, clr.bits(-2));
				if(stereo)
					checkWindowColor(dpy, win1, sclr.bits(-2), true);
				glDrawBuffer(GL_FRONT);
			}
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glPopAttrib() [f]:             ");
			glDrawBuffer(GL_BACK);  glFinish();
			checkFrame(dpy, win1, 1, lastFrame1);
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			glPopAttrib();  // Back buffer should now be current again & dirty flag
			                // should be set
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkBufferState(stereo ? GL_BACK_RIGHT : GL_BACK, GL_BACK, dpy, win1,
				win0, ctx1);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win1, sclr.bits(-2), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glXMakeCurrent() [f]:          ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glXMakeCurrent(dpy, win1, ctx0);  // readback should occur
			glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			glDrawBuffer(GL_FRONT);
			glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			checkBufferState(GL_FRONT, GL_BACK, dpy, win0, win0, ctx0);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win1, sclr.bits(-2), true);
			// Now try swapping one window when another is current (this will fail
			// with VGL 2.3.3 and earlier)
			glXSwapBuffers(dpy, win1);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-1));
			// Verify that the previous glXSwapBuffers() call was successful
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glXMakeCurrent(dpy, win1, ctx0);
			checkFrame(dpy, win0, 1, lastFrame0);
			checkWindowColor(dpy, win0, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win0, sclr.bits(-2), true);
			VERIFY_BUF_COLOR(GL_FRONT_LEFT, clr.bits(-3), "GL_FRONT_LEFT");
			if(stereo)
				VERIFY_BUF_COLOR(GL_FRONT_RIGHT, sclr.bits(-3), "GL_FRONT_RIGHT");
			glReadBuffer(GL_BACK);
			// Verify that glXSwapBuffers() works properly without a current context
			glXMakeCurrent(dpy, 0, 0);
			glXSwapBuffers(dpy, win0);
			checkFrame(dpy, win0, 1, lastFrame0);
			checkWindowColor(dpy, win0, clr.bits(-1));
			glXMakeCurrent(dpy, win0, ctx0);
			VERIFY_BUF_COLOR(GL_FRONT_LEFT, clr.bits(-1), "GL_FRONT_LEFT");
			if(stereo)
				VERIFY_BUF_COLOR(GL_FRONT_RIGHT, sclr.bits(-1), "GL_FRONT_RIGHT");
			glReadBuffer(GL_BACK);
			glDrawBuffer(GL_FRONT);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glXMakeContextCurrent() [f]:   ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glXMakeContextCurrent(dpy, win0, win0, ctx1);  // No readback should occur
			glXMakeContextCurrent(dpy, win1, win0, ctx1);  // readback should occur
			glDrawBuffer(GL_FRONT);
			glXMakeContextCurrent(dpy, win1, win0, ctx1);  // No readback should occur
			checkBufferState(GL_FRONT, GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(dpy, win0, 1, lastFrame0);
			checkWindowColor(dpy, win0, clr.bits(-2));
			if(stereo)
				checkWindowColor(dpy, win0, sclr.bits(-2), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		// Test for proper handling of GL_FRONT_AND_BACK
		try
		{
			printf("glXSwapBuffers() [f&b]:        ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_FRONT);
			glXSwapBuffers(dpy, win1);
			checkBufferState(stereo ? GL_RIGHT : GL_FRONT_AND_BACK, GL_FRONT, dpy,
				win1, win0, ctx1);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-1));
			if(stereo) checkWindowColor(dpy, win1, sclr.bits(-1), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glFlush() [f&b]:               ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glFlush();  glFlush();
			checkBufferState(stereo ? GL_RIGHT : GL_FRONT_AND_BACK, GL_BACK, dpy,
				win1, win0, ctx1);
			checkFrame(dpy, win1, 2, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-1));
			if(stereo) checkWindowColor(dpy, win1, sclr.bits(-1), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glFinish() [f&b]:              ");
			GLenum bufs[] = { GL_FRONT_LEFT, GL_BACK_LEFT };
			glDrawBuffers(2, bufs);
			glClearColor(clr.r(), clr.g(), clr.b(), 0.);
			glClear(GL_COLOR_BUFFER_BIT);
			clr.next();
			if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkBufferState(stereo ? GL_RIGHT : GL_FRONT_LEFT, GL_BACK, dpy, win1,
				win0, ctx1);
			checkFrame(dpy, win1, 2, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-1));
			if(stereo) checkWindowColor(dpy, win1, sclr.bits(-1), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glXWaitGL() [f&b]:             ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glXWaitGL();  glXWaitGL();
			checkBufferState(stereo ? GL_RIGHT : GL_FRONT_AND_BACK, GL_BACK, dpy,
				win1, win0, ctx1);
			checkFrame(dpy, win1, 2, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-1));
			if(stereo) checkWindowColor(dpy, win1, sclr.bits(-1), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glXMakeCurrent() [f&b]:        ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glXMakeCurrent(dpy, win0, ctx0);  // readback should occur
			glDrawBuffer(GL_FRONT);
			glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			checkBufferState(GL_FRONT, GL_BACK, dpy, win0, win0, ctx0);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-1));
			if(stereo) checkWindowColor(dpy, win1, sclr.bits(-1), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("glXMakeContextCurrent() [f&b]: ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glXMakeContextCurrent(dpy, win0, win0, ctx1);  // No readback should occur
			glXMakeContextCurrent(dpy, win1, win0, ctx1);  // readback should occur
			glDrawBuffer(GL_FRONT);
			glXMakeContextCurrent(dpy, win1, win0, ctx1);  // No readback should occur
			checkBufferState(GL_FRONT, GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(dpy, win0, 1, lastFrame0);
			checkWindowColor(dpy, win0, clr.bits(-1));
			if(stereo) checkWindowColor(dpy, win0, sclr.bits(-1), true);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	if(ctx0 && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx0);
	}
	if(ctx1 && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx1);
	}
	if(win0) XDestroyWindow(dpy, win0);
	if(win1) XDestroyWindow(dpy, win1);
	if(vis0) XFree(vis0);
	if(vis1) XFree(vis1);
	if(configs) XFree(configs);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


// This tests whether a multisampled off-screen buffer can be read back, both
// internally within frame trigger functions and externally with
// glReadPixels(), when using the EGL back end.
int readbackTestMS(void)
{
	TestColor clr(0);
	Display *dpy = NULL;  Window win = 0;
	int dpyw, dpyh, lastFrame = 0, retval = 1;
	int glxattribs13[] = { GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, GLX_SAMPLES, 2, None };
	XVisualInfo *vis = NULL;
	GLXFBConfig config = 0, *configs = NULL;
	int n = 0;
	GLXContext ctx = 0;
	XSetWindowAttributes swa;
	GLuint fbo = 0, rbo0 = 0, rbo1 = 0;

	printf("Multisampled readback test:\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
		dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

		if((configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattribs13, &n))
			== NULL || n == 0)
			THROW("Could not find a suitable FB config");
		config = configs[0];
		XFree(configs);  configs = NULL;
		if((vis = glXGetVisualFromFBConfig(dpy, config)) == NULL)
			THROW("glXGetVisualFromFBConfig()");

		Window root = RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel = 0;
		swa.event_mask = 0;
		if((win = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0,
			vis->depth, InputOutput, vis->visual,
			CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
			THROW("Could not create window");
		XFree(vis);  vis = NULL;

		if((ctx = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, NULL, True))
			== NULL)
			THROW("Could not establish GLX context");
		if(!glXMakeContextCurrent(dpy, win, win, ctx))
			THROW("Could not make context current");
		checkCurrent(dpy, win, win, ctx, dpyw / 2, dpyh / 2);

		XMapWindow(dpy, win);

		clr.clear(GL_FRONT);
		VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "GL_FRONT");
		clr.clear(GL_BACK);
		VERIFY_BUF_COLOR(GL_BACK, clr.bits(-1), "GL_BACK");
		glGenFramebuffers(1, &fbo);
		glGenRenderbuffers(1, &rbo0);
		glGenRenderbuffers(1, &rbo1);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo0);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, dpyw / 2, dpyh / 2);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo1);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, dpyw / 2, dpyh / 2);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_RENDERBUFFER, rbo0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
			GL_RENDERBUFFER, rbo1);
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, bufs);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glXSwapBuffers(dpy, win);
		VERIFY_FBO(fbo, GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, fbo,
			GL_COLOR_ATTACHMENT1);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_BACK);
		checkBufferState(GL_BACK, GL_BACK, dpy, win, win, ctx);
		clr.clear(0);
		VERIFY_BUF_COLOR(0, clr.bits(-1), "GL_BACK");
		// When using the EGL back end in VirtualGL 3.0 beta1 through 3.0.1, the
		// following command will fail, because glBindFramebuffer(..., 0) didn't
		// restore the previous draw/read buffer state for the default framebuffer.
		// Swapping the buffers of a GLX drawable causes the EGL back end to
		// create a new FBO to serve as the drawable's default framebuffer, with
		// the new FBO containing the previous RBOs bound in reverse order.  The
		// default draw/read buffer for a new FBO is GL_COLOR_ATTACHMENT0, which
		// the EGL back end uses as the front left buffer.  Since the default
		// framebuffer isn't bound when the buffers are swapped in the code above,
		// the FBO's initial draw/read buffer remains unchanged.  Thus, at this
		// point in the code, VirtualGL reported that GL_BACK was active, but the
		// RBO corresponding to GL_FRONT was actually active.  As a result, the
		// previous two commands affected the front buffer rather than the back
		// buffer.
		VERIFY_BUF_COLOR(GL_BACK, clr.bits(-1), "GL_BACK");
		VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-2), "GL_FRONT");
		checkFrame(dpy, win, 1, lastFrame);
		checkWindowColor(dpy, win, clr.bits(-2));

		printf("SUCCESS\n");
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	if(ctx && dpy)
	{
		if(fbo)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
				GL_RENDERBUFFER, 0);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			if(rbo0) glDeleteRenderbuffers(1, &rbo0);
			if(rbo1) glDeleteRenderbuffers(1, &rbo1);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &fbo);
		}
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);
	}
	if(win) XDestroyWindow(dpy, win);
	if(vis) XFree(vis);
	if(configs) XFree(configs);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


// This tests the faker's ability to handle the 2000 Flushes issue
int flushTest(void)
{
	TestColor clr(0);
	Display *dpy = NULL;  Window win = 0;
	int dpyw, dpyh, lastFrame = 0, retval = 1;
	int glxattribs[] = { GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	XVisualInfo *vis = NULL;
	GLXContext ctx = 0;
	XSetWindowAttributes swa;

	putenv((char *)"VGL_SPOIL=1");
	printf("10000 flushes test:\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
		dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

		if((vis = glXChooseVisual(dpy, DefaultScreen(dpy), glxattribs)) == NULL)
			THROW("Could not find a suitable visual");

		Window root = RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel = 0;
		swa.event_mask = 0;
		if((win = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0, vis->depth,
			InputOutput, vis->visual, CWBorderPixel | CWColormap | CWEventMask,
			&swa)) == 0)
			THROW("Could not create window");

		if((ctx = glXCreateContext(dpy, vis, 0, True)) == NULL)
			THROW("Could not establish GLX context");
		XFree(vis);  vis = NULL;
		if(!glXMakeCurrent(dpy, win, ctx))
			THROW("Could not make context current");
		checkCurrent(dpy, win, win, ctx, dpyw / 2, dpyh / 2);
		if(!doubleBufferTest())
			THROW("This test requires double buffering, which appears to be broken.");
		glReadBuffer(GL_FRONT);
		XMapWindow(dpy, win);

		clr.clear(GL_BACK);
		clr.clear(GL_FRONT);
		for(int i = 0; i < 10000; i++)
		{
			printf("%.4d\b\b\b\b", i);  glFlush();
		}
		checkFrame(dpy, win, -1, lastFrame);
		printf("Read back %d of 10000 frames\n", lastFrame);
		checkBufferState(GL_FRONT, GL_FRONT, dpy, win, win, ctx);
		checkWindowColor(dpy, win, clr.bits(-1), 0);
		printf("SUCCESS\n");
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	fflush(stdout);
	putenv((char *)"VGL_SPOIL=0");

	if(ctx && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);
	}
	if(win) XDestroyWindow(dpy, win);
	if(vis) XFree(vis);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


int cfgid(Display *dpy, GLXFBConfig config);


#define GET_CFG_ATTRIB(config, attrib, ctemp) \
{ \
	ctemp = -10; \
	glXGetFBConfigAttrib(dpy, config, attrib, &ctemp); \
	if(ctemp == -10) THROWNL(#attrib " cfg attrib not supported"); \
}

#define GET_VIS_ATTRIB(vis, attrib, vtemp) \
{ \
	vtemp = -20; \
	glXGetConfig(dpy, vis, attrib, &vtemp); \
	if(vtemp == -20) THROWNL(#attrib " vis attrib not supported"); \
}

#define COMPARE_ATTRIB(config, vis, attrib, ctemp) \
{ \
	GET_CFG_ATTRIB(config, attrib, ctemp); \
	GET_VIS_ATTRIB(vis, attrib, vtemp); \
	if(ctemp != vtemp) \
		PRERROR5("%s=%d in C%.2x & %d in V%.2x", #attrib, ctemp, \
			cfgid(dpy, config), vtemp, vis ? (unsigned int)vis->visualid : 0); \
}


static int visualClassToGLXVisualType(int visualClass)
{
	switch(visualClass)
	{
		case StaticGray:   return GLX_STATIC_GRAY;
		case GrayScale:    return GLX_GRAY_SCALE;
		case StaticColor:  return GLX_STATIC_COLOR;
		case PseudoColor:  return GLX_PSEUDO_COLOR;
		case TrueColor:    return GLX_TRUE_COLOR;
		case DirectColor:  return GLX_DIRECT_COLOR;
		default:           return GLX_NONE;
	}
}


void configVsVisual(Display *dpy, GLXFBConfig config, XVisualInfo *vis)
{
	int ctemp, vtemp, r, g, b, bs;
	if(!dpy) THROWNL("Invalid display handle");
	if(!config) THROWNL("Invalid FB config");
	if(!vis) THROWNL("Invalid visual pointer");
	GLXFBConfig config0 = glXGetFBConfigFromVisualSGIX(dpy, vis);
	if(cfgid(dpy, config0) != cfgid(dpy, config))
		THROWNL("glXGetFBConfigFromVisualSGIX()");
	GET_CFG_ATTRIB(config, GLX_VISUAL_ID, ctemp);
	if(ctemp != (int)vis->visualid)
		THROWNL("Visual ID mismatch");
	GET_CFG_ATTRIB(config, GLX_RENDER_TYPE, ctemp);
	GET_VIS_ATTRIB(vis, GLX_RGBA, vtemp);
	if((ctemp & GLX_RGBA_BIT) != 0 && vtemp != 1)
		THROWNL("GLX_RGBA mismatch w/ X visual");
	COMPARE_ATTRIB(config, vis, GLX_BUFFER_SIZE, bs);
	COMPARE_ATTRIB(config, vis, GLX_LEVEL, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_DOUBLEBUFFER, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_STEREO, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_AUX_BUFFERS, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_RED_SIZE, r);
	COMPARE_ATTRIB(config, vis, GLX_GREEN_SIZE, g);
	COMPARE_ATTRIB(config, vis, GLX_BLUE_SIZE, b);
	COMPARE_ATTRIB(config, vis, GLX_ALPHA_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_DEPTH_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_STENCIL_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_ACCUM_RED_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_ACCUM_GREEN_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_ACCUM_BLUE_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_ACCUM_ALPHA_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_SAMPLE_BUFFERS_ARB, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_SAMPLES_ARB, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_X_VISUAL_TYPE, ctemp);
	if(visualClassToGLXVisualType(vis->c_class) != ctemp)
		THROWNL("GLX_X_VISUAL_TYPE mismatch w/ X visual");
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_TYPE_EXT, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_INDEX_VALUE_EXT, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_RED_VALUE_EXT, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_GREEN_VALUE_EXT, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_BLUE_VALUE_EXT, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_ALPHA_VALUE_EXT, ctemp);
}


int cfgid(Display *dpy, GLXFBConfig config)
{
	int temp = 0;
	if(!config) THROWNL("config==NULL in cfgid()");
	if(!dpy) THROWNL("display==NULL in cfgid()");
	GET_CFG_ATTRIB(config, GLX_FBCONFIG_ID, temp);
	return temp;
}


void queryContextTest(Display *dpy, XVisualInfo *vis, GLXFBConfig config)
{
	GLXContext ctx = 0;  int fbcid, temp;
	try
	{
		int visual_caveat;
		GET_CFG_ATTRIB(config, GLX_CONFIG_CAVEAT, visual_caveat);
		if(visual_caveat == GLX_NON_CONFORMANT_CONFIG) return;
		fbcid = cfgid(dpy, config);

		if(!(ctx = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, NULL, True)))
			THROWNL("glXCreateNewContext");
		temp = -20;
		glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &temp);
		if(temp != fbcid) THROWNL("glXQueryContext FB cfg ID");
		glXDestroyContext(dpy, ctx);  ctx = 0;

		if(!(ctx = glXCreateContext(dpy, vis, NULL, True)))
			THROWNL("glXCreateNewContext");
		temp = -20;
		glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &temp);
		if(temp != fbcid) THROWNL("glXQueryContext FB cfg ID");
		temp = -20;
		glXQueryContext(dpy, ctx, GLX_RENDER_TYPE, &temp);
		if(temp != GLX_RGBA_TYPE) THROWNL("glXQueryContext render type");
		temp = -20;
		glXQueryContext(dpy, ctx, GLX_SCREEN, &temp);
		if(temp != DefaultScreen(dpy)) THROWNL("glXQueryContext screen");

		if(GLX_EXTENSION_EXISTS(GLX_EXT_import_context))
		{
			temp = -20;
			glXQueryContextInfoEXT(dpy, ctx, GLX_SCREEN_EXT, &temp);
			if(temp != DefaultScreen(dpy)) THROWNL("glXQueryContextInfoEXT screen");
			temp = -20;
			glXQueryContextInfoEXT(dpy, ctx, GLX_VISUAL_ID_EXT, &temp);
			if((VisualID)temp != vis->visualid)
				THROWNL("glXQueryContextInfoEXT visual ID");
		}

		glXDestroyContext(dpy, ctx);
	}
	catch(...)
	{
		if(ctx) glXDestroyContext(dpy, ctx);
		throw;
	}
}


GLXFBConfig getFBConfigFromVisual(Display *dpy, XVisualInfo *vis)
{
	GLXContext ctx = 0;  int temp, fbcid = 0, n = 0;
	GLXFBConfig *configs = NULL, config = 0;
	try
	{
		if(!(ctx = glXCreateContext(dpy, vis, NULL, True)))
			THROWNL("glXCreateNewContext");
		glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &fbcid);
		glXDestroyContext(dpy, ctx);  ctx = 0;
		if(!(configs = glXGetFBConfigs(dpy, DefaultScreen(dpy), &n)) || n == 0)
			THROWNL("Cannot map visual to FB config");
		for(int i = 0; i < n; i++)
		{
			temp = cfgid(dpy, configs[i]);
			if(temp == fbcid) { config = configs[i];  break; }
		}
		XFree(configs);  configs = NULL;
		if(!config) THROWNL("Cannot map visual to FB config");
		return config;
	}
	catch(...)
	{
		if(ctx) glXDestroyContext(dpy, ctx);
		if(configs) XFree(configs);
		throw;
	}
}


#define TEST_ATTRIB(vattribs, cattribs, desc) \
	vis0 = glXChooseVisual(dpy, DefaultScreen(dpy), vattribs); \
	if(vis0) THROW(desc " visual found"); \
	XFree(vis0);  vis0 = NULL; \
	configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), cattribs, &n); \
	if(configs) THROW(desc " FB config found"); \
	XFree(configs);  configs = NULL; \

// This tests the faker's client/server visual matching heuristics
int visTest(void)
{
	Display *dpy = NULL;
	XVisualInfo **visuals = NULL, *vis0 = NULL, vtemp;
	GLXFBConfig config = 0, *configs = NULL;  int n = 0, i, retval = 1;

	printf("Visual matching heuristics test\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");

		// This will fail with VGL 2.2.x and earlier
		if(!(configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), NULL, &n))
			|| n == 0)
			THROW("No FB configs found");
		XFree(configs);  configs = NULL;

		// These should fail, since we no longer support transparent overlays.
		int attribs0[] = { GLX_LEVEL, 1, None };
		TEST_ATTRIB(attribs0, attribs0, "Overlay")
		int vattribs1[] = { None };
		int cattribs1[] = { GLX_RENDER_TYPE, GLX_COLOR_INDEX_BIT, None };
		TEST_ATTRIB(vattribs1, cattribs1, "CI")
		int vattribs2[] = { GLX_RGBA, GLX_TRANSPARENT_TYPE, GLX_TRANSPARENT_RGB,
			None };
		int cattribs2[] = { GLX_TRANSPARENT_TYPE, GLX_TRANSPARENT_RGB, None };
		TEST_ATTRIB(vattribs2, cattribs2, "Transparent")

		try
		{
			printf("GLX attributes:         ");

			// Iterate through RGBA attributes
			int rgbattrib[] = { GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, 0, GLX_DEPTH_SIZE, 0, GLX_AUX_BUFFERS, 0,
				GLX_STENCIL_SIZE, 0, GLX_ACCUM_RED_SIZE, 0, GLX_ACCUM_GREEN_SIZE, 0,
				GLX_ACCUM_BLUE_SIZE, 0, GLX_ACCUM_ALPHA_SIZE, 0, GLX_SAMPLE_BUFFERS, 0,
				GLX_SAMPLES, 1, GLX_RGBA, GLX_X_VISUAL_TYPE, GLX_NONE, None, None,
				None };
			int rgbattrib13[] = { GLX_LEVEL, 0, GLX_DOUBLEBUFFER, 1, GLX_STEREO, 1,
				GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, 0, GLX_DEPTH_SIZE, 0, GLX_AUX_BUFFERS, 0,
				GLX_STENCIL_SIZE, 0, GLX_ACCUM_RED_SIZE, 0, GLX_ACCUM_GREEN_SIZE, 0,
				GLX_ACCUM_BLUE_SIZE, 0, GLX_ACCUM_ALPHA_SIZE, 0, GLX_SAMPLE_BUFFERS, 0,
				GLX_SAMPLES, 1, GLX_X_VISUAL_TYPE, GLX_NONE, None };
			unsigned int visualTypes[3] = { GLX_TRUE_COLOR, GLX_DIRECT_COLOR,
				GLX_DONT_CARE };
			if(DefaultDepth(dpy, DefaultScreen(dpy)) == 30)
			{
				rgbattrib[1] = rgbattrib[3] = rgbattrib[5] = 10;
				rgbattrib13[7] = rgbattrib13[9] = rgbattrib13[11] = 10;
			}

			for(int db = 0; db <= 1; db++)
			{
				rgbattrib13[0] = db ? GLX_TRANSPARENT_TYPE : GLX_LEVEL;
				rgbattrib13[1] = db ? GLX_NONE : 0;

				rgbattrib13[3] = db;
				rgbattrib[29] = db ? GLX_DOUBLEBUFFER : 0;

				for(int stereo = 0; stereo <= 1; stereo++)
				{
					rgbattrib13[5] = stereo;
					rgbattrib[db ? 30 : 29] = stereo ? GLX_STEREO : 0;

					for(int alpha = 0; alpha <= 1; alpha++)
					{
						rgbattrib13[13] = rgbattrib[7] = alpha;

						for(int depth = 0; depth <= 1; depth++)
						{
							rgbattrib13[15] = rgbattrib[9] = depth;

							for(int aux = 0; aux <= 1; aux++)
							{
								rgbattrib13[17] = rgbattrib[11] = aux;

								for(int stencil = 0; stencil <= 1; stencil++)
								{
									rgbattrib13[19] = rgbattrib[13] = stencil;

									for(int accum = 0; accum <= 1; accum++)
									{
										rgbattrib13[21] = rgbattrib13[23] =
											rgbattrib13[25] = accum;
										rgbattrib[15] = rgbattrib[17] = rgbattrib[19] = accum;
										if(alpha) { rgbattrib13[27] = rgbattrib[21] = accum; }
										else { rgbattrib13[27] = rgbattrib[21] = 0; }

										for(int samples = 0; samples <= 16;
											samples == 0 ? samples = 1 : samples *= 2)
										{
											rgbattrib13[31] = rgbattrib[25] = samples;
											rgbattrib13[29] = rgbattrib[23] = samples ? 1 : 0;

											for(int visualTypeIndex = 0; visualTypeIndex < 3;
												visualTypeIndex++)
											{
												rgbattrib13[33] = rgbattrib[28] =
													visualTypes[visualTypeIndex];
												if((!(configs = glXChooseFBConfig(dpy,
													DefaultScreen(dpy), rgbattrib13, &n)) || n == 0)
													&& !stereo && !samples && !aux && !accum
													&& visualTypeIndex != 1)
													THROW("No FB configs found");
												if(!(vis0 = glXChooseVisual(dpy, DefaultScreen(dpy),
													rgbattrib)) && !stereo && !samples && !aux && !accum
													&& visualTypeIndex != 1)
													THROW("Could not find visual");
												if(vis0 && configs)
													configVsVisual(dpy, configs[0], vis0);
												if(vis0) XFree(vis0);
												if(configs) XFree(configs);
											}
										}
									}
								}
							}
						}
					}
				}
			}
			printf("SUCCESS!\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		printf("FB configs->X visuals:  ");

		if(!(configs = glXGetFBConfigs(dpy, DefaultScreen(dpy), &n)) || n == 0)
			THROW("No FB configs found");

		int fbcid = 0;
		if(!(visuals = (XVisualInfo **)malloc(sizeof(XVisualInfo *) * n)))
			THROW("Memory allocation error");
		memset(visuals, 0, sizeof(XVisualInfo *) * n);

		int numFloatCfgs = 0, numNVFloatCfgs = 0;

		for(i = 0; i < n; i++)
		{
			if(!configs[i]) continue;
			try
			{
				int drawableType, renderType, transparentType, visualID, visualType,
					xRenderable, floatComponents = -1;
				fbcid = cfgid(dpy, configs[i]);
				// The old fglrx driver unfortunately assigns the same FB config ID to
				// multiple FB configs with different attributes, some of which support
				// X rendering and some of which don't.  Thus, we skip the validation
				// of visual matching for duplicate FB config IDs, since VirtualGL is
				// likely to hash those incorrectly.
				if(i > 0 && cfgid(dpy, configs[i]) == cfgid(dpy, configs[i - 1]))
					continue;
				GET_CFG_ATTRIB(configs[i], GLX_DRAWABLE_TYPE, drawableType);
				GET_CFG_ATTRIB(configs[i], GLX_RENDER_TYPE, renderType);
				if(renderType & GLX_RGBA_FLOAT_BIT_ARB)
				{
					if(!GLX_EXTENSION_EXISTS(GLX_ARB_fbconfig_float))
						THROWNL("GLX_ARB_fbconfig_float not advertised");
					numFloatCfgs++;
				}
				GET_CFG_ATTRIB(configs[i], GLX_TRANSPARENT_TYPE, transparentType);
				GET_CFG_ATTRIB(configs[i], GLX_VISUAL_ID, visualID);
				GET_CFG_ATTRIB(configs[i], GLX_X_VISUAL_TYPE, visualType);
				GET_CFG_ATTRIB(configs[i], GLX_X_RENDERABLE, xRenderable);
				glXGetFBConfigAttrib(dpy, configs[i], GLX_FLOAT_COMPONENTS_NV,
					&floatComponents);
				if(floatComponents > 0)
				{
					if(!GLX_EXTENSION_EXISTS(GLX_NV_float_buffer))
						THROWNL("GLX_NV_float_buffer not advertised");
					numNVFloatCfgs++;
				}
				visuals[i] = glXGetVisualFromFBConfig(dpy, configs[i]);
				// VirtualGL should return a visual only for opaque GLXFBConfigs that
				// support X rendering.
				bool hasVis = (visuals[i] != NULL);
				bool shouldHaveVis = (drawableType & GLX_WINDOW_BIT) != 0
					&& (renderType & GLX_RGBA_BIT) != 0 && transparentType == GLX_NONE
					&& visualID != 0 && visualType != GLX_NONE && xRenderable != 0;
				if(hasVis != shouldHaveVis)
					PRERROR1(hasVis ?
						"CFG 0x%.2x shouldn't have matching X visual but does" :
						"No matching X visual for CFG 0x%.2x", fbcid);
			}
			catch(std::exception &e)
			{
				printf("Failed! (%s)\n", e.what());  retval = 0;
			}
		}

		if(numFloatCfgs)
		{
			int nc = 0, glxattribs[] = { GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
				GLX_RENDER_TYPE, GLX_RGBA_FLOAT_BIT_ARB, None };
			GLXFBConfig *fbconfigs =
				glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattribs, &nc);
			if(fbconfigs) XFree(fbconfigs);
			if(nc != numFloatCfgs)
				THROWNL("Wrong number of FB configs with GLX_RGBA_FLOAT_BIT");
		}

		if(numNVFloatCfgs)
		{
			int nc = 0, glxattribs[] = { GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
				GLX_FLOAT_COMPONENTS_NV, 1, None };
			GLXFBConfig *fbconfigs =
				glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattribs, &nc);
			if(fbconfigs) XFree(fbconfigs);
			if(nc != numNVFloatCfgs)
				THROWNL("Wrong number of FB configs with GLX_FLOAT_COMPONENTS_NV");
		}

		XVisualInfo *vis1 = NULL;
		try
		{
			for(i = 0; i < n; i++)
			{
				if(!configs[i]) continue;
				fbcid = cfgid(dpy, configs[i]);
				if(!visuals[i]) continue;

				if(!(vis1 = glXGetVisualFromFBConfig(dpy, configs[i])))
					PRERROR1("No matching X visual for CFG 0x%.2x", fbcid);

				configVsVisual(dpy, configs[i], visuals[i]);
				configVsVisual(dpy, configs[i], vis1);
				queryContextTest(dpy, visuals[i], configs[i]);
				queryContextTest(dpy, vis1, configs[i]);

				config = getFBConfigFromVisual(dpy, visuals[i]);
				if(!config || cfgid(dpy, config) != fbcid)
					PRERROR1("CFG 0x%.2x: getFBConfigFromVisual()", fbcid);
				config = getFBConfigFromVisual(dpy, vis1);
				if(!config || cfgid(dpy, config) != fbcid)
					PRERROR1("CFG 0x%.2x: getFBConfigFromVisual()", fbcid);

				XFree(vis1);  vis1 = NULL;
			}
			printf("SUCCESS!\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		if(vis1) { XFree(vis1);  vis1 = NULL; }

		XFree(configs);  configs = NULL;
		for(i = 0; i < n; i++) { if(visuals[i]) XFree(visuals[i]); }
		free(visuals);  visuals = NULL;  n = 0;
		fflush(stdout);

		printf("X visuals->FB configs:  ");

		vtemp.screen = DefaultScreen(dpy);
		if(!(vis0 = XGetVisualInfo(dpy, VisualScreenMask, &vtemp, &n)) || n == 0)
			THROW("No X Visuals found");

		XVisualInfo *vis2 = NULL;
		try
		{
			for(i = 0; i < n; i++)
			{
				int level = 0, useGL = 1;
				glXGetConfig(dpy, &vis0[i], GLX_LEVEL, &level);
				if(level) continue;
				glXGetConfig(dpy, &vis0[i], GLX_USE_GL, &useGL);
				if(!useGL) continue;

				if(!(config = getFBConfigFromVisual(dpy, &vis0[i])))
					PRERROR1("No matching CFG for X Visual 0x%.2x",
						(int)vis0[i].visualid);
				configVsVisual(dpy, config, &vis0[i]);

				vis2 = glXGetVisualFromFBConfig(dpy, config);
				configVsVisual(dpy, config, vis2);
				XFree(vis2);  vis2 = NULL;
			}
			printf("SUCCESS!\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		if(vis2) XFree(vis2);
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	fflush(stdout);

	if(visuals && n)
	{
		for(i = 0; i < n; i++) { if(visuals[i]) XFree(visuals[i]); }
		free(visuals);
	}
	if(vis0) XFree(vis0);
	if(configs) XFree(configs);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


#define DEFTHREADS  30
#define MAXTHREADS  100
bool deadYet = false;

class TestThread : public Runnable
{
	public:

		TestThread(int myRank_, Display *dpy_, Window win_, GLXContext ctx_) :
			myRank(myRank_), dpy(dpy_), win(win_), ctx(ctx_), doResize(false)
		{
			#ifdef USEHELGRIND
			ANNOTATE_BENIGN_RACE_SIZED(&doResize, sizeof(bool), );
			ANNOTATE_BENIGN_RACE_SIZED(&width, sizeof(int), );
			ANNOTATE_BENIGN_RACE_SIZED(&height, sizeof(int), );
			#endif
		}

		void run(void)
		{
			int clr = myRank % NC, lastFrame = 0;
			bool seenResize = false;

			if(!(glXMakeContextCurrent(dpy, win, win, ctx)))
				THROWNL("Could not make context current");
			int iter = 0;
			while(!deadYet || !seenResize || iter < 3)
			{
				if(doResize)
				{
					glViewport(0, 0, width, height);
					doResize = false;
					seenResize = true;
				}
				glClearColor(colors[clr].r, colors[clr].g, colors[clr].b, 0.);
				glClear(GL_COLOR_BUFFER_BIT);
				glReadBuffer(GL_FRONT);
				glXSwapBuffers(dpy, win);
				CHECK_GL_ERROR();
				checkBufferState(GL_BACK, GL_FRONT, dpy, win, win, ctx);
				checkFrame(dpy, win, 1, lastFrame);
				checkWindowColor(dpy, win, colors[clr].bits, false);
				clr = (clr + 1) % NC;
				iter++;
			}
		}

		void resize(int width_, int height_)
		{
			width = width_;  height = height_;
			doResize = true;
		}

	private:

		int myRank;
		Display *dpy;
		Window win;
		GLXContext ctx;
		bool doResize;
		int width, height;
};


int multiThreadTest(int nThreads)
{
	int glxattribs[] = { GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	XVisualInfo *vis = NULL;
	Display *dpy = NULL;  Window windows[MAXTHREADS];
	GLXContext contexts[MAXTHREADS];
	TestThread *testThreads[MAXTHREADS];  Thread *threads[MAXTHREADS];
	XSetWindowAttributes swa;
	int i, retval = 1;

	if(nThreads == 0) return 1;
	for(i = 0; i < nThreads; i++)
	{
		windows[i] = 0;  contexts[i] = 0;  testThreads[i] = NULL;
		threads[i] = NULL;
	}
	#ifdef USEHELGRIND
	ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
	#endif

	printf("Multithreaded rendering test (%d threads)\n\n", nThreads);

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");

		if((vis = glXChooseVisual(dpy, DefaultScreen(dpy), glxattribs)) == NULL)
			THROW("Could not find a suitable visual");
		Window root = RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel = 0;
		swa.event_mask = StructureNotifyMask | ExposureMask;
		for(i = 0; i < nThreads; i++)
		{
			int winX = (i % 10) * 100, winY = (i / 10) * 120;
			if((windows[i] = XCreateWindow(dpy, root, winX, winY, 100, 100, 0,
				vis->depth, InputOutput, vis->visual,
				CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
				THROW("Could not create window");
			XMapWindow(dpy, windows[i]);
			if(!(contexts[i] = glXCreateContext(dpy, vis, NULL, True)))
				THROW("Could not establish GLX context");
			XMoveResizeWindow(dpy, windows[i], winX, winY, 100, 100);
		}
		XSync(dpy, False);
		XFree(vis);  vis = NULL;

		for(i = 0; i < nThreads; i++)
		{
			testThreads[i] = new TestThread(i, dpy, windows[i], contexts[i]);
			threads[i] = new Thread(testThreads[i]);
			if(!testThreads[i] || !threads[i])
				PRERROR1("Could not create thread %d", i);
			threads[i]->start();
		}
		printf("Phase 1\n");
		for(i = 0; i < nThreads; i++)
		{
			int winX = (i % 10) * 100, winY = i / 10 * 120;
			XMoveResizeWindow(dpy, windows[i], winX, winY, 200, 200);
			testThreads[i]->resize(200, 200);
			if(i < 5) usleep(0);
			XResizeWindow(dpy, windows[i], 100, 100);
			testThreads[i]->resize(100, 100);
		}
		XSync(dpy, False);
		fflush(stdout);

		printf("Phase 2\n");
		for(i = 0; i < nThreads; i++)
		{
			XWindowChanges xwc;
			xwc.width = xwc.height = 200;
			XConfigureWindow(dpy, windows[i], CWWidth | CWHeight, &xwc);
			testThreads[i]->resize(200, 200);
		}
		XSync(dpy, False);
		fflush(stdout);

		printf("Phase 3\n");
		for(i = 0; i < nThreads; i++)
		{
			XResizeWindow(dpy, windows[i], 100, 100);
			testThreads[i]->resize(100, 100);
		}
		XSync(dpy, False);
		deadYet = true;
		for(i = 0; i < nThreads; i++) threads[i]->stop();
		for(i = 0; i < nThreads; i++)
		{
			try
			{
				threads[i]->checkError();
			}
			catch(std::exception &e)
			{
				printf("Thread %d failed! (%s)\n", i, e.what());  retval = 0;
			}
		}
		if(retval == 1) printf("SUCCESS!\n");
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	fflush(stdout);

	for(i = 0; i < nThreads; i++) delete threads[i];
	for(i = 0; i < nThreads; i++) delete testThreads[i];
	if(vis) XFree(vis);
	for(i = 0; i < nThreads; i++)
	{
		if(dpy && contexts[i])
		{
			glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, contexts[i]);
		}
	}
	for(i = 0; i < nThreads; i++)
	{
		if(dpy && windows[i]) XDestroyWindow(dpy, windows[i]);
	}
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


#define COMPARE_DRAW_ATTRIB(dpy, draw, value, attrib) \
{ \
	if(value >= 0) \
	{ \
		unsigned int temp = 0xffffffff; \
		glXQueryDrawable(dpy, draw, attrib, &temp); \
		if(temp == 0xffffffff) \
			THROW(#attrib " attribute not supported"); \
		if(temp != (unsigned int)value) \
			PRERROR3("%s=%d (should be %d)", #attrib, temp, value); \
	} \
}

void checkDrawable(Display *dpy, GLXDrawable draw, int width, int height,
	int preservedContents, int largestPbuffer, int fbcid)
{
	if(!dpy || !draw) THROW("Invalid argument to checkdrawable()");
	COMPARE_DRAW_ATTRIB(dpy, draw, width, GLX_WIDTH);
	COMPARE_DRAW_ATTRIB(dpy, draw, height, GLX_HEIGHT);
	COMPARE_DRAW_ATTRIB(dpy, draw, preservedContents, GLX_PRESERVED_CONTENTS);
	COMPARE_DRAW_ATTRIB(dpy, draw, largestPbuffer, GLX_LARGEST_PBUFFER);
	COMPARE_DRAW_ATTRIB(dpy, draw, fbcid, GLX_FBCONFIG_ID);
}

// Test off-screen rendering
int offScreenTest(bool dbPixmap, bool doUseXFont, bool doSelectEvent)
{
	Display *dpy = NULL;  Window win = 0;  Pixmap pm0 = 0, pm1 = 0, pm2 = 0;
	GLXPixmap glxpm0 = 0, glxpm1 = 0;  GLXPbuffer pb = 0;  GLXWindow glxwin = 0;
	int dpyw, dpyh, lastFrame = 0, retval = 1;
	int glxattribs[] = { GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT | GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_STENCIL_SIZE, 0,
		None };
	XVisualInfo *vis = NULL;  GLXFBConfig config = 0, *configs = NULL;
	int n = 0;
	GLXContext ctx = 0, ctx2 = 0;
	XSetWindowAttributes swa;
	XFontStruct *fontInfo = NULL;  int minChar = 0, maxChar = 0;
	int fontListBase = 0;
	GLuint fbo = 0, rbo = 0;
	TestColor clr(0);

	printf("Off-screen rendering test\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
		dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

		if(doUseXFont)
		{
			if(!(fontInfo = XLoadQueryFont(dpy, "fixed")))
				THROW("Could not load X font");
			minChar = fontInfo->min_char_or_byte2;
			maxChar = fontInfo->max_char_or_byte2;
		}

		if((configs = glXChooseFBConfigSGIX(dpy, DefaultScreen(dpy), glxattribs,
			&n)) == NULL || n == 0)
			THROW("Could not find a suitable FB config");
		config = configs[0];
		int fbcid = cfgid(dpy, config);
		XFree(configs);  configs = NULL;
		if((vis = glXGetVisualFromFBConfigSGIX(dpy, config)) == NULL)
			THROW("Could not find matching visual for FB config");

		Window root = RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel = 0;
		swa.background_pixel = 0;
		swa.event_mask = 0;
		if((win = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0, vis->depth,
			InputOutput, vis->visual, CWBorderPixel | CWColormap | CWEventMask,
			&swa)) == 0)
			THROW("Could not create window");
		XMapWindow(dpy, win);
		if((glxwin = glXCreateWindow(dpy, config, win, NULL)) == 0)
			THROW("Could not create GLX window");
		checkDrawable(dpy, glxwin, dpyw / 2, dpyh / 2, -1, -1, fbcid);

		if((pm0 = XCreatePixmap(dpy, win, dpyw / 2, dpyh / 2, vis->depth)) == 0
			|| (pm1 = XCreatePixmap(dpy, win, dpyw / 2, dpyh / 2, vis->depth)) == 0
			|| (pm2 = XCreatePixmap(dpy, win, dpyw / 2, dpyh / 2, vis->depth)) == 0)
			THROW("Could not create pixmap");
		if((glxpm0 = glXCreateGLXPixmap(dpy, vis, pm0)) == 0
			|| (glxpm1 = glXCreatePixmap(dpy, config, pm1, NULL)) == 0)
			THROW("Could not create GLX pixmap");
		checkDrawable(dpy, glxpm0, dpyw / 2, dpyh / 2, -1, -1, fbcid);
		checkDrawable(dpy, glxpm1, dpyw / 2, dpyh / 2, -1, -1, fbcid);

		int pbattribs[] = { GLX_PBUFFER_WIDTH, dpyw / 2,
			GLX_PBUFFER_HEIGHT, dpyh / 2, GLX_PRESERVED_CONTENTS, True,
			GLX_LARGEST_PBUFFER, False, None };
		if((pb = glXCreatePbuffer(dpy, config, pbattribs)) == 0)
			THROW("Could not create Pbuffer");
		checkDrawable(dpy, pb, dpyw / 2, dpyh / 2, 1, 0, fbcid);
		unsigned int tempw = 0, temph = 0;
		PFNGLXQUERYGLXPBUFFERSGIXPROC __glXQueryGLXPbufferSGIX =
			(PFNGLXQUERYGLXPBUFFERSGIXPROC)glXGetProcAddress(
				(const GLubyte *)"glXQueryGLXPbufferSGIX");
		if(!__glXQueryGLXPbufferSGIX)
			THROW("GLX_SGIX_pbuffer does not work");
		__glXQueryGLXPbufferSGIX(dpy, pb, GLX_WIDTH_SGIX, &tempw);
		glXQueryDrawable(dpy, pb, GLX_HEIGHT, &temph);

		if(tempw != (unsigned int)dpyw / 2 || temph != (unsigned int)dpyh / 2)
			THROW("Could not query context");

		if(!(ctx = glXCreateContextWithConfigSGIX(dpy, config, GLX_RGBA_TYPE, NULL,
			True)))
			THROW("Could not create context");

		if(!glXMakeContextCurrent(dpy, glxwin, glxwin, ctx))
			THROW("Could not make context current");
		checkCurrent(dpy, glxwin, glxwin, ctx, dpyw / 2, dpyh / 2);
		if(!doubleBufferTest())
			THROW("Double buffering appears to be broken");

		if(!glXMakeContextCurrent(dpy, pb, pb, ctx))
			THROW("Could not make context current");
		checkCurrent(dpy, pb, pb, ctx, dpyw / 2, dpyh / 2);
		if(!doubleBufferTest())
			THROW("Double-buffered off-screen rendering not available");
		checkFrame(dpy, win, -1, lastFrame);

		if(doSelectEvent)
		{
			// In most modern implementations, glXSelectEvent() doesn't do anything
			// meaningful.  This just verifies that glXGetSelectedEvent() is
			// returning the correct mask.
			unsigned long mask = 1;
			glXGetSelectedEvent(dpy, glxwin, &mask);
			if(mask) THROW("glXGetSelectedEvent()");
			mask = 1;
			glXGetSelectedEvent(dpy, pb, &mask);
			if(mask) THROW("glXGetSelectedEvent()");
			glXSelectEvent(dpy, glxwin, GLX_PBUFFER_CLOBBER_MASK);
			glXSelectEvent(dpy, pb, GLX_PBUFFER_CLOBBER_MASK);
			glXGetSelectedEvent(dpy, glxwin, &mask);
			if(mask != GLX_PBUFFER_CLOBBER_MASK) THROW("glXGetSelectedEvent()");
			mask = 0;
			glXGetSelectedEvent(dpy, pb, &mask);
			if(mask != GLX_PBUFFER_CLOBBER_MASK) THROW("glXGetSelectedEvent()");
			glXSelectEvent(dpy, glxwin, 0);
			glXSelectEvent(dpy, pb, 0);
		}

		try
		{
			printf("Pbuffer->Window:                ");
			if(!(glXMakeContextCurrent(dpy, pb, pb, ctx)))
				THROWNL("Could not make context current");
			checkCurrent(dpy, pb, pb, ctx, dpyw / 2, dpyh / 2);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "PB");
			if(!(glXMakeContextCurrent(dpy, glxwin, pb, ctx)))
				THROWNL("Could not make context current");
			CHECK_GL_ERROR();
			checkCurrent(dpy, glxwin, pb, ctx, dpyw / 2, dpyh / 2);
			glReadBuffer(GL_BACK);  glDrawBuffer(GL_BACK);
			glCopyPixels(0, 0, dpyw / 2, dpyh / 2, GL_COLOR);
			glReadBuffer(GL_FRONT);
			glXSwapBuffers(dpy, glxwin);
			checkFrame(dpy, win, 1, lastFrame);
			checkBufferState(GL_BACK, GL_FRONT, dpy, glxwin, pb, ctx);
			checkWindowColor(dpy, win, clr.bits(-2), false);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("Window->Pbuffer:                ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				THROWNL("Could not make context current");
			CHECK_GL_ERROR();
			if(doUseXFont)
			{
				fontListBase = glGenLists(maxChar + 1);
				glXUseXFont(fontInfo->fid, minChar, maxChar - minChar + 1,
					fontListBase + minChar);
			}
			checkCurrent(dpy, glxwin, glxwin, ctx, dpyw / 2, dpyh / 2);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "Win");
			if(!(glXMakeContextCurrent(dpy, pb, glxwin, ctx)))
				THROWNL("Could not make context current");
			CHECK_GL_ERROR();
			if(doUseXFont)
			{
				fontListBase = glGenLists(maxChar + 1);
				glXUseXFont(fontInfo->fid, minChar, maxChar - minChar + 1,
					fontListBase + minChar);
			}
			checkCurrent(dpy, pb, glxwin, ctx, dpyw / 2, dpyh / 2);
			checkFrame(dpy, win, 1, lastFrame);
			glReadBuffer(GL_BACK);  glDrawBuffer(GL_BACK);
			glCopyPixels(0, 0, dpyw / 2, dpyh / 2, GL_COLOR);
			glXSwapBuffers(dpy, pb);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "PB");

			// Verify that the EGL back end manages FBO bindings properly when
			// attaching a context to an OpenGL window, destroying the window, then
			// attaching the same context to a different drawable.  This will fail
			// when using the EGL back end with VirtualGL 3.0.
			Window win2;
			if((win2 = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0,
				vis->depth, InputOutput, vis->visual,
				CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
				THROW("Could not create window");
			XMapWindow(dpy, win2);
			glXMakeCurrent(dpy, win2, ctx);
			glXMakeCurrent(dpy, 0, 0);
			XDestroyWindow(dpy, win2);
			glXMakeContextCurrent(dpy, pb, pb, ctx);
			CHECK_GL_ERROR();

			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			printf("FBO->Window:                    ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				THROWNL("Could not make context current");
			// The following command will fail unless the implementation of
			// glXMake*Current() in the EGL back end restores the context's previous
			// read buffer state for the default framebuffer.  The multithreaded
			// rendering test will fail unless the implementation of
			// glXMake*Current() in the EGL back end restores the context's previous
			// draw buffer state for the default framebuffer.
			VERIFY_BUF_COLOR(0, clr.bits(-2), "Win");
			CHECK_GL_ERROR();
			checkCurrent(dpy, glxwin, glxwin, ctx, dpyw / 2, dpyh / 2);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "Win");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				THROWNL("Could not make context current");
			CHECK_GL_ERROR();
			checkCurrent(dpy, glxwin, glxwin, ctx, dpyw / 2, dpyh / 2);
			glDrawBuffer(GL_FRONT_AND_BACK);
			glReadBuffer(GL_FRONT);
			glGenFramebuffersEXT(1, &fbo);
			glGenRenderbuffersEXT(1, &rbo);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbo);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, dpyw / 2,
				dpyh / 2);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
			VERIFY_FBO(fbo, GL_COLOR_ATTACHMENT0_EXT, GL_NONE, fbo,
				GL_COLOR_ATTACHMENT0_EXT);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
				GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rbo);
			clr.clear(0);
			VERIFY_BUF_COLOR(0, clr.bits(-1), "FBO");
			if(!glXMakeContextCurrent(dpy, pb, pb, ctx))
				THROW("Could not make context current");
			checkCurrent(dpy, pb, pb, ctx, dpyw / 2, dpyh / 2);
			VERIFY_FBO(fbo, GL_COLOR_ATTACHMENT0_EXT, GL_NONE, fbo,
				GL_COLOR_ATTACHMENT0_EXT);
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				THROWNL("Could not make context current");
			checkCurrent(dpy, glxwin, glxwin, ctx, dpyw / 2, dpyh / 2);
			VERIFY_FBO(fbo, GL_COLOR_ATTACHMENT0_EXT, GL_NONE, fbo,
				GL_COLOR_ATTACHMENT0_EXT);
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
			VERIFY_FBO(0, GL_FRONT_AND_BACK, GL_NONE, fbo, GL_COLOR_ATTACHMENT0_EXT);
			glDrawBuffer(GL_BACK);
			glXSwapBuffers(dpy, glxwin);
			checkFrame(dpy, win, 1, lastFrame);
			checkBufferState(GL_BACK, GL_COLOR_ATTACHMENT0_EXT, dpy, glxwin, glxwin,
				ctx);
			checkWindowColor(dpy, win, clr.bits(-3), false);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_FRONT);
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				THROWNL("Could not make context current");
			VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_FRONT);
			clr.clear(GL_FRONT);
			clr.clear(GL_BACK);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
			VERIFY_FBO(fbo, GL_COLOR_ATTACHMENT0_EXT, GL_NONE, fbo,
				GL_COLOR_ATTACHMENT0_EXT);
			VERIFY_BUF_COLOR(0, clr.bits(-3), "FBO");
			glXSwapBuffers(dpy, glxwin);
			checkFrame(dpy, win, 1, lastFrame);
			checkWindowColor(dpy, win, clr.bits(-1), false);
			glDeleteRenderbuffersEXT(1, &rbo);  rbo = 0;
			glDeleteFramebuffersEXT(1, &fbo);  fbo = 0;
			VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "Win");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				THROWNL("Could not make context current");
			VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_FRONT);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		if(rbo) { glDeleteRenderbuffersEXT(1, &rbo);  rbo = 0; }
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		if(fbo) { glDeleteFramebuffersEXT(1, &fbo);  fbo = 0; }

		try
		{
			GLenum expectedBuf = dbPixmap ? GL_BACK : GL_FRONT;

			printf("GLX Pixmap->Window:             ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				THROWNL("Could not make context current");
			CHECK_GL_ERROR();
			if(doUseXFont)
			{
				fontListBase = glGenLists(maxChar + 1);
				glXUseXFont(fontInfo->fid, minChar, maxChar - minChar + 1,
					fontListBase + minChar);
			}
			checkCurrent(dpy, glxpm0, glxpm0, ctx, dpyw / 2, dpyh / 2);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, win, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw / 2, dpyh / 2, 0, 0);
			checkBufferState(expectedBuf, expectedBuf, dpy, glxpm0, glxpm0, ctx);
			checkFrame(dpy, win, 1, lastFrame);
			checkWindowColor(dpy, win, clr.bits(-1), false);
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			GLenum expectedBuf = dbPixmap ? GL_BACK : GL_FRONT;

			printf("Window->GLX Pixmap:             ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				THROWNL("Could not make context current");
			if(doUseXFont)
			{
				fontListBase = glGenLists(maxChar + 1);
				glXUseXFont(fontInfo->fid, minChar, maxChar - minChar + 1,
					fontListBase + minChar);
			}
			checkCurrent(dpy, glxwin, glxwin, ctx, dpyw / 2, dpyh / 2);
			clr.clear(GL_FRONT);
			if(dbPixmap) clr.clear(GL_BACK);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(dbPixmap ? -2 : -1), "Win");
			if(!(glXMakeContextCurrent(dpy, glxpm1, glxpm1, ctx)))
				THROWNL("Could not make context current");
			checkCurrent(dpy, glxpm1, glxpm1, ctx, dpyw / 2, dpyh / 2);
			checkFrame(dpy, win, 1, lastFrame);
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, win, pm1, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw / 2, dpyh / 2, 0, 0);
			checkBufferState(expectedBuf, expectedBuf, dpy, glxpm1, glxpm1, ctx);
			checkFrame(dpy, win, 0, lastFrame);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(dbPixmap ? -2 : -1), "PM1");
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(dbPixmap ? -2 : -1), "PM1");
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			GLenum expectedBuf = dbPixmap ? GL_BACK : GL_FRONT;

			printf("GLX Pixmap->GLX Pixmap:         ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				THROWNL("Could not make context current");
			if(doUseXFont)
			{
				fontListBase = glGenLists(maxChar + 1);
				glXUseXFont(fontInfo->fid, minChar, maxChar - minChar + 1,
					fontListBase + minChar);
				XFreeFont(dpy, fontInfo);  fontInfo = NULL;
			}
			checkCurrent(dpy, glxpm0, glxpm0, ctx, dpyw / 2, dpyh / 2);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM0");
			if(!(glXMakeContextCurrent(dpy, glxpm1, glxpm1, ctx)))
				THROWNL("Could not make context current");
			checkCurrent(dpy, glxpm1, glxpm1, ctx, dpyw / 2, dpyh / 2);
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, pm1, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw / 2, dpyh / 2, 0, 0);
			checkBufferState(expectedBuf, expectedBuf, dpy, glxpm1, glxpm1, ctx);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-1), "PM1");
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM1");
			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			GLenum expectedBuf = dbPixmap ? GL_BACK : GL_FRONT;

			printf("GLX Pixmap->2D Pixmap:          ");
			lastFrame = 0;
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				THROWNL("Could not make context current");
			checkCurrent(dpy, glxpm0, glxpm0, ctx, dpyw / 2, dpyh / 2);

			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, pm2, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw / 2, dpyh / 2, 0, 0);
			checkBufferState(expectedBuf, expectedBuf, dpy, glxpm0, glxpm0, ctx);
			checkFrame(dpy, pm0, 1, lastFrame);
			checkWindowColor(dpy, pm0, clr.bits(-1), false);

			clr.clear(GL_FRONT);
			if(dbPixmap) clr.clear(GL_BACK);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(dbPixmap ? -2 : -1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XImage *xi = XGetImage(dpy, pm0, 0, 0, dpyw / 2, dpyh / 2, AllPlanes,
				ZPixmap);
			if(xi) XDestroyImage(xi);
			checkBufferState(expectedBuf, expectedBuf, dpy, glxpm0, glxpm0, ctx);
			checkFrame(dpy, pm0, 1, lastFrame);
			checkWindowColor(dpy, pm0, clr.bits(dbPixmap ? -2 : -1), false);

			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

		try
		{
			// Same as above, but with a deleted GLX pixmap
			printf("Deleted GLX Pixmap->2D Pixmap:  ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				THROWNL("Could not make context current");
			checkCurrent(dpy, glxpm0, glxpm0, ctx, dpyw / 2, dpyh / 2);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			if(!glXMakeContextCurrent(dpy, 0, 0, 0))
				THROWNL("Could not make context current");
			glXDestroyPixmap(dpy, glxpm0);  glxpm0 = 0;
			XCopyArea(dpy, pm0, pm2, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw / 2, dpyh / 2, 0, 0);
			checkFrame(dpy, pm0, 1, lastFrame);
			checkWindowColor(dpy, pm0, clr.bits(-1), false);

			// VirtualGL 2.6.3 and prior incorrectly read back the current drawable
			// instead of the specified GLXPixmap if glXDestroy[GLX]Pixmap() was
			// called while a drawable other than the specified GLXPixmap was
			// current.  This caused a [GLX]BadDrawable error if the current drawable
			// had already been deleted, and it caused other errors (including
			// TempContext exceptions) on some systems if the current drawable and
			// the specified GLXPixmap were created with incompatible GLXFBConfigs.
			glXDestroyGLXPixmap(dpy, glxpm1);  glxpm1 = 0;
			glxattribs[1] = 0;  glxattribs[13] = 8;
			if((configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattribs,
				&n)) == NULL || n == 0)
				THROW("Could not find a suitable FB config");
			config = configs[0];
			XFree(configs);  configs = NULL;
			if((glxpm1 = glXCreatePixmap(dpy, config, pm1, NULL)) == 0)
				THROW("Could not create GLX pixmap");
			if(!(ctx2 = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, NULL, True)))
				THROW("Could not create context");
			if(!glXMakeContextCurrent(dpy, glxpm1, glxpm1, ctx2))
				THROW("Could not make context current");
			checkCurrent(dpy, glxpm1, glxpm1, ctx2, dpyw / 2, dpyh / 2);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM1");
			if(!glXMakeContextCurrent(dpy, pb, pb, ctx))
				THROW("Could not make context current");
			checkCurrent(dpy, pb, pb, ctx, dpyw / 2, dpyh / 2);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PB");
			clr.clear(GL_BACK);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-1), "PB");
			glXDestroyPixmap(dpy, glxpm1);  glxpm1 = 0;
			checkWindowColor(dpy, pm1, clr.bits(-3), false);

			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);

	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	if(rbo) { glDeleteRenderbuffersEXT(1, &rbo);  rbo = 0; }
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	if(fbo) { glDeleteFramebuffersEXT(1, &fbo);  fbo = 0; }
	if(ctx && dpy)
	{
		glXMakeContextCurrent(dpy, 0, 0, 0);
		glXDestroyContext(dpy, ctx);
	}
	if(ctx2 && dpy)
	{
		glXMakeContextCurrent(dpy, 0, 0, 0);
		glXDestroyContext(dpy, ctx2);
	}
	if(fontInfo && dpy) XFreeFont(dpy, fontInfo);
	if(pb && dpy) glXDestroyPbuffer(dpy, pb);
	if(glxpm1 && dpy) glXDestroyGLXPixmap(dpy, glxpm1);
	if(glxpm0 && dpy) glXDestroyGLXPixmap(dpy, glxpm0);
	if(pm2 && dpy) XFreePixmap(dpy, pm2);
	if(pm1 && dpy) XFreePixmap(dpy, pm1);
	if(pm0 && dpy) XFreePixmap(dpy, pm0);
	if(glxwin && dpy) glXDestroyWindow(dpy, glxwin);
	if(win && dpy) XDestroyWindow(dpy, win);
	if(vis) XFree(vis);
	if(configs) XFree(configs);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


// Test whether glXMakeCurrent() can handle mismatches between the FB config
// of the context and the off-screen drawable

int contextMismatchTest(void)
{
	Display *dpy = NULL;  Window win = 0;
	int dpyw, dpyh, retval = 1;
	int glxattribs1[] = { GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT | GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	int glxattribs2[] = { GLX_DOUBLEBUFFER, 0, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT | GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
		None };
	XVisualInfo *vis = NULL;
	GLXFBConfig config1 = 0, config2 = 0, *configs = NULL;  int n = 0;
	GLXContext ctx1 = 0, ctx2 = 0;
	XSetWindowAttributes swa;

	printf("Context FB config mismatch test:\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
		dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh = DisplayHeight(dpy, DefaultScreen(dpy));
		if(DefaultDepth(dpy, DefaultScreen(dpy)) == 30)
		{
			glxattribs1[7] = glxattribs1[9] = glxattribs1[11] = 10;
			glxattribs2[7] = glxattribs2[9] = glxattribs2[11] = 10;
			glxattribs2[13] = 2;
		}

		if((configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattribs1,
			&n)) == NULL || n == 0)
			THROW("Could not find a suitable FB config");
		config1 = configs[0];
		if(!(vis = glXGetVisualFromFBConfig(dpy, config1)))
			THROW("glXGetVisualFromFBConfig()");
		XFree(configs);  configs = NULL;

		if((configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattribs2,
			&n)) == NULL || n == 0)
			THROW("Could not find a suitable FB config");
		config2 = configs[0];
		XFree(configs);  configs = NULL;

		Window root = RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel = 0;
		swa.background_pixel = 0;
		swa.event_mask = 0;
		if((win = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0, vis->depth,
			InputOutput, vis->visual, CWBorderPixel | CWColormap | CWEventMask,
			&swa)) == 0)
			THROW("Could not create window");
		XMapWindow(dpy, win);

		try
		{
			int major, minor, mask, flags;

			if(!(ctx1 =
				glXCreateNewContext(dpy, config1, GLX_RGBA_TYPE, NULL, True)))
				THROW("Could not create context");
			int attribs[] = { GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
				GLX_CONTEXT_MINOR_VERSION_ARB, 2, GLX_CONTEXT_FLAGS_ARB,
				GLX_CONTEXT_DEBUG_BIT_ARB | GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
				GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None,
				None, None };
			if(GLX_EXTENSION_EXISTS(GLX_ARB_create_context_robustness))
			{
				attribs[5] |= GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB;
				attribs[8] = GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB;
				attribs[9] = GLX_LOSE_CONTEXT_ON_RESET_ARB;
			}
			if(GLX_EXTENSION_EXISTS(GLX_ARB_create_context))
			{
				PFNGLXCREATECONTEXTATTRIBSARBPROC __glXCreateContextAttribsARB =
					(PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress(
						(const GLubyte *)"glXCreateContextAttribsARB");
				if(__glXCreateContextAttribsARB)
					ctx2 = __glXCreateContextAttribsARB(dpy, config2, NULL, True,
						attribs);
			}
			if(!ctx2)
				ctx2 = glXCreateNewContext(dpy, config2, GLX_RGBA_TYPE, NULL, True);
			if(!ctx2) THROW("Could not create context");

			if(!(glXMakeCurrent(dpy, win, ctx1)))
				THROWNL("Could not make context current");
			if(!(glXMakeCurrent(dpy, win, ctx2)))
				THROWNL("Could not make context current");

			if(!(glXMakeContextCurrent(dpy, win, win, ctx1)))
				THROWNL("Could not make context current");

			if(GLX_EXTENSION_EXISTS(GLX_ARB_create_context))
			{
				glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
				if(flags)
					PRERROR1("Context flags 0x%.8x != 0x00000000", flags);
				if(!(glXMakeContextCurrent(dpy, win, win, ctx2)))
					THROWNL("Could not make context current");
				major = minor = mask = flags = -20;
				glGetIntegerv(GL_MAJOR_VERSION, &major);
				glGetIntegerv(GL_MINOR_VERSION, &minor);
				if(major < 3 || (major == 3 && minor < 2))
					PRERROR2("Incorrect context version %d.%d", major, minor);
				glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &mask);
				if(mask != attribs[7])
					PRERROR2("Context profile mask 0x%.8x != 0x%.8x", mask, attribs[7]);
				glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
				if(flags != attribs[5])
					PRERROR2("Context flags 0x%.8x != 0x%.8x", flags, attribs[5]);
			}
			if(GLX_EXTENSION_EXISTS(GLX_ARB_create_context_robustness))
			{
				int notificationStrategy = -20;
				glGetIntegerv(GL_RESET_NOTIFICATION_STRATEGY_ARB,
					&notificationStrategy);
				if(notificationStrategy != attribs[9])
					PRERROR2("Reset notification strategy 0x%.8x != 0x%.8x",
						notificationStrategy, attribs[9]);
			}
			// Make sure that glXCreateContextAttribsARB() accepts GLX_RENDER_TYPE
			// as an attribute.  This will fail when using the EGL back end with
			// VirtualGL 3.0.
			if(GLX_EXTENSION_EXISTS(GLX_ARB_create_context))
			{
				int attribs2[] = { GLX_RENDER_TYPE, GLX_RGBA_TYPE, None };
				glXDestroyContext(dpy, ctx2);  ctx2 = 0;
				PFNGLXCREATECONTEXTATTRIBSARBPROC __glXCreateContextAttribsARB =
					(PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress(
						(const GLubyte *)"glXCreateContextAttribsARB");
				if(__glXCreateContextAttribsARB)
					ctx2 = __glXCreateContextAttribsARB(dpy, config2, NULL, True,
						attribs2);
				if(!ctx2) THROW("Could not create context");
			}

			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	if(ctx1 && dpy)
	{
		glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx1);
	}
	if(ctx2 && dpy)
	{
		glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx2);
	}
	if(win && dpy) XDestroyWindow(dpy, win);
	if(vis) XFree(vis);
	if(configs) XFree(configs);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


// Test whether glXCopyContext() works properly

#define CHECK_BOOL(param, expected) \
{ \
	glGetBooleanv(param, &b); \
	if(b != expected) THROWNL(#param); \
}

#define CHECK_FLOAT1(param, expected) \
{ \
	glGetFloatv(param, f); \
	if(f[0] != expected) THROWNL(#param); \
}

#define CHECK_FLOAT4(param, expected) \
{ \
	glGetFloatv(param, f); \
	if(f[0] != expected || f[1] != expected || f[2] != expected \
		|| f[3] != expected) \
		THROWNL(#param); \
}

#define CHECK_INT1(param, expected) \
{ \
	glGetIntegerv(param, i); \
	if(i[0] != expected) THROWNL(#param); \
}

#define CHECK_INT4(param, expected) \
{ \
	glGetIntegerv(param, i); \
	if(i[0] != expected || i[1] != expected || i[2] != expected \
		|| i[3] != expected) \
		THROWNL(#param); \
}

static void checkContext(unsigned long mask)
{
	GLfloat f[4];
	GLboolean b;
	GLint i[4];
	GLubyte stippleImage[32 * 32], expectedStippleImage[32 * 32];

	GLfloat expectedAccumClearValue = mask & GL_ACCUM_BUFFER_BIT ? 0.5 : 0.0;
	GLfloat expectedColorClearValue = mask & GL_COLOR_BUFFER_BIT ? 0.5 : 0.0;
	GLboolean expectedEdgeFlag = mask & GL_CURRENT_BIT ? GL_FALSE : GL_TRUE;
	GLfloat expectedDepthClearValue = mask & GL_DEPTH_BUFFER_BIT ? 0.5 : 1.0;
	GLboolean expectedLight0 = mask & GL_ENABLE_BIT ? GL_TRUE : GL_FALSE;
	GLint expectedMap1GridSegments = mask & GL_EVAL_BIT ? 2 : 1;
	GLint expectedFogMode = mask & GL_FOG_BIT ? GL_LINEAR : GL_EXP;
	GLint expectedFogHint = mask & GL_HINT_BIT ? GL_FASTEST : GL_DONT_CARE;
	GLint expectedShadeModel = mask & GL_LIGHTING_BIT ? GL_FLAT : GL_SMOOTH;
	GLint expectedLineWidth = mask & GL_LINE_BIT ? 2 : 1;
	GLint expectedListBase = mask & GL_LIST_BIT ? 1 : 0;
//GLboolean expectedSampleCoverageInvert =
//  mask & GL_MULTISAMPLE_BIT ? GL_TRUE : GL_FALSE;
	GLfloat expectedRedScale = mask & GL_PIXEL_MODE_BIT ? 0.5 : 1.0;
	GLint expectedPointSize = mask & GL_POINT_BIT ? 2 : 1;
	GLint expectedCullFaceMode = mask & GL_POLYGON_BIT ? GL_FRONT : GL_BACK;
	memset(expectedStippleImage, mask & GL_POLYGON_STIPPLE_BIT ? 170 : 255,
		32 * 4);
	GLint expectedScissorBox = mask & GL_SCISSOR_BIT ? 10 : 20;
	GLint expectedStencilClearValue = mask & GL_STENCIL_BUFFER_BIT ? 128 : 0;
	GLint expectedTextureGenMode =
		mask & GL_TEXTURE_BIT ? GL_OBJECT_LINEAR : GL_EYE_LINEAR;
	GLint expectedMatrixMode =
		mask & GL_TRANSFORM_BIT ? GL_PROJECTION : GL_MODELVIEW;
	GLint expectedViewport = mask & GL_VIEWPORT_BIT ? 10 : 20;

	CHECK_FLOAT4(GL_ACCUM_CLEAR_VALUE, expectedAccumClearValue);
	CHECK_FLOAT4(GL_COLOR_CLEAR_VALUE, expectedColorClearValue);
	CHECK_BOOL(GL_EDGE_FLAG, expectedEdgeFlag);
	CHECK_FLOAT1(GL_DEPTH_CLEAR_VALUE, expectedDepthClearValue);
	CHECK_BOOL(GL_LIGHT0, expectedLight0);
	CHECK_INT1(GL_MAP1_GRID_SEGMENTS, expectedMap1GridSegments);
	CHECK_INT1(GL_FOG_MODE, expectedFogMode);
	CHECK_INT1(GL_FOG_HINT, expectedFogHint);
	CHECK_INT1(GL_SHADE_MODEL, expectedShadeModel);
	CHECK_INT1(GL_LINE_WIDTH, expectedLineWidth);
	CHECK_INT1(GL_LIST_BASE, expectedListBase);
//CHECK_BOOL(GL_SAMPLE_COVERAGE_INVERT, expectedSampleCoverageInvert);
	CHECK_FLOAT1(GL_RED_SCALE, expectedRedScale);
	CHECK_INT1(GL_POINT_SIZE, expectedPointSize);
	CHECK_INT1(GL_CULL_FACE_MODE, expectedCullFaceMode);
	glGetPolygonStipple(stippleImage);
	if(memcmp(stippleImage, expectedStippleImage, 32 * 4))
		THROWNL("Stipple image");
	CHECK_INT4(GL_SCISSOR_BOX, expectedScissorBox);
	CHECK_INT1(GL_STENCIL_CLEAR_VALUE, expectedStencilClearValue);
	glGetTexGeniv(GL_Q, GL_TEXTURE_GEN_MODE, i);
	if(i[0] != expectedTextureGenMode)
		THROWNL("GL_TEXTURE_GEN_MODE");
	CHECK_INT1(GL_MATRIX_MODE, expectedMatrixMode);
	CHECK_INT4(GL_VIEWPORT, expectedViewport);
}

#define CHECK_CONTEXT(m) \
{ \
	if(!(glXMakeCurrent(dpy, win, ctx1))) \
		THROW("Could not make context current"); \
	glXCopyContext(dpy, ctx1, ctx2, m); \
	mask |= m; \
	if(!(glXMakeCurrent(dpy, win, ctx2))) \
		THROW("Could not make context current"); \
	checkContext(mask); \
}

int copyContextTest(void)
{
	Display *dpy = NULL;  Window win = 0;
	int dpyw, dpyh, retval = 1;
	int glxattribs[] = { GLX_DOUBLEBUFFER, GLX_RGBA, None };
	XVisualInfo *vis = NULL;
	GLXContext ctx1 = 0, ctx2 = 0;
	XSetWindowAttributes swa;

	printf("glXCopyContext() test:\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
		dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

		if(!(vis = glXChooseVisual(dpy, DefaultScreen(dpy), glxattribs)))
			THROW("glXChooseVisual()");

		Window root = RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel = 0;
		swa.background_pixel = 0;
		swa.event_mask = 0;
		if((win = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0, vis->depth,
			InputOutput, vis->visual, CWBorderPixel | CWColormap | CWEventMask,
			&swa)) == 0)
			THROW("Could not create window");
		XMapWindow(dpy, win);

		try
		{
			if(!(ctx1 = glXCreateContext(dpy, vis, NULL, True)))
				THROW("Could not create context");
			if(!(ctx2 = glXCreateContext(dpy, vis, NULL, True)))
				THROW("Could not create context");

			if(!(glXMakeCurrent(dpy, win, ctx1)))
				THROW("Could not make context current");

			while(glGetError() != GL_NO_ERROR) {}
			glClearAccum(0.5, 0.5, 0.5, 0.5);     // GL_ACCUM_BUFFER_BIT
			CHECK_GL_ERROR();
			glClearColor(0.5, 0.5, 0.5, 0.5);     // GL_COLOR_BUFFER_BIT
			CHECK_GL_ERROR();
			glEdgeFlag(GL_FALSE);                 // GL_CURRENT_BIT
			CHECK_GL_ERROR();
			glClearDepth(0.5);                    // GL_DEPTH_BUFFER_BIT
			CHECK_GL_ERROR();
			glEnable(GL_LIGHT0);                  // GL_ENABLE_BIT
			CHECK_GL_ERROR();
			glMapGrid1f(2, 0.5, 0.5);             // GL_EVAL_BIT
			CHECK_GL_ERROR();
			glFogi(GL_FOG_MODE, GL_LINEAR);       // GL_FOG_BIT
			CHECK_GL_ERROR();
			glHint(GL_FOG_HINT, GL_FASTEST);      // GL_HINT_BIT
			CHECK_GL_ERROR();
			glShadeModel(GL_FLAT);                // GL_LIGHTING_BIT
			CHECK_GL_ERROR();
			glLineWidth(2);                       // GL_LINE_BIT
			CHECK_GL_ERROR();
			glListBase(1);                        // GL_LIST_BIT
			CHECK_GL_ERROR();
// NOTE: We can't check GL_MULTISAMPLE_BIT because nVidia's implementation of
// glXCopyContext() copies those parameters with GL_COLOR_BUFFER_BIT.
//    glSampleCoverage(0.5, GL_TRUE);       // GL_MULTISAMPLE_BIT
//    CHECK_GL_ERROR();
			glPixelTransferf(GL_RED_SCALE, 0.5);  // GL_PIXEL_MODE_BIT
			CHECK_GL_ERROR();
			glPointSize(2);                       // GL_POINT_BIT
			CHECK_GL_ERROR();
			glCullFace(GL_FRONT);                 // GL_POLYGON_BIT
			CHECK_GL_ERROR();
			GLubyte stippleImage[32 * 4];
			memset(stippleImage, 170, 32 * 4);
			glPolygonStipple(stippleImage);       // GL_POLYGON_STIPPLE_BIT
			CHECK_GL_ERROR();
			glScissor(10, 10, 10, 10);            // GL_SCISSOR_BIT
			CHECK_GL_ERROR();
			glClearStencil(128);                  // GL_STENCIL_BUFFER_BIT
			CHECK_GL_ERROR();
			glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			                                      // GL_TEXTURE_BIT
			CHECK_GL_ERROR();
			glMatrixMode(GL_PROJECTION);          // GL_TRANSFORM_BIT
			CHECK_GL_ERROR();
			glViewport(10, 10, 10, 10);           // GL_VIEWPORT_BIT
			CHECK_GL_ERROR();

			if(!(glXMakeCurrent(dpy, win, ctx2)))
				THROW("Could not make context current");

			glScissor(20, 20, 20, 20);
			glViewport(20, 20, 20, 20);
			unsigned long mask = 0;
			checkContext(mask);

			CHECK_CONTEXT(GL_ACCUM_BUFFER_BIT);
			CHECK_CONTEXT(GL_COLOR_BUFFER_BIT);
			CHECK_CONTEXT(GL_CURRENT_BIT);
			CHECK_CONTEXT(GL_DEPTH_BUFFER_BIT);
			CHECK_CONTEXT(GL_ENABLE_BIT);
			CHECK_CONTEXT(GL_EVAL_BIT);
			CHECK_CONTEXT(GL_FOG_BIT);
			CHECK_CONTEXT(GL_HINT_BIT);
			CHECK_CONTEXT(GL_LIGHTING_BIT);
			CHECK_CONTEXT(GL_LINE_BIT);
			CHECK_CONTEXT(GL_LIST_BIT);
//    CHECK_CONTEXT(GL_MULTISAMPLE_BIT);
			CHECK_CONTEXT(GL_PIXEL_MODE_BIT);
			CHECK_CONTEXT(GL_POINT_BIT);
			CHECK_CONTEXT(GL_POLYGON_BIT);
			CHECK_CONTEXT(GL_POLYGON_STIPPLE_BIT);
			CHECK_CONTEXT(GL_SCISSOR_BIT);
			CHECK_CONTEXT(GL_STENCIL_BUFFER_BIT);
			CHECK_CONTEXT(GL_TEXTURE_BIT);
			CHECK_CONTEXT(GL_TRANSFORM_BIT);
			CHECK_CONTEXT(GL_VIEWPORT_BIT);

			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	if(ctx1 && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx1);
	}
	if(ctx2 && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx2);
	}
	if(win && dpy) XDestroyWindow(dpy, win);
	if(vis) XFree(vis);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


// Test whether VirtualGL properly handles explicit and implicit destruction of
// subwindows

int subWinTest(void)
{
	Display *dpy = NULL;  Window win = 0, win1 = 0, win2 = 0;
	#ifdef FAKEXCB
	xcb_connection_t *conn = NULL;
	#endif
	TestColor clr(0);
	int dpyw, dpyh, retval = 1, lastFrame = 0;
	int glxattribs[] = { GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	XVisualInfo *vis = NULL;
	GLXContext ctx = 0;
	XSetWindowAttributes swa;

	printf("Subwindow destruction test:\n\n");

	try
	{
		try
		{
			for(int i = 0; i < 20; i++)
			{
				if(!dpy && !(dpy = XOpenDisplay(0)))
					THROW("Could not open display");
				dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
				dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

				if((vis = glXChooseVisual(dpy, DefaultScreen(dpy),
					glxattribs)) == NULL)
					THROW("Could not find a suitable visual");

				Window root = RootWindow(dpy, DefaultScreen(dpy));
				swa.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
				swa.border_pixel = 0;
				swa.background_pixel = 0;
				swa.event_mask = 0;
				if(!win && (win = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0,
					vis->depth, InputOutput, vis->visual,
					CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
					THROW("Could not create window");
				if((win1 = XCreateWindow(dpy, win, 0, 0, dpyw / 2, dpyh / 2, 0,
					vis->depth, InputOutput, vis->visual,
					CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
					THROW("Could not create subwindow");
				if((win2 = XCreateWindow(dpy, win1, 0, 0, dpyw / 2, dpyh / 2, 0,
					vis->depth, InputOutput, vis->visual,
					CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
					THROW("Could not create subwindow");
				XMapSubwindows(dpy, win);
				XMapWindow(dpy, win);

				lastFrame = 0;
				if(!(ctx = glXCreateContext(dpy, vis, NULL, True)))
					THROW("Could not create context");
				XFree(vis);  vis = NULL;
				if(!(glXMakeCurrent(dpy, win2, ctx)))
					THROWNL("Could not make context current");
				clr.clear(GL_BACK);
				glXSwapBuffers(dpy, win2);
				checkFrame(dpy, win2, 1, lastFrame);
				checkWindowColor(dpy, win2, clr.bits(-1), false);
				glXMakeCurrent(dpy, 0, 0);
				glXDestroyContext(dpy, ctx);  ctx = 0;

				if(i % 3 == 0) { XCloseDisplay(dpy);  dpy = NULL;  win = 0; }
				else if(i % 3 == 1) { XDestroyWindow(dpy, win);  win = 0; }
				else XDestroySubwindows(dpy, win);
			}

			if(win && dpy) { XDestroyWindow(dpy, win);  win = 0; }
			if(dpy) { XCloseDisplay(dpy);  dpy = NULL; }

			#ifdef FAKEXCB
			for(int i = 0; i < 20; i++)
			{
				xcb_void_cookie_t cookie = { 0 };

				if(!dpy && !(dpy = XOpenDisplay(0)))
					THROW("Could not open display");
				if(!(conn = XGetXCBConnection(dpy)))
					THROW("Could not get XCB connection");
				XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
				dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
				dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

				if((vis = glXChooseVisual(dpy, DefaultScreen(dpy),
					glxattribs)) == NULL)
					THROW("Could not find a suitable visual");

				Window root = RootWindow(dpy, DefaultScreen(dpy));

				if(!win)
				{
					if((win = xcb_generate_id(conn)) == 0)
						THROW("Could not create window");
					cookie = xcb_create_window(conn, vis->depth, win, root, 0, 0,
						dpyw / 2, dpyh / 2, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
						vis->visualid, 0, NULL);
					if(!cookie.sequence)
						THROW("Could not create window");
				}

				if((win1 = xcb_generate_id(conn)) == 0)
					THROW("Could not create subwindow");
				cookie.sequence = 0;
				cookie = xcb_create_window_checked(conn, vis->depth, win1, win, 0, 0,
					dpyw / 2, dpyh / 2, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, vis->visualid,
					0, NULL);
				if(!cookie.sequence)
					THROW("Could not create subwindow");

				if((win2 = xcb_generate_id(conn)) == 0)
					THROW("Could not create subwindow");
				cookie.sequence = 0;
				if(i % 2 == 0)
					cookie = xcb_create_window_aux(conn, vis->depth, win2, win1, 0, 0,
						dpyw / 2, dpyh / 2, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
						vis->visualid, 0, NULL);
				else
					cookie = xcb_create_window_aux_checked(conn, vis->depth, win2, win1,
						0, 0, dpyw / 2, dpyh / 2, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
						vis->visualid, 0, NULL);
				if(!cookie.sequence)
					THROW("Could not create subwindow");

				xcb_map_subwindows(conn, win);
				xcb_map_window(conn, win);

				lastFrame = 0;
				if(!(ctx = glXCreateContext(dpy, vis, NULL, True)))
					THROW("Could not create context");
				XFree(vis);  vis = NULL;
				if(!(glXMakeCurrent(dpy, win2, ctx)))
					THROWNL("Could not make context current");
				clr.clear(GL_BACK);
				glXSwapBuffers(dpy, win2);
				checkFrame(dpy, win2, 1, lastFrame);
				checkWindowColor(dpy, win2, clr.bits(-1), false);
				glXMakeCurrent(dpy, 0, 0);
				glXDestroyContext(dpy, ctx);  ctx = 0;

				if(i % 3 == 0) { XCloseDisplay(dpy);  dpy = NULL;  win = 0; }
				else if(i % 3 == 1)
				{
					if(i % 2 == 0)
						xcb_destroy_window(conn, win);
					else
						xcb_destroy_window_checked(conn, win);
					win = 0;
				}
				else
				{
					if(i % 2 == 0)
						xcb_destroy_subwindows(conn, win);
					else
						xcb_destroy_subwindows_checked(conn, win);
				}
			}
			#endif

			printf("SUCCESS\n");
		}
		catch(std::exception &e)
		{
			printf("Failed! (%s)\n", e.what());  retval = 0;
		}
		fflush(stdout);
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	if(ctx && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);
	}
	if(win && dpy) XDestroyWindow(dpy, win);
	if(vis) XFree(vis);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


int x11Error = -1, x11MinorCode = -1;
char x11ErrorString[174] = { 0 };

int xhandler(Display *dpy, XErrorEvent *xe)
{
	x11Error = xe->error_code;
	x11MinorCode = xe->minor_code;
	XGetErrorText(dpy, x11Error, x11ErrorString, 174);
	return 0;
}

#define XERRTEST_START() \
{ \
	x11Error = x11MinorCode = -1; \
	x11ErrorString[0] = 0; \
	prevHandler = XSetErrorHandler(xhandler); \
}

#define GLXERRTEST_STOP(expectedErrCode, expectedMinorCode) \
{ \
	XSetErrorHandler(prevHandler); \
	if(x11Error != expectedErrCode) \
		PRERROR3("%d: X11 error code %d != %d", __LINE__, x11Error, \
			expectedErrCode); \
	if(expectedMinorCode >= 0 && x11MinorCode != expectedMinorCode) \
		PRERROR3("%d: X11 minor code %d != %d", __LINE__, x11MinorCode, \
			expectedMinorCode); \
}

#define XERRTEST_STOP(expectedErrCode, expectedMinorCode, expectedErrString) \
{ \
	GLXERRTEST_STOP(expectedErrCode, expectedMinorCode); \
	if(strcmp(x11ErrorString, expectedErrString)) \
		PRERROR3("%d: X11 error string %s != %s", __LINE__, x11ErrorString, \
			expectedErrString); \
}

int extensionQueryTest(void)
{
	Display *dpy = NULL;  int retval = 1;
	int majorOpcode = -1, eventBase = -1, errorBase = -1, n;
	XErrorHandler prevHandler;
	XVisualInfo *vis = NULL;
	GLXContext ctx = 0;
	Pixmap pm = 0;
	GLXPixmap glxpm = 0;

	printf("Extension query and error handling test:\n\n");

	try
	{
		int major = -1, minor = -1, dummy1;
		unsigned int dummy2;
		unsigned long dummy3;
		GLXFBConfig *configs = NULL;

		if((dpy = XkbOpenDisplay(0, &dummy1, &dummy1, NULL, NULL,
			&dummy1)) == NULL)
			THROW("Could not open display");

		if(!XQueryExtension(dpy, "GLX", &majorOpcode, &eventBase, &errorBase)
			|| majorOpcode <= 0 || eventBase <= 0 || errorBase <= 0)
			THROW("GLX Extension not reported as present");
		#ifdef FAKEXCB
		xcb_connection_t *conn = XGetXCBConnection(dpy);
		if(!conn) THROW("Could not get XCB connection for display");
		const xcb_query_extension_reply_t *qeReply =
			xcb_get_extension_data(conn, &xcb_glx_id);
		if(!qeReply || qeReply->major_opcode <= 0 || qeReply->first_event <= 0
			|| qeReply->first_error <= 0)
			THROW("GLX extension not reported as present in XCB");
		xcb_glx_query_version_cookie_t cookie = xcb_glx_query_version(conn, 1, 4);
		xcb_generic_error_t *error;
		xcb_glx_query_version_reply_t *qvReply =
			xcb_glx_query_version_reply(conn, cookie, &error);
		if(!qvReply) THROW("Could not get GLX version in XCB");
		if(qvReply->major_version != 1 || qvReply->minor_version != 4)
		{
			free(qvReply);
			THROW("Incorrect GLX version returned in XCB");
		}
		free(qvReply);
		#endif
		// For some reason, the GLX error string table isn't initialized until a
		// substantial GLX command completes.
		if((configs = glXGetFBConfigs(dpy, DefaultScreen(dpy), &dummy1)) != NULL)
			XFree(configs);
		XERRTEST_START();
		glXQueryDrawable(dpy, 0, GLX_WIDTH, &dummy2);
		GLXERRTEST_STOP(errorBase + GLXBadDrawable, -1);

		int errorBase2 = -1, eventBase2 = -1;
		if(!glXQueryExtension(dpy, &errorBase2, &eventBase2) || errorBase2 <= 0
			|| eventBase2 <= 0 || !glXQueryExtension(dpy, NULL, NULL))
			THROW("GLX Extension not reported as present");
		if(errorBase != errorBase2 || eventBase != eventBase2)
			THROW("XQueryExtension()/glXQueryExtension() mismatch");

		char *vendor = XServerVendor(dpy);
		if(!vendor || strcmp(vendor, "Spacely Sprockets, Inc."))
			THROW("XServerVendor()");
		vendor = ServerVendor(dpy);
		if(!vendor || strcmp(vendor, "Spacely Sprockets, Inc."))
			THROW("ServerVendor()");

		glXQueryVersion(dpy, &major, &minor);
		if(major != 1 || minor != 4)
			THROW("glXQueryVersion()");

		if(strcmp(glXGetClientString(dpy, GLX_VERSION), "1.4"))
			THROW("glXGetClientString(..., GLX_VERSION)");
		if(strcmp(glXGetClientString(dpy, GLX_VENDOR),
			"Slate Rock and Gravel Company"))
			THROW("glXGetClientString(..., GLX_VENDOR)");
		if(strcmp(glXGetClientString(dpy, GLX_EXTENSIONS),
			glXQueryExtensionsString(dpy, DefaultScreen(dpy))))
			THROW("glXGetClientString(..., GLX_EXTENSIONS)");

		if(strcmp(glXQueryServerString(dpy, DefaultScreen(dpy), GLX_VERSION),
			"1.4"))
			THROW("glXQueryServerString(..., GLX_VERSION)");
		if(strcmp(glXQueryServerString(dpy, DefaultScreen(dpy), GLX_VENDOR),
			"Slate Rock and Gravel Company"))
			THROW("glXQueryServerString(..., GLX_VENDOR)");
		if(strcmp(glXQueryServerString(dpy, DefaultScreen(dpy), GLX_EXTENSIONS),
			glXQueryExtensionsString(dpy, DefaultScreen(dpy))))
			THROW("glXQueryServerString(..., GLX_EXTENSIONS)");

		// Make sure that GL_EXT_x11_sync_object isn't exposed.  Since that
		// extension allows OpenGL functions (which live on the 3D X server) to
		// operate on XSyncFence objects (which live on the 2D X server), the
		// extension does not currently work with the VirtualGL Faker.

		int vattribs[] = { GLX_RGBA, None };
		if(!(vis = glXChooseVisual(dpy, DefaultScreen(dpy), vattribs)))
			THROW("Could not find visual");
		if(!(ctx = glXCreateContext(dpy, vis, NULL, True)))
			THROW("Could not create context");
		if(!(pm = XCreatePixmap(dpy, DefaultRootWindow(dpy), 10, 10,
			DefaultDepth(dpy, DefaultScreen(dpy)))))
			THROW("Could not create Pixmap");
		if(!(glxpm = glXCreateGLXPixmap(dpy, vis, pm)))
			THROW("Could not create GLX Pixmap");
		if(!glXMakeCurrent(dpy, glxpm, ctx))
			THROW("Could not make context current");

		char *exts = (char *)glGetString(GL_EXTENSIONS);
		if(!exts) THROW("Could not get OpenGL extensions");
		if(strstr(exts, "GL_EXT_x11_sync_object"))
			THROW("GL_EXT_x11_sync_object is exposed by glGetString()");
		glGetIntegerv(GL_NUM_EXTENSIONS, &n);
		for(int i = 0; i < n; i++)
		{
			char *ext = (char *)glGetStringi(GL_EXTENSIONS, i);
			if(!ext) THROW("Could not get OpenGL extension");
			if(!strcmp(ext, "GL_EXT_x11_sync_object"))
				THROW("GL_EXT_x11_sync_object is exposed by glGetStringi()");
		}

		glXMakeCurrent(dpy, 0, 0);
		glXDestroyGLXPixmap(dpy, glxpm);  glxpm = 0;
		XFreePixmap(dpy, pm);  pm = 0;
		glXDestroyContext(dpy, ctx);  ctx = 0;
		XFree(vis);  vis = NULL;
		if(glXGetProcAddress((const GLubyte *)"glImportSyncEXT"))
			THROW("glImportSyncExt() is exposed but shouldn't be");

		// Test whether particular GLX functions trigger the errors that the spec
		// says they should

		if(!(vis = glXChooseVisual(dpy, DefaultScreen(dpy), vattribs)))
			THROW("Could not find visual");

		int cattribs[] = { None };
		GLXFBConfig config;
		if(!(configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), cattribs, &n)))
			THROW("Could not find FB config");
		config = configs[0];
		XFree(configs);

		//-------------------------------------------------------------------------
		//    glXBindTexImageEXT()
		//-------------------------------------------------------------------------

		if(GLX_EXTENSION_EXISTS(GLX_EXT_texture_from_pixmap))
		{
			PFNGLXBINDTEXIMAGEEXTPROC __glXBindTexImageEXT =
				(PFNGLXBINDTEXIMAGEEXTPROC)glXGetProcAddress(
					(GLubyte *)"glXBindTexImageEXT");
			if(__glXBindTexImageEXT)
			{
				// GLXBadPixmap error thrown if drawable is not valid or is not a
				// Pixmap
				for(int badValue = 0; badValue <= 1; badValue++)
				{
					XERRTEST_START();
					__glXBindTexImageEXT(dpy, (GLXDrawable)badValue, GL_BACK, NULL);
					GLXERRTEST_STOP(errorBase + GLXBadPixmap, X_GLXVendorPrivate)
				}
				XERRTEST_START();
				__glXBindTexImageEXT(dpy, DefaultRootWindow(dpy), GL_BACK, NULL);
				GLXERRTEST_STOP(errorBase + GLXBadPixmap, X_GLXVendorPrivate)
			}
		}

		//-------------------------------------------------------------------------
		//    glXChooseFBConfig()
		//-------------------------------------------------------------------------

		// NULL returned if display doesn't support GLX or screen is not valid
		if(glXChooseFBConfig(NULL, DefaultScreen(dpy), cattribs, &n))
			THROW("glXChooseFBConfig(NULL display) != NULL");
		if(glXChooseFBConfig(dpy, -1, cattribs, &n))
			THROW("glXChooseFBConfig(bogus screen) != NULL");

		//-------------------------------------------------------------------------
		//    glXCreateContext()
		//-------------------------------------------------------------------------

		// BadValue error thrown and NULL returned if visual is not valid
		//
		// (NOTE: VirtualGL can't distinguish between a bogus non-NULL visual and a
		// valid visual, so passing the former will result in a segfault.)
		XERRTEST_START();
		if(glXCreateContext(dpy, NULL, NULL, True))
			THROW("glXCreateContext(bad visual) != NULL");
		XERRTEST_STOP(BadValue, X_GLXCreateContext,
			"BadValue (integer parameter out of range for operation)");

		//-------------------------------------------------------------------------
		//    glXCreateGLXPixmap()
		//    glXCreatePixmap()
		//-------------------------------------------------------------------------

		if(!(pm = XCreatePixmap(dpy, DefaultRootWindow(dpy), 10, 10,
			DefaultDepth(dpy, DefaultScreen(dpy)))))
			THROW("Could not create Pixmap");

		// BadValue error thrown and NULL returned if visual is not valid
		XERRTEST_START();
		if(glXCreateGLXPixmap(dpy, NULL, pm))
			THROW("glXCreateGLXPixmap(bad visual) != NULL");
		XERRTEST_STOP(BadValue, X_GLXCreateGLXPixmap,
			"BadValue (integer parameter out of range for operation)");

		// GLXBadFBConfig error thrown and NULL returned if FB config is not valid
		XERRTEST_START();
		if(glXCreatePixmap(dpy, NULL, pm, NULL))
			THROW("glXCreatePixmap(bogus FB config) != NULL");
		GLXERRTEST_STOP(errorBase + GLXBadFBConfig, X_GLXCreatePixmap)

		// BadMatch error thrown and NULL returned if FB config doesn't support
		// window rendering
		if(GLX_EXTENSION_EXISTS(GLX_ARB_fbconfig_float))
		{
			int floatAttribs[] = { GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
				GLX_RENDER_TYPE, GLX_RGBA_FLOAT_BIT_ARB, None };
			if((configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), floatAttribs,
				&n)))
			{
				XERRTEST_START();
				if(glXCreatePixmap(dpy, configs[0], pm, NULL))
					THROW("glXCreatePixmap(non-X-renderable FB config) != NULL");
				XERRTEST_STOP(BadMatch, X_GLXCreatePixmap,
					"BadMatch (invalid parameter attributes)");
				XFree(configs);  configs = NULL;
			}
		}

		// BadPixmap error thrown and NULL returned if Pixmap is not valid
		for(int badValue = 0; badValue <= 1; badValue++)
		{
			XERRTEST_START();
			if(glXCreateGLXPixmap(dpy, vis, (Pixmap)badValue))
				THROW("glXCreateGLXPixmap(bad Pixmap) != NULL");
			XERRTEST_STOP(BadPixmap, X_GLXCreateGLXPixmap,
				"BadPixmap (invalid Pixmap parameter)");

			XERRTEST_START();
			if(glXCreatePixmap(dpy, config, (Pixmap)badValue, NULL))
				THROW("glXCreateGLXPixmap(bogus Pixmap) != NULL");
			XERRTEST_STOP(BadPixmap, X_GLXCreatePixmap,
				"BadPixmap (invalid Pixmap parameter)");
		}

		//-------------------------------------------------------------------------
		//    glXCreateWindow()
		//-------------------------------------------------------------------------

		// GLXBadFBConfig error thrown and NULL returned if FB config is not valid
		XERRTEST_START();
		if(glXCreateWindow(dpy, NULL, DefaultRootWindow(dpy), NULL))
			THROW("glXCreateWindow(bogus FB config) != NULL");
		GLXERRTEST_STOP(errorBase + GLXBadFBConfig, X_GLXCreateWindow)

		// BadWindow error thrown and NULL returned if Window is not valid or is
		// not a window
		for(int badValue = 0; badValue <= 1; badValue++)
		{
			XERRTEST_START();
			if(glXCreateWindow(dpy, config, (Window)badValue, NULL))
				THROW("glXCreateGLXWindow(bogus Window) != NULL");
			XERRTEST_STOP(BadWindow, X_GLXCreateWindow,
				"BadWindow (invalid Window parameter)");
		}

		XERRTEST_START();
		if(glXCreateWindow(dpy, config, pm, NULL))
			THROW("glXCreateGLXWindow(bogus Window) != NULL");
		XERRTEST_STOP(BadWindow, X_GLXCreateWindow,
			"BadWindow (invalid Window parameter)");

		XFreePixmap(dpy, pm);  pm = 0;

		//-------------------------------------------------------------------------
		//    glXDestroyContext()
		//-------------------------------------------------------------------------

		// Technically this function should throw a GLXBadContext error if passed a
		// NULL context, but all of the major GLX implementations treat that
		// situation as a no-op.
		glXDestroyContext(dpy, NULL);

		//-------------------------------------------------------------------------
		//    glXDestroyGLXPixmap()
		//    glXDestroyPixmap()
		//-------------------------------------------------------------------------

		// These functions should throw a GLXBadPixmap error when passed an invalid
		// GLXPixmap, but VirtualGL's implementation has historically treated that
		// situation as a no-op.
		for(int badValue = 0; badValue <= 1; badValue++)
			glXDestroyGLXPixmap(dpy, (GLXPixmap)badValue);
		for(int badValue = 0; badValue <= 1; badValue++)
			glXDestroyPixmap(dpy, (GLXPixmap)badValue);

		//-------------------------------------------------------------------------
		//    glXGetConfig()
		//-------------------------------------------------------------------------

		// GLX_NO_EXTENSION returned if display doesn't support GLX
		if(glXGetConfig(NULL, vis, GLX_RGBA, &dummy1) != GLX_NO_EXTENSION)
			THROW("glXGetConfig(bad display) != GLX_NO_EXTENSION");
		// GLX_BAD_VISUAL returned if visual doesn't support GLX and an attribute
		// other than GLX_USE_GL is requested
		if(glXGetConfig(dpy, NULL, GLX_RGBA, &dummy1) != GLX_BAD_VISUAL)
			THROW("glXGetConfig(bad visual) != GLX_BAD_VISUAL");
		// GLX_BAD_ATTRIBUTE returned if attribute is not valid
		if(glXGetConfig(dpy, vis, 0x42424242, &dummy1) != GLX_BAD_ATTRIBUTE)
			THROW("glXGetConfig(bad attrib) != GLX_BAD_ATTRIBUTE");

		//-------------------------------------------------------------------------
		//    glXGetFBConfigAttrib()
		//-------------------------------------------------------------------------

		// GLX_NO_EXTENSION returned if display doesn't support GLX
		if(glXGetFBConfigAttrib(NULL, NULL, GLX_RENDER_TYPE, &dummy1)
			!= GLX_NO_EXTENSION)
			THROW("glXGetFBConfigAttrib(bad display) != GLX_NO_EXTENSION");
		// GLX_BAD_ATTRIBUTE returned if attribute is not valid
		if(glXGetFBConfigAttrib(dpy, config, 0x42424242, &dummy1)
			!= GLX_BAD_ATTRIBUTE)
			THROW("glXGetFBConfigAttrib(bad attrib) != GLX_BAD_ATTRIBUTE");

		//-------------------------------------------------------------------------
		//    glXGetSelectedEvent()
		//-------------------------------------------------------------------------

		// GLXBadDrawable error thrown if drawable is not valid
		for(int badValue = 0; badValue <= 1; badValue++)
		{
			XERRTEST_START();
			glXGetSelectedEvent(dpy, (GLXDrawable)badValue, &dummy3);
			GLXERRTEST_STOP(errorBase + GLXBadDrawable, -1);
		}

		//-------------------------------------------------------------------------
		//    glXGetVisualFromFBConfig()
		//-------------------------------------------------------------------------

		// NULL returned if FB config is not valid
		if(glXGetVisualFromFBConfig(dpy, NULL))
			THROW("glXGetVisualFromFBConfig(bogus FB config) != NULL");

		//-------------------------------------------------------------------------
		//    glXMakeCurrent()
		//    glXMakeContextCurrent()
		//-------------------------------------------------------------------------

		if(!(ctx = glXCreateContext(dpy, vis, NULL, True)))
			THROW("Could not create context");

		// VirtualGL's implementation of these functions has historically tolerated
		// a NULL drawable, because other implementations tend to tolerate that.
		// However, this should technically trigger a BadMatch (glXMakeCurrent())
		// or GLXBadDrawable (glXMakeContextCurrent()) error.  The return value
		// differs among different underlying GLX implementations.
		glXMakeCurrent(dpy, None, ctx);
		glXMakeContextCurrent(dpy, None, None, ctx);

		// GLXBadDrawable error thrown and False returned if drawable is not valid
		for(int i = 0; i < 2; i++)
		{
			// We do this twice, in order to test what happens when the unknown
			// drawable is hashed
			XERRTEST_START();
			if(glXMakeCurrent(dpy, (GLXDrawable)1, ctx))
				THROW("glXMakeCurrent(bogus drawable) succeeded");
			GLXERRTEST_STOP(errorBase + GLXBadDrawable, X_GLXMakeCurrent)

			XERRTEST_START();
			if(glXMakeContextCurrent(dpy, (GLXDrawable)2, None, ctx))
				THROW("glXMakeContextCurrent(bogus drawable) succeeded");
			GLXERRTEST_STOP(errorBase + GLXBadDrawable, X_GLXMakeContextCurrent)
		}

		// Technically these functions should throw a GLXBadContext error if passed
		// a dead context, but other implementations just return False, and at
		// least one commercial application relies on that behavior.
		glXDestroyContext(dpy, ctx);
		if(glXMakeCurrent(dpy, DefaultRootWindow(dpy), ctx)
			|| glXMakeContextCurrent(dpy, DefaultRootWindow(dpy),
				DefaultRootWindow(dpy), ctx))
		{
			ctx = 0;
			THROW("glXMake[Context]Current(dead context) succeeded");
		}
		ctx = 0;

		// GLXBadContext error thrown and False returned if context is not valid
		XERRTEST_START();
		if(glXMakeCurrent(dpy, DefaultRootWindow(dpy), NULL))
			THROW("glXMakeCurrent(NULL context) succeeded");
		GLXERRTEST_STOP(errorBase + GLXBadContext, X_GLXMakeCurrent)

		XERRTEST_START();
		if(glXMakeContextCurrent(dpy, DefaultRootWindow(dpy),
			DefaultRootWindow(dpy), NULL))
			THROW("glXMakeContextCurrent(NULL context) succeeded");
		GLXERRTEST_STOP(errorBase + GLXBadContext, X_GLXMakeContextCurrent)

		// VirtualGL can't distinguish between a dead context and a bogus non-zero
		// one, but at least we can verify that this doesn't cause a segfault.
		if(glXMakeCurrent(dpy, DefaultRootWindow(dpy), (GLXContext)1))
			THROW("glXMakeCurrent(bogus context) succeeded");
		if(glXMakeContextCurrent(dpy, DefaultRootWindow(dpy),
			DefaultRootWindow(dpy), (GLXContext)2))
			THROW("glXMakeContextCurrent(bogus context) succeeded");

		//-------------------------------------------------------------------------
		//    glXSwapIntervalEXT()
		//    glXSwapIntervalSGI()
		//-------------------------------------------------------------------------

		if(GLX_EXTENSION_EXISTS(GLX_EXT_swap_control))
		{
			PFNGLXSWAPINTERVALEXTPROC __glXSwapIntervalEXT =
				(PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress(
					(GLubyte *)"glXSwapIntervalEXT");
			if(__glXSwapIntervalEXT)
			{
				// Technically this function should throw a BadValue error if passed
				// an interval < 0, but nVidia's implementation doesn't, so we emulate
				// that behavior.
				__glXSwapIntervalEXT(dpy, DefaultRootWindow(dpy), -1);

				// Technically this function should throw a BadWindow error if passed
				// an invalid drawable, but nVidia's implementation doesn't, so we
				// emulate that behavior.
				__glXSwapIntervalEXT(dpy, 0, 1);
				__glXSwapIntervalEXT(dpy, (GLXDrawable)1, 1);
			}
		}

		if(GLX_EXTENSION_EXISTS(GLX_SGI_swap_control))
		{
			PFNGLXSWAPINTERVALSGIPROC __glXSwapIntervalSGI =
				(PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddress(
					(GLubyte *)"glXSwapIntervalSGI");
			if(__glXSwapIntervalSGI)
			{
				// Return GLX_BAD_VALUE if interval < 0
				if(__glXSwapIntervalSGI(-1) != GLX_BAD_VALUE)
					THROW("glXSwapIntervalSGI(0) != GLX_BAD_VALUE");
				// Return GLX_BAD_CONTEXT if no context is current
				if(__glXSwapIntervalSGI(1) != GLX_BAD_CONTEXT)
					THROW("glXSwapIntervalSGI() w/ no context != GLX_BAD_CONTEXT");
			}
		}

		//-------------------------------------------------------------------------
		//    glXUseXFont()
		//-------------------------------------------------------------------------

		// BadFont error thrown if font is not valid
		if(!(ctx = glXCreateContext(dpy, vis, NULL, True)))
			THROW("Could not create context");
		if(!glXMakeCurrent(dpy, DefaultRootWindow(dpy), ctx))
			THROW("Could not make context current");
		int fontListBase = glGenLists(2);
		for(int badValue = 0; badValue <= 1; badValue++)
		{
			XERRTEST_START();
			glXUseXFont((Font)badValue, 0, 2, fontListBase);
			XERRTEST_STOP(BadFont, X_GLXUseXFont,
				"BadFont (invalid Font parameter)");
		}
		glXDestroyContext(dpy, ctx);  ctx = 0;

		printf("SUCCESS!\n");
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	fflush(stdout);
	if(glxpm) glXDestroyGLXPixmap(dpy, glxpm);
	if(pm) XFreePixmap(dpy, pm);
	if(vis) XFree(vis);
	if(ctx) glXDestroyContext(dpy, ctx);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


#define TEST_PROC_SYM_OPT(f) \
	if((sym1 = (void *)glXGetProcAddressARB((const GLubyte *)#f)) == NULL) \
		THROW("glXGetProcAddressARB(\"" #f "\") returned NULL"); \
	if((sym2 = (void *)dlsym(RTLD_DEFAULT, #f)) == NULL) \
		THROW("dlsym(RTLD_DEFAULT, \"" #f "\") returned NULL"); \
	if(sym1 != sym2) \
		THROW("glXGetProcAddressARB(\"" #f "\")!=dlsym(RTLD_DEFAULT, \"" #f "\")");

#define TEST_PROC_SYM(f) \
	TEST_PROC_SYM_OPT(f) \
	if(sym1 != (void *)f) \
		THROW("glXGetProcAddressARB(\"" #f "\")!=" #f); \
	if(sym2 != (void *)f) \
		THROW("dlsym(RTLD_DEFAULT, \"" #f "\")!=" #f);

int procAddrTest(void)
{
	int retval = 1;  void *sym1 = NULL, *sym2 = NULL;
	Display *dpy = NULL;

	printf("glXGetProcAddress test:\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");

		// GLX 1.0
		TEST_PROC_SYM(glXChooseVisual)
		TEST_PROC_SYM(glXCopyContext)
		TEST_PROC_SYM(glXCreateContext)
		TEST_PROC_SYM(glXCreateGLXPixmap)
		TEST_PROC_SYM(glXDestroyContext)
		TEST_PROC_SYM(glXDestroyGLXPixmap)
		TEST_PROC_SYM(glXGetConfig)
		TEST_PROC_SYM(glXGetCurrentContext)
		TEST_PROC_SYM(glXGetCurrentDrawable)
		TEST_PROC_SYM(glXIsDirect)
		TEST_PROC_SYM(glXMakeCurrent)
		TEST_PROC_SYM(glXQueryExtension)
		TEST_PROC_SYM(glXQueryVersion)
		TEST_PROC_SYM(glXSwapBuffers)
		TEST_PROC_SYM(glXUseXFont)
		TEST_PROC_SYM(glXWaitGL)

		// GLX 1.1
		TEST_PROC_SYM(glXGetClientString)
		TEST_PROC_SYM(glXQueryServerString)
		TEST_PROC_SYM(glXQueryExtensionsString)

		// GLX 1.2
		TEST_PROC_SYM(glXGetCurrentDisplay)

		// GLX 1.3
		TEST_PROC_SYM(glXChooseFBConfig)
		TEST_PROC_SYM(glXCreateNewContext)
		TEST_PROC_SYM(glXCreatePbuffer)
		TEST_PROC_SYM(glXCreatePixmap)
		TEST_PROC_SYM(glXCreateWindow)
		TEST_PROC_SYM(glXDestroyPbuffer)
		TEST_PROC_SYM(glXDestroyPixmap)
		TEST_PROC_SYM(glXDestroyWindow)
		TEST_PROC_SYM(glXGetCurrentReadDrawable)
		TEST_PROC_SYM(glXGetFBConfigAttrib)
		TEST_PROC_SYM(glXGetFBConfigs)
		TEST_PROC_SYM(glXGetSelectedEvent)
		TEST_PROC_SYM(glXGetVisualFromFBConfig)
		TEST_PROC_SYM(glXMakeContextCurrent)
		TEST_PROC_SYM(glXQueryContext)
		TEST_PROC_SYM(glXQueryDrawable)
		TEST_PROC_SYM(glXSelectEvent)

		// GLX 1.4
		TEST_PROC_SYM(glXGetProcAddress)

		// GLX_ARB_create_context
		if(GLX_EXTENSION_EXISTS(GLX_ARB_create_context))
		{
			TEST_PROC_SYM_OPT(glXCreateContextAttribsARB)
		}

		// GLX_ARB_get_proc_address
		TEST_PROC_SYM(glXGetProcAddressARB)

		// GLX_EXT_import_context
		if(GLX_EXTENSION_EXISTS(GLX_EXT_import_context))
		{
			TEST_PROC_SYM_OPT(glXFreeContextEXT)
			TEST_PROC_SYM_OPT(glXImportContextEXT)
			TEST_PROC_SYM_OPT(glXQueryContextInfoEXT)
		}
		TEST_PROC_SYM_OPT(glXGetCurrentDisplayEXT)

		// GLX_EXT_swap_control
		TEST_PROC_SYM_OPT(glXSwapIntervalEXT)

		// GLX_EXT_texture_from_pixmap
		if(GLX_EXTENSION_EXISTS(GLX_EXT_texture_from_pixmap))
		{
			TEST_PROC_SYM_OPT(glXBindTexImageEXT)
			TEST_PROC_SYM_OPT(glXReleaseTexImageEXT)
		}

		// GLX_SGI_make_current_read
		TEST_PROC_SYM_OPT(glXGetCurrentReadDrawableSGI)
		TEST_PROC_SYM_OPT(glXMakeCurrentReadSGI)

		// GLX_SGI_swap_control
		TEST_PROC_SYM_OPT(glXSwapIntervalSGI)

		// GLX_SGIX_fbconfig
		TEST_PROC_SYM(glXChooseFBConfigSGIX)
		TEST_PROC_SYM(glXCreateContextWithConfigSGIX)
		TEST_PROC_SYM(glXCreateGLXPixmapWithConfigSGIX)
		TEST_PROC_SYM(glXGetFBConfigAttribSGIX)
		TEST_PROC_SYM(glXGetFBConfigFromVisualSGIX)
		TEST_PROC_SYM(glXGetVisualFromFBConfigSGIX)

		// GLX_SGIX_pbuffer
		TEST_PROC_SYM_OPT(glXCreateGLXPbufferSGIX)
		TEST_PROC_SYM_OPT(glXDestroyGLXPbufferSGIX)
		TEST_PROC_SYM_OPT(glXGetSelectedEventSGIX)
		TEST_PROC_SYM_OPT(glXQueryGLXPbufferSGIX)
		TEST_PROC_SYM_OPT(glXSelectEventSGIX)

		// OpenGL
		TEST_PROC_SYM(glBindFramebuffer)
		TEST_PROC_SYM(glBindFramebufferEXT)
		TEST_PROC_SYM(glDeleteFramebuffers)
		TEST_PROC_SYM(glDeleteFramebuffersEXT)
		TEST_PROC_SYM(glFinish)
		TEST_PROC_SYM(glFlush)
		TEST_PROC_SYM(glDrawBuffer)
		TEST_PROC_SYM(glDrawBuffers)
		TEST_PROC_SYM(glDrawBuffersARB)
		TEST_PROC_SYM(glDrawBuffersATI)
		TEST_PROC_SYM_OPT(glFramebufferDrawBufferEXT);
		TEST_PROC_SYM_OPT(glFramebufferDrawBuffersEXT);
		TEST_PROC_SYM_OPT(glFramebufferReadBufferEXT);
		TEST_PROC_SYM(glGetBooleanv)
		TEST_PROC_SYM(glGetDoublev)
		TEST_PROC_SYM(glGetFloatv)
		TEST_PROC_SYM(glGetFramebufferAttachmentParameteriv)
		TEST_PROC_SYM_OPT(glGetFramebufferParameteriv)
		TEST_PROC_SYM(glGetIntegerv)
		TEST_PROC_SYM(glGetInteger64v)
		TEST_PROC_SYM_OPT(glGetNamedFramebufferParameteriv)
		TEST_PROC_SYM(glGetString)
		TEST_PROC_SYM(glGetStringi)
		TEST_PROC_SYM_OPT(glNamedFramebufferDrawBuffer);
		TEST_PROC_SYM_OPT(glNamedFramebufferDrawBuffers);
		TEST_PROC_SYM_OPT(glNamedFramebufferReadBuffer);
		TEST_PROC_SYM(glPopAttrib)
		TEST_PROC_SYM(glReadBuffer)
		TEST_PROC_SYM(glReadPixels);
		TEST_PROC_SYM(glViewport)

		printf("SUCCESS!\n");
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	fflush(stdout);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-n <n> = Use <n> threads (0 <= <n> <= %d) in the multithreaded rendering test\n",
		MAXTHREADS);
	fprintf(stderr, "         (default: %d).  <n>=0 disables the multithreaded rendering test.\n",
		DEFTHREADS);
	fprintf(stderr, "-nostereo = Disable stereo tests\n");
	fprintf(stderr, "-nomultisample = Disable multisampling tests\n");
	fprintf(stderr, "-nodbpixmap = Assume GLXPixmaps are always single-buffered, even if created\n");
	fprintf(stderr, "              with a double-buffered visual or FB config.\n");
	fprintf(stderr, "-nocopycontext = Disable glXCopyContext() tests\n");
	fprintf(stderr, "-nousexfont = Disable glXUseXFont() tests\n");
	fprintf(stderr, "-nonamedfb = Disable named framebuffer function tests\n");
	fprintf(stderr, "-selectevent = Enable glXSelectEvent() tests\n");
	fprintf(stderr, "\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int ret = 0, nThreads = DEFTHREADS;
	bool doStereo = true, doMultisample = true, doDBPixmap = true,
		doCopyContext = true, doUseXFont = true, doSelectEvent = false,
		doNamedFB = true;

	if(putenv((char *)"VGL_AUTOTEST=1") == -1
		|| putenv((char *)"VGL_SPOIL=0") == -1
		|| putenv((char *)"VGL_XVENDOR=Spacely Sprockets, Inc.") == -1
		|| putenv((char *)"VGL_GLXVENDOR=Slate Rock and Gravel Company"))
	{
		printf("putenv() failed!\n");  return -1;
	}

	if(argc > 1) for(int i = 1; i < argc; i++)
	{
		if(!strcasecmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
		else if(!strcasecmp(argv[i], "-n") && i < argc - 1)
		{
			nThreads = atoi(argv[++i]);
			if(nThreads < 0 || nThreads > MAXTHREADS) usage(argv);
		}
		else if(!strcasecmp(argv[i], "-nostereo")) doStereo = false;
		else if(!strcasecmp(argv[i], "-nomultisample")) doMultisample = false;
		else if(!strcasecmp(argv[i], "-nodbpixmap")) doDBPixmap = false;
		else if(!strcasecmp(argv[i], "-nocopycontext")) doCopyContext = false;
		else if(!strcasecmp(argv[i], "-nousexfont")) doUseXFont = false;
		else if(!strcasecmp(argv[i], "-nonamedfb")) doNamedFB = false;
		else if(!strcasecmp(argv[i], "-selectevent")) doSelectEvent = true;
		else usage(argv);
	}

	// Intentionally leave a pending dlerror()
	dlsym(RTLD_NEXT, "ifThisSymbolExistsI'llEatMyHat");
	dlsym(RTLD_NEXT, "ifThisSymbolExistsI'llEatMyHat2");

	if(!XInitThreads())
		THROW("XInitThreads() failed");
	if(!extensionQueryTest()) ret = -1;
	printf("\n");
	if(!procAddrTest()) ret = -1;
	printf("\n");
	if(!readbackTest(false, doNamedFB)) ret = -1;
	printf("\n");
	if(doStereo)
	{
		if(!readbackTest(true, doNamedFB)) ret = -1;
		printf("\n");
	}
	if(doMultisample)
	{
		if(!readbackTestMS()) ret = -1;
		printf("\n");
	}
	if(!contextMismatchTest()) ret = -1;
	printf("\n");
	if(doCopyContext)
	{
		if(!copyContextTest()) ret = -1;
		printf("\n");
	}
	if(!flushTest()) ret = -1;
	printf("\n");
	if(!visTest()) ret = -1;
	printf("\n");
	if(!multiThreadTest(nThreads)) ret = -1;
	printf("\n");
	if(!offScreenTest(doDBPixmap, doUseXFont, doSelectEvent)) ret = -1;
	printf("\n");
	if(!subWinTest()) ret = -1;
	printf("\n");

	return ret;
}
