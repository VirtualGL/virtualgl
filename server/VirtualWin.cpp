// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009-2015, 2017-2021 D. R. Commander
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

#include "VirtualWin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fakerconfig.h"
#include "glxvisual.h"
#include "vglutil.h"

using namespace util;
using namespace common;
using namespace faker;
using namespace server;


static const int trans2pf[RRTRANS_FORMATOPT] =
{
	PF_RGB, PF_RGBX, PF_BGR, PF_BGRX, PF_XBGR, PF_XRGB
};


#define LEYE(buf) \
	(buf == GL_BACK ? GL_BACK_LEFT : (buf == GL_FRONT ? GL_FRONT_LEFT : buf))
#define REYE(buf) \
	(buf == GL_BACK ? GL_BACK_RIGHT : (buf == GL_FRONT ? GL_FRONT_RIGHT : buf))
#define IS_ANAGLYPHIC(mode) \
	(mode >= RRSTEREO_REDCYAN && mode <= RRSTEREO_BLUEYELLOW)
#define IS_PASSIVE(mode) \
	(mode >= RRSTEREO_INTERLEAVED && mode <= RRSTEREO_SIDEBYSIDE)


// This class encapsulates the 3D off-screen drawable, its most recent
// ancestor, and information specific to its corresponding X window

VirtualWin::VirtualWin(Display *dpy_, Window win) :
	VirtualDrawable(dpy_, win)
{
	eventdpy = NULL;
	oldDraw = NULL;  newWidth = newHeight = -1;
	x11trans = NULL;
	#ifdef USEXV
	xvtrans = NULL;
	#endif
	vglconn = NULL;
	profGamma.setName("Gamma     ");
	profAnaglyph.setName("Anaglyph  ");
	profPassive.setName("Stereo Gen");
	syncdpy = false;
	dirty = false;
	rdirty = false;
	fconfig_setdefaultsfromdpy(dpy);
	plugin = NULL;
	deletedByWM = false;
	handleWMDelete = false;
	newConfig = false;
	swapInterval = 0;
	alreadyWarnedPluginRenderMode = false;
	XWindowAttributes xwa;
	if(!XGetWindowAttributes(dpy, win, &xwa) || !xwa.visual)
		throw(Error(__FUNCTION__, "Invalid window", -1));
	if(!fconfig.wm && !(xwa.your_event_mask & StructureNotifyMask))
	{
		if(!(eventdpy = _XOpenDisplay(DisplayString(dpy))))
			THROW("Could not clone X display connection");
		XSelectInput(eventdpy, win, StructureNotifyMask);
		if(fconfig.verbose)
			vglout.println("[VGL] Selecting structure notify events in window 0x%.8x",
				win);
	}
	stereoVisual = false;
	#ifdef EGLBACKEND
	if(edpy != EGL_NO_DISPLAY)
	#endif
		stereoVisual = glxvisual::visAttrib(dpy, DefaultScreen(dpy),
			xwa.visual->visualid, GLX_STEREO);
}


VirtualWin::~VirtualWin(void)
{
	mutex.lock(false);
	delete oldDraw;  oldDraw = NULL;
	delete x11trans;  x11trans = NULL;
	delete vglconn;  vglconn = NULL;
	#ifdef USEXV
	delete xvtrans;  xvtrans = NULL;
	#endif
	if(plugin)
	{
		try
		{
			delete plugin;  plugin = NULL;
		}
		catch(std::exception &e)
		{
			if(fconfig.verbose)
				vglout.println("[VGL] WARNING: %s", e.what());
		}
	}
	if(eventdpy) { _XCloseDisplay(eventdpy);  eventdpy = NULL; }
	mutex.unlock(false);
}


int VirtualWin::init(int w, int h, VGLFBConfig config_)
{
	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	return VirtualDrawable::init(w, h, config_);
}


// The resize doesn't actually occur until the next time updatedrawable() is
// called

void VirtualWin::resize(int width, int height)
{
	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	if(width == 0 && oglDraw) width = oglDraw->getWidth();
	if(height == 0 && oglDraw) height = oglDraw->getHeight();
	if(oglDraw && oglDraw->getWidth() == width && oglDraw->getHeight() == height)
	{
		newWidth = newHeight = -1;
		return;
	}
	newWidth = width;  newHeight = height;
}


// The FB config change doesn't actually occur until the next time
// updatedrawable() is called

void VirtualWin::checkConfig(VGLFBConfig config_)
{
	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	if(FBCID(config_) != FBCID(config))
	{
		config = config_;  newConfig = true;
	}
}


