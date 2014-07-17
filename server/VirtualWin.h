/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2009-2014 D. R. Commander
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

#ifndef __VIRTUALWIN_H__
#define __VIRTUALWIN_H__

#include "VirtualDrawable.h"
#include "VGLTrans.h"
#ifdef USEXV
#include "XVTrans.h"
#endif
#include "TransPlugin.h"


namespace vglserver
{
	class VirtualWin : public VirtualDrawable
	{
		public:

			VirtualWin(Display *dpy, Window win);
			~VirtualWin(void);
			void clear(void);
			void cleanup(void);
			GLXDrawable getGLXDrawable(void);
			GLXDrawable updateGLXDrawable(void);
			void checkConfig(GLXFBConfig config);
			void resize(int width, int height);
			void checkResize(void);
			void initFromWindow(GLXFBConfig config);
			void readback(GLint drawBuf, bool spoilLast, bool sync);
			void swapBuffers(void);
			bool isStereo(void);
			void wmDelete(void);
			int getSwapInterval(void) { return swapInterval; }
			void setSwapInterval(int swapInterval_) { swapInterval=swapInterval_; }

			bool dirty, rdirty;

		private:

			int init(int w, int h, GLXFBConfig config);
			void readPixels(GLint x, GLint y, GLint width, GLint pitch, GLint height,
				GLenum format, int pixelSize, GLubyte *bits, GLint buf, bool stereo);
			void makeAnaglyph(vglcommon::Frame *f, int drawBuf, int stereoMode);
			void makePassive(vglcommon::Frame *f, int drawBuf, int format,
				int stereoMode);
			void sendVGL(GLint drawBuf, bool spoilLast,	bool doStereo,
				int stereoMode, int compress, int qual, int subsamp);
			void sendX11(GLint drawBuf, bool spoilLast, bool sync, bool doStereo,
				int stereoMode);
			void sendPlugin(GLint drawBuf, bool spoilLast, bool sync, bool doStereo,
				int stereoMode);
			#ifdef USEXV
			void sendXV(GLint drawBuf, bool spoilLast, bool sync, bool doStereo,
				int stereoMode);
			#endif

			Display *eventdpy;
			OGLDrawable *oldDraw;
			int newWidth, newHeight;
			X11Trans *x11trans;
			#ifdef USEXV
			XVTrans *xvtrans;
			#endif
			VGLTrans *vglconn;
			vglcommon::Profiler profGamma, profAnaglyph, profPassive;
			bool syncdpy;
			TransPlugin *plugin;
			bool trueColor;
			bool stereoVisual;
			vglcommon::Frame rFrame, gFrame, bFrame, frame, stereoFrame;
			bool doWMDelete;
			bool newConfig;
			int swapInterval;
	};
}

#endif // __VIRTUALWIN_H__
