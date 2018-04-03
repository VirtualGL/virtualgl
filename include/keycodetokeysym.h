/* Copyright (C)2014, 2016 D. R. Commander
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

#ifndef __KEYCODETOKEYSYM_H__
#define __KEYCODETOKEYSYM_H__

#include <X11/Xlib.h>


static KeySym KeycodeToKeysym(Display *dpy, KeyCode keycode, int index)
{
	KeySym ks = NoSymbol, *keysyms;  int n = 0;

	keysyms = XGetKeyboardMapping(dpy, keycode, 1, &n);
	if(keysyms)
	{
		if(n >= 1 && index >= 0 && index < n)
			ks = keysyms[index];
		XFree(keysyms);
	}

	return ks;
}

#endif
