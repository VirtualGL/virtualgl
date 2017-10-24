/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2010-2011, 2013-2014, 2017 D. R. Commander
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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "vglutil.h"
#include "Timer.h"
#include "Error.h"
#include <errno.h>
#define GL_GLEXT_PROTOTYPES
#include "../common/glx.h"
#include <GL/glu.h>
#include "x11err.h"
#ifdef USEIFR
#include "NvIFROpenGL.h"
#include "NvIFR_API.h"
#endif
#include "glpf.h"

using namespace vglutil;


#define PAD(w) (((w)+(align-1))&(~(align-1)))
#define BMPPAD(pitch) ((pitch+(sizeof(int)-1))&(~(sizeof(int)-1)))


GLenum glFormat[PIXELFORMATS];

const char *formatName[PIXELFORMATS]=
{
	"RGB", "RGBA", "RGB10_A2", "BGR", "BGRA", "BGR10_A2", "ABGR", "A2_BGR10",
	"ARGB", "A2_RGB10", "RED"
};


#define BENCH_NAME       "GLreadtest"

#define WIDTH            701
#define HEIGHT           701
#define ALIGN            1
#define BENCHTIME        1.0

int width=WIDTH, height=HEIGHT, align=ALIGN;
Display *dpy=NULL;  Window win=0;  Pixmap pm=0;  GLXPixmap glxpm=0;
XVisualInfo *v=NULL;  GLXFBConfig c=0;
GLXContext ctx=0;
GLXPbuffer pbuffer=0;

Timer timer;
bool useWindow=false, usePixmap=false, useFBO=false, useRTT=false,
	useAlpha=false;
int visualID=0, loops=1;
#ifdef USEIFR
bool useIFR=false;
NvIFRAPI ifr;
NV_IFROGL_SESSION_HANDLE ifrSession=NULL;
#endif
#ifdef GL_EXT_framebuffer_object
GLuint fbo=0, rbo=0, texture=0;
#endif
#ifdef GL_VERSION_1_5
int pbo=0;
#endif
double benchTime=BENCHTIME;

#define STRLEN 256


char *sigFig(int fig, char *string, double value)
{
	char format[80];
	double _l=(value==0.0)? 0.0:log10(fabs(value));  int l;
	if(_l<0.)
	{
		l=(int)fabs(floor(_l));
		snprintf(format, 79, "%%%d.%df", fig+l+1, fig+l-1);
	}
	else
	{
		l=(int)_l+1;
		if(fig<=l) snprintf(format, 79, "%%.0f");
		else snprintf(format, 79, "%%%d.%df", fig+1, fig-l);
	}
	snprintf(string, STRLEN-1, format, value);
	return(string);
}


extern "C" {
int xhandler(Display *dpy, XErrorEvent *xe)
{
	fprintf(stderr, "X11 Error: %s\n", x11error(xe->error_code));
	return 0;
}
} // extern "C"


