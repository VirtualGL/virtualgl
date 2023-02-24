// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2010-2015, 2017-2021, 2023 D. R. Commander
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
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <dlfcn.h>
#include <unistd.h>
#define _eglGetError  eglGetError
#include "EGLError.h"
#include "Thread.h"
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif

using namespace util;


#define CLEAR_BUFFER(r, g, b, a) \
{ \
	glClearColor(r, g, b, a); \
	glClear(GL_COLOR_BUFFER_BIT); \
}


#define VERIFY_BUF_COLOR(colorShouldBe, tag) \
{ \
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


#define EGL_EXTENSION_EXISTS(ext) \
	strstr(eglQueryString(edpy, EGL_EXTENSIONS), #ext)


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


XVisualInfo *getVisualFromConfig(Display *dpy, int screen, EGLDisplay edpy,
	EGLConfig config)
{
	if(!dpy || !edpy || !config) return NULL;

	EGLint redSize, greenSize, blueSize;
	int depth = 24;

	if(eglGetConfigAttrib(edpy, config, EGL_RED_SIZE, &redSize)
		&& eglGetConfigAttrib(edpy, config, EGL_GREEN_SIZE, &greenSize)
		&& eglGetConfigAttrib(edpy, config, EGL_BLUE_SIZE, &blueSize)
		&& redSize == 10 && greenSize == 10 && blueSize == 10)
		depth = 30;

	XVisualInfo vtemp;  int nv = 0;

	vtemp.depth = depth;
	vtemp.c_class = TrueColor;
	vtemp.screen = screen;

	return XGetVisualInfo(dpy,
		VisualDepthMask | VisualClassMask | VisualScreenMask, &vtemp, &nv);
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


void checkWindowColor(Display *dpy, Window win, unsigned int color)
{
	int fakerColor;
	typedef int (*_vgl_getAutotestColorType)(Display *, Window, int);
	_vgl_getAutotestColorType _vgl_getAutotestColor;

	_vgl_getAutotestColor =
		(_vgl_getAutotestColorType)dlsym(RTLD_DEFAULT, "_vgl_getAutotestColor");
	if(!_vgl_getAutotestColor)
		THROWNL("Can't communicate w/ faker");
	fakerColor = _vgl_getAutotestColor(dpy, win, 0);
	if(fakerColor < 0 || fakerColor > 0xffffff)
		THROWNL("Bogus data read back");
	if((unsigned int)fakerColor != color)
		PRERROR2("Color is 0x%.6x, should be 0x%.6x", fakerColor, color)
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


void checkCurrent(EGLDisplay edpy, EGLSurface draw, EGLSurface read,
	EGLContext ctx, int width, int height)
{
	if(eglGetCurrentDisplay() != edpy)
		THROW("eglGetCurrentDisplay() returned incorrect value");
	if(eglGetCurrentSurface(EGL_DRAW) != draw)
		THROW("eglGetCurrentSurface(EGL_DRAW) returned incorrect value");
	if(eglGetCurrentSurface(EGL_READ) != read)
		THROW("eglGetCurrentSurface(EGL_READ) returned incorrect value");
	if(eglGetCurrentContext() != ctx)
		THROW("eglGetCurrentContext() returned incorrect value");
	int viewport[4] = { 0, 0, 0, 0 };
	glGetIntegerv(GL_VIEWPORT, viewport);
	if(viewport[2] < 1 || viewport[3] < 1)
		THROW("Invalid viewport dimensions");
	if(viewport[2] != width || viewport[3] != height)
		PRERROR4("Viewport is %dx%d, should be %dx%d\n", viewport[2], viewport[3],
			width, height);
}


void checkReadbackState(EGLDisplay edpy, EGLSurface draw, EGLSurface read,
	EGLContext ctx)
{
	if(eglGetCurrentDisplay() != edpy)
		THROWNL("Current display changed");
	if(eglGetCurrentSurface(EGL_DRAW) != draw
		|| eglGetCurrentSurface(EGL_READ) != read)
		THROWNL("Current surface changed");
	if(eglGetCurrentContext() != ctx)
		THROWNL("Context changed");
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

		void clear(void)
		{
			CLEAR_BUFFER(r(), g(), b(), 0.)
			next();
		}

	private:

		int index;
};


// This tests the faker's readback heuristics
int readbackTest(void)
{
	TestColor clr(0), sclr(3);
	Display *dpy = NULL;  Window win0 = 0, win1 = 0;
	EGLDisplay edpy = 0;
	EGLSurface surface0 = 0, surface1 = 0;
	int dpyw, dpyh, lastFrame0 = 0, lastFrame1 = 0, retval = 1;
	int cfgAttribs[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
		EGL_CONFORMANT, EGL_OPENGL_BIT | EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT,
		EGL_NONE };
	XVisualInfo *vis = NULL;
	EGLConfig config = 0;  EGLint nc = 0;
	EGLContext ctxgl = 0, ctxes = 0;
	XSetWindowAttributes swa;

	printf("Readback heuristics test\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
		dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

		EGLAttrib displayAttribs[] = { EGL_PLATFORM_X11_SCREEN_EXT,
			DefaultScreen(dpy), EGL_NONE };
		if((edpy = eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, dpy,
			displayAttribs)) == EGL_NO_DISPLAY)
			THROW_EGL("eglGetPlatformDisplay()");
		if(!eglInitialize(edpy, NULL, NULL))
			THROW_EGL("eglInitialize()");

		if(!eglChooseConfig(edpy, cfgAttribs, &config, 1, &nc))
			THROW_EGL("eglChooseConfig()");
		if((vis = getVisualFromConfig(dpy, DefaultScreen(dpy), edpy,
			config)) == NULL)
			THROW("Could not find a suitable visual");

		Window root = RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel = 0;
		swa.event_mask = 0;
		if((win0 = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0,
			vis->depth, InputOutput, vis->visual,
			CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
			THROW("Could not create window");
		if((win1 = XCreateWindow(dpy, root, dpyw / 2, 0, dpyw / 2, dpyh / 2, 0,
			vis->depth, InputOutput, vis->visual,
			CWBorderPixel | CWColormap | CWEventMask, &swa)) == 0)
			THROW("Could not create window");
		XFree(vis);  vis = NULL;

		if(!eglBindAPI(EGL_OPENGL_ES_API))
			THROW_EGL("eglBindAPI()");
		if((ctxes = eglCreateContext(edpy, config, NULL, NULL)) == 0)
			THROW_EGL("eglCreateContext()");
		if(!eglBindAPI(EGL_OPENGL_API))
			THROW_EGL("eglBindAPI()");
		if((ctxgl = eglCreateContext(edpy, config, NULL, NULL)) == 0)
			THROW_EGL("eglCreateContext()");
		if(!eglBindAPI(EGL_OPENGL_ES_API))
			THROW_EGL("eglBindAPI()");

		if((surface0 = eglCreateWindowSurface(edpy, config, win0, NULL)) == 0)
			THROW_EGL("eglCreateWindowSurface()");
		if((surface1 = eglCreatePlatformWindowSurface(edpy, config, &win1,
			NULL)) == 0)
			THROW_EGL("eglCreatePlatformWindowSurface()");

		if(!eglMakeCurrent(edpy, surface1, surface1, ctxgl))
			THROW_EGL("eglMakeCurrent()");
		checkCurrent(edpy, surface1, surface1, ctxgl, dpyw / 2, dpyh / 2);

		if(!eglMakeCurrent(edpy, surface1, surface0, ctxes))
			THROW_EGL("eglMakeCurrent()");
		checkCurrent(edpy, surface1, surface0, ctxes, dpyw / 2, dpyh / 2);

		XMapWindow(dpy, win0);
		XMapWindow(dpy, win1);

		// Faker should read back pixels on a call to eglSwapBuffers()
		try
		{
			clr.clear();
			// Intentionally leave a pending GL error (VirtualGL should clear the
			// error state prior to readback)
			char pixel[4];
			glReadPixels(0, 0, 1, 1, 0, GL_BYTE, pixel);
			eglSwapBuffers(edpy, surface1);
			checkReadbackState(edpy, surface1, surface0, ctxes);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-1));

			// When using desktop OpenGL, no readback should occur unless the render
			// mode is GL_RENDER.
			if(!eglMakeCurrent(edpy, surface1, surface0, ctxgl))
				THROW_EGL("eglMakeCurrent()");
			clr.clear();
			GLfloat fbBuffer[2];
			glFeedbackBuffer(1, GL_2D, fbBuffer);
			glRenderMode(GL_FEEDBACK);
			eglSwapBuffers(edpy, surface1);
			glRenderMode(GL_RENDER);
			checkReadbackState(edpy, surface1, surface0, ctxgl);
			checkFrame(dpy, win1, 0, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-2));

			clr.clear();
			GLuint selectBuffer[2];
			glSelectBuffer(1, selectBuffer);
			glRenderMode(GL_SELECT);
			eglSwapBuffers(edpy, surface1);
			glRenderMode(GL_RENDER);
			checkReadbackState(edpy, surface1, surface0, ctxgl);
			checkFrame(dpy, win1, 0, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-3));

			// Now try swapping the buffers of one window while another is current
			clr.clear();
			if(!eglMakeCurrent(edpy, surface0, surface0, ctxes))
				THROW_EGL("eglMakeCurrent()");
			eglSwapBuffers(edpy, surface1);
			checkReadbackState(edpy, surface0, surface0, ctxes);
			checkFrame(dpy, win1, 1, lastFrame1);
			checkWindowColor(dpy, win1, clr.bits(-1));

			// Verify that eglSwapBuffers() works properly without a current context
			clr.clear();
			eglMakeCurrent(edpy, 0, 0, 0);
			eglSwapBuffers(edpy, surface0);
			checkReadbackState(0, 0, 0, 0);
			checkFrame(dpy, win0, 1, lastFrame0);
			checkWindowColor(dpy, win0, clr.bits(-1));

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
	if(ctxgl && edpy)
	{
		eglMakeCurrent(edpy, 0, 0, 0);  eglDestroyContext(edpy, ctxgl);
	}
	if(ctxes && edpy)
	{
		eglMakeCurrent(edpy, 0, 0, 0);  eglDestroyContext(edpy, ctxes);
	}
	if(surface0) eglDestroySurface(edpy, surface0);
	if(surface1) eglDestroySurface(edpy, surface1);
	if(win0) XDestroyWindow(dpy, win0);
	if(win1) XDestroyWindow(dpy, win1);
	if(vis) XFree(vis);
	if(edpy) eglTerminate(edpy);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


#define GET_CFG_ATTRIB(config, attrib, ctemp) \
{ \
	ctemp = -10; \
	if(!eglGetConfigAttrib(edpy, config, attrib, &ctemp) \
		|| ctemp == -10) \
		THROWNL(#attrib " cfg attrib not supported"); \
}


int EGLConfigID(EGLDisplay edpy, EGLConfig config)
{
	EGLint temp = 0;
	if(!config) THROWNL("config==NULL in EGLConfigID()");
	if(!edpy) THROWNL("display==NULL in EGLConfigID()");
	GET_CFG_ATTRIB(config, EGL_CONFIG_ID, temp);
	return temp;
}


// This tests the faker's client/server visual matching heuristics
int visTest(void)
{
	Display *dpy = NULL;
	EGLDisplay edpy = 0;
	EGLConfig *configs = NULL;
	EGLint nc = 0;
	int retval = 1;

	printf("Visual matching heuristics test\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");

		if((edpy = eglGetDisplay(dpy)) == EGL_NO_DISPLAY)
			THROW_EGL("eglGetDisplay()");
		if(!eglInitialize(edpy, NULL, NULL))
			THROW_EGL("eglInitialize()");

		if(!(eglGetConfigs(edpy, NULL, 1, &nc)))
			THROW_EGL("eglGetConfigs()");
		configs = new EGLConfig[nc];
		if(!(eglGetConfigs(edpy, configs, nc, &nc)))
			THROW_EGL("eglGetConfigs()");

		for(EGLint i = 0; i < nc; i++)
		{
			EGLint surfaceType, transparentType, nativeRenderable, visualID,
				visualType;
			GET_CFG_ATTRIB(configs[i], EGL_SURFACE_TYPE, surfaceType);
			GET_CFG_ATTRIB(configs[i], EGL_TRANSPARENT_TYPE, transparentType);
			GET_CFG_ATTRIB(configs[i], EGL_NATIVE_RENDERABLE, nativeRenderable);
			GET_CFG_ATTRIB(configs[i], EGL_NATIVE_VISUAL_ID, visualID);
			GET_CFG_ATTRIB(configs[i], EGL_NATIVE_VISUAL_TYPE, visualType);

			// VirtualGL should return a visual only for opaque GLXFBConfigs that
			// support native rendering.
			XVisualInfo *vis =
				getVisualFromConfig(dpy, DefaultScreen(dpy), edpy, configs[i]);
			bool hasVis = (vis != NULL);
			if(vis) XFree(vis);
			bool shouldHaveVis =
				((surfaceType & (EGL_WINDOW_BIT | EGL_PIXMAP_BIT)) != 0
					&& transparentType == EGL_NONE && nativeRenderable == EGL_TRUE
					&& visualID != 0 && visualType == TrueColor);
			if(hasVis != shouldHaveVis)
				PRERROR1(hasVis ?
					"CFG 0x%.2x shouldn't have matching X visual but does" :
					"No matching X visual for CFG 0x%.2x",
					EGLConfigID(edpy, configs[i]));
		}

		printf("SUCCESS!\n");
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	fflush(stdout);

	if(configs) delete [] configs;
	if(edpy) eglTerminate(edpy);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


#define DEFTHREADS  30
#define MAXTHREADS  100
bool deadYet = false;

class TestThread : public Runnable
{
	public:

		TestThread(int myRank_, Display *dpy_, Window win_, EGLDisplay edpy_) :
			myRank(myRank_), dpy(dpy_), win(win_), edpy(edpy_), doResize(false)
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
			EGLint cfgAttribs[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
				EGL_BLUE_SIZE, 8, EGL_NONE }, nc = 0;
			EGLConfig config = 0;
			EGLContext ctx = 0;
			EGLSurface surface = 0;

			try
			{
				if(!eglChooseConfig(edpy, cfgAttribs, &config, 1, &nc))
					THROW_EGL("eglChooseConfig()");
				if(!(ctx = eglCreateContext(edpy, config, NULL, NULL)))
					THROW_EGL("eglCreateContext()");
				if((surface = eglCreateWindowSurface(edpy, config, win, NULL)) == 0)
					THROW_EGL("eglCreateWindowSurface()");
				if(!(eglMakeCurrent(edpy, surface, surface, ctx)))
					THROW_EGL("eglMakeCurrent()");

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
					eglSwapBuffers(edpy, surface);
					checkReadbackState(edpy, surface, surface, ctx);
					checkFrame(dpy, win, 1, lastFrame);
					checkWindowColor(dpy, win, colors[clr].bits);
					clr = (clr + 1) % NC;
					iter++;
				}
			}
			catch(...)
			{
				if(ctx)
				{
					eglMakeCurrent(edpy, 0, 0, 0);
					eglDestroyContext(edpy, ctx);
				}
				if(surface) eglDestroySurface(edpy, surface);
				throw;
			}
			if(ctx)
			{
				eglMakeCurrent(edpy, 0, 0, 0);
				eglDestroyContext(edpy, ctx);
			}
			if(surface) eglDestroySurface(edpy, surface);
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
		EGLDisplay edpy;
		bool doResize;
		int width, height;
};


int multiThreadTest(int nThreads)
{
	EGLint cfgAttribs[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
		EGL_NONE }, nc = 0;
	EGLConfig config = 0;
	XVisualInfo *vis = NULL;
	Display *dpy = NULL;
	EGLDisplay edpy = 0;
	Window windows[MAXTHREADS];
	TestThread *testThreads[MAXTHREADS];  Thread *threads[MAXTHREADS];
	XSetWindowAttributes swa;
	int i, retval = 1;

	if(nThreads == 0) return 1;
	for(i = 0; i < nThreads; i++)
	{
		windows[i] = 0;  testThreads[i] = NULL;  threads[i] = NULL;
	}
	#ifdef USEHELGRIND
	ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
	#endif

	printf("Multithreaded rendering test (%d threads)\n\n", nThreads);

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");

		if((edpy = eglGetDisplay(dpy)) == EGL_NO_DISPLAY)
			THROW_EGL("eglGetDisplay()");
		if(!eglInitialize(edpy, NULL, NULL))
			THROW_EGL("eglInitialize()");

		if(!eglChooseConfig(edpy, cfgAttribs, &config, 1, &nc))
			THROW_EGL("eglChooseConfig()");
		if(!(vis = getVisualFromConfig(dpy, DefaultScreen(dpy), edpy, config)))
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
			XMoveResizeWindow(dpy, windows[i], winX, winY, 100, 100);
		}
		XSync(dpy, False);
		XFree(vis);  vis = NULL;

		for(i = 0; i < nThreads; i++)
		{
			testThreads[i] = new TestThread(i, dpy, windows[i], edpy);
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
		if(dpy && windows[i]) XDestroyWindow(dpy, windows[i]);
	}
	if(edpy) eglTerminate(edpy);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


#define COMPARE_DRAW_ATTRIB(edpy, draw, value, attrib) \
{ \
	if(value >= 0) \
	{ \
		EGLint temp = -1; \
		if(!eglQuerySurface(edpy, draw, attrib, &temp) || temp == -1) \
			THROW(#attrib " attribute not supported"); \
		if(temp != value) \
			PRERROR3("%s=%d (should be %d)", #attrib, temp, value); \
	} \
}

void checkSurface(EGLDisplay edpy, EGLSurface draw, int width, int height,
	int largestPbuffer, int cfgid)
{
	if(!edpy || !draw) THROW("Invalid argument to checkSurface()");
	COMPARE_DRAW_ATTRIB(edpy, draw, width, EGL_WIDTH);
	COMPARE_DRAW_ATTRIB(edpy, draw, height, EGL_HEIGHT);
	COMPARE_DRAW_ATTRIB(edpy, draw, largestPbuffer, EGL_LARGEST_PBUFFER);
	COMPARE_DRAW_ATTRIB(edpy, draw, cfgid, EGL_CONFIG_ID);
}

// Test off-screen rendering
int offScreenTest(void)
{
	Display *dpy = NULL;  Window win = 0;
	EGLDisplay edpy = 0;
	EGLSurface pb = 0, winSurface = 0;
	int dpyw, dpyh, lastFrame = 0, retval = 1;
	int cfgAttribs[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_STENCIL_SIZE, 0,
		EGL_NONE };
	XVisualInfo *vis = NULL;  EGLConfig config = 0;
	EGLint nc = 0;
	EGLContext ctx = 0;
	XSetWindowAttributes swa;
	GLuint fbo = 0, rbo = 0;
	TestColor clr(0);

	printf("Off-screen rendering test\n\n");

	try
	{
		if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
		dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

		if((edpy = eglGetDisplay(dpy)) == EGL_NO_DISPLAY)
			THROW_EGL("eglGetDisplay()");
		if(!eglInitialize(edpy, NULL, NULL))
			THROW_EGL("eglInitialize()");

		if(!eglChooseConfig(edpy, cfgAttribs, &config, 1, &nc))
			THROW_EGL("eglChooseConfig()");
		int cfgid = EGLConfigID(edpy, config);
		if((vis = getVisualFromConfig(dpy, DefaultScreen(dpy), edpy,
			config)) == NULL)
			THROW("Could not find matching visual for config");

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
		if((winSurface = eglCreateWindowSurface(edpy, config, win, NULL)) == 0)
			THROW_EGL("eglCreateWindowSurface()");
		checkSurface(edpy, winSurface, dpyw / 2, dpyh / 2, -1, cfgid);

		int pbAttribs[] = { EGL_WIDTH, dpyw / 2, EGL_HEIGHT, dpyh / 2, EGL_NONE };
		if((pb = eglCreatePbufferSurface(edpy, config, pbAttribs)) == 0)
			THROW_EGL("eglCreatePbufferSurface()");
		checkSurface(edpy, pb, dpyw / 2, dpyh / 2, 0, cfgid);

		if(!eglBindAPI(EGL_OPENGL_API))
			THROW_EGL("eglBindAPI()");
		if(!(ctx = eglCreateContext(edpy, config, NULL, NULL)))
			THROW_EGL("eglCreateContext()");
		if(!eglBindAPI(EGL_OPENGL_ES_API))
			THROW_EGL("eglBindAPI()");

		if(!eglMakeCurrent(edpy, winSurface, winSurface, ctx))
			THROW_EGL("eglMakeCurrent()");
		checkCurrent(edpy, winSurface, winSurface, ctx, dpyw / 2, dpyh / 2);

		if(!eglMakeCurrent(edpy, pb, pb, ctx))
			THROW_EGL("eglMakeCurrent()");
		checkCurrent(edpy, pb, pb, ctx, dpyw / 2, dpyh / 2);

		try
		{
			printf("Pbuffer->Window:                ");
			clr.clear();
			VERIFY_BUF_COLOR(clr.bits(-1), "PB");
			if(!(eglMakeCurrent(edpy, winSurface, pb, ctx)))
				THROW_EGL("eglMakeCurrent()");
			checkCurrent(edpy, winSurface, pb, ctx, dpyw / 2, dpyh / 2);
			glCopyPixels(0, 0, dpyw / 2, dpyh / 2, GL_COLOR);
			eglSwapBuffers(edpy, winSurface);
			checkFrame(dpy, win, 1, lastFrame);
			checkReadbackState(edpy, winSurface, pb, ctx);
			checkWindowColor(dpy, win, clr.bits(-1));
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
			if(!(eglMakeCurrent(edpy, winSurface, winSurface, ctx)))
				THROW_EGL("eglMakeCurrent()");
			checkCurrent(edpy, winSurface, winSurface, ctx, dpyw / 2, dpyh / 2);
			clr.clear();
			VERIFY_BUF_COLOR(clr.bits(-1), "Win");
			if(!(eglMakeCurrent(edpy, pb, winSurface, ctx)))
				THROW_EGL("eglMakeCurrent()");
			checkCurrent(edpy, pb, winSurface, ctx, dpyw / 2, dpyh / 2);
			glCopyPixels(0, 0, dpyw / 2, dpyh / 2, GL_COLOR);
			VERIFY_BUF_COLOR(clr.bits(-1), "PB");
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
			if(!(eglMakeCurrent(edpy, winSurface, winSurface, ctx)))
				THROW_EGL("eglMakeCurrent()");
			glReadBuffer(GL_BACK);
			checkCurrent(edpy, winSurface, winSurface, ctx, dpyw / 2, dpyh / 2);
			clr.clear();
			VERIFY_BUF_COLOR(clr.bits(-1), "Win");
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
			clr.clear();
			VERIFY_BUF_COLOR(clr.bits(-1), "FBO");
			if(!eglMakeCurrent(edpy, pb, pb, ctx))
				THROW_EGL("eglMakeCurrent()");
			checkCurrent(edpy, pb, pb, ctx, dpyw / 2, dpyh / 2);
			VERIFY_FBO(fbo, GL_COLOR_ATTACHMENT0_EXT, GL_NONE, fbo,
				GL_COLOR_ATTACHMENT0_EXT);
			if(!(eglMakeCurrent(edpy, winSurface, winSurface, ctx)))
				THROW_EGL("eglMakeCurrent()");
			checkCurrent(edpy, winSurface, winSurface, ctx, dpyw / 2, dpyh / 2);
			VERIFY_FBO(fbo, GL_COLOR_ATTACHMENT0_EXT, GL_NONE, fbo,
				GL_COLOR_ATTACHMENT0_EXT);
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
			VERIFY_FBO(0, GL_BACK, GL_NONE, fbo, GL_COLOR_ATTACHMENT0_EXT);
			glFramebufferRenderbufferEXT(GL_DRAW_FRAMEBUFFER_EXT,
				GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, 0);
			eglSwapBuffers(edpy, winSurface);
			checkFrame(dpy, win, 1, lastFrame);
			checkReadbackState(edpy, winSurface, winSurface, ctx);
			checkWindowColor(dpy, win, clr.bits(-2));
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_BACK);
			clr.clear();
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
			VERIFY_FBO(fbo, GL_COLOR_ATTACHMENT0_EXT, GL_NONE, fbo,
				GL_COLOR_ATTACHMENT0_EXT);
			VERIFY_BUF_COLOR(clr.bits(-2), "FBO");
			eglSwapBuffers(edpy, winSurface);
			checkFrame(dpy, win, 1, lastFrame);
			checkWindowColor(dpy, win, clr.bits(-1));
			glDeleteRenderbuffersEXT(1, &rbo);  rbo = 0;
			glDeleteFramebuffersEXT(1, &fbo);  fbo = 0;
			VERIFY_FBO(0, GL_BACK, GL_NONE, 0, GL_BACK);
			VERIFY_BUF_COLOR(clr.bits(-1), "Win");

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
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_RENDERBUFFER_EXT, 0);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	if(rbo) { glDeleteRenderbuffersEXT(1, &rbo);  rbo = 0; }
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	if(fbo) { glDeleteFramebuffersEXT(1, &fbo);  fbo = 0; }
	if(ctx && edpy)
	{
		eglMakeCurrent(edpy, 0, 0, 0);  eglDestroyContext(edpy, ctx);
	}
	if(pb && edpy) eglDestroySurface(edpy, pb);
	if(winSurface && edpy) eglDestroySurface(edpy, winSurface);
	if(win && dpy) XDestroyWindow(dpy, win);
	if(vis) XFree(vis);
	if(edpy) eglTerminate(edpy);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


#define EGLERRTEST(f, expectedError) \
{ \
	if(f) \
		PRERROR2("%d: %s did not fail as expected", __LINE__, #f); \
	EGLint error = eglGetError(); \
	if(error != expectedError) \
		PRERROR3("%d: EGL error code %.4X != %.4X", __LINE__, error, \
			expectedError); \
}

int extensionQueryTest(void)
{
	Display *dpy = NULL;  int retval = 1;
	EGLDisplay edpy = EGL_NO_DISPLAY;
	int n = 0;
	EGLContext ctx = 0;
	EGLSurface surface = 0;

	printf("Extension query and error handling test:\n\n");

	try
	{
		int major = -1, minor = -1, dummy1;
		EGLConfig config = 0;

		if((dpy = XkbOpenDisplay(0, &dummy1, &dummy1, NULL, NULL,
			&dummy1)) == NULL)
			THROW("Could not open display");

		if((edpy = eglGetDisplay(dpy)) == EGL_NO_DISPLAY)
			THROW_EGL("eglGetDisplay()");
		if(!eglInitialize(edpy, &major, &minor))
			THROW_EGL("eglInitialize()");
		if(major != 1 || minor != 5)
			THROW("Incorrect EGL version returned from eglInitialize()");

		char *vendor = XServerVendor(dpy);
		if(!vendor || strcmp(vendor, "Spacely Sprockets, Inc."))
			THROW("XServerVendor()");
		vendor = ServerVendor(dpy);
		if(!vendor || strcmp(vendor, "Spacely Sprockets, Inc."))
			THROW("ServerVendor()");

		vendor = (char *)eglQueryString(edpy, EGL_VENDOR);
		if(!vendor || strcmp(vendor, "VirtualGL"))
			THROW_EGL("eglQueryString(..., EGL_VENDOR)");
		const char *version = eglQueryString(edpy, EGL_VERSION);
		if(!version || strcmp(version, "1.5"))
			THROW_EGL("eglQueryString(..., EGL_VERSION)");

		// Make sure that GL_EXT_x11_sync_object isn't exposed.  Since that
		// extension allows OpenGL functions (which live on the GPU) to operate on
		// XSyncFence objects (which live on the 2D X server), the extension does
		// not currently work with the VirtualGL Faker.

		EGLint nc;
		if(!eglChooseConfig(edpy, NULL, &config, 1, &nc) || nc < 1)
			THROW_EGL("eglChooseConfig()");
		if(!(ctx = eglCreateContext(edpy, config, NULL, NULL)))
			THROW_EGL("eglCreateContext()");
		EGLint pbAttribs[] = { EGL_WIDTH, 10, EGL_HEIGHT, 10, EGL_NONE };
		if(!(surface = eglCreatePbufferSurface(edpy, config, pbAttribs)))
			THROW_EGL("eglCreatePbufferSurface()");
		if(!eglMakeCurrent(edpy, surface, surface, ctx))
			THROW_EGL("eglMakeCurrent()");

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

		if(!eglMakeCurrent(edpy, 0, 0, 0))
			THROW_EGL("eglMakeCurrent()");
		if(!eglDestroySurface(edpy, surface))
			THROW_EGL("eglDestroySurface()");
		surface = 0;
		if(!eglDestroyContext(edpy, ctx))
			THROW_EGL("eglDestroyContext()");
		ctx = 0;
		if(eglGetProcAddress("glImportSyncEXT"))
			THROW("glImportSyncExt() is exposed but shouldn't be");

		// Test whether particular EGL functions trigger the errors that the spec
		// says they should.  We only test for the errors that the EGL/X11
		// interposer generates, not for errors that the underlying EGL
		// implementation generates.

		//-------------------------------------------------------------------------
		//    eglChooseConfig()
		//-------------------------------------------------------------------------

		EGLint cfgAttribs[] = { EGL_NONE };

		if(!eglTerminate(edpy))
			THROW_EGL("eglTerminate()");
		EGLERRTEST(eglChooseConfig(edpy, cfgAttribs, &config, 1, &nc),
			EGL_NOT_INITIALIZED);
		if(!eglInitialize(edpy, &major, &minor))
			THROW_EGL("eglInitialize()");

		EGLERRTEST(eglChooseConfig(edpy, cfgAttribs, &config, 1, NULL),
			EGL_BAD_PARAMETER);

		//-------------------------------------------------------------------------
		//    eglCreateWindowSurface()
		//-------------------------------------------------------------------------

		if(!eglChooseConfig(edpy, cfgAttribs, &config, 1, &nc))
			THROW_EGL("eglChooseConfig()");

		EGLERRTEST(eglCreateWindowSurface(edpy, (EGLConfig)0,
			DefaultRootWindow(dpy), NULL), EGL_BAD_CONFIG)
		EGLERRTEST(eglCreatePlatformWindowSurface(edpy, config, (void *)0, NULL),
			EGL_BAD_NATIVE_WINDOW);

		if(!(surface = eglCreateWindowSurface(edpy, config, DefaultRootWindow(dpy),
			NULL)))
			THROW_EGL("eglCreateWindowSurface()");
		EGLERRTEST(eglCreateWindowSurface(edpy, config, DefaultRootWindow(dpy),
			NULL), EGL_BAD_ALLOC);
		if(!eglDestroySurface(edpy, surface))
			THROW_EGL("eglDestroySurface()");
		surface = 0;

		// Test whether Pbuffer-specific attributes are accepted.
		EGLint pbAttribs2[] = { EGL_LARGEST_PBUFFER, EGL_TRUE, EGL_NONE };
		PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC
			__eglCreatePlatformWindowSurfaceEXT =
				(PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)eglGetProcAddress(
					"eglCreatePlatformWindowSurfaceEXT");
		if(!__eglCreatePlatformWindowSurfaceEXT)
			THROW("eglCreatePlatformWindowSurfaceEXT() is not available");
		Window defaultRootWindow = DefaultRootWindow(dpy);
		EGLERRTEST(__eglCreatePlatformWindowSurfaceEXT(edpy, config,
			&defaultRootWindow, pbAttribs2), EGL_BAD_ATTRIBUTE);
		pbAttribs2[0] = EGL_WIDTH;  pbAttribs2[1] = 10;
		EGLERRTEST(eglCreateWindowSurface(edpy, config, DefaultRootWindow(dpy),
			pbAttribs2), EGL_BAD_ATTRIBUTE);
		EGLAttrib pbAttribs3[] = { EGL_HEIGHT, 10, EGL_NONE };
		EGLERRTEST(eglCreatePlatformWindowSurface(edpy, config, &defaultRootWindow,
			pbAttribs3), EGL_BAD_ATTRIBUTE);

		//-------------------------------------------------------------------------
		//    eglGetPlatformDisplay()
		//-------------------------------------------------------------------------

		EGLAttrib displayAttribs[] = { EGL_PLATFORM_X11_SCREEN_EXT,
			ScreenCount(dpy), EGL_NONE };

		EGLERRTEST(eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, dpy,
			displayAttribs), EGL_BAD_ATTRIBUTE);

		//-------------------------------------------------------------------------
		//    eglMakeCurrent()
		//-------------------------------------------------------------------------

		if(!(ctx = eglCreateContext(edpy, config, NULL, NULL)))
			THROW_EGL("eglCreateContext()");

		if(!eglTerminate(edpy))
			THROW_EGL("eglTerminate()");
		EGLERRTEST(eglMakeCurrent(edpy, NULL, NULL, ctx), EGL_NOT_INITIALIZED);
		if(!eglMakeCurrent(edpy, NULL, NULL, NULL))
			THROW_EGL("eglMakeCurrent(edpy, NULL, NULL, NULL)");
		if(!eglInitialize(edpy, &major, &minor))
			THROW_EGL("eglInitialize()");

		if(!eglDestroyContext(edpy, ctx))
			THROW_EGL("eglDestroyContext()");
		ctx = 0;

		printf("SUCCESS!\n");
	}
	catch(std::exception &e)
	{
		printf("Failed! (%s)\n", e.what());  retval = 0;
	}
	fflush(stdout);
	if(surface) eglDestroySurface(edpy, surface);
	if(ctx)
	{
		eglMakeCurrent(edpy, 0, 0, 0);
		eglDestroyContext(edpy, ctx);
	}
	if(edpy) eglTerminate(edpy);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}


#define TEST_PROC_SYM_OPT(f) \
	if((sym1 = (void *)eglGetProcAddress(#f)) == NULL) \
		THROW("eglGetProcAddress(\"" #f "\") returned NULL"); \
	if((sym2 = (void *)dlsym(RTLD_DEFAULT, #f)) == NULL) \
		THROW("dlsym(RTLD_DEFAULT, \"" #f "\") returned NULL"); \
	if(sym1 != sym2) \
		THROW("eglGetProcAddress(\"" #f "\")!=dlsym(RTLD_DEFAULT, \"" #f "\")");

#define TEST_PROC_SYM(f) \
	TEST_PROC_SYM_OPT(f) \
	if(sym1 != (void *)f) \
		THROW("eglGetProcAddress(\"" #f "\")!=" #f); \
	if(sym2 != (void *)f) \
		THROW("dlsym(RTLD_DEFAULT, \"" #f "\")!=" #f);

int procAddrTest(void)
{
	int retval = 1;  void *sym1 = NULL, *sym2 = NULL;

	printf("eglGetProcAddress test:\n\n");

	try
	{
		// EGL 1.0
		TEST_PROC_SYM(eglChooseConfig);
		TEST_PROC_SYM(eglCopyBuffers);
		TEST_PROC_SYM(eglCreateContext);
		TEST_PROC_SYM(eglCreatePbufferSurface);
		TEST_PROC_SYM(eglCreatePixmapSurface);
		TEST_PROC_SYM(eglCreateWindowSurface);
		TEST_PROC_SYM(eglDestroyContext);
		TEST_PROC_SYM(eglDestroySurface);
		TEST_PROC_SYM(eglGetConfigAttrib);
		TEST_PROC_SYM(eglGetConfigs);
		TEST_PROC_SYM(eglGetCurrentDisplay);
		TEST_PROC_SYM(eglGetCurrentSurface);
		TEST_PROC_SYM(eglGetDisplay);
		TEST_PROC_SYM(eglGetError);
		TEST_PROC_SYM(eglGetProcAddress);
		TEST_PROC_SYM(eglInitialize);
		TEST_PROC_SYM(eglMakeCurrent);
		TEST_PROC_SYM(eglQueryContext);
		TEST_PROC_SYM(eglQueryString);
		TEST_PROC_SYM(eglQuerySurface);
		TEST_PROC_SYM(eglSwapBuffers);
		TEST_PROC_SYM(eglTerminate);

		// EGL 1.1
		TEST_PROC_SYM(eglBindTexImage);
		TEST_PROC_SYM(eglReleaseTexImage);
		TEST_PROC_SYM(eglSurfaceAttrib);
		TEST_PROC_SYM(eglSwapInterval);

		// EGL 1.2
		TEST_PROC_SYM(eglCreatePbufferFromClientBuffer);

		// EGL 1.5
		TEST_PROC_SYM(eglClientWaitSync);
		TEST_PROC_SYM(eglCreateImage);
		TEST_PROC_SYM(eglCreatePlatformPixmapSurface);
		TEST_PROC_SYM(eglCreatePlatformWindowSurface);
		TEST_PROC_SYM(eglCreateSync);
		TEST_PROC_SYM(eglDestroyImage);
		TEST_PROC_SYM(eglDestroySync);
		TEST_PROC_SYM(eglGetPlatformDisplay);
		TEST_PROC_SYM(eglGetSyncAttrib);
		TEST_PROC_SYM(eglWaitSync);

		// EGL_EXT_device_query
		TEST_PROC_SYM_OPT(eglQueryDisplayAttribEXT);

		// EGL_EXT_platform_base
		TEST_PROC_SYM_OPT(eglCreatePlatformPixmapSurfaceEXT);
		TEST_PROC_SYM_OPT(eglCreatePlatformWindowSurfaceEXT);
		TEST_PROC_SYM_OPT(eglGetPlatformDisplayEXT);

		// EGL_KHR_cl_event2
		TEST_PROC_SYM_OPT(eglCreateSync64KHR);

		// EGL_KHR_fence_sync
		TEST_PROC_SYM_OPT(eglClientWaitSyncKHR);
		TEST_PROC_SYM_OPT(eglCreateSyncKHR);
		TEST_PROC_SYM_OPT(eglDestroySyncKHR);
		TEST_PROC_SYM_OPT(eglGetSyncAttribKHR);

		// EGL_KHR_image
		TEST_PROC_SYM_OPT(eglCreateImageKHR);
		TEST_PROC_SYM_OPT(eglDestroyImageKHR);

		// EGL_KHR_reusable_sync
		TEST_PROC_SYM_OPT(eglSignalSyncKHR);

		// EGL_KHR_wait_sync
		TEST_PROC_SYM_OPT(eglWaitSyncKHR);

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
	fprintf(stderr, "\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int ret = 0, nThreads = DEFTHREADS;

	if(putenv((char *)"VGL_AUTOTEST=1") == -1
		|| putenv((char *)"VGL_SPOIL=0") == -1
		|| putenv((char *)"VGL_XVENDOR=Spacely Sprockets, Inc.") == -1)
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
	if(!readbackTest()) ret = -1;
	printf("\n");
	if(!visTest()) ret = -1;
	printf("\n");
	if(!multiThreadTest(nThreads)) ret = -1;
	printf("\n");
	if(!offScreenTest()) ret = -1;
	printf("\n");

	return ret;
}
