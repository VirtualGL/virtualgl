/* Copyright (C)2005 Sun Microsystems, Inc.
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#define ALIGNPTR(p) (mlib_u8 *)(((unsigned long)(p)+7L)&(~7L))

// BGR

inline mlib_status mlib_VideoColorJFIFYCC2BGR420 (mlib_u8 *rgb0, mlib_u8 *rgb1,
	const mlib_u8 *y0, const mlib_u8 *y1, const mlib_u8 *cb, const mlib_u8 *cr,
	mlib_s32 n)
{
	mlib_status ret;
	if((ret=mlib_VideoColorJFIFYCC2RGB420_Nearest(rgb0, rgb1, y0, y1, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VectorReverseByteOrder_Inp(rgb0, n, 3))!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb1, n, 3);
}

inline mlib_status mlib_VideoColorJFIFYCC2BGR422 (mlib_u8 *rgb, const mlib_u8 *y,
	const mlib_u8 *cb, const mlib_u8 *cr, mlib_s32 n)
{
	mlib_status ret;
	if((ret=mlib_VideoColorJFIFYCC2RGB422_Nearest(rgb, y, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb, n, 3);
}

inline mlib_status mlib_VideoColorJFIFYCC2BGR444 (mlib_u8 *rgb, const mlib_u8 *y,
	const mlib_u8 *cb, const mlib_u8 *cr, mlib_s32 n)
{
	mlib_status ret;
	if((ret=mlib_VideoColorJFIFYCC2RGB444(rgb, y, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb, n, 3);
}

// RGBA

inline mlib_status mlib_VideoColorJFIFYCC2RGBA420 (mlib_u8 *rgb0, mlib_u8 *rgb1,
	const mlib_u8 *y0, const mlib_u8 *y1, const mlib_u8 *cb, const mlib_u8 *cr,
	mlib_s32 n)
{
	mlib_status ret;  mlib_u8 tmpbuf[16*2*4+7], *tmpptr=ALIGNPTR(tmpbuf);
	if((ret=mlib_VideoColorJFIFYCC2RGB420_Nearest(tmpptr, tmpptr+16*4, y0, y1, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VideoColorRGB2ABGR(rgb0, tmpptr, n))!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VideoColorRGB2ABGR(rgb1, tmpptr+16*4, n))!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VectorReverseByteOrder_Inp(rgb0, n, 4))!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb1, n, 4);
}

inline mlib_status mlib_VideoColorJFIFYCC2RGBA422 (mlib_u8 *rgb, const mlib_u8 *y,
	const mlib_u8 *cb, const mlib_u8 *cr, mlib_s32 n)
{
	mlib_status ret;  mlib_u8 tmpbuf[16*4+7], *tmpptr=ALIGNPTR(tmpbuf);
	if((ret=mlib_VideoColorJFIFYCC2RGB422_Nearest(tmpptr, y, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VideoColorRGB2ABGR(rgb, tmpptr, n))!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb, n, 4);
}

inline mlib_status mlib_VideoColorJFIFYCC2RGBA444 (mlib_u8 *rgb, const mlib_u8 *y,
	const mlib_u8 *cb, const mlib_u8 *cr, mlib_s32 n)
{
	mlib_status ret;
	if((ret=mlib_VideoColorJFIFYCC2ABGR444(rgb, y, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb, n, 4);
}

// BGRA

inline mlib_status mlib_VideoColorJFIFYCC2BGRA420 (mlib_u8 *rgb0, mlib_u8 *rgb1,
	const mlib_u8 *y0, const mlib_u8 *y1, const mlib_u8 *cb, const mlib_u8 *cr,
	mlib_s32 n)
{
	mlib_status ret;  mlib_u8 tmpbuf[16*2*4+7], *tmpptr=ALIGNPTR(tmpbuf);
	if((ret=mlib_VideoColorJFIFYCC2RGB420_Nearest(tmpptr, tmpptr+16*4, y0, y1, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VideoColorRGB2ARGB(rgb0, tmpptr, n))!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VideoColorRGB2ARGB(rgb1, tmpptr+16*4, n))!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VectorReverseByteOrder_Inp(rgb0, n, 4))!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb1, n, 4);
}

inline mlib_status mlib_VideoColorJFIFYCC2BGRA422 (mlib_u8 *rgb, const mlib_u8 *y,
	const mlib_u8 *cb, const mlib_u8 *cr, mlib_s32 n)
{
	mlib_status ret;  mlib_u8 tmpbuf[16*4+7], *tmpptr=ALIGNPTR(tmpbuf);
	if((ret=mlib_VideoColorJFIFYCC2RGB422_Nearest(tmpptr, y, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VideoColorRGB2ARGB(rgb, tmpptr, n))!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb, n, 4);
}

inline mlib_status mlib_VideoColorJFIFYCC2BGRA444 (mlib_u8 *rgb, const mlib_u8 *y,
	const mlib_u8 *cb, const mlib_u8 *cr, mlib_s32 n)
{
	mlib_status ret;
	if((ret=mlib_VideoColorJFIFYCC2ARGB444(rgb, y, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	return mlib_VectorReverseByteOrder_Inp(rgb, n, 4);
}

// ARGB

inline mlib_status mlib_VideoColorJFIFYCC2ARGB420 (mlib_u8 *rgb0, mlib_u8 *rgb1,
	const mlib_u8 *y0, const mlib_u8 *y1, const mlib_u8 *cb, const mlib_u8 *cr,
	mlib_s32 n)
{
	mlib_status ret;  mlib_u8 tmpbuf[16*2*4+7], *tmpptr=ALIGNPTR(tmpbuf);
	if((ret=mlib_VideoColorJFIFYCC2RGB420_Nearest(tmpptr, tmpptr+16*4, y0, y1, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VideoColorRGB2ARGB(rgb0, tmpptr, n))!=MLIB_SUCCESS) return ret;
	return mlib_VideoColorRGB2ARGB(rgb1, tmpptr+16*4, n);
}

inline mlib_status mlib_VideoColorJFIFYCC2ARGB422 (mlib_u8 *rgb, const mlib_u8 *y,
	const mlib_u8 *cb, const mlib_u8 *cr, mlib_s32 n)
{
	mlib_status ret;  mlib_u8 tmpbuf[16*4+7], *tmpptr=ALIGNPTR(tmpbuf);
	if((ret=mlib_VideoColorJFIFYCC2RGB422_Nearest(tmpptr, y, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	return mlib_VideoColorRGB2ARGB(rgb, tmpptr, n);
}

// ABGR

inline mlib_status mlib_VideoColorJFIFYCC2ABGR420 (mlib_u8 *rgb0, mlib_u8 *rgb1,
	const mlib_u8 *y0, const mlib_u8 *y1, const mlib_u8 *cb, const mlib_u8 *cr,
	mlib_s32 n)
{
	mlib_status ret;  mlib_u8 tmpbuf[16*2*4+7], *tmpptr=ALIGNPTR(tmpbuf);
	if((ret=mlib_VideoColorJFIFYCC2RGB420_Nearest(tmpptr, tmpptr+16*4, y0, y1, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	if((ret=mlib_VideoColorRGB2ABGR(rgb0, tmpptr, n))!=MLIB_SUCCESS) return ret;
	return mlib_VideoColorRGB2ABGR(rgb1, tmpptr+16*4, n);
}

inline mlib_status mlib_VideoColorJFIFYCC2ABGR422 (mlib_u8 *rgb, const mlib_u8 *y,
	const mlib_u8 *cb, const mlib_u8 *cr, mlib_s32 n)
{
	mlib_status ret;  mlib_u8 tmpbuf[16*4+7], *tmpptr=ALIGNPTR(tmpbuf);
	if((ret=mlib_VideoColorJFIFYCC2RGB422_Nearest(tmpptr, y, cb, cr, n))
		!=MLIB_SUCCESS) return ret;
	return mlib_VideoColorRGB2ABGR(rgb, tmpptr, n);
}
