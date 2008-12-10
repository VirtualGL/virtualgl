/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
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

#define __case(m) case m:  return #m;

static const char *x11error(int code)
{
	if(code>=FirstExtensionError && code<=LastExtensionError)
		return "Extension error";
	switch(code)
	{
		__case(BadRequest);
		__case(BadValue);
		__case(BadWindow);
		__case(BadPixmap);
		__case(BadAtom);
		__case(BadCursor);
		__case(BadFont);
		__case(BadMatch);
		__case(BadDrawable);
		__case(BadAccess);
		__case(BadAlloc);
		__case(BadColor);
		__case(BadGC);
		__case(BadIDChoice);
		__case(BadName);
		__case(BadLength);
		__case(BadImplementation);
		default:  return "Unknown error code";
	}
}
