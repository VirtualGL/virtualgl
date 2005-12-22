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

// This implements a JPEG compressor/decompressor using the Intel Performance Primitives

#ifdef _WIN32
 #include <windows.h>
 #if defined(_X86_) || defined(_AMD64_)
 #define USECPUID
 #endif
#else
 #if defined(__i386__) || defined(__x86_64__)
 #define USECPUID
 #endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ippcore.h"
#include "ippi.h"
#include "ippj.h"
#include "ipps.h"
#include "turbojpeg.h"

#define CACHE_LINE 32

static const char *lasterror="No error";
static const int _mcuw[NUMSUBOPT]={8, 16, 16};
static const int _mcuh[NUMSUBOPT]={8, 8, 16};
static int ippstaticinitcalled=0;

#define checkhandle(h) jpgstruct *jpg=(jpgstruct *)h; \
	if(!jpg) {lasterror="Invalid handle";  return -1;}

#if IPP_VERSION_MAJOR>=5
#define ippCoreGetStatusString ippGetStatusString
#define ippCoreGetCpuType ippGetCpuType
#define ippStaticInitBest ippStaticInit
#endif

#define _throw(c) {lasterror=c;  return -1;}
#define _ipp(a) {IppStatus __err;  if((__err=(a))<ippStsNoErr) _throw(ippCoreGetStatusString(__err));}
#define _ippn(a) {IppStatus __err;  if((__err=(a))<ippStsNoErr) {lasterror=ippCoreGetStatusString(__err); return NULL;}}
#define _catch(a) {if((a)==-1) return -1;}

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

typedef struct _jpgstruct
{
	Ipp8u *bmpbuf, *bmpptr, *jpgbuf, *jpgptr;
	int xmcus, ymcus, width, height, pitch, ps, subsamp, qual, flags;
	unsigned long bytesprocessed, bytesleft;
	IppiEncodeHuffmanState *e_huffstate;
	IppiDecodeHuffmanState *d_huffstate;
	IppiEncodeHuffmanSpec *e_dclumtable, *e_aclumtable, *e_dcchromtable, *e_acchromtable;
	IppiDecodeHuffmanSpec *d_dclumtable, *d_aclumtable, *d_dcchromtable, *d_acchromtable;
	Ipp16u __chromqtable[64+7], __lumqtable[64+7], *chromqtable, *lumqtable;
	Ipp8u __mcubuf[384*2+CACHE_LINE-1];  Ipp16s *mcubuf;
	int initc, initd;
} jpgstruct;

// If IPP CPU type is unknown (i.e. non-Intel), use CPUID instruction to figure
// out what type of vector code is supported
#define MMXMASK (1<<23)
#define SSEMASK (1<<25)
#define SSE2MASK (1<<26)
#define SSE3MASK 1

static __inline void _cpuid(int *flags, int *flags2)
{
	int a=0, c=0, d=0;
	#ifdef USECPUID

	__asm__ __volatile__ (
		#ifdef __i386__
		"pushl %%ebx\n"
		#endif
		"cpuid\n"
		#ifdef __i386__
		"popl %%ebx\n"
		#endif
			: "=a" (a) : "a" (0)
		#ifndef __i386__
			: "bx"
		#endif
	);
	if(a<1) return;
	__asm__ __volatile__ (
		#ifdef __i386__
		"pushl %%ebx\n"
		#endif
		"cpuid\n"
		#ifdef __i386__
		"popl %%ebx\n"
		#endif
			: "=c" (c), "=d" (d) : "a" (1)
		#ifndef __i386__
			: "bx"
		#endif
	);
	*flags=d;  *flags2=c;

	#endif
}

static __inline IppCpuType AutoDetect(void)
{	
	int flags=0, flags2=0;  IppCpuType cpu=ippCpuUnknown;
	_cpuid(&flags, &flags2);
	if(flags2&SSE3MASK) cpu=ippCpuEM64T;
	else if(flags&SSE2MASK) cpu=ippCpuP4;
	else if(flags&SSEMASK) cpu=ippCpuPIII;
	else if(flags&MMXMASK) cpu=ippCpuPII;
	return cpu;
}


//////////////////////////////////////////////////////////////////////////////
//    COMPRESSOR
//////////////////////////////////////////////////////////////////////////////

