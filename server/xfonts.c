/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
 * Copyright (C) 2011-2012, 2014-2015, 2017-2021  D. R. Commander
 *                                                All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/* xfonts.c -- glXUseXFont() for Mesa written by
 * Copyright (C) 1995 Thorsten.Ohl @ Physik.TH-Darmstadt.de
 */

#include <X11/Xatom.h>
#include "vglutil.h"


/* Implementation.  */

/* Fill a BITMAP with a character C from thew current font
   in the graphics context GC.  WIDTH is the width in bytes
   and HEIGHT is the height in bits.

   Note that the generated bitmaps must be used with

        glPixelStorei (GL_UNPACK_SWAP_BYTES, GL_FALSE);
        glPixelStorei (GL_UNPACK_LSB_FIRST, GL_FALSE);
        glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

   Possible optimizations:

     * use only one reusable pixmap with the maximum dimensions.
     * draw the entire font into a single pixmap (careful with
       proportional fonts!).
*/


/*
 * determine if a given glyph is valid and return the
 * corresponding XCharStruct.
 */
static XCharStruct *
isvalid(XFontStruct * fs, unsigned int which)
{
   unsigned int rows, pages;
   unsigned int byte1 = 0, byte2 = 0;
   int i, valid = 1;

   rows = fs->max_byte1 - fs->min_byte1 + 1;
   pages = fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1;

   if (rows == 1) {
      /* "linear" fonts */
      if ((fs->min_char_or_byte2 > which) || (fs->max_char_or_byte2 < which))
         valid = 0;
   }
   else {
      /* "matrix" fonts */
      byte2 = which & 0xff;
      byte1 = which >> 8;
      if ((fs->min_char_or_byte2 > byte2) ||
          (fs->max_char_or_byte2 < byte2) ||
          (fs->min_byte1 > byte1) || (fs->max_byte1 < byte1))
         valid = 0;
   }

   if (valid) {
      if (fs->per_char) {
         if (rows == 1) {
            /* "linear" fonts */
            return (fs->per_char + (which - fs->min_char_or_byte2));
         }
         else {
            /* "matrix" fonts */
            i = ((byte1 - fs->min_byte1) * pages) +
               (byte2 - fs->min_char_or_byte2);
            return (fs->per_char + i);
         }
      }
      else {
         return (&fs->min_bounds);
      }
   }
   return (NULL);
}


