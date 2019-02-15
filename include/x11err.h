/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2019 D. R. Commander
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

#define CASE(m)  case m:  return #m;

static const char *x11error(int code)
{
	if(code >= FirstExtensionError && code <= LastExtensionError)
		return "Extension error";
	switch(code)
	{
		CASE(BadRequest)
		CASE(BadValue)
		CASE(BadWindow)
		CASE(BadPixmap)
		CASE(BadAtom)
		CASE(BadCursor)
		CASE(BadFont)
		CASE(BadMatch)
		CASE(BadDrawable)
		CASE(BadAccess)
		CASE(BadAlloc)
		CASE(BadColor)
		CASE(BadGC)
		CASE(BadIDChoice)
		CASE(BadName)
		CASE(BadLength)
		CASE(BadImplementation)
		default:  return "Unknown error code";
	}
}
