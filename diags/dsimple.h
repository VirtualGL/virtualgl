/* $Xorg: dsimple.h,v 1.4 2001/02/09 02:05:54 xorgcvs Exp $ */
/*

Copyright 1993, 1998  The Open Group

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
/* $XFree86: xc/programs/xlsfonts/dsimple.h,v 1.6 2001/07/29 21:23:21 tsi Exp $ */

/*
 * Just_display.h: This file contains the definitions needed to use the
 *                 functions in just_display.c.  It also declares the global
 *                 variables dpy, screen, and program_name which are needed to
 *                 use just_display.c.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

    /* Global variables used by routines in just_display.c */

extern char *program_name;                   /* Name of this program */
extern Display *dpy;                         /* The current display */
extern int screen;                           /* The current screen */

#define INIT_NAME program_name=argv[0]        /* use this in main to setup
                                                 program_name */

    /* Declaritions for functions in just_display.c */

#if NeedFunctionPrototypes
char *Malloc(unsigned);
char *Realloc(char *, int);
char *Get_Display_Name(int *, char **);
Display *Open_Display(char *);
void Setup_Display_And_Screen(int *, char **);
XFontStruct *Open_Font(char *);
void Beep(void);
Pixmap ReadBitmapFile(Drawable, char *, int *, int *, int *, int *);
void WriteBitmapFile(char *, Pixmap, int, int, int, int);
Window Select_Window_Args(int *, char **);
void usage(void);
#else
char *Malloc();
char *Realloc();
char *Get_Display_Name();
Display *Open_Display();
void Setup_Display_And_Screen();
XFontStruct *Open_Font();
void Beep();
Pixmap ReadBitmapFile();
void WriteBitmapFile();
Window Select_Window_Args();
void usage();
#endif

#define X_USAGE "[host:display]"              /* X arguments handled by
						 Get_Display_Name */

/*
 * Other_stuff.h: Definitions of routines in other_stuff.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

#if NeedFunctionPrototypes
unsigned long Resolve_Color(Window, char *);
Pixmap Bitmap_To_Pixmap(Display *, Drawable, GC, Pixmap, int, int);
Window Select_Window(Display *);
void blip(void);
Window Window_With_Name(Display *, Window, char *);
#else
unsigned long Resolve_Color();
Pixmap Bitmap_To_Pixmap();
Window Select_Window();
void blip();
Window Window_With_Name();
#endif
#if __GNUC__
void Fatal_Error(char *, ...) __attribute__((__noreturn__));
#else
void Fatal_Error(char *, ...);
#endif
void outl(char *, ...);