// Default quantization tables per JPEG spec
static const Ipp8u lumqtable[64]=
{
	16,  11,  12,  14,  12,  10,  16,  14,
	13,  14,  18,  17,  16,  19,  24,  40,
	26,  24,  22,  22,  24,  49,  35,  37,
	29,  40,  58,  51,  61,  60,  57,  51,
	56,  55,  64,  72,  92,  78,  64,  68,
	87,  69,  55,  56,  80,  109, 81,  87,
	95,  98,  103, 104, 103, 62,  77,  113,
	121, 112, 100, 120, 92,  101, 103, 99
};

static const Ipp8u chromqtable[64]=
{
	17, 18, 18, 24, 21, 24, 47, 26,
	26, 47, 99, 66, 56, 66, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};

// Huffman tables per JPEG spec
static const Ipp8u dclumbits[16]=
{
	0, 1, 5, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 0
};
static const Ipp8u dclumvals[]=
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

static const Ipp8u dcchrombits[16]=
{
	0, 3, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 0, 0, 0, 0, 0
};
static const Ipp8u dcchromvals[]=
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

static const Ipp8u aclumbits[16]=
{
	0, 2, 1, 3, 3, 2, 4, 3,
	5, 5, 4, 4, 0, 0, 1, 0x7d
};
static const Ipp8u aclumvals[]=
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

static const Ipp8u acchrombits[16]=
{
	0, 2, 1, 2, 4, 4, 3, 4,
	7, 5, 4, 4, 0, 1, 2, 0x77
};
static const Ipp8u acchromvals[]=
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

static int e_mcu_color_convert(jpgstruct *jpg, int curxmcu, int curymcu)
{
	Ipp8u __tmpbuf[16*16*3+CACHE_LINE-1];
	Ipp8u *tmpbuf, *srcptr, *dstptr, *bmpptr=jpg->bmpptr;
	Ipp16s *mcuptr[3];
	IppStatus (__STDCALL *ccfct)(const Ipp8u *, int, Ipp16s *[3]);
	int mcuw=_mcuw[jpg->subsamp], mcuh=_mcuh[jpg->subsamp], w, h, i, j;
	IppiSize imgsize;

	tmpbuf=(Ipp8u *)ippAlignPtr(__tmpbuf, CACHE_LINE);

	switch(jpg->subsamp)
	{
		case TJ_411:
			if(jpg->flags&TJ_BGR) ccfct=ippiBGRToYCbCr411LS_MCU_8u16s_C3P3R;
			else ccfct=ippiRGBToYCbCr411LS_MCU_8u16s_C3P3R;
			break;
		case TJ_422:
			if(jpg->flags&TJ_BGR) ccfct=ippiBGRToYCbCr422LS_MCU_8u16s_C3P3R;
			else ccfct=ippiRGBToYCbCr422LS_MCU_8u16s_C3P3R;
			break;
		case TJ_444:
			if(jpg->flags&TJ_BGR) ccfct=ippiBGRToYCbCr444LS_MCU_8u16s_C3P3R;
			else ccfct=ippiRGBToYCbCr444LS_MCU_8u16s_C3P3R;
			break;
		default:
			_throw("Invalid argument to mcu_color_convert()");
	}

	w=mcuw;  h=mcuh;
	if(curxmcu==jpg->xmcus-1 && jpg->width%mcuw!=0) w=jpg->width%mcuw;
	if(curymcu==jpg->ymcus-1 && jpg->height%mcuh!=0) h=jpg->height%mcuh;

	imgsize.width=w;  imgsize.height=h;

	if(jpg->flags&TJ_BOTTOMUP)
	{
		bmpptr=bmpptr-(h-1)*jpg->pitch;
		if(jpg->ps==4)
		{
			_ipp(ippiCopy_8u_AC4C3R(bmpptr, jpg->pitch, tmpbuf, mcuw*3, imgsize));
			_ipp(ippiMirror_8u_C3IR(tmpbuf, mcuw*3, imgsize, ippAxsHorizontal));
		}
		else
		{
			_ipp(ippiMirror_8u_C3R(bmpptr, jpg->pitch, tmpbuf, mcuw*3, imgsize, ippAxsHorizontal));
		}
		if(mcuw>w)
		{
			for(j=0; j<h; j++)
			{
				srcptr=&tmpbuf[j*mcuw*3];
				for(i=w; i<mcuw; i++)
					_ipp(ippsCopy_8u(&srcptr[(w-1)*3], &srcptr[i*3], 3));
			}
		}
	}
	else
	{
		if(jpg->ps==4)
		{
			_ipp(ippiCopy_8u_AC4C3R(bmpptr, jpg->pitch, tmpbuf, mcuw*3, imgsize));
		}
		else
		{
			_ipp(ippiCopy_8u_C3R(bmpptr, jpg->pitch, tmpbuf, mcuw*3, imgsize));
		}
		if(mcuw>w)
		{
			for(j=0; j<h; j++)
			{
				srcptr=bmpptr+j*jpg->pitch+(w-1)*jpg->ps;
				dstptr=&tmpbuf[j*mcuw*3];
				for(i=w; i<mcuw; i++)
					_ipp(ippsCopy_8u(srcptr, &dstptr[i*3], 3));
			}
		}
	}
	if(mcuh>h)
	{
		srcptr=&tmpbuf[(h-1)*mcuw*3];
		for(j=h; j<mcuh; j++)
		{
			dstptr=&tmpbuf[j*mcuw*3];
			_ipp(ippsCopy_8u(srcptr, dstptr, mcuw*3));
		}
	}

	mcuptr[0]=jpg->mcubuf;
	mcuptr[1]=&jpg->mcubuf[mcuw*mcuh];
	mcuptr[2]=&jpg->mcubuf[mcuw*mcuh+8*8];

	_ipp(ccfct(tmpbuf, mcuw*3, mcuptr));

	if(curxmcu==jpg->xmcus-1)
	{
		if(jpg->flags&TJ_BOTTOMUP) jpg->bmpptr=&jpg->bmpbuf[(jpg->height-1-mcuh*(curymcu+1))*jpg->pitch];
		else jpg->bmpptr=&jpg->bmpbuf[mcuh*(curymcu+1)*jpg->pitch];
	}
	else jpg->bmpptr+=mcuw*jpg->ps;

	return 0;
}

