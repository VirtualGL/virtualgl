/* Copyright (C)2004 Landmark Graphics
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

// This implements a JPEG compressor/decompressor using the Sun Medialib

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mlib.h>
#include "hpjpeg.h"

const char *lasterror="No error";
const int _mcuw[NUMSUBOPT]={8, 16, 16};
const int _mcuh[NUMSUBOPT]={8, 8, 16};

#define checkhandle(h) jpgstruct *jpg=(jpgstruct *)h; \
	if(!jpg) {lasterror="Invalid handle";  return -1;}

#define _throw(c) {printf("%s (%d):\n", __FILE__, __LINE__);  lasterror=c;  return -1;}
#define _mlib(a) {mlib_status __err;  if((__err=(a))!=MLIB_SUCCESS) _throw("MLIB failure in "#a"()");}
#define _mlibn(a) {if(!(a)) {lasterror="Failure in "#a"()"; return NULL;}}
#define _catch(a) {if((a)==-1) {printf("Caught in %s (%d)\n", __FILE__, __LINE__);  return -1;}}

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define write_byte(j, b) {if(j->bytesleft<=0) _throw("Not enough space in buffer");  \
	*j->jpgptr=b;  j->jpgptr++;  j->bytesprocessed++;  j->bytesleft--;}
#define write_word(j, w) {write_byte(j, (w>>8)&0xff);  write_byte(j, w&0xff);}

#define read_byte(j, b) {if(j->bytesleft<=0) _throw("Unexpected end of image");  \
	b=*j->jpgptr;  j->jpgptr++;  j->bytesleft--;  j->bytesprocessed++;}
#define read_word(j, w) {mlib_u8 __b;  read_byte(j, __b);  w=(__b<<8);  \
	read_byte(j, __b);  w|=(__b&0xff);}

typedef struct
{
	unsigned int ehufco[256];	/* code for each symbol */
  char ehufsi[256];		/* length of code for each symbol */
  /* If no code has been allocated for a symbol S, ehufsi[S] contains 0 */
} c_derived_tbl;

#define HUFF_LOOKAHEAD	8	/* # of bits of lookahead */

typedef struct
{
	/* Basic tables: (element [0] of each array is unused) */
	int maxcode[18];		/* largest code of length k (-1 if none) */
	/* (maxcode[17] is a sentinel to ensure jpeg_huff_decode terminates) */
	int valoffset[18];		/* huffval[] offset for codes of length k */
	/* valoffset[k] = huffval[] index of 1st symbol of code length k, less
	 * the smallest code of length k; so given a code of length k, the
	 * corresponding symbol is huffval[code + valoffset[k]]
	 */

	/* Lookahead tables: indexed by the next HUFF_LOOKAHEAD bits of
	 * the input data stream.  If the next Huffman code is no more
	 * than HUFF_LOOKAHEAD bits long, we can obtain its length and
	 * the corresponding symbol directly from these tables.
	 */
	int look_nbits[1<<HUFF_LOOKAHEAD]; /* # bits, or 0 if too long */
	mlib_u8 look_sym[1<<HUFF_LOOKAHEAD]; /* symbol, or unused */
	mlib_u8 huffval[256];		/* The symbols, in order of incr code length */
} d_derived_tbl;

typedef struct _jpgstruct
{
	mlib_u8 *bmpbuf, *bmpptr, *jpgbuf, *jpgptr;
	int xmcus, ymcus, width, height, pitch, ps, subsamp, qual, flags;
	unsigned long bytesprocessed, bytesleft;

	int huffbits, huffbuf, unread_marker, insufficient_data;
	c_derived_tbl *e_dclumtable, *e_aclumtable, *e_dcchromtable, *e_acchromtable;
	d_derived_tbl *d_dclumtable, *d_aclumtable, *d_dcchromtable, *d_acchromtable;
	mlib_d64 *chromqtable, *lumqtable;
	mlib_u8 *mcubuf8, *tmpbuf;  mlib_s16 *mcubuf;
	int initc, initd;
} jpgstruct;


//////////////////////////////////////////////////////////////////////////////
//    COMPRESSOR
//////////////////////////////////////////////////////////////////////////////

// Default quantization tables per JPEG spec

const mlib_s16 lumqtable[64]=
{
  16,  11,  10,  16,  24,  40,  51,  61,
  12,  12,  14,  19,  26,  58,  60,  55,
  14,  13,  16,  24,  40,  57,  69,  56,
  14,  17,  22,  29,  51,  87,  80,  62,
  18,  22,  37,  56,  68, 109, 103,  77,
  24,  35,  55,  64,  81, 104, 113,  92,
  49,  64,  78,  87, 103, 121, 120, 101,
  72,  92,  95,  98, 112, 100, 103,  99
};

const mlib_s16 chromqtable[64]=
{
  17,  18,  24,  47,  99,  99,  99,  99,
  18,  21,  26,  66,  99,  99,  99,  99,
  24,  26,  56,  99,  99,  99,  99,  99,
  47,  66,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99
};

// Huffman tables per JPEG spec
const mlib_u8 dclumbits[16]=
{
	0, 1, 5, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 0
};
const mlib_u8 dclumvals[]=
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

const mlib_u8 dcchrombits[16]=
{
	0, 3, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 0, 0, 0, 0, 0
};
const mlib_u8 dcchromvals[]=
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

