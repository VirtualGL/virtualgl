/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2011, 2014 D. R. Commander
 * Copyright (C)2015 Open Text SA and/or Open Text ULC (in Canada).
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

#ifndef __SAFECRITICALSECTION_H__
#define __SAFECRITICALSECTION_H__

#include "Mutex.h"

// A CriticalSection instance that is safe to run even if constructors for
// static C++ objects have not yet been called.

namespace vglserver
{
	class SafeCriticalSection : public vglutil::CriticalSection
	{
		public:

			static SafeCriticalSection *getInstance(void)
			{
				if(instance==NULL)
				{
					vglutil::CriticalSection::SafeLock l(instanceMutex);
					if(instance==NULL) instance=new SafeCriticalSection;
				}
				return instance;
			}
		private:

			static SafeCriticalSection *instance;
			static vglutil::CriticalSection instanceMutex;
	};
}

#define globalMutex (*(vglserver::SafeCriticalSection::getInstance()))

#endif // __SAFECRITICALSECTION_H__
