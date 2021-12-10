// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2011, 2014-2015, 2021 D. R. Commander
// Copyright (C)2015 Open Text SA and/or Open Text ULC (in Canada)
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

#ifndef __GLOBALCRITICALSECTION_H__
#define __GLOBALCRITICALSECTION_H__

#include "Mutex.h"

// If a shared library loaded by the 3D application calls one of the interposed
// functions from within its constructor (_init() or a function with the GCC
// "constructor" attribute), this previously caused a deadlock in
// faker::init().  The deadlock occurred because the static C++ constructors in
// libvglfaker (including the constructor for globalMutex) hadn't been called
// yet, and thus the pthread mutex associated with globalMutex hadn't yet been
// made recursive.  This class implements a singleton CriticalSection instance
// that is initialized on first use (within faker::init()), thus avoiding the
// deadlock.

namespace faker
{
	class GlobalCriticalSection : public util::CriticalSection
	{
		public:

			static GlobalCriticalSection *getInstance(bool create = true)
			{
				if(instance == NULL && create)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new GlobalCriticalSection;
				}
				return instance;
			}

		private:

			static GlobalCriticalSection *instance;
			static util::CriticalSection instanceMutex;
	};
}


#define globalMutex  (*(faker::GlobalCriticalSection::getInstance()))

#endif  // __GLOBALCRITICALSECTION_H__
