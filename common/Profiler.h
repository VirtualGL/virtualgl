/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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

#ifndef __PROFILER_H__
#define __PROFILER_H__

#include "Timer.h"


namespace vglcommon
{
	class Profiler
	{
		public:

			Profiler(const char *name="Profiler", double interval=2.0);
			~Profiler(void);
			void setName(char *name);
			void setName(const char *name);
			void startFrame(void);
			void endFrame(long pixels, long bytes, double incFrames);

		private:

			char *name;
			double interval;
			double mbytes, mpixels, totalTime, start, frames, lastFrame;
			bool profile;
			vglutil::Timer timer;
			bool freestr;
	};
}

#endif
