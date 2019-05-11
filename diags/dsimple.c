/* $Xorg: dsimple.c,v 1.4 2001/02/09 02:05:54 xorgcvs Exp $ */
/*

Copyright 1993, 1998  The Open Group
Copyright 2019  D. R. Commander

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/xlsfonts/dsimple.c,v 3.6 2001/12/14 20:02:09 dawes Exp $ */

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
/*
 * Written by Mark Lillibridge.   Last updated 7/1/87
 */

#include "dsimple.h"


char *program_name = "unknown_program";


/*
 * Routine to let user select a window using the mouse
 */

Window Select_Window(Display *dpy)
{
  int status;
  Cursor cursor;
  XEvent event;
  Window target_win = None, root = DefaultRootWindow(dpy);
  int buttons = 0;

  /* Make the target cursor */
  cursor = XCreateFontCursor(dpy, XC_crosshair);

  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(dpy, root, False,
                        ButtonPressMask|ButtonReleaseMask, GrabModeSync,
                        GrabModeAsync, root, cursor, CurrentTime);
  if (status != GrabSuccess) Fatal_Error("Can't grab the mouse.");

  /* Let the user select a window... */
  while ((target_win == None) || (buttons != 0)) {
    /* allow one more event */
    XAllowEvents(dpy, SyncPointer, CurrentTime);
    XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
    switch (event.type) {
    case ButtonPress:
      if (target_win == None) {
        target_win = event.xbutton.subwindow; /* window selected */
        if (target_win == None) target_win = root;
      }
      buttons++;
      break;
    case ButtonRelease:
      if (buttons > 0) /* there may have been some down before we started */
        buttons--;
       break;
    }
  }

  XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */

  return(target_win);
}


/*
 * Standard fatal error routine - call like printf but maximum of 7 arguments.
 * Does not require dpy or screen defined.
 */
void Fatal_Error(char *msg, ...)
{
        va_list args;
        fflush(stdout);
        fflush(stderr);
        fprintf(stderr, "%s: error: ", program_name);
        va_start(args, msg);
        vfprintf(stderr, msg, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(1);
}