#define write_byte(j, b) {if(j->bytesleft<=0) _throw("Not enough space in buffer");  \
	*j->jpgptr=b;  j->jpgptr++;  j->bytesprocessed++;  j->bytesleft--;}
#define write_word(j, w) {write_byte(j, (w>>8)&0xff);  write_byte(j, w&0xff);}

static int encode_jpeg_init(jpgstruct *jpg)
{
	Ipp8u rawqtable[64];  int i, nval, len;
	int mcuw=_mcuw[jpg->subsamp], mcuh=_mcuh[jpg->subsamp];

	jpg->xmcus=(jpg->width+mcuw-1)/mcuw;
	jpg->ymcus=(jpg->height+mcuh-1)/mcuh;

	jpg->bytesprocessed=0;
	if(jpg->flags&TJ_BOTTOMUP) jpg->bmpptr=&jpg->bmpbuf[jpg->pitch*(jpg->height-1)];
	else jpg->bmpptr=jpg->bmpbuf;
	jpg->jpgptr=jpg->jpgbuf;

	_ipp(ippiEncodeHuffmanStateInit_JPEG_8u(jpg->e_huffstate));

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
	memcpy(rawqtable, lumqtable, 64);
	_ipp(ippiQuantFwdRawTableInit_JPEG_8u(rawqtable, jpg->qual));
	_ipp(ippiQuantFwdTableInit_JPEG_8u16u(rawqtable, jpg->lumqtable));

	write_byte(jpg, 0xff);  write_byte(jpg, 0xdb);  // DQT marker
	write_word(jpg, 67);  // DQT length
	write_byte(jpg, 0);  // Index for luminance
	for(i=0; i<64; i++) write_byte(jpg, rawqtable[i]);

	memcpy(rawqtable, chromqtable, 64);
	_ipp(ippiQuantFwdRawTableInit_JPEG_8u(rawqtable, jpg->qual));
	_ipp(ippiQuantFwdTableInit_JPEG_8u16u(rawqtable, jpg->chromqtable));

	write_byte(jpg, 0xff);  write_byte(jpg, 0xdb);  // DQT marker
	write_word(jpg, 67);  // DQT length
	write_byte(jpg, 1);  // Index for chrominance
	for(i=0; i<64; i++) write_byte(jpg, rawqtable[i]);

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
	_ipp(ippiEncodeHuffmanSpecInit_JPEG_8u(dclumbits, dclumvals, jpg->e_dclumtable));
	_ipp(ippiEncodeHuffmanSpecInit_JPEG_8u(aclumbits, aclumvals, jpg->e_aclumtable));
	_ipp(ippiEncodeHuffmanSpecInit_JPEG_8u(dcchrombits, dcchromvals, jpg->e_dcchromtable));
	_ipp(ippiEncodeHuffmanSpecInit_JPEG_8u(acchrombits, acchromvals, jpg->e_acchromtable));

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

static int encode_jpeg(jpgstruct *jpg)
{
	int i, j, k, pos;
	int mcuw=_mcuw[jpg->subsamp], mcuh=_mcuh[jpg->subsamp];
	int mcusize=mcuw*mcuh+128;
	Ipp16s lastdc[3]={0, 0, 0};

	_catch(encode_jpeg_init(jpg));

	for(j=0; j<jpg->ymcus; j++)
	{
		for(i=0; i<jpg->xmcus; i++)
		{
			_catch(e_mcu_color_convert(jpg, i, j));

			for(k=0; k<mcusize; k+=64)  // Perform DCT on all 8x8 blocks
				_ipp(ippiDCT8x8Fwd_16s_C1I(&jpg->mcubuf[k]));

			for(k=0; k<mcuw*mcuh; k+=64)  // Quantize the luminance blocks
				_ipp(ippiQuantFwd8x8_JPEG_16s_C1I(&jpg->mcubuf[k], jpg->lumqtable));
			for(k=mcuw*mcuh; k<mcusize; k+=64)  // Quantize the chrominance blocks
				_ipp(ippiQuantFwd8x8_JPEG_16s_C1I(&jpg->mcubuf[k], jpg->chromqtable));

			for(k=0; k<mcuw*mcuh; k+=64)  // Huffman encode the luminance blocks
			{
				pos=0;
				_ipp(ippiEncodeHuffman8x8_JPEG_16s1u_C1(&jpg->mcubuf[k], jpg->jpgptr,
					jpg->bytesleft, &pos, &lastdc[0], jpg->e_dclumtable, jpg->e_aclumtable,
					jpg->e_huffstate, 0));
				jpg->jpgptr+=pos;  jpg->bytesprocessed+=pos;  jpg->bytesleft-=pos;
			}

			// Huffman encode the Cb block
			pos=0;
			_ipp(ippiEncodeHuffman8x8_JPEG_16s1u_C1(&jpg->mcubuf[k], jpg->jpgptr,
				jpg->bytesleft, &pos, &lastdc[1], jpg->e_dcchromtable, jpg->e_acchromtable,
				jpg->e_huffstate, 0));
			jpg->jpgptr+=pos;  jpg->bytesprocessed+=pos;  jpg->bytesleft-=pos;
			k+=64;

			// Huffman encode the Cr block
			pos=0;
			_ipp(ippiEncodeHuffman8x8_JPEG_16s1u_C1(&jpg->mcubuf[k], jpg->jpgptr,
				jpg->bytesleft, &pos, &lastdc[2], jpg->e_dcchromtable, jpg->e_acchromtable,
				jpg->e_huffstate, 0));
			jpg->jpgptr+=pos;  jpg->bytesprocessed+=pos;  jpg->bytesleft-=pos;

		} // xmcus
	} // ymcus

	// Flush Huffman state
	pos=0;
	_ipp(ippiEncodeHuffman8x8_JPEG_16s1u_C1(0, jpg->jpgptr, jpg->bytesleft, &pos,
		0, jpg->e_dclumtable, jpg->e_aclumtable, jpg->e_huffstate, 1));
	jpg->jpgptr+=pos;  jpg->bytesprocessed+=pos;  jpg->bytesleft-=pos;

	write_byte(jpg, 0xff);  write_byte(jpg, 0xd9);  // EOI marker

	return 0;
}

DLLEXPORT tjhandle DLLCALL tjInitCompress(void)
{
	jpgstruct *jpg;  int huffbufsize;

	if((jpg=(jpgstruct *)malloc(sizeof(jpgstruct)))==NULL)
		{lasterror="Memory allocation failure";  return NULL;}
	memset(jpg, 0, sizeof(jpgstruct));

	if(!ippstaticinitcalled)
	{
		IppCpuType cpu=ippCoreGetCpuType();
		if(cpu==ippCpuUnknown) {cpu=AutoDetect();  ippStaticInitCpu(cpu);}
		else ippStaticInitBest();  ippstaticinitcalled=1;
	}

	_ippn(ippiEncodeHuffmanStateGetBufSize_JPEG_8u(&huffbufsize));
	if((jpg->e_huffstate=(IppiEncodeHuffmanState *)ippMalloc(huffbufsize))==NULL)
		{lasterror="Memory Allocation failure";  return NULL;}

	jpg->lumqtable=(Ipp16u *)ippAlignPtr(jpg->__lumqtable, 8);
	jpg->chromqtable=(Ipp16u *)ippAlignPtr(jpg->__chromqtable, 8);
	jpg->mcubuf=(Ipp16s *)ippAlignPtr(jpg->__mcubuf, CACHE_LINE);

	_ippn(ippiEncodeHuffmanSpecGetBufSize_JPEG_8u(&huffbufsize));
	if((jpg->e_dclumtable=(IppiEncodeHuffmanSpec *)ippMalloc(huffbufsize))==NULL
	|| (jpg->e_aclumtable=(IppiEncodeHuffmanSpec *)ippMalloc(huffbufsize))==NULL
	|| (jpg->e_dcchromtable=(IppiEncodeHuffmanSpec *)ippMalloc(huffbufsize))==NULL
	|| (jpg->e_acchromtable=(IppiEncodeHuffmanSpec *)ippMalloc(huffbufsize))==NULL)
		{lasterror="Memory Allocation failure";  return NULL;}

	jpg->initc=1;

	return (tjhandle)jpg;
}

DLLEXPORT unsigned long DLLCALL TJBUFSIZE(int width, int height)
{
	// This allows enough room in case the image doesn't compress
	return ((width+15)&(~15)) * ((height+15)&(~15)) * 3 + 2048;
}

DLLEXPORT int DLLCALL tjCompress(tjhandle h,
	unsigned char *srcbuf, int width, int pitch, int height, int ps,
	unsigned char *dstbuf, unsigned long *size,
	int jpegsub, int qual, int flags)
{
	checkhandle(h);

	if(srcbuf==NULL || width<=0 || height<=0
		|| dstbuf==NULL || size==NULL
		|| jpegsub<0 || jpegsub>=NUMSUBOPT || qual<0 || qual>100)
		_throw("Invalid argument in tjCompress()");
	if(qual<1) qual=1;
	if(!jpg->initc) _throw("Instance has not been initialized for compression");

	if(ps!=3 && ps!=4) _throw("This JPEG codec supports only 24-bit or 32-bit true color");

	if(pitch==0) pitch=width*ps;

	if(flags&TJ_FORCEMMX) ippStaticInitCpu(ippCpuPII);
	if(flags&TJ_FORCESSE) ippStaticInitCpu(ippCpuPIII);
	if(flags&TJ_FORCESSE2) ippStaticInitCpu(ippCpuP4);

	if(flags&TJ_ALPHAFIRST) jpg->bmpbuf=&srcbuf[1];
	else jpg->bmpbuf=srcbuf;

	jpg->jpgbuf=dstbuf;
	jpg->width=width;  jpg->height=height;  jpg->pitch=pitch;
	jpg->ps=ps;  jpg->subsamp=jpegsub;  jpg->qual=qual;  jpg->flags=flags;
	jpg->bytesleft=TJBUFSIZE(width, height);

	_catch(encode_jpeg(jpg));

	*size=jpg->bytesprocessed;
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//   DECOMPRESSOR
//////////////////////////////////////////////////////////////////////////////

#define read_byte(j, b) {if(j->bytesleft<=0) _throw("Unexpected end of image");  \
	b=*j->jpgptr;  j->jpgptr++;  j->bytesleft--;  j->bytesprocessed++;}
#define read_word(j, w) {Ipp8u __b;  read_byte(j, __b);  w=(__b<<8);  \
	read_byte(j, __b);  w|=(__b&0xff);}