void VirtualWin::clear(void)
{
	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	VirtualDrawable::clear();
}


void VirtualWin::cleanup(void)
{
	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	delete oldDraw;  oldDraw = NULL;
}


void VirtualWin::initFromWindow(VGLFBConfig config_)
{
	#ifdef EGLBACKEND
	if(edpy != EGL_NO_DISPLAY)
		THROW("VirtualWin::initFromWindow() method not supported with EGL/X11");
	#endif

	XSync(dpy, False);
	XWindowAttributes xwa;
	XGetWindowAttributes(dpy, x11Draw, &xwa);
	init(xwa.width, xwa.height, config_);
}


// Get the current 3D off-screen drawable

GLXDrawable VirtualWin::getGLXDrawable(void)
{
	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	return VirtualDrawable::getGLXDrawable();
}


void VirtualWin::checkResize(void)
{
	if(eventdpy)
	{
		XSync(dpy, False);
		while(XPending(eventdpy) > 0)
		{
			XEvent event;
			_XNextEvent(eventdpy, &event);
			if(event.type == ConfigureNotify && event.xconfigure.window == x11Draw
				&& event.xconfigure.width > 0 && event.xconfigure.height > 0)
				resize(event.xconfigure.width, event.xconfigure.height);
		}
	}
}


// Get the current 3D off-screen drawable, but resize the drawable (or change
// its FB config) first if necessary

GLXDrawable VirtualWin::updateGLXDrawable(void)
{
	#ifdef EGLBACKEND
	if(edpy != EGL_NO_DISPLAY)
		THROW("VirtualWin::updateGLXDrawable() method not supported with EGL/X11");
	#endif

	GLXDrawable retval = 0;
	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	if(newConfig)
	{
		if(newWidth <= 0 && oglDraw) newWidth = oglDraw->getWidth();
		if(newHeight <= 0 && oglDraw) newHeight = oglDraw->getHeight();
		newConfig = false;
	}
	if(newWidth > 0 && newHeight > 0)
	{
		OGLDrawable *draw = oglDraw;
		if(init(newWidth, newHeight, config)) oldDraw = draw;
		newWidth = newHeight = -1;
	}
	retval = oglDraw->getGLXDrawable();
	return retval;
}


void VirtualWin::swapBuffers(void)
{
	#ifdef EGLBACKEND
	if(edpy != EGL_NO_DISPLAY)
		THROW("VirtualWin::swapBuffers() method not supported with EGL/X11");
	#endif

	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	if(oglDraw)
	{
		if(fconfig.amdgpuHack)
			copyPixels(0, 0, oglDraw->getWidth(), oglDraw->getHeight(), 0, 0,
				getGLXDrawable(), GL_BACK, GL_FRONT);
		else
			oglDraw->swap();
	}
}


void VirtualWin::wmDeleted(void)
{
	CriticalSection::SafeLock l(mutex);
	deletedByWM = handleWMDelete;
}


void VirtualWin::enableWMDeleteHandler(void)
{
	CriticalSection::SafeLock l(mutex);
	handleWMDelete = true;
}