void findVisual(int bpc)
{
	int winattribs[]={GLX_RGBA, GLX_RED_SIZE, bpc, GLX_GREEN_SIZE, bpc,
		GLX_BLUE_SIZE, bpc, None, None, None};
	int winattribsdb[]={GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, bpc,
		GLX_GREEN_SIZE, bpc, GLX_BLUE_SIZE, bpc, None, None, None};

	int pbattribs[]={GLX_RED_SIZE, bpc, GLX_GREEN_SIZE, bpc, GLX_BLUE_SIZE, bpc,
		GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT, None,
		None, None};
	int pbattribsdb[]={GLX_DOUBLEBUFFER, 1, GLX_RED_SIZE, bpc, GLX_GREEN_SIZE,
		bpc, GLX_BLUE_SIZE, bpc, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE,
		GLX_PBUFFER_BIT, None, None, None};
	GLXFBConfig *fbconfigs=NULL;  int nelements=0;

	// Use GLX 1.1 functions here in case we're remotely displaying to
	// something that doesn't support GLX 1.3
	if(useWindow || usePixmap || useFBO)
	{
		try
		{
			if(visualID)
			{
				XVisualInfo vtemp;  int n=0;
				vtemp.visualid=visualID;
				v=XGetVisualInfo(dpy, VisualIDMask, &vtemp, &n);
				if(!v || !n) _throw("Could not obtain visual");
				printf("Visual = 0x%.2x\n", (unsigned int)v->visualid);
				return;
			}
			if(useAlpha)
			{
				winattribs[7]=GLX_ALPHA_SIZE;  winattribs[8]=32-bpc*3;
				winattribsdb[8]=GLX_ALPHA_SIZE;  winattribsdb[9]=32-bpc*3;
			}
			if(!(v=glXChooseVisual(dpy, DefaultScreen(dpy), winattribs))
				&& !(v=glXChooseVisual(dpy, DefaultScreen(dpy), winattribsdb)))
				_throw("Could not obtain Visual");
			printf("Visual = 0x%.2x\n", (unsigned int)v->visualid);
			return;
		}
		catch(...)
		{
			if(v) { XFree(v);  v=NULL; }  throw;
		}
	}

	if(useAlpha)
	{
		pbattribs[10]=GLX_ALPHA_SIZE;  pbattribs[11]=32-bpc*3;
		pbattribsdb[12]=GLX_ALPHA_SIZE;  pbattribsdb[13]=32-bpc*3;
	}
	fbconfigs=glXChooseFBConfig(dpy, DefaultScreen(dpy), pbattribs, &nelements);
	if(!fbconfigs) fbconfigs=glXChooseFBConfig(dpy, DefaultScreen(dpy),
		pbattribsdb, &nelements);
	if(!nelements || !fbconfigs)
	{
		if(fbconfigs) XFree(fbconfigs);
		_throw("Could not obtain Visual");
	}
	c=fbconfigs[0];  XFree(fbconfigs);

	int fbcid=-1;
	glXGetFBConfigAttrib(dpy, c, GLX_FBCONFIG_ID, &fbcid);
	printf("FB Config = 0x%.2x\n", fbcid);
}


void drawableInit(void)
{
	int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0, None};

	// Use GLX 1.1 functions here in case we're remotely displaying to
	// something that doesn't support GLX 1.3
	if(useWindow || usePixmap || useFBO)
	{
		if(!(ctx=glXCreateContext(dpy, v, NULL, True)))
			_throw("Could not create GL context");

		if(usePixmap)
		{
			if(!(glxpm=glXCreateGLXPixmap(dpy, v, pm)))
				_throw("Could not creaate GLX pixmap");
		}
		glXMakeCurrent(dpy, usePixmap? glxpm:win, ctx);

		#ifdef GL_EXT_framebuffer_object
		if(useFBO)
		{
			GLenum format=useAlpha? GL_RGBA8:GL_RGB8;
			if(v->depth==30) format=useAlpha? GL_RGB10_A2:GL_RGB10;
			glGenFramebuffersEXT(1, &fbo);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
			if(useRTT)
			{
				glGenTextures(1, &texture);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER,
					GL_LINEAR);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER,
					GL_LINEAR);
				glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, format,	width, height, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
					GL_TEXTURE_RECTANGLE_ARB, texture, 0);
			}
			else
			{
				glGenRenderbuffersEXT(1, &rbo);
				glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbo);
				glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, format, width, height);
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
					GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rbo);
			}
			if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)
				!=GL_FRAMEBUFFER_COMPLETE_EXT)
				_throw("Could not create FBO");
		}
		#endif
		return;
	}

	ctx=glXCreateNewContext(dpy, c, GLX_RGBA_TYPE, NULL, True);
	if(!ctx) _throw("Could not create GL context");

	pbattribs[1]=width;  pbattribs[3]=height;
	pbuffer=glXCreatePbuffer(dpy, c, pbattribs);
	if(!pbuffer) _throw("Could not create Pbuffer");

	glXMakeContextCurrent(dpy, pbuffer, pbuffer, ctx);
}


char glerrstr[STRLEN]="No error";

static void check_errors(const char *tag)
{
	int i, error=0;  char *s;
	i=glGetError();
	if(i!=GL_NO_ERROR) error=1;
	while(i!=GL_NO_ERROR)
	{
		s=(char *)gluErrorString(i);
		if(s) snprintf(glerrstr, STRLEN-1, "OpenGL ERROR in %s: %s\n", tag, s);
		else snprintf(glerrstr, STRLEN-1, "OpenGL ERROR #%d in %s\n", i, tag);
		i=glGetError();
	}
	if(error) _throw(glerrstr);
}