void
Fake_glXUseXFont(Font font, int first, int count, int listbase)
{
   Display *dpy = NULL;
   Window win = 0;  bool newwin = false;
   Pixmap pixmap = 0;  XImage *image = NULL;
   GC gc = 0;
   XGCValues values;
   unsigned long valuemask;
   XFontStruct *fs = NULL;
   GLint swapbytes, lsbfirst, rowlength;
   GLint skiprows, skippixels, alignment;
   unsigned int max_width, max_height, max_bm_width, max_bm_height;
   GLubyte *bm = NULL;
   int i, j, ngroups, groupsize, n;
   faker::VirtualWin *pbw;
   typedef struct {
      int bm_width, bm_height, width, height, valid;
      GLfloat x0, y0, dx, dy;
   } charinfo;
   charinfo *ci = NULL;

   try {

   GLXDrawable draw = backend::getCurrentDrawable();
   if (!draw) return;
   if ((pbw = winhash.find(NULL, draw)) != NULL) {
      /* Current drawable is a virtualized Window */
      ERRIFNOT(dpy = pbw->getX11Display());
      ERRIFNOT(win = pbw->getX11Drawable());
   }
   else if ((win = pmhash.reverseFind(draw)) != 0) {
      /* Current drawable is a virtualized Pixmap */
      ERRIFNOT(dpy = glxdhash.getCurrentDisplay(draw));
   }
   else {
      /* Current drawable must be a Pbuffer that the application created.
         Blerg.  There is no window attached, so we have to create one
         temporarily. */
      XVisualInfo *v = NULL, vtemp;
      XSetWindowAttributes swa;
      int nv = 0;

      ERRIFNOT(dpy = glxdhash.getCurrentDisplay(draw));
      vtemp.c_class = TrueColor;
      vtemp.depth = DefaultDepth(dpy, DefaultScreen(dpy));
      vtemp.screen = DefaultScreen(dpy);
      Window root = DefaultRootWindow(dpy);
      if((v = XGetVisualInfo(dpy, VisualDepthMask | VisualClassMask |
                                  VisualScreenMask, &vtemp, &nv)) == NULL ||
         nv < 1)
         THROW("Could not create temporary window for font rendering");
      swa.colormap = XCreateColormap(dpy, root, v->visual, AllocNone);
      swa.border_pixel = 0;
      swa.event_mask = 0;
      if ((win = _XCreateWindow(dpy, root, 0, 0, 1, 1, 0, v->depth,
                                InputOutput, v->visual,
                                CWBorderPixel | CWColormap | CWEventMask,
                                &swa)) == 0) {
         _XFree(v);
         THROW("Could not create temporary window for font rendering");
      }
      _XFree(v);
      newwin = true;
   }

   fs = XQueryFont(dpy, font);
   if (!fs) {
      faker::sendGLXError(dpy, X_GLXUseXFont, BadFont, true);
      return;
   }

   if (fconfig.trace) {
      unsigned long name_value;

      if (XGetFontProperty(fs, XA_FONT, &name_value)) {
         char *name = XGetAtomName(dpy, name_value);

         if (name) {
            PRARGS(name);  _XFree(name);
         }
      }
   }

   /* Allocate a bitmap that can fit all characters.  */
   max_width = fs->max_bounds.rbearing - fs->min_bounds.lbearing;
   max_height = fs->max_bounds.ascent + fs->max_bounds.descent;
   max_bm_width = (max_width + 7) / 8;
   max_bm_height = max_height;

   bm = (GLubyte *) malloc((max_bm_width * max_bm_height) * sizeof(GLubyte));
   if (!bm)
      THROW("Couldn't allocate bitmap in glXUseXFont()");

   /* Save the current packing mode for bitmaps.  */
   _glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
   _glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
   _glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
   _glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
   _glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
   _glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

   /* Enforce a standard packing mode which is compatible with
      fill_bitmap() from above.  This is actually the default mode,
      except for the (non)alignment.  */
   _glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   _glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
   _glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   _glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   _glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   _glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   /* X.org can't handle pixmaps more than 32767 pixels in width or height,
      so we have to split the font into multiple groups if it would cause the
      temporary pixmap to exceed those limits */
   ngroups = (8 * max_bm_width * count + 32766) / 32767;
   groupsize = (count + ngroups - 1) / ngroups;

   ci = (charinfo *) malloc(groupsize * sizeof(charinfo));
   if (!ci)
      THROW("Couldn't allocate character info structure in glXUseXFont()");

   for (j = 0; j < count; j += groupsize) {
      n = min(groupsize, count - j);
      pixmap = XCreatePixmap(dpy, win, 8 * max_bm_width * n, max_bm_height, 1);
      if (!pixmap)
         THROW("Couldn't allocate pixmap in glXUseXFont()");
      values.foreground = BlackPixel(dpy, DefaultScreen(dpy));
      values.background = WhitePixel(dpy, DefaultScreen(dpy));
      values.font = fs->fid;
      valuemask = GCForeground | GCBackground | GCFont;
      gc = XCreateGC(dpy, pixmap, valuemask, &values);

      XSetForeground(dpy, gc, 0);
      XFillRectangle(dpy, pixmap, gc, 0, 0, 8 * max_bm_width * n,
                     max_bm_height);
      XSetForeground(dpy, gc, 1);

      for (i = 0; i < n; i++) {
         XCharStruct *ch;
         int x, y;
         unsigned int c = first + i;

         /* check on index validity and get the bounds */
         ch = isvalid(fs, c);
         if (!ch) {
            ch = &fs->max_bounds;
            ci[i].valid = 0;
         }
         else {
            ci[i].valid = 1;
         }

         /* glBitmap()' parameters:
            straight from the glXUseXFont(3) manpage.  */
         ci[i].width = ch->rbearing - ch->lbearing;
         ci[i].height = ch->ascent + ch->descent;
         ci[i].x0 = -ch->lbearing;
         ci[i].y0 = ch->descent - 0;    /* XXX used to subtract 1 here */
         /* but that caused a conformace failure */
         ci[i].dx = ch->width;
         ci[i].dy = 0;

         /* X11's starting point.  */
         x = -ch->lbearing;
         y = ch->ascent;

         /* Round the width to a multiple of eight.  We will use this also
            for the pixmap for capturing the X11 font.  This is slightly
            inefficient, but it makes the OpenGL part real easy.  */
         ci[i].bm_width = (ci[i].width + 7) / 8;
         ci[i].bm_height = ci[i].height;

         if (ci[i].valid && (ci[i].bm_width > 0) && (ci[i].bm_height > 0)) {
            XChar2b char2b;

            char2b.byte1 = (c >> 8) & 0xff;
            char2b.byte2 = (c & 0xff);
            XDrawString16(dpy, pixmap, gc, x + i * max_bm_width * 8, y,
                          &char2b, 1);
         }
      }

      XFreeGC(dpy, gc);  gc = 0;
      ERRIFNOT(image = _XGetImage(dpy, pixmap, 0, 0, 8 * max_bm_width * n,
                                  max_bm_height, 1, XYPixmap));
      XFreePixmap(dpy, pixmap);  pixmap = 0;

      for (i = 0; i < n; i++) {
         int list = listbase + j + i;

         _glNewList(list, GL_COMPILE);
         if (ci[i].valid && (ci[i].bm_width > 0) && (ci[i].bm_height > 0)) {
            int x, y;

            memset(bm, '\0', ci[i].bm_width * ci[i].bm_height);
            /* Fill the bitmap (X11 and OpenGL are upside down wrt each
               other).  */
            for (y = 0; y < ci[i].bm_height; y++)
               for (x = 0; x < 8 * ci[i].bm_width; x++)
                  if (XGetPixel(image, x + i * max_bm_width * 8, y))
                     bm[ci[i].bm_width * (ci[i].bm_height - y - 1) + x / 8] |=
                        (1 << (7 - (x % 8)));
            _glBitmap(ci[i].width, ci[i].height, ci[i].x0, ci[i].y0, ci[i].dx,
                      ci[i].dy, bm);
         }
         else {
            _glBitmap(0, 0, 0.0, 0.0, ci[i].dx, ci[i].dy, NULL);
         }
         _glEndList();
      }

      XDestroyImage(image);  image = NULL;
   }

   XFreeFontInfo(NULL, fs, 1);  fs = NULL;
   free(bm);  bm = NULL;
   free(ci);  ci = NULL;

   /* Restore saved packing modes.  */
   _glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
   _glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
   _glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
   _glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
   _glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
   _glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

   } catch(...) {
      if (fs) XFreeFontInfo(NULL, fs, 1);
      if (gc && dpy) XFreeGC(dpy, gc);
      if (pixmap && dpy) XFreePixmap(dpy, pixmap);
      if (image) XDestroyImage(image);
      if (win && dpy && newwin) _XDestroyWindow(dpy, win);
      free(bm);
      free(ci);
      throw;
   }
}