void VirtualWin::readback(GLint drawBuf, bool spoilLast, bool sync)
{
	fconfig_reloadenv();
	bool doStereo = false;  int stereoMode = fconfig.stereo;

	if(fconfig.readback == RRREAD_NONE || !checkRenderMode())
		return;

	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");

	dirty = false;

	int compress = fconfig.compress;
	if(sync && strlen(fconfig.transport) == 0) compress = RRCOMP_PROXY;

	if(isStereo() && stereoMode != RRSTEREO_LEYE && stereoMode != RRSTEREO_REYE)
	{
		if(DrawingToRight() || rdirty) doStereo = true;
		rdirty = false;
		if(doStereo && compress == RRCOMP_YUV && strlen(fconfig.transport) == 0)
		{
			static bool message3 = false;
			if(!message3)
			{
				vglout.println("[VGL] NOTICE: Quad-buffered stereo cannot be used with YUV encoding.");
				vglout.println("[VGL]    Using anaglyphic stereo instead.");
				message3 = true;
			}
			stereoMode = RRSTEREO_REDCYAN;
		}
		else if(doStereo && _Trans[compress] != RRTRANS_VGL
			&& stereoMode == RRSTEREO_QUADBUF && strlen(fconfig.transport) == 0)
		{
			static bool message = false;
			if(!message)
			{
				vglout.println("[VGL] NOTICE: Quad-buffered stereo requires the VGL Transport.");
				vglout.println("[VGL]    Using anaglyphic stereo instead.");
				message = true;
			}
			stereoMode = RRSTEREO_REDCYAN;
		}
		else if(doStereo && !stereoVisual && stereoMode == RRSTEREO_QUADBUF
			&& strlen(fconfig.transport) == 0)
		{
			static bool message2 = false;
			if(!message2)
			{
				vglout.println("[VGL] NOTICE: Cannot use quad-buffered stereo because no stereo visuals are");
				vglout.println("[VGL]    available on the 2D X server.  Using anaglyphic stereo instead.");
				message2 = true;
			}
			stereoMode = RRSTEREO_REDCYAN;
		}
	}

	if(strlen(fconfig.transport) > 0)
	{
		sendPlugin(drawBuf, spoilLast, sync, doStereo, stereoMode);
		return;
	}

	switch(compress)
	{
		case RRCOMP_PROXY:
			sendX11(drawBuf, spoilLast, sync, doStereo, stereoMode);
			break;

		case RRCOMP_JPEG:
		case RRCOMP_RGB:
		case RRCOMP_YUV:
			if(!vglconn)
			{
				vglconn = new VGLTrans();
				vglconn->connect(
					strlen(fconfig.client) > 0 ? fconfig.client : DisplayString(dpy),
					fconfig.port);
			}
			sendVGL(drawBuf, spoilLast, doStereo, stereoMode, compress, fconfig.qual,
				fconfig.subsamp);
			break;
		#ifdef USEXV
		case RRCOMP_XV:
			sendXV(drawBuf, spoilLast, sync, doStereo, stereoMode);
		#endif
	}
}


TempContext *VirtualWin::setupPluginTempContext(GLint drawBuf)
{
	// This code is largely copied from VirtualDrawable::readPixels().  It
	// establishes a temporary OpenGL context suitable for creating GPU-based
	// buffer objects in RRTransGetFrame() and reading back the rendered frame in
	// RRTransSendFrame(), should a plugin choose to do so.
	TempContext *tc = NULL;

	int renderMode = 0;
	_glGetIntegerv(GL_RENDER_MODE, &renderMode);
	if(renderMode != GL_RENDER && renderMode != 0)
	{
		if(!alreadyWarnedPluginRenderMode && fconfig.verbose)
		{
			vglout.print("[VGL] WARNING: Failed to establish temporary OpenGL context for image\n");
			vglout.print("[VGL]    transport plugin one or more times because render mode != GL_RENDER.\n");
			alreadyWarnedPluginRenderMode = true;
		}
	}
	else
	{
		initReadbackContext();
		#ifdef EGLBACKEND
		tc = new TempContext(edpy != EGL_NO_DISPLAY ? (Display *)edpy : dpy,
			getGLXDrawable(), getGLXDrawable(), ctx, edpy != EGL_NO_DISPLAY);
		#else
		tc = new TempContext(dpy, getGLXDrawable(), getGLXDrawable(), ctx);
		#endif
		backend::readBuffer(drawBuf);
	}

	return tc;
}


