/* Copyright (C) 2015 Open Text SA and/or Open Text ULC (in Canada).
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

#include "SafeCriticalSection.h"

using namespace vglserver;

SafeCriticalSection *SafeCriticalSection::instance=NULL;
vglutil::CriticalSection SafeCriticalSection::instanceMutex;
