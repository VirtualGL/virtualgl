/* Copyright (C)2011 D. R. Commander
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

#include "pbdrawable.h"
#include "rrframe.h"

class pbpm : public pbdrawable
{
	public:

		pbpm(Display *, Pixmap, Visual *v);
		~pbpm();
		void readback(void);

	private:

		rrprofiler _prof_pmblit;
		rrfb *_fb;
};