void initBuf(int x, int y, int width, int height, PF &pf, unsigned char *buf)
{
	int i, j, maxRGB=(1<<pf.bpc);
	for(j=0; j<height; j++)
	{
		for(i=0; i<width; i++)
		{
			if(pf.id==PF_COMP)
				buf[j*width+i]=((j+y)*(i+x))%maxRGB;
			else
			{
				pf.setRGB(&buf[(j*width+i)*pf.size], ((j+y)*(i+x))%maxRGB,
					((j+y)*(i+x)*2)%maxRGB, ((j+y)*(i+x)*3)%maxRGB);
			}
		}
	}
}


int cmpBuf(int x, int y, int width, int height, PF &pf, unsigned char *buf,
	int bassackwards)
{
	int i, j, l, maxRGB=(1<<pf.bpc);
	for(j=0; j<height; j++)
	{
		l=bassackwards? height-j-1:j;
		for(i=0; i<width; i++)
		{
			if(pf.id==PF_COMP)
			{
				if(buf[l*PAD(width*pf.size)+i]!=((j+y)*(i+x))%maxRGB) return 0;
			}
			else
			{
				int r, g, b;
				pf.getRGB(&buf[l*PAD(width*pf.size)+i*pf.size], &r, &g, &b);
				if(r!=((j+y)*(i+x))%maxRGB || g!=((j+y)*(i+x)*2)%maxRGB
					|| b!=((j+y)*(i+x)*3)%maxRGB)
					return 0;
			}
		}
	}
	return 1;
}


// Makes sure the frame buffer has been cleared prior to a write
void clearFB(void)
{
	unsigned char *buf=NULL;

	if((buf=(unsigned char *)malloc(width*height*3))==NULL)
		_throw("Could not allocate buffer");
	memset(buf, 0xFF, width*height*3);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	#ifdef GL_EXT_framebuffer_object
	if(useFBO)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	}
	else
	#endif
	{
		glDrawBuffer(GL_FRONT);
		glReadBuffer(GL_FRONT);
	}
	glClearColor(0., 0., 0., 0.);
	glClear(GL_COLOR_BUFFER_BIT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);
	check_errors("frame buffer read");
	for(int i=0; i<width*height*3; i++)
	{
		if(buf[i]!=0)
		{
			fprintf(stderr, "Buffer was not cleared\n");  break;
		}
	}
	if(buf) free(buf);
}


// Generic GL write test
int writeTest(int format)
{
	PF pf=pf_get(format);
	unsigned char *rgbaBuffer=NULL;  int n, retval=0;
	double rbtime;
	char temps[STRLEN];

	try
	{
		fprintf(stderr, "glDrawPixels():   ");
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glShadeModel(GL_FLAT);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		clearFB();
		if((rgbaBuffer=(unsigned char *)malloc(width*height*pf.size))==NULL)
			_throw("Could not allocate buffer");
		initBuf(0, 0, width, height, pf, rgbaBuffer);
		n=0;
		timer.start();
		do
		{
			glDrawPixels(width, height, glFormat[format], pf_gldatatype[format],
				rgbaBuffer);
			glFinish();
			n++;
		} while((rbtime=timer.elapsed())<benchTime || n<2);

		double avgmps=(double)n*(double)(width*height)/((double)1000000.*rbtime);
		check_errors("frame buffer write");
		fprintf(stderr, "%s Mpixels/sec\n", sigFig(4, temps, avgmps));

	} catch(Error &e) { fprintf(stderr, "%s\n", e.getMessage());  retval=-1; }

	if(rgbaBuffer) free(rgbaBuffer);
	return retval;
}


