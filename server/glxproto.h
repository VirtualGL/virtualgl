#ifndef _GLX_glxproto_h_
#define _GLX_glxproto_h_

/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/*****************************************************************************/

/*
** Errors.
*/
#define GLXBadContext		0
#define GLXBadContextState	1
#define GLXBadDrawable		2
#define GLXBadPixmap		3
#define GLXBadContextTag	4
#define GLXBadCurrentWindow	5
#define GLXBadRenderRequest	6
#define GLXBadLargeRequest	7
#define GLXUnsupportedPrivateRequest	8
#define GLXBadFBConfig		9
#define GLXBadPbuffer		10
#define GLXBadCurrentDrawable	11
#define GLXBadWindow		12
#define GLXBadProfileARB        13

#define __GLX_NUMBER_ERRORS 14

/*****************************************************************************/

/* Opcodes for GLX commands */

#define X_GLXRender                       1
#define X_GLXRenderLarge                  2
#define X_GLXCreateContext                3
#define X_GLXDestroyContext               4
#define X_GLXMakeCurrent                  5
#define X_GLXIsDirect                     6
#define X_GLXQueryVersion                 7
#define X_GLXWaitGL                       8
#define X_GLXWaitX                        9
#define X_GLXCopyContext                 10
#define X_GLXSwapBuffers                 11
#define X_GLXUseXFont                    12
#define X_GLXCreateGLXPixmap             13
#define X_GLXGetVisualConfigs            14
#define X_GLXDestroyGLXPixmap            15
#define X_GLXVendorPrivate               16
#define X_GLXVendorPrivateWithReply      17
#define X_GLXQueryExtensionsString       18
#define X_GLXQueryServerString           19
#define X_GLXClientInfo                  20
#define X_GLXGetFBConfigs                21
#define X_GLXCreatePixmap                22
#define X_GLXDestroyPixmap               23
#define X_GLXCreateNewContext            24
#define X_GLXQueryContext                25
#define X_GLXMakeContextCurrent          26
#define X_GLXCreatePbuffer               27
#define X_GLXDestroyPbuffer              28
#define X_GLXGetDrawableAttributes       29
#define X_GLXChangeDrawableAttributes    30
#define X_GLXCreateWindow                31
#define X_GLXDestroyWindow               32
#define X_GLXSetClientInfoARB            33
#define X_GLXCreateContextAtrribsARB     34
#define X_GLXSetConfigInfo2ARB           35

#endif /* _GLX_glxproto_h_ */