void VirtualWin::sendPlugin(GLint drawBuf, bool spoilLast, bool sync,
	bool doStereo, int stereoMode)
{
	Frame f;
	int w = oglDraw->getWidth(), h = oglDraw->getHeight();
	RRFrame *rrframe = NULL;
	TempContext *tc = NULL;

	try
	{
		if(!plugin)
		{
			tc = setupPluginTempContext(drawBuf);
			plugin = new TransPlugin(dpy, x11Draw, fconfig.transport);
			plugin->connect(
				strlen(fconfig.client) > 0 ? fconfig.client : DisplayString(dpy),
				fconfig.port);
		}

		if(spoilLast && fconfig.spoil && !plugin->ready())
		{
			delete tc;  return;
		}
		if(!tc) tc = setupPluginTempContext(drawBuf);
		if(!fconfig.spoil) plugin->synchronize();

		if(oglDraw->getRGBSize() != 24)
			THROW("Transport plugins require 8 bits per component");
		int desiredFormat = RRTRANS_RGB;
		if(oglDraw->getFormat() == GL_BGR) desiredFormat = RRTRANS_BGR;
		else if(oglDraw->getFormat() == GL_BGRA) desiredFormat = RRTRANS_BGRA;
		else if(oglDraw->getFormat() == GL_RGBA) desiredFormat = RRTRANS_RGBA;

		rrframe = plugin->getFrame(w, h, desiredFormat,
			doStereo && stereoMode == RRSTEREO_QUADBUF);
		if(rrframe->bits)
		{
			f.init(rrframe->bits, rrframe->w, rrframe->pitch, rrframe->h,
				trans2pf[rrframe->format], FRAME_BOTTOMUP);

			if(doStereo && stereoMode == RRSTEREO_QUADBUF && rrframe->rbits == NULL)
			{
				static bool message = false;
				if(!message)
				{
					vglout.println("[VGL] NOTICE: Quad-buffered stereo is not supported by the plugin.");
					vglout.println("[VGL]    Using anaglyphic stereo instead.");
					message = true;
				}
				stereoMode = RRSTEREO_REDCYAN;
			}
			if(doStereo && IS_ANAGLYPHIC(stereoMode))
			{
				stereoFrame.deInit();
				makeAnaglyph(&f, drawBuf, stereoMode);
			}
			else if(doStereo && IS_PASSIVE(stereoMode))
			{
				rFrame.deInit();  gFrame.deInit();  bFrame.deInit();
				makePassive(&f, drawBuf, GL_NONE, stereoMode);
			}
			else
			{
				rFrame.deInit();  gFrame.deInit();  bFrame.deInit();
				stereoFrame.deInit();
				GLint readBuf = drawBuf;
				if(doStereo || stereoMode == RRSTEREO_LEYE) readBuf = LEYE(drawBuf);
				if(stereoMode == RRSTEREO_REYE) readBuf = REYE(drawBuf);
				readPixels(0, 0, rrframe->w, rrframe->pitch, rrframe->h, GL_NONE, f.pf,
					rrframe->bits, readBuf, doStereo);
				if(doStereo && rrframe->rbits)
					readPixels(0, 0, rrframe->w, rrframe->pitch, rrframe->h, GL_NONE,
						f.pf, rrframe->rbits, REYE(drawBuf), doStereo);
			}
			if(!syncdpy) { XSync(dpy, False);  syncdpy = true; }
			if(fconfig.logo) f.addLogo();
		}
		plugin->sendFrame(rrframe, sync);
	}
	catch(...)
	{
		delete tc;
		throw;
	}
	delete tc;
}


void VirtualWin::sendVGL(GLint drawBuf, bool spoilLast, bool doStereo,
	int stereoMode, int compress, int qual, int subsamp)
{
	int w = oglDraw->getWidth(), h = oglDraw->getHeight();

	if(spoilLast && fconfig.spoil && !vglconn->isReady())
		return;
	Frame *f;

	if(oglDraw->getRGBSize() != 24)
		THROW("The VGL Transport requires 8 bits per component");
	int glFormat = GL_RGB, pixelFormat = PF_RGB;
	if(compress != RRCOMP_RGB)
	{
		glFormat = oglDraw->getFormat();
		if(glFormat == GL_RGBA) pixelFormat = PF_RGBX;
		else if(glFormat == GL_BGR) pixelFormat = PF_BGR;
		else if(glFormat == GL_BGRA) pixelFormat = PF_BGRX;
	}

	if(!fconfig.spoil) vglconn->synchronize();
	ERRIFNOT(f = vglconn->getFrame(w, h, pixelFormat, FRAME_BOTTOMUP,
		doStereo && stereoMode == RRSTEREO_QUADBUF));
	if(doStereo && IS_ANAGLYPHIC(stereoMode))
	{
		stereoFrame.deInit();
		makeAnaglyph(f, drawBuf, stereoMode);
	}
	else if(doStereo && IS_PASSIVE(stereoMode))
	{
		rFrame.deInit();  gFrame.deInit();  bFrame.deInit();
		makePassive(f, drawBuf, glFormat, stereoMode);
	}
	else
	{
		rFrame.deInit();  gFrame.deInit();  bFrame.deInit();  stereoFrame.deInit();
		GLint readBuf = drawBuf;
		if(doStereo || stereoMode == RRSTEREO_LEYE) readBuf = LEYE(drawBuf);
		if(stereoMode == RRSTEREO_REYE) readBuf = REYE(drawBuf);
		readPixels(0, 0, f->hdr.framew, f->pitch, f->hdr.frameh, glFormat, f->pf,
			f->bits, readBuf, doStereo);
		if(doStereo && f->rbits)
			readPixels(0, 0, f->hdr.framew, f->pitch, f->hdr.frameh, glFormat, f->pf,
				f->rbits, REYE(drawBuf), doStereo);
	}
	f->hdr.winid = x11Draw;
	f->hdr.framew = f->hdr.width;
	f->hdr.frameh = f->hdr.height;
	f->hdr.x = 0;
	f->hdr.y = 0;
	f->hdr.qual = qual;
	f->hdr.subsamp = subsamp;
	f->hdr.compress = (unsigned char)compress;
	if(!syncdpy) { XSync(dpy, False);  syncdpy = true; }
	if(fconfig.logo) f->addLogo();
	vglconn->sendFrame(f);
}