// Generic OpenGL readback test
int readTest(int format)
{
	PF pf=pf_get(format);
	unsigned char *rgbaBuffer=NULL;  int n, retval=0;
	double rbtime, readPixelsTime;  Timer timer2;
	#ifdef GL_VERSION_1_5
	GLuint bufferID=0;
	#endif
	#ifdef USEIFR
	NV_IFROGL_TRANSFEROBJECT_HANDLE ifrTransfer=NULL;
	#endif
	char temps[STRLEN];

	try
	{
		fprintf(stderr, "glReadPixels():   ");

		glPixelStorei(GL_UNPACK_ALIGNMENT, align);
		glPixelStorei(GL_PACK_ALIGNMENT, align);

		#ifdef GL_EXT_framebuffer_object
		if(useFBO) glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		else
		#endif
		glReadBuffer(GL_FRONT);

		#ifdef GL_VERSION_1_5
		if(pbo)
		{
			const char *ext=(const char *)glGetString(GL_EXTENSIONS);
			if(!ext || !strstr(ext, "GL_ARB_pixel_buffer_object"))
				_throw("GL_ARB_pixel_buffer_object extension not available");
			glGenBuffers(1, &bufferID);
			if(!bufferID) _throw("Could not generate PBO buffer");
			glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, bufferID);
			glBufferData(GL_PIXEL_PACK_BUFFER_EXT, PAD(width*pf.size)*height, NULL,
				GL_STREAM_READ);
			check_errors("PBO initialization");
			int temp=0;
			glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE,
				&temp);
			if(temp!=PAD(width*pf.size)*height) _throw("Could not generate PBO buffer");
			temp=0;
			glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_EXT, &temp);
			if(temp!=pbo) _throw("Could not bind PBO buffer");
		}
		#endif
		#ifdef USEIFR
		if(useIFR)
		{
			NV_IFROGL_TO_SYS_CONFIG ifrConfig;
			ifrConfig.format=NV_IFROGL_TARGET_FORMAT_CUSTOM;
			ifrConfig.flags=NV_IFROGL_TRANSFER_OBJECT_FLAG_NONE;
			ifrConfig.customFormat=glFormat[format];
			ifrConfig.customType=pf_gldatatype[format];
			if(ifr.nvIFROGLCreateTransferToSysObject(ifrSession, &ifrConfig,
				&ifrTransfer)!=NV_IFROGL_SUCCESS)
				_throw("Could not create IFR transfer object\n");
		}
		#endif

		if((rgbaBuffer=(unsigned char *)malloc(PAD(width*pf.size)*height))==NULL)
			_throw("Could not allocate buffer");
		memset(rgbaBuffer, 0, PAD(width*pf.size)*height);
		n=0;  rbtime=readPixelsTime=0.;
		double tmin=0., tmax=0., ssq=0., sum=0.;  bool first=true;
		do
		{
			timer.start();
			#ifdef GL_VERSION_1_5
			if(pbo)
			{
				unsigned char *pixels=NULL;
				glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, bufferID);
				timer2.start();
				glReadPixels(0, 0, width, height, glFormat[format],
					pf_gldatatype[format], NULL);
				readPixelsTime+=timer2.elapsed();
				pixels=(unsigned char *)glMapBuffer(GL_PIXEL_PACK_BUFFER_EXT,
					GL_READ_ONLY);
				if(!pixels) _throw("Could not map buffer");
				memcpy(rgbaBuffer, pixels, PAD(width*pf.size)*height);
				glUnmapBuffer(GL_PIXEL_PACK_BUFFER_EXT);
			}
			else
			#endif
			#ifdef USEIFR
			if(useIFR)
			{
				const void *pixels=NULL;
				uintptr_t bufSize=0;
				timer2.start();
				if(ifr.nvIFROGLTransferFramebufferToSys(ifrTransfer, useFBO? fbo:0,
					useFBO? GL_COLOR_ATTACHMENT0_EXT:GL_FRONT,
					NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_NONE, 0, 0, 0, 0)
					!=NV_IFROGL_SUCCESS)
					_throw("Could not transfer pixels from the framebuffer");
				readPixelsTime+=timer2.elapsed();
				if(ifr.nvIFROGLLockTransferData(ifrTransfer, &bufSize, &pixels)
					!=NV_IFROGL_SUCCESS)
					_throw("Could not lock transferred pixels");
				if(bufSize!=(uintptr_t(PAD(width*pf.size)*height)))
					_throw("Transferred pixel buffer is the wrong size");
				memcpy(rgbaBuffer, (unsigned char *)pixels, PAD(width*pf.size)*height);
				if(ifr.nvIFROGLReleaseTransferData(ifrTransfer)!=NV_IFROGL_SUCCESS)
					_throw("Could not release transferred pixels");
			}
			else
			#endif
			{
				timer2.start();
				glReadPixels(0, 0, width, height, glFormat[format],
					pf_gldatatype[format], rgbaBuffer);
				readPixelsTime+=timer2.elapsed();
			}

			double elapsed=timer.elapsed();
			if(first)
			{
				tmin=tmax=elapsed;  first=false;
			}
			else
			{
				if(elapsed<tmin) tmin=elapsed;
				if(elapsed>tmax) tmax=elapsed;
			}
			n++;
			rbtime+=elapsed;
			ssq+=pow((double)(width*height)/((double)1000000.*elapsed), 2.0);
			sum+=(double)(width*height)/((double)1000000.*elapsed);
		} while(rbtime<benchTime || n<2);

		if(!cmpBuf(0, 0, width, height, pf, rgbaBuffer, 0))
			_throw("ERROR: Bogus data read back.");

		double mean=sum/(double)n;
		double stddev=sqrt((ssq - 2.0*mean*sum + mean*mean*(double)n)/(double)n);
		double avgmps=(double)n*(double)(width*height)/((double)1000000.*rbtime);
		double minmps=(double)(width*height)/((double)1000000.*tmax);
		double maxmps=(double)(width*height)/((double)1000000.*tmin);
		check_errors("frame buffer read");

		fprintf(stderr, "%s Mpixels/sec ", sigFig(4, temps, avgmps));
		fprintf(stderr, "(min = %s, ", sigFig(4, temps, minmps));
		fprintf(stderr, "max = %s, ", sigFig(4, temps, maxmps));
		fprintf(stderr, "sdev = %s)\n", sigFig(4, temps, stddev));
		fprintf(stderr, "glReadPixels() accounted for %s%% of total readback time\n",
			sigFig(4, temps, readPixelsTime/rbtime*100.0));

	} catch(Error &e) { fprintf(stderr, "%s\n", e.getMessage());  retval=-1; }

	if(rgbaBuffer) free(rgbaBuffer);
	#ifdef GL_VERSION_1_5
	if(pbo && bufferID>0)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
		glDeleteBuffers(1, &bufferID);
	}
	#endif
	#ifdef USEIFR
	if(useIFR) ifr.nvIFROGLDestroyTransferObject(ifrTransfer);
	#endif
	return retval;
}


