/* Copyright (C)2017, 2019 D. R. Commander
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

#ifndef __GLPF_H__
#define __GLPF_H__

#include "pf.h"
#include "boost/endian.h"
#include <GL/gl.h>


static const GLenum pf_glformat[PIXELFORMATS] =
{
	GL_RGB,       /* PF_RGB */
	GL_RGBA,      /* PF_RGBX */
	GL_RGBA,      /* PF_RGB10_X2 */
	GL_BGR,       /* PF_BGR */
	GL_BGRA,      /* PF_BGRX */
	GL_BGRA,      /* PF_BGR10_X2 */
	#ifdef GL_ABGR_EXT
	GL_ABGR_EXT,  /* PF_XBGR */
	#else
	GL_NONE,
	#endif
	GL_RGBA,      /* PF_X2_BGR10 */
	GL_BGRA,      /* PF_XRGB */
	GL_BGRA,      /* PF_X2_RGB10 */
	GL_RED        /* PF_COMP */
};


static const GLenum pf_gldatatype[PIXELFORMATS] =
{
	GL_UNSIGNED_BYTE,                /* PF_RGB */
	GL_UNSIGNED_BYTE,                /* PF_RGBX */
	#ifdef BOOST_BIG_ENDIAN
	GL_UNSIGNED_INT_10_10_10_2,      /* PF_RGB10_X2 */
	#else
	GL_UNSIGNED_INT_2_10_10_10_REV,  /* PF_RGB10_X2 */
	#endif
	GL_UNSIGNED_BYTE,                /* PF_BGR */
	GL_UNSIGNED_BYTE,                /* PF_BGRX */
	#ifdef BOOST_BIG_ENDIAN
	GL_UNSIGNED_INT_10_10_10_2,      /* PF_BGR10_X2 */
	#else
	GL_UNSIGNED_INT_2_10_10_10_REV,  /* PF_BGR10_X2 */
	#endif
	GL_UNSIGNED_BYTE,                /* PF_XBGR */
	#ifdef BOOST_BIG_ENDIAN
	GL_UNSIGNED_INT_2_10_10_10_REV,  /* PF_X2_BGR10 */
	#else
	GL_UNSIGNED_INT_10_10_10_2,      /* PF_X2_BGR10 */
	#endif
	#ifdef BOOST_BIG_ENDIAN
	GL_UNSIGNED_INT_8_8_8_8_REV,     /* PF_XRGB */
	#else
	GL_UNSIGNED_INT_8_8_8_8,         /* PF_XRGB */
	#endif
	#ifdef BOOST_BIG_ENDIAN
	GL_UNSIGNED_INT_2_10_10_10_REV,  /* PF_X2_RGB10 */
	#else
	GL_UNSIGNED_INT_10_10_10_2,      /* PF_X2_RGB10 */
	#endif
	GL_UNSIGNED_BYTE                 /* PF_COMP */
};

#endif  /* __GLPF_H__ */