static int find_marker(jpgstruct *jpg, unsigned char *marker)
{
	unsigned char b;
	while(1)
	{
		do {read_byte(jpg, b);} while(b!=0xff);
		read_byte(jpg, b);
		if(b!=0 && b!=0xff) {*marker=b;  return 0;}
		else {jpg->jpgptr--;  jpg->bytesleft++;  jpg->bytesprocessed--;}
	}
	return 0;
}

#define check_byte(j, b) {unsigned char __b;  read_byte(j, __b);  \
	if(__b!=(b)) _throw("JPEG bitstream error");}

static int d_mcu_color_convert(jpgstruct *jpg, int curxmcu, int curymcu)
{
	Ipp8u __tmpbuf[16*16*3+CACHE_LINE-1];
	Ipp8u *tmpbuf, *bmpptr=jpg->bmpptr;
	const Ipp16s *mcuptr[3];
	IppStatus (__STDCALL *ccfct)(const Ipp16s *[3], Ipp8u *, int);
	int mcuw=_mcuw[jpg->subsamp], mcuh=_mcuh[jpg->subsamp], w, h;
	IppiSize imgsize;

	tmpbuf=(Ipp8u *)ippAlignPtr(__tmpbuf, CACHE_LINE);

	switch(jpg->subsamp)
	{
		case TJ_411:
			if(jpg->flags&TJ_BGR) ccfct=ippiYCbCr411ToBGRLS_MCU_16s8u_P3C3R;
			else ccfct=ippiYCbCr411ToRGBLS_MCU_16s8u_P3C3R;
			break;
		case TJ_422:
			if(jpg->flags&TJ_BGR) ccfct=ippiYCbCr422ToBGRLS_MCU_16s8u_P3C3R;
			else ccfct=ippiYCbCr422ToRGBLS_MCU_16s8u_P3C3R;
			break;
		case TJ_444:
			if(jpg->flags&TJ_BGR) ccfct=ippiYCbCr444ToBGRLS_MCU_16s8u_P3C3R;
			else ccfct=ippiYCbCr444ToRGBLS_MCU_16s8u_P3C3R;
			break;
		default:
			_throw("Invalid argument to mcu_color_convert()");
	}

	w=mcuw;  h=mcuh;
	if(curxmcu==jpg->xmcus-1 && jpg->width%mcuw!=0) w=jpg->width%mcuw;
	if(curymcu==jpg->ymcus-1 && jpg->height%mcuh!=0) h=jpg->height%mcuh;

	imgsize.width=w;  imgsize.height=h;

	mcuptr[0]=jpg->mcubuf;
	mcuptr[1]=&jpg->mcubuf[mcuw*mcuh];
	mcuptr[2]=&jpg->mcubuf[mcuw*mcuh+8*8];

	_ipp(ccfct(mcuptr, tmpbuf, mcuw*3));

	if(jpg->flags&TJ_BOTTOMUP)
	{
		bmpptr=bmpptr-(h-1)*jpg->pitch;
		if(jpg->ps==4)
		{
			_ipp(ippiMirror_8u_C3IR(tmpbuf, mcuw*3, imgsize, ippAxsHorizontal));
			_ipp(ippiCopy_8u_C3AC4R(tmpbuf, mcuw*3, bmpptr, jpg->pitch, imgsize));
		}
		else
		{
			_ipp(ippiMirror_8u_C3R(tmpbuf, mcuw*3, bmpptr, jpg->pitch, imgsize, ippAxsHorizontal));
		}
	}
	else
	{
		if(jpg->ps==4)
		{
			_ipp(ippiCopy_8u_C3AC4R(tmpbuf, mcuw*3, bmpptr, jpg->pitch, imgsize));
		}
		else
		{
			_ipp(ippiCopy_8u_C3R(tmpbuf, mcuw*3, bmpptr, jpg->pitch, imgsize));
		}
	}

	if(curxmcu==jpg->xmcus-1)
	{
		if(jpg->flags&TJ_BOTTOMUP) jpg->bmpptr=&jpg->bmpbuf[(jpg->height-1-mcuh*(curymcu+1))*jpg->pitch];
		else jpg->bmpptr=&jpg->bmpbuf[mcuh*(curymcu+1)*jpg->pitch];
	}
	else jpg->bmpptr+=mcuw*jpg->ps;

	return 0;
}