void display(void)
{
	int format, status=0;

	for(format=0; format<PIXELFORMATS; format++)
	{
		if(glFormat[format]==GL_NONE) continue;

		fprintf(stderr, ">>>>>>>>>>  PIXEL FORMAT:  %s  <<<<<<<<<<\n",
			formatName[format]);

		#ifdef GL_ABGR_EXT
		const char *ext=(const char *)glGetString(GL_EXTENSIONS), *compext=NULL;
		if(glFormat[format]==GL_ABGR_EXT) compext="GL_EXT_abgr";
		if(compext && (!ext || !strstr(ext, compext)))
		{
			fprintf(stderr, "%s extension not available.  Skipping ...\n\n",
				compext);
			continue;
		}
		#endif

		if(writeTest(format)<0) status=-1;
		for(int i=0; i<loops; i++) if(readTest(format)<0) status=-1;
		fprintf(stderr, "\n");
	}

	exit(status);
}

void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-window = Render to a window instead of a Pbuffer\n");
	fprintf(stderr, "-pm = Render to a pixmap instead of a Pbuffer\n");
	#ifdef GL_EXT_framebuffer_object
	fprintf(stderr, "-fbo = Render to a framebuffer object (FBO) instead of a Pbuffer\n");
	fprintf(stderr, "-rtt = Render to a texture instead of a renderbuffer object\n");
	fprintf(stderr, "       (implies -fbo)\n");
	#endif
	#ifdef GL_VERSION_1_5
	fprintf(stderr, "-pbo = Use pixel buffer objects to perform readback\n");
	#endif
	#ifdef USEIFR
	fprintf(stderr, "-ifr = Use nVidia Inband Frame Readback\n");
	#endif
	fprintf(stderr, "-width <w> = Set drawable width to <w> pixels (default: %d)\n",
		WIDTH);
	fprintf(stderr, "-height <h> = Set drawable height to <h> pixels (default: %d)\n",
		HEIGHT);
	fprintf(stderr, "-align <n> = Set row alignment to <n> bytes [<n> is a power of 2] (default: %d)\n",
		ALIGN);
	fprintf(stderr, "-visualid <xx> = Ignore visual selection and use this visual ID (hex) instead\n");
	fprintf(stderr, "                 (this has no effect when rendering to a Pbuffer)\n");
	fprintf(stderr, "-alpha = Create Pbuffer/window using a visual with an alpha channel\n");
	fprintf(stderr, "-rgb = Test only RGB pixel format\n");
	fprintf(stderr, "-rgba = Test only RGBA pixel format\n");
	fprintf(stderr, "-rgb10a2 = Test only RGB10_A2 pixel format\n");
	fprintf(stderr, "-bgr = Test only BGR pixel format\n");
	fprintf(stderr, "-bgra = Test only BGRA pixel format\n");
	fprintf(stderr, "-bgr10a2 = Test only BGR10_A2 pixel format\n");
	#ifdef GL_ABGR_EXT
	fprintf(stderr, "-abgr = Test only ABGR pixel format\n");
	#endif
	fprintf(stderr, "-a2bgr10 = Test only A2_BGR10 pixel format\n");
	fprintf(stderr, "-argb = Test only ARGB pixel format\n");
	fprintf(stderr, "-a2rgb10 = Test only A2_RGB10 pixel format\n");
	fprintf(stderr, "-time <t> = Run each test for <t> seconds (default: %.1f)\n",
		BENCHTIME);
	fprintf(stderr, "-loop <l> = Run readback test <l> times in a row\n");
	fprintf(stderr, "\n");
	exit(1);
}