void VirtualWin::sendX11(GLint drawBuf, bool spoilLast, bool sync,
	bool doStereo, int stereoMode)
{
	int width = oglDraw->getWidth(), height = oglDraw->getHeight();

	FBXFrame *f;
	if(!x11trans) x11trans = new X11Trans();
	if(spoilLast && fconfig.spoil && !x11trans->isReady()) return;
	if(!fconfig.spoil) x11trans->synchronize();
	ERRIFNOT(f = x11trans->getFrame(dpy, x11Draw, width, height));
	f->flags |= FRAME_BOTTOMUP;
	if(doStereo && IS_ANAGLYPHIC(stereoMode))
	{
		stereoFrame.deInit();
		makeAnaglyph(f, drawBuf, stereoMode);
	}
	else
	{
		rFrame.deInit();  gFrame.deInit();  bFrame.deInit();
		if(doStereo && IS_PASSIVE(stereoMode))
			makePassive(f, drawBuf, GL_NONE, stereoMode);
		else
		{
			stereoFrame.deInit();
			GLint readBuf = drawBuf;
			if(stereoMode == RRSTEREO_REYE) readBuf = REYE(drawBuf);
			else if(stereoMode == RRSTEREO_LEYE) readBuf = LEYE(drawBuf);
			readPixels(0, 0, min(width, f->hdr.framew), f->pitch,
				min(height, f->hdr.frameh), GL_NONE, f->pf, f->bits, readBuf, false);
		}
	}
	if(fconfig.logo) f->addLogo();
	x11trans->sendFrame(f, sync);
}


#ifdef USEXV

void VirtualWin::sendXV(GLint drawBuf, bool spoilLast, bool sync,
	bool doStereo, int stereoMode)
{
	int width = oglDraw->getWidth(), height = oglDraw->getHeight();

	XVFrame *f;
	if(!xvtrans) xvtrans = new XVTrans();
	if(spoilLast && fconfig.spoil && !xvtrans->isReady()) return;
	if(!fconfig.spoil) xvtrans->synchronize();
	ERRIFNOT(f = xvtrans->getFrame(dpy, x11Draw, width, height));
	rrframeheader hdr;
	hdr.x = hdr.y = 0;
	hdr.width = hdr.framew = width;
	hdr.height = hdr.frameh = height;

	if(oglDraw->getRGBSize() != 24)
		THROW("The XV Transport requires 8 bits per component");
	int glFormat = oglDraw->getFormat(), pixelFormat = PF_RGB;
	if(glFormat == GL_RGBA) pixelFormat = PF_RGBX;
	else if(glFormat == GL_BGR) pixelFormat = PF_BGR;
	else if(glFormat == GL_BGRA) pixelFormat = PF_BGRX;

	frame.init(hdr, pixelFormat, FRAME_BOTTOMUP, false);

	if(doStereo && IS_ANAGLYPHIC(stereoMode))
	{
		stereoFrame.deInit();
		makeAnaglyph(&frame, drawBuf, stereoMode);
	}
	else if(doStereo && IS_PASSIVE(stereoMode))
	{
		rFrame.deInit();  gFrame.deInit();  bFrame.deInit();
		makePassive(&frame, drawBuf, glFormat, stereoMode);
	}
	else
	{
		rFrame.deInit();  gFrame.deInit();  bFrame.deInit();  stereoFrame.deInit();
		GLint readBuf = drawBuf;
		if(stereoMode == RRSTEREO_REYE) readBuf = REYE(drawBuf);
		else if(stereoMode == RRSTEREO_LEYE) readBuf = LEYE(drawBuf);
		readPixels(0, 0, min(width, frame.hdr.framew), frame.pitch,
			min(height, frame.hdr.frameh), glFormat, frame.pf, frame.bits, readBuf,
			false);
	}

	if(fconfig.logo) frame.addLogo();

	*f = frame;
	xvtrans->sendFrame(f, sync);
}

#endif


