#ifndef __rrglx_h__
#define __rrglx_h__

/*
** The contents of this file are subject to the GLX Public License Version 1.0
** (the "License"). You may not use this file except in compliance with the
** License. You may obtain a copy of the License at Silicon Graphics, Inc.,
** attn: Legal Services, 2011 N. Shoreline Blvd., Mountain View, CA 94043
** or at http://www.sgi.com/software/opensource/glx/license.html.
**
** Software distributed under the License is distributed on an "AS IS"
** basis. ALL WARRANTIES ARE DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY
** IMPLIED WARRANTIES OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR
** PURPOSE OR OF NON- INFRINGEMENT. See the License for the specific
** language governing rights and limitations under the License.
**
** The Original Software is GLX version 1.2 source code, released February,
** 1999. The developer of the Original Software is Silicon Graphics, Inc.
** Those portions of the Subject Software created by Silicon Graphics, Inc.
** are Copyright (c) 1991-9 Silicon Graphics, Inc. All Rights Reserved.
**
** $Header: /home/drc/cvs/vgl/rr/glx.h,v 1.3 2005-02-17 05:42:15 dcommander Exp $
*/

#include <GL/glx.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GLXFBConfigSGIX
typedef GLXFBConfig GLXFBConfigSGIX;
#endif

#ifndef GLX_RGBA_BIT
#define GLX_RGBA_BIT                    0x00000001
#endif

#ifndef GLX_RGBA_TYPE
#define GLX_RGBA_TYPE                   0x8014
#endif

#ifndef GLX_WIDTH
#define GLX_WIDTH                       0x801D
#endif

#ifndef GLX_HEIGHT
#define GLX_HEIGHT                      0x801E
#endif

#ifndef GLX_PBUFFER_HEIGHT
#define GLX_PBUFFER_HEIGHT              0x8040  /* New for GLX 1.3 */
#endif

#ifndef GLX_PBUFFER_WIDTH
#define GLX_PBUFFER_WIDTH               0x8041  /* New for GLX 1.3 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* !__rrglx_h__ */
