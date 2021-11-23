// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2009-2014, 2017-2021 D. R. Commander
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

#ifndef __VIRTUALWIN_H__
#define __VIRTUALWIN_H__

#include "VirtualDrawable.h"
#include "VGLTrans.h"
#ifdef USEXV
#include "XVTrans.h"
#endif
#include "TransPlugin.h"
#include "TempContext.h"


namespace faker
{
	class VirtualWin : public VirtualDrawable
	{
		public:

			VirtualWin(Display *dpy, Window win);
			virtual ~VirtualWin(void);
			void clear(void);
			void cleanup(void);
			GLXDrawable getGLXDrawable(void);
			virtual GLXDrawable updateGLXDrawable(void);
			void checkConfig(VGLFBConfig config);
			void resize(int width, int height);
			void checkResize(void);
			void initFromWindow(VGLFBConfig config);
			void readback(GLint drawBuf, bool spoilLast, bool sync);
			void swapBuffers(void);
			bool isStereo(void);
			void wmDeleted(void);
			void enableWMDeleteHandler(void);
			int getSwapInterval(void) { return swapInterval; }
			void setSwapInterval(int swapInterval_) { swapInterval = swapInterval_; }

			bool dirty, rdirty;

		protected:

			int init(int w, int h, VGLFBConfig config);
			void readPixels(GLint x, GLint y, GLint width, GLint pitch, GLint height,
				GLenum glFormat, PF *pf, GLubyte *bits, GLint buf, bool stereo);
			void makeAnaglyph(common::Frame *f, int drawBuf, int stereoMode);
			void makePassive(common::Frame *f, int drawBuf, GLenum glFormat,
				int stereoMode);
			void sendVGL(GLint drawBuf, bool spoilLast, bool doStereo,
				int stereoMode, int compress, int qual, int subsamp);
			void sendX11(GLint drawBuf, bool spoilLast, bool sync, bool doStereo,
				int stereoMode);
			void sendPlugin(GLint drawBuf, bool spoilLast, bool sync, bool doStereo,
				int stereoMode);
			#ifdef USEXV
			void sendXV(GLint drawBuf, bool spoilLast, bool sync, bool doStereo,
				int stereoMode);
			#endif
			TempContext *setupPluginTempContext(GLint drawBuf);

			Display *eventdpy;
			OGLDrawable *oldDraw;
			int newWidth, newHeight;
			server::X11Trans *x11trans;
			#ifdef USEXV
			server::XVTrans *xvtrans;
			#endif
			server::VGLTrans *vglconn;
			common::Profiler profGamma, profAnaglyph, profPassive;
			bool syncdpy;
			server::TransPlugin *plugin;
			bool stereoVisual;
			common::Frame rFrame, gFrame, bFrame, frame, stereoFrame;
			bool deletedByWM;
			bool handleWMDelete;
			bool newConfig;
			int swapInterval;
			bool alreadyWarnedPluginRenderMode;
	};
}

#endif  // __VIRTUALWIN_H__