const mlib_u8 aclumbits[16]=
{
	0, 2, 1, 3, 3, 2, 4, 3,
	5, 5, 4, 4, 0, 0, 1, 0x7d
};
const mlib_u8 aclumvals[]=
{
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

const mlib_u8 acchrombits[16]=
{
	0, 2, 1, 2, 4, 4, 3, 4,
	7, 5, 4, 4, 0, 1, 2, 0x77
};
const mlib_u8 acchromvals[]=
{
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

#include "hpjpeg-mlibhuff.c"

int e_mcu_color_convert(jpgstruct *jpg, int curxmcu, int curymcu)
{
	mlib_u8 *srcptr, *dstptr, *bmpptr=jpg->bmpptr, *mcuptr;
	mlib_status (DLLCALL *ccfct)(mlib_u8 *, mlib_u8 *, mlib_u8 *,
		const mlib_u8 *, mlib_s32);
	mlib_status (DLLCALL *ccfct420)(mlib_u8 *, mlib_u8 *, mlib_u8 *, mlib_u8 *,
		const mlib_u8 *, const mlib_u8 *, mlib_s32);
	int mcuw=_mcuw[jpg->subsamp], mcuh=_mcuh[jpg->subsamp], w, h, i, j;
	int flags=jpg->flags, ps=jpg->ps, pitch=jpg->pitch;
	int yindex, cbindex, crindex, ystride, cstride;

	w=mcuw;  h=mcuh;
	if(curxmcu==jpg->xmcus-1 && jpg->width%mcuw!=0) w=jpg->width%mcuw;
	if(curymcu==jpg->ymcus-1 && jpg->height%mcuh!=0) h=jpg->height%mcuh;

	if(jpg->flags&HPJ_BOTTOMUP) bmpptr=bmpptr-(h-1)*jpg->pitch;

	// Convert BGR to ABGR
	if(ps==3 && flags&HPJ_BGR)
	{
		mlib_VideoColorBGRint_to_ABGRint((mlib_u32 *)jpg->tmpbuf, bmpptr, NULL,
			0, w, h, mcuw*4, jpg->pitch, 0);
		ps=4; flags|=HPJ_ALPHAFIRST;  bmpptr=jpg->tmpbuf;  pitch=mcuw*4;
	}
	// Convert BGRA to ABGR
	else if(ps==4 && flags&HPJ_BGR && !(flags&HPJ_ALPHAFIRST))
	{
		mlib_VideoColorBGRAint_to_ABGRint((mlib_u32 *)jpg->tmpbuf, (mlib_u32 *)bmpptr,
			w, h, mcuw*4, jpg->pitch);
		flags|=HPJ_ALPHAFIRST;  bmpptr=jpg->tmpbuf;  pitch=mcuw*4;
	}
	// Convert RGBA to ABGR
	else if(ps==4 && !(flags&HPJ_BGR) && !(flags&HPJ_ALPHAFIRST))
	{
		mlib_VideoColorRGBAint_to_ARGBint((mlib_u32 *)jpg->tmpbuf, (mlib_u32 *)bmpptr,
			w, h, mcuw*4, jpg->pitch);
		flags|=HPJ_ALPHAFIRST;  bmpptr=jpg->tmpbuf;  pitch=mcuw*4;
	}

	switch(jpg->subsamp)
	{
		case HPJ_411:
			if(flags&HPJ_BGR && flags&HPJ_ALPHAFIRST && ps==4)
				ccfct420=mlib_VideoColorABGR2JFIFYCC420;
			else if(!(flags&HPJ_BGR) && flags&HPJ_ALPHAFIRST && ps==4)
				ccfct420=mlib_VideoColorARGB2JFIFYCC420;
			else if(!(flags&HPJ_BGR) && ps==3) ccfct420=mlib_VideoColorRGB2JFIFYCC420;
			break;
		case HPJ_422:
			if(flags&HPJ_BGR && flags&HPJ_ALPHAFIRST && ps==4)
				ccfct=mlib_VideoColorABGR2JFIFYCC422;
			else if(!(flags&HPJ_BGR) && flags&HPJ_ALPHAFIRST && ps==4)
				ccfct=mlib_VideoColorARGB2JFIFYCC422;
			else if(!(flags&HPJ_BGR) && ps==3) ccfct=mlib_VideoColorRGB2JFIFYCC422;
			break;
		case HPJ_444:
			if(flags&HPJ_BGR && flags&HPJ_ALPHAFIRST && ps==4)
				ccfct=mlib_VideoColorABGR2JFIFYCC444;
			else if(!(flags&HPJ_BGR) && flags&HPJ_ALPHAFIRST && ps==4)
				ccfct=mlib_VideoColorARGB2JFIFYCC444;
			else if(!(flags&HPJ_BGR) && ps==3) ccfct=mlib_VideoColorRGB2JFIFYCC444;
			break;
		default:
			_throw("Invalid argument to mcu_color_convert()");
	}

	// Copy into temp buffer (so we can take advantage of locality)
	if(bmpptr!=jpg->tmpbuf)
	{
		srcptr=bmpptr;  dstptr=jpg->tmpbuf;
		for(j=0; j<h; j++, srcptr+=jpg->pitch, dstptr+=mcuw*ps)
			mlib_memcpy(dstptr, srcptr, w*ps);
		bmpptr=jpg->tmpbuf;  pitch=mcuw*ps;
	}

	// Extend to a full MCU (if necessary)
	if(mcuw>w || mcuh>h)
	{
		if(mcuw>w)
		{
			for(j=0; j<h; j++)
			{
				srcptr=&jpg->tmpbuf[j*mcuw*ps];
				for(i=w; i<mcuw; i++)
					mlib_memcpy(&srcptr[i*ps], &srcptr[(w-1)*ps], ps);
			}
		}
		if(mcuh>h)
		{
			srcptr=&jpg->tmpbuf[(h-1)*mcuw*ps];
			for(j=h; j<mcuh; j++)
			{
				dstptr=&jpg->tmpbuf[j*mcuw*ps];
				mlib_memcpy(dstptr, srcptr, mcuw*ps);
			}
		}
	}

	// Do color conversion
	srcptr=bmpptr;
	yindex=0;  cbindex=mcuw*mcuh;  crindex=mcuw*mcuh+8*8;  ystride=mcuw;  cstride=8;
	if(flags&HPJ_BOTTOMUP)
	{
		yindex+=(mcuh-1)*mcuw;  cbindex+=(8-1)*8;  crindex+=(8-1)*8;
		ystride=-ystride;  cstride=-cstride;
	}
	if(jpg->subsamp==HPJ_411)
	{
		for(j=0; j<mcuh; j+=2, srcptr+=pitch*2, yindex+=ystride*2, cbindex+=cstride, crindex+=cstride)
		{
			_mlib(ccfct420(&jpg->mcubuf8[yindex], &jpg->mcubuf8[yindex+ystride], &jpg->mcubuf8[cbindex],
				&jpg->mcubuf8[crindex], srcptr, srcptr+pitch, mcuw));
		}
	}
	else
	{
		for(j=0; j<mcuh; j++, srcptr+=pitch, yindex+=ystride, cbindex+=cstride, crindex+=cstride)
		{
			_mlib(ccfct(&jpg->mcubuf8[yindex], &jpg->mcubuf8[cbindex],
				&jpg->mcubuf8[crindex], srcptr, mcuw));
		}
	}

	if(curxmcu==jpg->xmcus-1)
	{
		if(jpg->flags&HPJ_BOTTOMUP) jpg->bmpptr=&jpg->bmpbuf[(jpg->height-1-mcuh*(curymcu+1))*jpg->pitch];
		else jpg->bmpptr=&jpg->bmpbuf[mcuh*(curymcu+1)*jpg->pitch];
	}
	else jpg->bmpptr+=mcuw*jpg->ps;

	return 0;
}

void QuantFwdRawTableInit(mlib_s16 *rawqtable, int quality)
{
	int scale=quality, temp;  int i;
	if(scale<=0) scale=1;  if(scale>100) scale=100;
	if(scale<50) scale=5000/scale;  else scale=200-scale*2;

	for(i=0; i<64; i++)
	{
		temp=((int)rawqtable[i]*scale+50)/100;
		if(temp<=0) temp=1;
		if(temp>255) temp=255;
		rawqtable[i]=(mlib_s16)temp;
  }
}

int encode_jpeg_init(jpgstruct *jpg)
{
	mlib_s16 rawqtable[64];  int i, nval, len;
	int mcuw=_mcuw[jpg->subsamp], mcuh=_mcuh[jpg->subsamp];

	jpg->xmcus=(jpg->width+mcuw-1)/mcuw;
	jpg->ymcus=(jpg->height+mcuh-1)/mcuh;

	jpg->bytesprocessed=0;
	jpg->huffbits = jpg->huffbuf = jpg->unread_marker = jpg->insufficient_data = 0;

	if(jpg->flags&HPJ_BOTTOMUP) jpg->bmpptr=&jpg->bmpbuf[jpg->pitch*(jpg->height-1)];
	else jpg->bmpptr=jpg->bmpbuf;
	jpg->jpgptr=jpg->jpgbuf;

	write_byte(jpg, 0xff);  write_byte(jpg, 0xd8);  // Start of image marker

	// JFIF header
	write_byte(jpg, 0xff);  write_byte(jpg, 0xe0);  // JFIF marker
	write_word(jpg, 16);  // JFIF header length
	write_byte(jpg, 'J');  write_byte(jpg, 'F');
	write_byte(jpg, 'I');  write_byte(jpg, 'F');  write_byte(jpg, 0);
	write_word(jpg, 0x0101);  // JFIF version
	write_byte(jpg, 0);  // JFIF density units
	write_word(jpg, 1);  // JFIF X density
	write_word(jpg, 1);  // JFIF Y density
	write_byte(jpg, 0);  // thumbnail width
	write_byte(jpg, 0);  // thumbnail height

	// Generate and write quant. tables
	memcpy(rawqtable, lumqtable, 64*sizeof(mlib_s16));
	QuantFwdRawTableInit(rawqtable, jpg->qual);
	_mlib(mlib_VideoQuantizeInit_S16(jpg->lumqtable, rawqtable));

	write_byte(jpg, 0xff);  write_byte(jpg, 0xdb);  // DQT marker
	write_word(jpg, 67);  // DQT length
	write_byte(jpg, 0);  // Index for luminance
	for(i=0; i<64; i++) write_byte(jpg, (mlib_u8)rawqtable[jpeg_natural_order[i]]);

	memcpy(rawqtable, chromqtable, 64*sizeof(mlib_s16));
	QuantFwdRawTableInit(rawqtable, jpg->qual);
	_mlib(mlib_VideoQuantizeInit_S16(jpg->chromqtable, rawqtable));

	write_byte(jpg, 0xff);  write_byte(jpg, 0xdb);  // DQT marker
	write_word(jpg, 67);  // DQT length
	write_byte(jpg, 1);  // Index for chrominance
	for(i=0; i<64; i++) write_byte(jpg, (mlib_u8)rawqtable[jpeg_natural_order[i]]);

	// Write default Huffman tables
	write_byte(jpg, 0xff);  write_byte(jpg, 0xc4);  // DHT marker
	nval=0;  for(i=0; i<16; i++) nval+=dclumbits[i];
	len=19+nval;
	write_word(jpg, len);  // DHT length
	write_byte(jpg, 0x00);  // Huffman class
	for(i=0; i<16; i++) write_byte(jpg, dclumbits[i]);
	for(i=0; i<nval; i++) write_byte(jpg, dclumvals[i]);

	write_byte(jpg, 0xff);  write_byte(jpg, 0xc4);  // DHT marker
	nval=0;  for(i=0; i<16; i++) nval+=aclumbits[i];
	len=19+nval;
	write_word(jpg, len);  // DHT length
	write_byte(jpg, 0x10);  // Huffman class
	for(i=0; i<16; i++) write_byte(jpg, aclumbits[i]);
	for(i=0; i<nval; i++) write_byte(jpg, aclumvals[i]);

	write_byte(jpg, 0xff);  write_byte(jpg, 0xc4);  // DHT marker
	nval=0;  for(i=0; i<16; i++) nval+=dcchrombits[i];
	len=19+nval;
	write_word(jpg, len);  // DHT length
	write_byte(jpg, 0x01);  // Huffman class
	for(i=0; i<16; i++) write_byte(jpg, dcchrombits[i]);
	for(i=0; i<nval; i++) write_byte(jpg, dcchromvals[i]);

	write_byte(jpg, 0xff);  write_byte(jpg, 0xc4);  // DHT marker
	nval=0;  for(i=0; i<16; i++) nval+=acchrombits[i];
	len=19+nval;
	write_word(jpg, len);  // DHT length
	write_byte(jpg, 0x11);  // Huffman class
	for(i=0; i<16; i++) write_byte(jpg, acchrombits[i]);
	for(i=0; i<nval; i++) write_byte(jpg, acchromvals[i]);

	// Initialize Huffman tables
	_catch(jpeg_make_c_derived_tbl(dclumbits, dclumvals, 1, jpg->e_dclumtable));
	_catch(jpeg_make_c_derived_tbl(aclumbits, aclumvals, 0, jpg->e_aclumtable));
	_catch(jpeg_make_c_derived_tbl(dcchrombits, dcchromvals, 1, jpg->e_dcchromtable));
	_catch(jpeg_make_c_derived_tbl(acchrombits, acchromvals, 0, jpg->e_acchromtable));

	// Write Start Of Frame
	write_byte(jpg, 0xff);  write_byte(jpg, 0xc0);  // SOF marker
	write_word(jpg, 17);  // SOF length
	write_byte(jpg, 8);  // precision
	write_word(jpg, jpg->height);
	write_word(jpg, jpg->width);
	write_byte(jpg, 3);  // Number of components

	write_byte(jpg, 1);  // Y Component ID
	write_byte(jpg, ((mcuw/8)<<4)+(mcuh/8));  // Horiz. and Vert. sampling factors
	write_byte(jpg, 0);  // Quantization table selector
	for(i=2; i<=3; i++)
	{
		write_byte(jpg, i);  // Component ID
		write_byte(jpg, 0x11);  // Horiz. and Vert. sampling factors
		write_byte(jpg, 1);  // Quantization table selector
	}

	// Write Start of Scan
	write_byte(jpg, 0xff);  write_byte(jpg, 0xda);  // SOS marker
	write_word(jpg, 12);  // SOS length
	write_byte(jpg, 3);  // Number of components
	for(i=1; i<=3; i++)
	{
		write_byte(jpg, i);  // Component ID
		write_byte(jpg, i==1? 0 : 0x11);  // Huffman table selector
	}
	write_byte(jpg, 0);  // Spectral start
	write_byte(jpg, 63);  // Spectral end
	write_byte(jpg, 0);  // Successive approximation (N/A unless progressive)

	return 0;
}

int encode_jpeg(jpgstruct *jpg)
{
	int i, j, k;
	int mcuw=_mcuw[jpg->subsamp], mcuh=_mcuh[jpg->subsamp];
	int mcusize=mcuw*mcuh+128;
	int lastdc[3]={0, 0, 0};
	int x,y;

	_catch(encode_jpeg_init(jpg));

	for(j=0; j<jpg->ymcus; j++)
	{
		for(i=0; i<jpg->xmcus; i++)
		{
			_catch(e_mcu_color_convert(jpg, i, j));

			k=0;    // Perform DCT on all luminance blocks
			for(y=0; y<mcuh; y+=8)
				for(x=0; x<mcuw; x+=8)
				{
					_mlib(mlib_VideoDCT8x8_S16_U8(&jpg->mcubuf[k], &jpg->mcubuf8[y*mcuw+x], mcuw));
					jpg->mcubuf[k]-=1024;
					k+=64;
				}
			for(k=mcuw*mcuh; k<mcusize; k+=64)  // Perform DCT on all chrominance blocks
			{
				_mlib(mlib_VideoDCT8x8_S16_U8(&jpg->mcubuf[k], &jpg->mcubuf8[k], 8));
				jpg->mcubuf[k]-=1024;
			}

			for(k=0; k<mcuw*mcuh; k+=64)  // Quantize the luminance blocks
				_mlib(mlib_VideoQuantize_S16(&jpg->mcubuf[k], jpg->lumqtable));
			for(k=mcuw*mcuh; k<mcusize; k+=64)  // Quantize the chrominance blocks
				_mlib(mlib_VideoQuantize_S16(&jpg->mcubuf[k], jpg->chromqtable));

			for(k=0; k<mcuw*mcuh; k+=64)  // Huffman encode the luminance blocks
				_catch( encode_one_block(jpg, &jpg->mcubuf[k], &lastdc[0],
					jpg->e_dclumtable, jpg->e_aclumtable) );

			// Huffman encode the Cb block
			_catch( encode_one_block(jpg, &jpg->mcubuf[k], &lastdc[1],
				jpg->e_dcchromtable, jpg->e_acchromtable) );
			k+=64;

			// Huffman encode the Cr block
			_catch( encode_one_block(jpg, &jpg->mcubuf[k], &lastdc[2],
				jpg->e_dcchromtable, jpg->e_acchromtable) );

		} // xmcus
	} // ymcus

	// Flush Huffman state
	_catch( flush_bits(jpg) );

	write_byte(jpg, 0xff);  write_byte(jpg, 0xd9);  // EOI marker

	return 0;
}

DLLEXPORT hpjhandle DLLCALL hpjInitCompress(void)
{
	jpgstruct *jpg;  int huffbufsize;

	if((jpg=(jpgstruct *)malloc(sizeof(jpgstruct)))==NULL)
		{lasterror="Memory allocation failure";  return NULL;}
	memset(jpg, 0, sizeof(jpgstruct));

	_mlibn(jpg->lumqtable=(mlib_d64 *)mlib_malloc(64*sizeof(mlib_d64)));
	_mlibn(jpg->chromqtable=(mlib_d64 *)mlib_malloc(64*sizeof(mlib_d64)));
	_mlibn(jpg->mcubuf8=(mlib_u8 *)mlib_malloc(384*sizeof(mlib_u8)));
	_mlibn(jpg->mcubuf=(mlib_s16 *)mlib_malloc(384*sizeof(mlib_s16)));
	_mlibn(jpg->tmpbuf=(mlib_u8 *)mlib_malloc(16*16*4*sizeof(mlib_u8)));

	if((jpg->e_dclumtable=(c_derived_tbl *)mlib_malloc(sizeof(c_derived_tbl)))==NULL
	|| (jpg->e_aclumtable=(c_derived_tbl *)mlib_malloc(sizeof(c_derived_tbl)))==NULL
	|| (jpg->e_dcchromtable=(c_derived_tbl *)mlib_malloc(sizeof(c_derived_tbl)))==NULL
	|| (jpg->e_acchromtable=(c_derived_tbl *)mlib_malloc(sizeof(c_derived_tbl)))==NULL)
		{lasterror="Memory Allocation failure";  return NULL;}

	jpg->initc=1;

	return (hpjhandle)jpg;
}

DLLEXPORT int DLLCALL hpjCompress(hpjhandle h,
	unsigned char *srcbuf, int width, int pitch, int height, int ps,
	unsigned char *dstbuf, unsigned long *size,
	int jpegsub, int qual, int flags)
{
	checkhandle(h);

	if(srcbuf==NULL || width<=0 || height<=0
		|| dstbuf==NULL || size==NULL
		|| jpegsub<0 || jpegsub>=NUMSUBOPT || qual<0 || qual>100)
		_throw("Invalid argument in hpjCompress()");
	if(qual<1) qual=1;
	if(!jpg->initc) _throw("Instance has not been initialized for compression");

	if(ps!=3 && ps!=4) _throw("This JPEG codec supports only 24-bit or 32-bit true color");

	if(pitch==0) pitch=width*ps;

	jpg->bmpbuf=srcbuf;

	jpg->jpgbuf=dstbuf;
	jpg->width=width;  jpg->height=height;  jpg->pitch=pitch;
	jpg->ps=ps;  jpg->subsamp=jpegsub;  jpg->qual=qual;  jpg->flags=flags;
	jpg->bytesleft=HPJBUFSIZE(width, height);

	_catch(encode_jpeg(jpg));

	*size=jpg->bytesprocessed;
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//   DECOMPRESSOR
//////////////////////////////////////////////////////////////////////////////

int find_marker(jpgstruct *jpg, unsigned char *marker)
{
	unsigned char b;
	while(1)
	{
		do {read_byte(jpg, b);} while(b!=0xff);
		read_byte(jpg, b);
		if(b!=0 && b!=0xff) {*marker=b;  return 0;}
		else {jpg->jpgptr--;  jpg->bytesleft++;  jpg->bytesprocessed--;}
	}
}

#define check_byte(j, b) {unsigned char __b;  read_byte(j, __b);  \
	if(__b!=(b)) _throw("JPEG bitstream error");}

int d_mcu_color_convert(jpgstruct *jpg, int curxmcu, int curymcu)
{
	mlib_u8 *bmpptr=jpg->bmpptr, *dstptr, *srcptr, *tmpptr,
		*tmpptr2=&jpg->tmpbuf[16*16*4];
	mlib_status (DLLCALL *ccfct)(mlib_u8 *, const mlib_u8 *, const mlib_u8 *,
		const mlib_u8 *, mlib_s32);
	mlib_status (DLLCALL *ccfct420)(mlib_u8 *, mlib_u8 *, const mlib_u8 *,
		const mlib_u8 *, const mlib_u8 *, const mlib_u8 *, mlib_s32);
	int mcuw=_mcuw[jpg->subsamp], mcuh=_mcuh[jpg->subsamp], w, h;
	int flags=jpg->flags, ps=jpg->ps, tmppitch=jpg->pitch;
	int yindex, cbindex, crindex, ystride, cstride, j, i;

	w=mcuw;  h=mcuh;
	if(curxmcu==jpg->xmcus-1 && jpg->width%mcuw!=0) w=jpg->width%mcuw;
	if(curymcu==jpg->ymcus-1 && jpg->height%mcuh!=0) h=jpg->height%mcuh;

	if(jpg->flags&HPJ_BOTTOMUP) bmpptr=bmpptr-(h-1)*jpg->pitch;
	tmpptr=jpg->tmpbuf;  tmppitch=mcuw*ps;

	switch(jpg->subsamp)
	{
		case HPJ_411:
			ccfct420=mlib_VideoColorJFIFYCC2RGB420_Nearest;
			break;
		case HPJ_422:
			ccfct=mlib_VideoColorJFIFYCC2RGB422_Nearest;
			break;
		case HPJ_444:
			ccfct=mlib_VideoColorJFIFYCC2RGB444;
			if(!(flags&HPJ_BGR) && flags&HPJ_ALPHAFIRST && ps==4)  // ARGB
				ccfct=mlib_VideoColorJFIFYCC2ARGB444;
			if(!(flags&HPJ_BGR) && ps==3)  // RGB
			{
				// Note: for some odd reason, it's faster to avoid
				// the temp. buffer only when converting 4:4:4 to RGB
				tmpptr=bmpptr;  tmppitch=jpg->pitch;
			}
			break;
		default:
			_throw("Invalid argument to mcu_color_convert()");
	}

	// Do color conversion
	dstptr=tmpptr;
	yindex=0;  cbindex=mcuw*mcuh;  crindex=mcuw*mcuh+8*8;  ystride=mcuw;  cstride=8;
	if(flags&HPJ_BOTTOMUP)
	{
		yindex+=(h-1)*mcuw;  cbindex+=(8-1)*8;  crindex+=(8-1)*8;
		ystride=-ystride;  cstride=-cstride;
	}
	if(jpg->subsamp==HPJ_411)
	{
		for(j=0; j<h; j+=2, dstptr+=tmppitch*2, yindex+=ystride*2, cbindex+=cstride, crindex+=cstride)
		{
			_mlib(ccfct420(dstptr, dstptr+tmppitch, &jpg->mcubuf8[yindex], &jpg->mcubuf8[yindex+ystride],
				&jpg->mcubuf8[cbindex], &jpg->mcubuf8[crindex], w));
		}
	}
	else
	{
		for(j=0; j<h; j++, dstptr+=tmppitch, yindex+=ystride, cbindex+=cstride, crindex+=cstride)
		{
			_mlib(ccfct(dstptr, &jpg->mcubuf8[yindex], &jpg->mcubuf8[cbindex],
				&jpg->mcubuf8[crindex], w));
		}
	}

	// Convert RGB to BGR
	if(flags&HPJ_BGR && ps==3)
	{
		for(j=0; j<h; j++)
			_mlib(mlib_VectorReverseByteOrder(&bmpptr[j*jpg->pitch], &tmpptr[j*tmppitch], w, 3));
		tmpptr=bmpptr;
	}
	// Convert RGB to ABGR
	if(flags&HPJ_BGR && flags&HPJ_ALPHAFIRST && ps==4)
	{
		mlib_VideoColorRGBint_to_ABGRint((mlib_u32 *)bmpptr, tmpptr, NULL, 0, w, h,
			jpg->pitch, tmppitch, 0);
		tmpptr=bmpptr;
	}
	// Convert RGB to ARGB
	if(!(flags&HPJ_BGR) && flags&HPJ_ALPHAFIRST && ps==4 &&
		jpg->subsamp!=HPJ_444)
	{
		mlib_VideoColorRGBint_to_ARGBint((mlib_u32 *)bmpptr, tmpptr, NULL, 0, w, h,
			jpg->pitch, tmppitch, 0);
		tmpptr=bmpptr;
	}
	// Convert RGB to BGRA
	if(flags&HPJ_BGR && !(flags&HPJ_ALPHAFIRST) && ps==4)
	{
		mlib_VideoColorRGBint_to_ARGBint((mlib_u32 *)tmpptr2, tmpptr, NULL, 0, w, h,
			mcuw*4, tmppitch, 0);
		for(j=0; j<h; j++)
			_mlib(mlib_VectorReverseByteOrder(&bmpptr[j*jpg->pitch], &tmpptr2[j*mcuw*4], w, 4));
		tmpptr=bmpptr;
	}
	// Convert RGB to RGBA
	if(!(flags&HPJ_BGR) && !(flags&HPJ_ALPHAFIRST) && ps==4)
	{
		mlib_VideoColorRGBint_to_ABGRint((mlib_u32 *)tmpptr2, tmpptr, NULL, 0, w, h,
			mcuw*4, tmppitch, 0);
		for(j=0; j<h; j++)
			_mlib(mlib_VectorReverseByteOrder(&bmpptr[j*jpg->pitch], &tmpptr2[j*mcuw*4], w, 4));
		tmpptr=bmpptr;
	}

	if(tmpptr!=bmpptr)
	{
		srcptr=tmpptr;  dstptr=bmpptr;
		for(j=0; j<h; j++, srcptr+=tmppitch, dstptr+=jpg->pitch)
			mlib_memcpy(dstptr, srcptr, w*ps);
	}

	if(curxmcu==jpg->xmcus-1)
	{
		if(jpg->flags&HPJ_BOTTOMUP) jpg->bmpptr=&jpg->bmpbuf[(jpg->height-1-mcuh*(curymcu+1))*jpg->pitch];
		else jpg->bmpptr=&jpg->bmpbuf[mcuh*(curymcu+1)*jpg->pitch];
	}
	else jpg->bmpptr+=mcuw*jpg->ps;

	return 0;
}

int decode_jpeg_init(jpgstruct *jpg)
{
	mlib_s16 rawqtable[64];  mlib_u8 rawhuffbits[16], rawhuffvalues[256];
	int i;  unsigned char tempbyte, tempbyte2, marker;  unsigned short tempword, length;
	int markerread=0;  unsigned char compid[3];

	jpg->bytesprocessed=0;
	jpg->huffbits = jpg->huffbuf = jpg->unread_marker = jpg->insufficient_data = 0;
	if(jpg->flags&HPJ_BOTTOMUP) jpg->bmpptr=&jpg->bmpbuf[jpg->pitch*(jpg->height-1)];
	else jpg->bmpptr=jpg->bmpbuf;
	jpg->jpgptr=jpg->jpgbuf;

	check_byte(jpg, 0xff);  check_byte(jpg, 0xd8);  // SOI

	while(1)
	{
		_catch(find_marker(jpg, &marker));

		switch(marker)
		{
			case 0xe0:  // JFIF
				read_word(jpg, length);
				if(length<8) _throw("JPEG bitstream error");
				check_byte(jpg, 'J');  check_byte(jpg, 'F');
				check_byte(jpg, 'I');  check_byte(jpg, 'F');  check_byte(jpg, 0);
				for(i=7; i<length; i++) read_byte(jpg, tempbyte) // We don't care about the rest
				markerread+=1;
				break;
			case 0xdb:  // DQT
			{
				int dqtbytecount;
				read_word(jpg, length);
				if(length<67 || length>(64+1)*2+2) _throw("JPEG bitstream error");
				dqtbytecount=2;
				while(dqtbytecount<length)
				{
					read_byte(jpg, tempbyte);  // Quant. table index
					dqtbytecount++;
					for(i=0; i<64; i++)
					{
						read_byte(jpg, tempbyte2);
						rawqtable[jpeg_natural_order[i]]=(mlib_s16)tempbyte2;
					}
					dqtbytecount+=64;

					if(tempbyte==0)
					{
						_mlib(mlib_VideoDeQuantizeInit_S16(jpg->lumqtable, rawqtable));
						markerread+=2;
					}
					else if(tempbyte==1)
					{
						_mlib(mlib_VideoDeQuantizeInit_S16(jpg->chromqtable, rawqtable));
						markerread+=4;
					}
				}
				break;
			}
			case 0xc4:  // DHT
			{
				int nval, dhtbytecount;
				read_word(jpg, length);
				if(length<19 || length>(17+256)*4+2) _throw("JPEG bitstream error");
				dhtbytecount=2;
				while(dhtbytecount<length)
				{
					read_byte(jpg, tempbyte);  // Huffman class
					dhtbytecount++;
					memset(rawhuffbits, 0, 16);
					memset(rawhuffvalues, 0, 256);
					nval=0;
					for(i=0; i<16; i++) {read_byte(jpg, rawhuffbits[i]);  nval+=rawhuffbits[i];}
					dhtbytecount+=16;
					if(nval>256) _throw("JPEG bitstream error");
					for(i=0; i<nval; i++) read_byte(jpg, rawhuffvalues[i]);
					dhtbytecount+=nval;
					if(tempbyte==0x00)  // DC luminance
					{
						_catch(jpeg_make_d_derived_tbl(rawhuffbits, rawhuffvalues, 1,
							jpg->d_dclumtable));
						markerread+=8;
					}
					else if(tempbyte==0x10)  // AC luminance
					{
						_catch(jpeg_make_d_derived_tbl(rawhuffbits, rawhuffvalues, 0,
							jpg->d_aclumtable));
						markerread+=16;
					}
					else if(tempbyte==0x01)  // DC chrominance
					{
						_catch(jpeg_make_d_derived_tbl(rawhuffbits, rawhuffvalues, 1,
							jpg->d_dcchromtable));
						markerread+=32;
					}
					else if(tempbyte==0x11)  // AC chrominance
					{
						_catch(jpeg_make_d_derived_tbl(rawhuffbits, rawhuffvalues, 0,
							jpg->d_acchromtable));
						markerread+=64;
					}
				}
				break;
			}
			case 0xc0:  // SOF
				read_word(jpg, length);
				if(length<11) _throw("JPEG bitstream error");
				read_byte(jpg, tempbyte);  // precision
				if(tempbyte!=8) _throw("Only 8-bit-per-component JPEGs are supported");
				read_word(jpg, tempword);  // height
				if(!jpg->height) jpg->height=tempword;
				if(tempword!=jpg->height) _throw("Height mismatch between JPEG and bitmap");
				read_word(jpg, tempword);  // width
				if(!jpg->width) jpg->width=tempword;
				if(tempword!=jpg->width) _throw("Width mismatch between JPEG and bitmap");
				read_byte(jpg, tempbyte);  // Number of components
				if(tempbyte!=3 || length<17) _throw("Only YCbCr JPEG's are supported");
				read_byte(jpg, compid[0]);  // Component ID
				read_byte(jpg, tempbyte);  // Horiz. and Vert. sampling factors
				if(tempbyte==0x11) jpg->subsamp=HPJ_444;
				else if(tempbyte==0x21) jpg->subsamp=HPJ_422;
				else if(tempbyte==0x22) jpg->subsamp=HPJ_411;
				else _throw("Unsupported subsampling type");
				check_byte(jpg, 0);  // Luminance
				for(i=1; i<3; i++)
				{
					read_byte(jpg, compid[i]);  // Component ID
					check_byte(jpg, 0x11);  // Sampling factors
					check_byte(jpg, 1);  // Chrominance
				}
				markerread+=128;
				break;
			case 0xda:  // SOS
				if(markerread!=255) _throw("JPEG bitstream error");
				read_word(jpg, length);
				if(length<12) _throw("JPEG bitstream error");
				check_byte(jpg, 3);  // Number of components
				for(i=0; i<3; i++)
				{
					check_byte(jpg, compid[i])
					check_byte(jpg, i==0? 0 : 0x11);  // Huffman table selector
				}
				for(i=0; i<3; i++) read_byte(jpg, tempbyte);
				goto done;
		}
	}
	done:
	return 0;
}

int decode_jpeg(jpgstruct *jpg)
{
	int i, j, k, mcuw, mcuh, mcusize, marker, x, y;
	int lastdc[3]={0, 0, 0};

	_catch(decode_jpeg_init(jpg));

	mcuw=_mcuw[jpg->subsamp];  mcuh=_mcuh[jpg->subsamp];
	jpg->xmcus=(jpg->width+mcuw-1)/mcuw;
	jpg->ymcus=(jpg->height+mcuh-1)/mcuh;
	mcusize=mcuw*mcuh+128;

	for(j=0; j<jpg->ymcus; j++)
	{
		for(i=0; i<jpg->xmcus; i++)
		{

			for(k=0; k<mcuw*mcuh; k+=64)  // Huffman decode the luminance blocks
			{
				_catch( decode_one_block(jpg, &jpg->mcubuf[k], &lastdc[0],
					jpg->d_dclumtable, jpg->d_aclumtable) );
			}

			// Huffman decode the Cb block
			_catch( decode_one_block(jpg, &jpg->mcubuf[k], &lastdc[1],
				jpg->d_dcchromtable, jpg->d_acchromtable) );
			k+=64;

			// Huffman decode the Cr block
			_catch( decode_one_block(jpg, &jpg->mcubuf[k], &lastdc[2],
				jpg->d_dcchromtable, jpg->d_acchromtable) );

			for(k=0; k<mcuw*mcuh; k+=64)  // Un-quantize the luminance blocks
				_mlib(mlib_VideoDeQuantize_S16(&jpg->mcubuf[k], jpg->lumqtable));
			for(k=mcuw*mcuh; k<mcusize; k+=64)  // Un-quantize the chrominance blocks
				_mlib(mlib_VideoDeQuantize_S16(&jpg->mcubuf[k], jpg->chromqtable));

			k=0;    // Perform inverse DCT on all luminance blocks
			for(y=0; y<mcuh; y+=8)
				for(x=0; x<mcuw; x+=8)
				{
					jpg->mcubuf[k]+=1024;
					_mlib(mlib_VideoIDCT8x8_U8_S16(&jpg->mcubuf8[y*mcuw+x], &jpg->mcubuf[k], mcuw));
					k+=64;
				}
			for(k=mcuw*mcuh; k<mcusize; k+=64)  // Perform inverse DCT on all chrominance blocks
			{
				jpg->mcubuf[k]+=1024;
				_mlib(mlib_VideoIDCT8x8_U8_S16(&jpg->mcubuf8[k], &jpg->mcubuf[k], 8));
			}

			_catch(d_mcu_color_convert(jpg, i, j));
		}
	}

	return 0;
}

DLLEXPORT hpjhandle DLLCALL hpjInitDecompress(void)
{
	jpgstruct *jpg;  int huffbufsize;

	if((jpg=(jpgstruct *)malloc(sizeof(jpgstruct)))==NULL)
		{lasterror="Memory allocation failure";  return NULL;}
	memset(jpg, 0, sizeof(jpgstruct));

	_mlibn(jpg->lumqtable=(mlib_d64 *)mlib_malloc(64*sizeof(mlib_d64)));
	_mlibn(jpg->chromqtable=(mlib_d64 *)mlib_malloc(64*sizeof(mlib_d64)));
	_mlibn(jpg->mcubuf8=(mlib_u8 *)mlib_malloc(384*sizeof(mlib_u8)));
	_mlibn(jpg->mcubuf=(mlib_s16 *)mlib_malloc(384*sizeof(mlib_s16)));
	_mlibn(jpg->tmpbuf=(mlib_u8 *)mlib_malloc(16*16*4*2*sizeof(mlib_u8)));

	if((jpg->d_dclumtable=(d_derived_tbl *)mlib_malloc(sizeof(d_derived_tbl)))==NULL
	|| (jpg->d_aclumtable=(d_derived_tbl *)mlib_malloc(sizeof(d_derived_tbl)))==NULL
	|| (jpg->d_dcchromtable=(d_derived_tbl *)mlib_malloc(sizeof(d_derived_tbl)))==NULL
	|| (jpg->d_acchromtable=(d_derived_tbl *)mlib_malloc(sizeof(d_derived_tbl)))==NULL)
		{lasterror="Memory Allocation failure";  return NULL;}

	jpg->initd=1;
	return (hpjhandle)jpg;
}

DLLEXPORT int DLLCALL hpjDecompressHeader(hpjhandle h,
	unsigned char *srcbuf, unsigned long size,
	int *width, int *height)
{
	checkhandle(h);

	if(srcbuf==NULL || size<=0 || width==NULL || height==NULL)
		_throw("Invalid argument in hpjDecompressHeader()");
	if(!jpg->initd) _throw("Instance has not been initialized for decompression");

	jpg->jpgbuf=srcbuf;
	jpg->bytesleft=size;

	jpg->width=jpg->height=0;

	_catch(decode_jpeg_init(jpg));
	*width=jpg->width;  *height=jpg->height;

	return 0;
}

DLLEXPORT int DLLCALL hpjDecompress(hpjhandle h,
	unsigned char *srcbuf, unsigned long size,
	unsigned char *dstbuf, int width, int pitch, int height, int ps,
	int flags)
{
	checkhandle(h);

	if(srcbuf==NULL || size<=0
		|| dstbuf==NULL || width<=0 || height<=0)
		_throw("Invalid argument in hpjDecompress()");
	if(!jpg->initd) _throw("Instance has not been initialized for decompression");

	if(ps!=3 && ps!=4) _throw("This JPEG codec supports only 24-bit or 32-bit true color");

	if(pitch==0) pitch=width*ps;

	jpg->bmpbuf=dstbuf;

	jpg->jpgbuf=srcbuf;
	jpg->width=width;  jpg->height=height;  jpg->pitch=pitch;
	jpg->ps=ps;  jpg->flags=flags;
	jpg->bytesleft=size;

	_catch(decode_jpeg(jpg));

	return 0;
}


// General

DLLEXPORT char* DLLCALL hpjGetErrorStr(void)
{
	return (char *)lasterror;
}

DLLEXPORT int DLLCALL hpjDestroy(hpjhandle h)
{
	checkhandle(h);

	if(jpg->mcubuf8) mlib_free(jpg->mcubuf8);
	if(jpg->mcubuf) mlib_free(jpg->mcubuf);
	if(jpg->tmpbuf) mlib_free(jpg->tmpbuf);
	if(jpg->lumqtable) mlib_free(jpg->lumqtable);
	if(jpg->chromqtable) mlib_free(jpg->chromqtable);

	if(jpg->initc)
	{
		if(jpg->e_dclumtable) mlib_free(jpg->e_dclumtable);
		if(jpg->e_aclumtable) mlib_free(jpg->e_aclumtable);
		if(jpg->e_dcchromtable) mlib_free(jpg->e_dcchromtable);
		if(jpg->e_acchromtable) mlib_free(jpg->e_acchromtable);
	}

	if(jpg->initd)
	{
		if(jpg->d_dclumtable) mlib_free(jpg->d_dclumtable);
		if(jpg->d_aclumtable) mlib_free(jpg->d_aclumtable);
		if(jpg->d_dcchromtable) mlib_free(jpg->d_dcchromtable);
		if(jpg->d_acchromtable) mlib_free(jpg->d_acchromtable);
	}

	free(jpg);
	return 0;
}