int main(int argc, char **argv)
{
	fprintf(stderr, "\n%s v%s (Build %s)\n", BENCH_NAME, __VERSION, __BUILD);

	memcpy(&glFormat, pf_glformat, sizeof(pf_glformat));

	if(argc>1) for(int i=1; i<argc; i++)
	{
		if(!stricmp(argv[i], "-h") || !stricmp(argv[i], "-?")) usage(argv);
		else if(!stricmp(argv[i], "-window")) useWindow=true;
		else if(!stricmp(argv[i], "-pm")) usePixmap=true;
		#ifdef GL_EXT_framebuffer_object
		else if(!stricmp(argv[i], "-fbo")) useFBO=true;
		else if(!stricmp(argv[i], "-rtt")) { useRTT=true;  useFBO=true; }
		#endif
		#ifdef GL_VERSION_1_5
		else if(!stricmp(argv[i], "-pbo")) pbo=1;
		#endif
		#ifdef USEIFR
		else if(!stricmp(argv[i], "-ifr")) useIFR=true;
		#endif
		else if(!stricmp(argv[i], "-alpha")) useAlpha=true;
		else if(!stricmp(argv[i], "-rgb"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_RGB) glFormat[i]=GL_NONE;
		}
		else if(!stricmp(argv[i], "-rgba"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_RGBX) glFormat[i]=GL_NONE;
		}
		else if(!stricmp(argv[i], "-rgb10a2"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_RGB10_X2) glFormat[i]=GL_NONE;
		}
		else if(!stricmp(argv[i], "-bgr"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_BGR) glFormat[i]=GL_NONE;
		}
		else if(!stricmp(argv[i], "-bgra"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_BGRX) glFormat[i]=GL_NONE;
		}
		else if(!stricmp(argv[i], "-bgr10a2"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_BGR10_X2) glFormat[i]=GL_NONE;
		}
		#ifdef GL_ABGR_EXT
		else if(!stricmp(argv[i], "-abgr"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_XBGR) glFormat[i]=GL_NONE;
		}
		#endif
		else if(!stricmp(argv[i], "-a2bgr10"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_X2_BGR10) glFormat[i]=GL_NONE;
		}
		else if(!stricmp(argv[i], "-argb"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_XRGB) glFormat[i]=GL_NONE;
		}
		else if(!stricmp(argv[i], "-a2rgb10"))
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(i!=PF_X2_RGB10) glFormat[i]=GL_NONE;
		}
		else if(!stricmp(argv[i], "-loop") && i<argc-1)
		{
			loops=atoi(argv[++i]);
			if(loops<1) usage(argv);
		}
		else if(!stricmp(argv[i], "-align") && i<argc-1)
		{
			align=atoi(argv[++i]);
			if(align<1 || (align&(align-1))!=0) usage(argv);
		}
		else if(!stricmp(argv[i], "-visualid") && i<argc-1)
		{
			if(sscanf(argv[++i], "%x", &visualID)<1 || visualID<=0) usage(argv);
		}
		else if(!stricmp(argv[i], "-width") && i<argc-1)
		{
			width=atoi(argv[++i]);
			if(width<1) usage(argv);
		}
		else if(!stricmp(argv[i], "-height") && i<argc-1)
		{
			height=atoi(argv[++i]);
			if(height<1) usage(argv);
		}
		else if(!stricmp(argv[i], "-time") && i<argc-1)
		{
			benchTime=atof(argv[++i]);
			if(benchTime<=0.0) usage(argv);
		}
		else usage(argv);
	}
	#ifdef USEIFR
	if(pbo && useIFR)
	{
		fprintf(stderr, "IFR cannot be used with PBOs.  Disabling PBO readback.\n");
		pbo=0;
	}
	#endif

	if(argc<2) fprintf(stderr, "\n%s -h for advanced usage.\n", argv[0]);

	try
	{
		XSetErrorHandler(xhandler);
		if(!(dpy=XOpenDisplay(0)))
		{
			fprintf(stderr, "Could not open display %s\n", XDisplayName(0));
			exit(1);
		}
		#ifdef GL_VERSION_1_5
		if(pbo) fprintf(stderr, "Using PBOs for readback\n");
		#endif
		#ifdef USEIFR
		if(useIFR) fprintf(stderr, "Using nVidia Inband Frame Readback\n");
		#endif

		if((DisplayWidth(dpy, DefaultScreen(dpy))<width ||
			DisplayHeight(dpy, DefaultScreen(dpy))<height) && useWindow)
		{
			fprintf(stderr,
				"ERROR: Please switch to a screen resolution of at least %d x %d.\n",
				width, height);
			exit(1);
		}

		int bpc=DefaultDepth(dpy, DefaultScreen(dpy))==30? 10:8;
		findVisual(bpc);
		if(bpc!=10)
		{
			for(int i=0; i<PIXELFORMATS; i++)
				if(pf_get(i).bpc==10) glFormat[i]=GL_NONE;
		}

		if(useWindow || usePixmap || useFBO)
		{
			XSetWindowAttributes swa;
			Window root=DefaultRootWindow(dpy);
			swa.border_pixel=0;
			swa.event_mask=0;

			swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
			_errifnot(win=XCreateWindow(dpy, root, 0, 0,
				(usePixmap || useFBO)? 1:width, (usePixmap || useFBO)? 1:height,
				0, v->depth, InputOutput, v->visual,
				CWBorderPixel|CWColormap|CWEventMask, &swa));
			if(usePixmap)
			{
				_errifnot(pm=XCreatePixmap(dpy, win, width, height, v->depth));
			}
			else if(!useFBO) XMapWindow(dpy, win);
			XSync(dpy, False);
		}
		fprintf(stderr, "Rendering to %s (size = %d x %d pixels)\n",
			usePixmap? "Pixmap" : useWindow? "Window" :
				useFBO? (useRTT? "FBO + Texture" : "FBO + RBO") : "Pbuffer",
			width, height);
		fprintf(stderr, "Using %d-byte row alignment\n\n", align);

		drawableInit();
		#ifdef USEIFR
		if(useIFR)
		{
			if(!ifr.initialize())
				_throw("Failed to initialize IFR");
			if(ifr.nvIFROGLCreateSession(&ifrSession, NULL)!=NV_IFROGL_SUCCESS)
				_throw("Could not create IFR session");
		}
		#endif
		display();
		return 0;

	} catch(Error &e) { fprintf(stderr, "%s\n", e.getMessage()); }

	if(dpy)
	{
		#ifdef GL_EXT_framebuffer_object
		if(useFBO)
		{
			if(texture) glDeleteTextures(1, &texture);
			if(rbo) glDeleteRenderbuffersEXT(1, &rbo);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			if(fbo) glDeleteFramebuffersEXT(1, &fbo);
		}
		#endif
		#if USEIFR
		if(useIFR)
		{
			if(ifrSession) ifr.nvIFROGLDestroySession(ifrSession);
		}
		#endif
		glXMakeCurrent(dpy, 0, 0);
		if(ctx) glXDestroyContext(dpy, ctx);
		if(pbuffer) glXDestroyPbuffer(dpy, pbuffer);
		if(glxpm) glXDestroyGLXPixmap(dpy, glxpm);
		if(pm) XFreePixmap(dpy, pm);
		if(win) XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	if(v) XFree(v);
	return -1;
}
