/* Compatibility macros for older TurboJPEG/IPP implementation */

#ifndef __TURBOJPEG_IPP_COMPAT_H__
#define __TURBOJPEG_IPP_COMPAT_H__

#ifdef USEIPP
#define TJBUFSIZEYUV(w, h, subsamp) TJBUFSIZE(w, h)
#define tjEncodeYUV(j, srcbuf, width, pitch, height, pixelsize, dstbuf, \
	subsamp, flags) -1
#define tjDecompressToYUV(j, srcbuf, size, dstbuf, flags) -1
#define tjDecompressHeader2(j, srcbuf, size, width, height, subsamp) \
	((*subsamp=TJ_444)-TJ_444 \
		+ tjDecompressHeader(j, srcbuf, size, width, height))
#endif

#endif /* __TURBOJPEG_IPP_COMPAT_H__ */