static int decode_jpeg_init(jpgstruct *jpg)
{
	Ipp8u rawqtable[64], rawhuffbits[16], rawhuffvalues[256];
	int i;  unsigned char tempbyte, marker;  unsigned short tempword, length;
	int markerread=0;  unsigned char compid[3];

	jpg->bytesprocessed=0;
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
					for(i=0; i<64; i++) read_byte(jpg, rawqtable[i]);
					dqtbytecount+=64;
					if(tempbyte==0)
					{
						_ipp(ippiQuantInvTableInit_JPEG_8u16u(rawqtable, jpg->lumqtable));
						markerread+=2;
					}
					else if(tempbyte==1)
					{
						_ipp(ippiQuantInvTableInit_JPEG_8u16u(rawqtable, jpg->chromqtable));
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
					_ipp(ippsZero_8u(rawhuffbits, 16));
					_ipp(ippsZero_8u(rawhuffvalues, 256));
					nval=0;
					for(i=0; i<16; i++) {read_byte(jpg, rawhuffbits[i]);  nval+=rawhuffbits[i];}
					dhtbytecount+=16;
					if(nval>256) _throw("JPEG bitstream error");
					for(i=0; i<nval; i++) read_byte(jpg, rawhuffvalues[i]);
					dhtbytecount+=nval;
					if(tempbyte==0x00)  // DC luminance
					{
						_ipp(ippiDecodeHuffmanSpecInit_JPEG_8u(rawhuffbits, rawhuffvalues,
							jpg->d_dclumtable));
						markerread+=8;
					}
					else if(tempbyte==0x10)  // AC luminance
					{
						_ipp(ippiDecodeHuffmanSpecInit_JPEG_8u(rawhuffbits, rawhuffvalues,
							jpg->d_aclumtable));
						markerread+=16;
					}
					else if(tempbyte==0x01)  // DC chrominance
					{
						_ipp(ippiDecodeHuffmanSpecInit_JPEG_8u(rawhuffbits, rawhuffvalues,
							jpg->d_dcchromtable));
						markerread+=32;
					}
					else if(tempbyte==0x11)  // AC chrominance
					{
						_ipp(ippiDecodeHuffmanSpecInit_JPEG_8u(rawhuffbits, rawhuffvalues,
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
				if(tempbyte==0x11) jpg->subsamp=TJ_444;
				else if(tempbyte==0x21) jpg->subsamp=TJ_422;
				else if(tempbyte==0x22) jpg->subsamp=TJ_411;
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

static int decode_jpeg(jpgstruct *jpg)
{
	int i, j, k, pos, mcuw, mcuh, mcusize, marker;
	Ipp16s lastdc[3]={0, 0, 0};

	if(jpg->flags&TJ_BOTTOMUP) jpg->bmpptr=&jpg->bmpbuf[jpg->pitch*(jpg->height-1)];
	else jpg->bmpptr=jpg->bmpbuf;

	_ipp(ippiDecodeHuffmanStateInit_JPEG_8u(jpg->d_huffstate));

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
				pos=0;  marker=0;
				_ipp(ippiDecodeHuffman8x8_JPEG_1u16s_C1(jpg->jpgptr, jpg->bytesleft, &pos,
					&jpg->mcubuf[k], &lastdc[0], &marker, jpg->d_dclumtable,
					jpg->d_aclumtable, jpg->d_huffstate));
				jpg->jpgptr+=pos;  jpg->bytesprocessed+=pos;  jpg->bytesleft-=pos;
			}

			// Huffman decode the Cb block
			pos=0;  marker=0;
			_ipp(ippiDecodeHuffman8x8_JPEG_1u16s_C1(jpg->jpgptr, jpg->bytesleft, &pos,
				&jpg->mcubuf[k], &lastdc[1], &marker, jpg->d_dcchromtable,
				jpg->d_acchromtable, jpg->d_huffstate));
			jpg->jpgptr+=pos;  jpg->bytesprocessed+=pos;  jpg->bytesleft-=pos;
			k+=64;

			// Huffman decode the Cr block
			pos=0;  marker=0;
			_ipp(ippiDecodeHuffman8x8_JPEG_1u16s_C1(jpg->jpgptr, jpg->bytesleft, &pos,
				&jpg->mcubuf[k], &lastdc[2], &marker, jpg->d_dcchromtable,
				jpg->d_acchromtable, jpg->d_huffstate));
			jpg->jpgptr+=pos;  jpg->bytesprocessed+=pos;  jpg->bytesleft-=pos;

			for(k=0; k<mcuw*mcuh; k+=64)  // Un-quantize the luminance blocks
				_ipp(ippiQuantInv8x8_JPEG_16s_C1I(&jpg->mcubuf[k], jpg->lumqtable));
			for(k=mcuw*mcuh; k<mcusize; k+=64)  // Un-quantize the chrominance blocks
				_ipp(ippiQuantInv8x8_JPEG_16s_C1I(&jpg->mcubuf[k], jpg->chromqtable));

			for(k=0; k<mcusize; k+=64)  // Perform inverse DCT on all 8x8 blocks
				_ipp(ippiDCT8x8Inv_16s_C1I(&jpg->mcubuf[k]));

			_catch(d_mcu_color_convert(jpg, i, j));
		}
	}

	return 0;
}

DLLEXPORT tjhandle DLLCALL tjInitDecompress(void)
{
	jpgstruct *jpg;  int huffbufsize;

	if((jpg=(jpgstruct *)malloc(sizeof(jpgstruct)))==NULL)
		{lasterror="Memory allocation failure";  return NULL;}
	memset(jpg, 0, sizeof(jpgstruct));

	if(!ippstaticinitcalled)
	{
		IppCpuType cpu=ippCoreGetCpuType();
		if(cpu==ippCpuUnknown) {cpu=AutoDetect();  ippStaticInitCpu(cpu);}
		else ippStaticInitBest();  ippstaticinitcalled=1;
	}

	_ippn(ippiDecodeHuffmanStateGetBufSize_JPEG_8u(&huffbufsize));
	if((jpg->d_huffstate=(IppiDecodeHuffmanState *)ippMalloc(huffbufsize))==NULL)
		{lasterror="Memory Allocation failure";  return NULL;}

	jpg->lumqtable=(Ipp16u *)ippAlignPtr(jpg->__lumqtable, 8);
	jpg->chromqtable=(Ipp16u *)ippAlignPtr(jpg->__chromqtable, 8);
	jpg->mcubuf=(Ipp16s *)ippAlignPtr(jpg->__mcubuf, CACHE_LINE);

	_ippn(ippiDecodeHuffmanSpecGetBufSize_JPEG_8u(&huffbufsize));
	if((jpg->d_dclumtable=(IppiDecodeHuffmanSpec *)ippMalloc(huffbufsize))==NULL
	|| (jpg->d_aclumtable=(IppiDecodeHuffmanSpec *)ippMalloc(huffbufsize))==NULL
	|| (jpg->d_dcchromtable=(IppiDecodeHuffmanSpec *)ippMalloc(huffbufsize))==NULL
	|| (jpg->d_acchromtable=(IppiDecodeHuffmanSpec *)ippMalloc(huffbufsize))==NULL)
		{lasterror="Memory Allocation failure";  return NULL;}

	jpg->initd=1;
	return (tjhandle)jpg;
}

DLLEXPORT int DLLCALL tjDecompressHeader(tjhandle h,
	unsigned char *srcbuf, unsigned long size,
	int *width, int *height)
{
	checkhandle(h);

	if(srcbuf==NULL || size<=0 || width==NULL || height==NULL)
		_throw("Invalid argument in tjDecompressHeader()");
	if(!jpg->initd) _throw("Instance has not been initialized for decompression");

	jpg->jpgbuf=srcbuf;
	jpg->bytesleft=size;

	jpg->width=jpg->height=0;

	*width=*height=0;
	_catch(decode_jpeg_init(jpg));
	*width=jpg->width;  *height=jpg->height;

	if(*width<1 || *height<1) _throw("Invalid data returned in header");
	return 0;
}

DLLEXPORT int DLLCALL tjDecompress(tjhandle h,
	unsigned char *srcbuf, unsigned long size,
	unsigned char *dstbuf, int width, int pitch, int height, int ps,
	int flags)
{
	checkhandle(h);

	if(srcbuf==NULL || size<=0
		|| dstbuf==NULL || width<=0 || height<=0)
		_throw("Invalid argument in tjDecompress()");
	if(!jpg->initd) _throw("Instance has not been initialized for decompression");

	if(ps!=3 && ps!=4) _throw("This JPEG codec supports only 24-bit or 32-bit true color");

	if(pitch==0) pitch=width*ps;

	if(flags&TJ_FORCEMMX) ippStaticInitCpu(ippCpuPII);
	if(flags&TJ_FORCESSE) ippStaticInitCpu(ippCpuPIII);
	if(flags&TJ_FORCESSE2) ippStaticInitCpu(ippCpuP4);

	if(flags&TJ_ALPHAFIRST) jpg->bmpbuf=&dstbuf[1];
	else jpg->bmpbuf=dstbuf;

	jpg->jpgbuf=srcbuf;
	jpg->width=width;  jpg->height=height;  jpg->pitch=pitch;
	jpg->ps=ps;  jpg->flags=flags;
	jpg->bytesleft=size;

	_catch(decode_jpeg(jpg));

	return 0;
}


// General

DLLEXPORT char* DLLCALL tjGetErrorStr(void)
{
	return (char *)lasterror;
}

DLLEXPORT int DLLCALL tjDestroy(tjhandle h)
{
	checkhandle(h);
	if(jpg->initc)
	{
		if(jpg->e_dclumtable) ippFree(jpg->e_dclumtable);
		if(jpg->e_aclumtable) ippFree(jpg->e_aclumtable);
		if(jpg->e_dcchromtable) ippFree(jpg->e_dcchromtable);
		if(jpg->e_acchromtable) ippFree(jpg->e_acchromtable);
		if(jpg->e_huffstate) ippFree(jpg->e_huffstate);
	}
	else if(jpg->initd)
	{
		if(jpg->d_dclumtable) ippFree(jpg->d_dclumtable);
		if(jpg->d_aclumtable) ippFree(jpg->d_aclumtable);
		if(jpg->d_dcchromtable) ippFree(jpg->d_dcchromtable);
		if(jpg->d_acchromtable) ippFree(jpg->d_acchromtable);
		if(jpg->d_huffstate) ippFree(jpg->d_huffstate);
	}
	free(jpg);
	return 0;
}