void VirtualWin::makeAnaglyph(Frame *f, int drawBuf, int stereoMode)
{
	int rbuf = LEYE(drawBuf), gbuf = REYE(drawBuf),  bbuf = REYE(drawBuf);
	if(stereoMode == RRSTEREO_GREENMAGENTA)
	{
		rbuf = REYE(drawBuf);  gbuf = LEYE(drawBuf);  bbuf = REYE(drawBuf);
	}
	else if(stereoMode == RRSTEREO_BLUEYELLOW)
	{
		rbuf = REYE(drawBuf);  gbuf = REYE(drawBuf);  bbuf = LEYE(drawBuf);
	}
	rFrame.init(f->hdr, PF_COMP, f->flags, false);
	readPixels(0, 0, rFrame.hdr.framew, rFrame.pitch, rFrame.hdr.frameh, GL_RED,
		rFrame.pf, rFrame.bits, rbuf, false);
	gFrame.init(f->hdr, PF_COMP, f->flags, false);
	readPixels(0, 0, gFrame.hdr.framew, gFrame.pitch, gFrame.hdr.frameh,
		GL_GREEN, gFrame.pf, gFrame.bits, gbuf, false);
	bFrame.init(f->hdr, PF_COMP, f->flags, false);
	readPixels(0, 0, bFrame.hdr.framew, bFrame.pitch, bFrame.hdr.frameh, GL_BLUE,
		bFrame.pf, bFrame.bits, bbuf, false);
	profAnaglyph.startFrame();
	f->makeAnaglyph(rFrame, gFrame, bFrame);
	profAnaglyph.endFrame(f->hdr.framew * f->hdr.frameh, 0, 1);
}


void VirtualWin::makePassive(Frame *f, int drawBuf, GLenum glFormat,
	int stereoMode)
{
	stereoFrame.init(f->hdr, f->pf->id, f->flags, true);
	readPixels(0, 0, stereoFrame.hdr.framew, stereoFrame.pitch,
		stereoFrame.hdr.frameh, glFormat, stereoFrame.pf, stereoFrame.bits,
		LEYE(drawBuf), true);
	readPixels(0, 0, stereoFrame.hdr.framew, stereoFrame.pitch,
		stereoFrame.hdr.frameh, glFormat, stereoFrame.pf, stereoFrame.rbits,
		REYE(drawBuf), true);
	profPassive.startFrame();
	f->makePassive(stereoFrame, stereoMode);
	profPassive.endFrame(f->hdr.framew * f->hdr.frameh, 0, 1);
}


void VirtualWin::readPixels(GLint x, GLint y, GLint width, GLint pitch,
	GLint height, GLenum glFormat, PF *pf, GLubyte *bits, GLint buf, bool stereo)
{
	VirtualDrawable::readPixels(x, y, width, pitch, height, glFormat, pf, bits,
		buf, stereo);

	// Gamma correction
	if(fconfig.gamma != 0.0 && fconfig.gamma != 1.0 && fconfig.gamma != -1.0)
	{
		profGamma.startFrame();
		static bool first = true;
		if(first)
		{
			first = false;
			if(fconfig.verbose)
				vglout.println("[VGL] Using software gamma correction (correction factor=%f)\n",
					fconfig.gamma);
		}
		if(pf->bpc == 10)
		{
			int h = height;
			while(h--)
			{
				int w = width;
				unsigned int *srcPixel = (unsigned int *)bits;
				while(w--)
				{
					unsigned int r =
						fconfig.gamma_lut10[(*srcPixel >> pf->rshift) & 1023];
					unsigned int g =
						fconfig.gamma_lut10[(*srcPixel >> pf->gshift) & 1023];
					unsigned int b =
						fconfig.gamma_lut10[(*srcPixel >> pf->bshift) & 1023];
					*srcPixel++ =
						(r << pf->rshift) | (g << pf->gshift) | (b << pf->bshift);
				}
				bits += pitch;
			}
		}
		else
		{
			unsigned short *ptr1, *ptr2 = (unsigned short *)(&bits[pitch * height]);
			for(ptr1 = (unsigned short *)bits; ptr1 < ptr2; ptr1++)
				*ptr1 = fconfig.gamma_lut16[*ptr1];
			if((pitch * height) % 2 != 0)
				bits[pitch * height - 1] = fconfig.gamma_lut[bits[pitch * height - 1]];
		}
		profGamma.endFrame(width * height, 0, stereo ? 0.5 : 1);
	}
}


bool VirtualWin::isStereo(void)
{
	return oglDraw && oglDraw->isStereo();
}
