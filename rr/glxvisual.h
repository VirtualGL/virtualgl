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

#include "faker-sym.h"
#ifdef USEGLP
#include <GL/glp.h>
#endif

class glxvisual
{
	public:

		glxvisual(GLXFBConfig);
		glxvisual(XVisualInfo *);
		~glxvisual(void);
		void init(GLXFBConfig);
		void init(XVisualInfo *);
		GLXFBConfig getpbconfig(void);
		XVisualInfo *getvisual(void);

	private:

		int buffer_size;
		int level;
		int doublebuffer;
		int stereo;
		int aux_buffers;
		int red_size;
		int green_size;
		int blue_size;
		int alpha_size;
		int depth_size;
		int stencil_size;
		int accum_red_size;
		int accum_green_size;
		int accum_blue_size;
		int accum_alpha_size;
		int render_type;
		int x_visual_type;
};

#ifdef USEGLP
int glPConfigDepth(GLPFBConfig);
int glPConfigClass(GLPFBConfig);
GLPFBConfig glPConfigFromVisAttribs(GLPDevice, int attribs[]);
#endif
