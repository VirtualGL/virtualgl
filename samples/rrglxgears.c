/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This is a port of the infamous "gears" demo to straight GLX (i.e. no GLUT)
 * Port by Brian Paul  23 March 2001
 *
 * Command line options:
 *    -info      print GL implementation information
 *
 */


/****************************** RRlib ******************************/

/* These comments indicate major sections of code which have been
   added or changed to support split rendering. */

/**************************** end RRlib ****************************/


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <rr.h>


#define BENCHMARK

#ifdef BENCHMARK

/* XXX this probably isn't very portable */

#include <sys/time.h>
#include <unistd.h>

/* return current time (in seconds) */
static int
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (int) tv.tv_sec;
}

#else /*BENCHMARK*/

/* dummy */
static int
current_time(void)
{
   return 0;
}

#endif /*BENCHMARK*/



#ifndef M_PI
#define M_PI 3.14159265
#endif


static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat angle = 0.0;


/****************************** RRlib ******************************/

#define DEFAULTSERVERDPY ":0.0"
#define DEFAULTQUAL 95
#define DEFAULTSUBSAMP RR_444
#define DEFAULTMOVFILE "glxgears.vgl"

RRDisplay rrdpy=0;
Display *serverdpy=NULL;
GLXPbuffer pbuffer=0;
GLXFBConfig fbcfg=0;
int pbwidth=-1, pbheight=-1;
int spoil=0, ssl=0, qual=DEFAULTQUAL, subsamp=DEFAULTSUBSAMP, dpynum=0,
	multithread=0, record=0, playback=0;
GLXContext ctx=0;
int movfile=-1;

#define rrtrap(f) { \
   if((f)==RR_ERROR) { \
      printf("%s--\n%s\n", RRErrorLocation(), RRErrorString()); \
      _die(1); \
   }}

#define unixtrap(f) { \
   if((f)==-1) { \
      printf("Error: %s\n", strerror(errno)); \
      _die(1); \
   }}

#define _die(rc) {if(rrdpy) RRCloseDisplay(rrdpy);  exit(rc);}

/* This effectively "resizes" the Pbuffer by destroying it and
   creating a new one with the new size.  This is called in
   response to an X window resize event */

static void
newPbuffer(int width, int height)
{
   int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0,
      GLX_PRESERVED_CONTENTS, True, None};

   if (width==pbwidth && height==pbheight)
      return;

   if (pbuffer) {
      glXMakeCurrent(serverdpy, 0, 0);
      glXDestroyPbuffer(serverdpy, pbuffer);
   }
   pbattribs[1]=width;  pbattribs[3]=height;
   pbuffer = glXCreatePbuffer(serverdpy, fbcfg, pbattribs);
   if (!pbuffer) {
      printf("Error: could not create Pbuffer\n");
      _die(1);
   }
   glXMakeCurrent( serverdpy, pbuffer, ctx );
   pbwidth=width;  pbheight=height;
}

/**************************** end RRlib ****************************/


/*
 *
 *  Draw a gear wheel.  You'll probably want to call this function when
 *  building a display list since we do a lot of trig here.
 * 
 *  Input:  inner_radius - radius of hole at center
 *          outer_radius - radius at center of teeth
 *          width - width of gear
 *          teeth - number of teeth
 *          tooth_depth - depth of tooth
 */
static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth)
{
   GLint i;
   GLfloat r0, r1, r2;
   GLfloat angle, da;
   GLfloat u, v, len;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   da = 2.0 * M_PI / teeth / 4.0;

   glShadeModel(GL_FLAT);

   glNormal3f(0.0, 0.0, 1.0);

   /* draw front face */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      if (i < teeth) {
	 glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
	 glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		    width * 0.5);
      }
   }
   glEnd();

   /* draw front sides of teeth */
   glBegin(GL_QUADS);
   da = 2.0 * M_PI / teeth / 4.0;
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
   }
   glEnd();

   glNormal3f(0.0, 0.0, -1.0);

   /* draw back face */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      if (i < teeth) {
	 glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		    -width * 0.5);
	 glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      }
   }
   glEnd();

   /* draw back sides of teeth */
   glBegin(GL_QUADS);
   da = 2.0 * M_PI / teeth / 4.0;
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
   }
   glEnd();

   /* draw outward faces of teeth */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      u = r2 * cos(angle + da) - r1 * cos(angle);
      v = r2 * sin(angle + da) - r1 * sin(angle);
      len = sqrt(u * u + v * v);
      u /= len;
      v /= len;
      glNormal3f(v, -u, 0.0);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      glNormal3f(cos(angle), sin(angle), 0.0);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
      v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
      glNormal3f(v, -u, 0.0);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      glNormal3f(cos(angle), sin(angle), 0.0);
   }

   glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
   glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

   glEnd();

   glShadeModel(GL_SMOOTH);

   /* draw inside radius cylinder */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glNormal3f(-cos(angle), -sin(angle), 0.0);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
   }
   glEnd();
}


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(view_rotx, 1.0, 0.0, 0.0);
   glRotatef(view_roty, 0.0, 1.0, 0.0);
   glRotatef(view_rotz, 0.0, 0.0, 1.0);

   glPushMatrix();
   glTranslatef(-3.0, -2.0, 0.0);
   glRotatef(angle, 0.0, 0.0, 1.0);
   glCallList(gear1);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(3.1, -2.0, 0.0);
   glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
   glCallList(gear2);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(-3.1, 4.2, 0.0);
   glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
   glCallList(gear3);
   glPopMatrix();

   glPopMatrix();
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   GLfloat h = (GLfloat) height / (GLfloat) width;

   newPbuffer(width, height);  /***** RRlib *****/
   glViewport(0, 0, (GLint) width, (GLint) height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -40.0);
}


static void
init(void)
{
   static GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };
   static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   static GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };

   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_CULL_FACE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   /* make the gears */
   gear1 = glGenLists(1);
   glNewList(gear1, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
   gear(1.0, 4.0, 1.0, 20, 0.7);
   glEndList();

   gear2 = glGenLists(1);
   glNewList(gear2, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
   gear(0.5, 2.0, 2.0, 10, 0.7);
   glEndList();

   gear3 = glGenLists(1);
   glNewList(gear3, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
   gear(1.3, 2.0, 0.5, 10, 0.7);
   glEndList();

   glEnable(GL_NORMALIZE);
}


/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
static void
make_window( Display *dpy, const char *name,
             int x, int y, int width, int height,
             Window *winRet, GLXContext *ctxRet)
{

   int attrib[] = { GLX_RENDER_TYPE, GLX_RGBA_BIT,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER, 1,
		    GLX_DEPTH_SIZE, 1,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   GLXContext ctx;
   XVisualInfo *visinfo;
   XVisualInfo vtemp;
   GLXFBConfig *configs = NULL;
   int n = 0;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );


   #if 0
   visinfo = glXChooseVisual( dpy, scrnum, attrib );
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }
   #endif

   /****************************** RRlib ******************************/

   /* Instead of calling glXChooseVisual(), we call glXChooseFBConfig()
      to obtain a Pbuffer-compatible framebuffer config on the server's
      display */

   configs = glXChooseFBConfig( serverdpy, DefaultScreen( serverdpy ), attrib,
                &n );
   if (!configs || !n) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      _die(1);
   }
   fbcfg = configs[0];
   XFree( configs );

   /* Next, we find an equivalent client-side visual for X11 to use */

   vtemp.depth = 24;
   vtemp.class = TrueColor;
   visinfo = XGetVisualInfo( dpy, VisualDepthMask | VisualClassMask, &vtemp,
                &n );
   if (!visinfo || !n) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      _die(1);
   }

   /**************************** end RRlib ****************************/


   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }


   #if 0
   ctx = glXCreateContext( dpy, visinfo, NULL, True );
   if (!ctx) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }
   #endif

   /****************************** RRlib ******************************/

   /* Instead of glXCreateContext(), we call glXCreateNewContext() on
      the server's display to establish the context using the FB config
      we just created */

   ctx = glXCreateNewContext( serverdpy, fbcfg, GLX_RGBA_TYPE, NULL, True );
   if (!ctx) {
      printf("Error: glXCreateNewContext failed\n");
      _die(1);
   }

   /**************************** end RRlib ****************************/


   XFree(visinfo);

   *winRet = win;
   *ctxRet = ctx;
}


static void
event_loop(Display *dpy, Window win)
{
   int ready=0;  RRFrame frame;  unsigned char *bits = NULL;
   int playbackwidth=-1, playbackheight=-1;

   while (1) {
      while (XPending(dpy) > 0) {
         XEvent event;
         XNextEvent(dpy, &event);
         switch (event.type) {
	 case Expose:
            /* we'll redraw below */
	    break;
	 case ConfigureNotify:
	    reshape(event.xconfigure.width, event.xconfigure.height);
	    break;
         case KeyPress:
            {
               char buffer[10];
               int r, code;
               code = XLookupKeysym(&event.xkey, 0);
               if (code == XK_Left) {
                  view_roty += 5.0;
               }
               else if (code == XK_Right) {
                  view_roty -= 5.0;
               }
               else if (code == XK_Up) {
                  view_rotx += 5.0;
               }
               else if (code == XK_Down) {
                  view_rotx -= 5.0;
               }
               else {
                  r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                                    NULL, NULL);
                  if (buffer[0] == 27) {
                     /* escape */
                     return;
                  }
               }
            }
         }
      }


      /****************************** RRlib ******************************/

      /* Play back the stored movie */

      if (playback) {
         int bytesread;  rrframeheader h;
         if(movfile==-1)
            unixtrap(movfile=open(DEFAULTMOVFILE, O_RDONLY));
         unixtrap(bytesread=read(movfile, &h, sizeof(rrframeheader)));
         if(bytesread!=sizeof(rrframeheader)) {
            printf("End of file.  Restarting\n");
            lseek(movfile, 0, SEEK_SET);  continue;
         }
         if(h.framew!=playbackwidth || h.frameh!=playbackheight) {
            XResizeWindow(dpy, win, h.framew, h.frameh);
            playbackwidth=h.framew;  playbackheight=h.frameh;
         }
         if ((bits=(unsigned char *)realloc(bits, h.size))==NULL) {
            printf("Error: Could not allocate buffer\n");  _die(1);
         }
         unixtrap(bytesread=read(movfile, bits, h.size));
         if(bytesread!=h.size) {
            printf("End of file.  Restarting\n");
            lseek(movfile, 0, SEEK_SET);  continue;
         }
         h.dpynum=dpynum;
         h.winid=win;
         rrtrap(RRSendRawFrame(rrdpy, &h, bits));
         goto benchmark;
      }

      /**************************** end RRlib ****************************/


      /* next frame */
      angle += 2.0;

      draw();


      /****************************** RRlib ******************************/

      /* Before we call glXSwapBuffers(), we get a new image buffer from
         RRlib and use glReadPixels() to fill the image buffer with the
         contents of the back framebuffer. */

      rrtrap(ready=RRFrameReady(rrdpy));
      if (!spoil || ready) {
         rrtrap(RRGetFrame(rrdpy, pbwidth, pbheight, RR_RGB, 1, &frame));
         glPixelStorei(GL_PACK_ALIGNMENT, 1);
         glReadPixels(0, 0, pbwidth, pbheight, GL_RGB, GL_UNSIGNED_BYTE,
            frame.bits);

         frame.h.qual=qual;
         frame.h.subsamp=subsamp;

      /* Use RRCompressFrame() to compress the frame for storage.  We'll
         go ahead and send it now as well, just so we can monitor the record
         process. */

         if (record) {
            int byteswritten;
            rrtrap(RRCompressFrame(rrdpy, &frame));
            if(movfile==-1)
               unixtrap(movfile=open(DEFAULTMOVFILE, O_WRONLY|O_CREAT|O_TRUNC,
                  0666));
            unixtrap(byteswritten=write(movfile, &frame.h, sizeof(rrframeheader)));
            if(byteswritten!=sizeof(rrframeheader)) {
               printf("Error: write truncated\n");  _die(1);
            }
            unixtrap(byteswritten=write(movfile, frame.bits, frame.h.size));
            if(byteswritten!=frame.h.size) {
               printf("Error: write truncated\n");  _die(1);
            }
         }

      /* Populate the header, then call RRSendFrame to add the frame to
         the outgoing queue (or send it directly if it has already been
         compressed.)  Sending the frame releases it as well. */

         frame.h.dpynum=dpynum;
         frame.h.winid=win;
         rrtrap(RRSendFrame(rrdpy, &frame));
      }

      /**************************** end RRlib ****************************/


      glXSwapBuffers(serverdpy, pbuffer);

      benchmark:
      /* calc framerate */
      {
         static int t0 = -1;
         static int frames = 0;
         int t = current_time();

         if (t0 < 0)
            t0 = t;

         frames++;

         if (t - t0 >= 5.0) {
            GLfloat seconds = t - t0;
            GLfloat fps = frames / seconds;
            printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds,
                   fps);
            t0 = t;
            frames = 0;
         }
      }
   }
}


int
main(int argc, char *argv[])
{
   Display *dpy;
   Window win;
   char *dpyName = DEFAULTSERVERDPY;
   char *clidpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   int i;
   unsigned short port = 0;


   for (i = 1; i < argc; i++) {
      if (strncasecmp(argv[i], "-d", 2) == 0 && i+1<argc) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strncasecmp(argv[i], "-cl", 3) == 0 && i+1<argc) {
         clidpyName = argv[i+1];
         i++;
      }
      else if (strcasecmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
      else if (strncasecmp(argv[i], "-ssl", 4) == 0) {
         ssl = 1;
      }
      else if (strncasecmp(argv[i], "-sp", 3) == 0) {
         spoil = 1;
      }
      else if (strncasecmp(argv[i], "-mt", 3) == 0) {
         multithread = 1;
      }
      else if (strncasecmp(argv[i], "-play", 5) == 0) {
         playback = 1;
      }
      else if (strncasecmp(argv[i], "-rec", 4) == 0) {
         record = 1;
      }
      else if (strncasecmp(argv[i], "-p", 2) == 0 && i+1<argc) {
         port = atoi(argv[i+1]);
         i++;
      }
      else if (strncasecmp(argv[i], "-sa", 3) == 0 && i+1<argc) {
         int temp = atoi(argv[i+1]);
         switch (temp) {
            case 411: subsamp=RR_411;  break;
            case 422: subsamp=RR_422;  break;
            case 444: subsamp=RR_444;  break;
         }
         i++;
      }
      else if (strncasecmp(argv[i], "-q", 2) == 0 && i+1<argc) {
         int temp = atoi(argv[i+1]);
         if(temp>=1 && temp<=100) qual=temp;
         i++;
      }
      else if (strncasecmp(argv[i], "-h", 2) == 0
         || strncasecmp(argv[i], "-\?", 2) == 0) {
         printf("\nUSAGE: %s\n", argv[0]);
         printf("       [-d <hostname:x.x>] [-cl <hostname:x.x>] [-p <xxxx>]\n");
         printf("       [-q <1-100>] [-samp <411|422|444>] [-sp] [-ssl] [-mt]\n");
         printf("       [-rec] [-play] [-info]\n\n");
         printf("-d    = Set display where 3D rendering will occur (default: %s)\n", DEFAULTSERVERDPY);
         printf("-cl   = Set display where the client is running\n");
         printf("        (default: read from DISPLAY environment)\n");
         printf("-p    = Set port to use when connecting to the client\n");
         printf("        (default: %d for non-SSL or %d for SSL)\n", RR_DEFAULTPORT, RR_DEFAULTSSLPORT);
         printf("-q    = Set compression quality [1-100] (default: %d)\n", DEFAULTQUAL);
         printf("-samp = Set YUV subsampling [411, 422, or 444] (default: %s)\n", DEFAULTSUBSAMP==RR_444?"444":(DEFAULTSUBSAMP==RR_422?"422":"411"));
         printf("-sp   = Turn on frame spoiling\n");
         printf("-ssl  = Communicate with the client using an SSL tunnel\n");
         printf("-mt   = Enable multi-threaded compression\n");
         printf("-rec  = Record movie (will be stored in a file named %s)\n", DEFAULTMOVFILE);
         printf("-play = Playback movie (from file %s)\n", DEFAULTMOVFILE);
         printf("-info = Print OpenGL info\n");
         printf("\n");
         return 0;
      }
   }
   if(port==0) port=ssl? RR_DEFAULTSSLPORT:RR_DEFAULTPORT;


   /****************************** RRlib ******************************/
   /* Open a connection to the client display to use for 2D rendering */

   dpy = XOpenDisplay(NULL);
   if (!dpy) {
      printf("Error: couldn't open display\n");
      return -1;
   }
   if (!clidpyName) clidpyName=DisplayString(dpy);

   /* Open a connection to the server display to use for 3D rendering */

   serverdpy = XOpenDisplay(dpyName);
   if (!serverdpy) {
      printf("Error: couldn't open display %s\n", dpyName);
      return -1;
   }

   /* Open a connection to the VirtualGL client, which is usually
      running on the same machine as the client X display.  Store
      the X display number for further use (dpynum is obtained by
      parsing clidpyName)

      NOTE: clidpyName can be NULL if all you want to do is compress
      images without sending them. */

   rrdpy = RROpenDisplay(clidpyName, port, ssl, multithread, &dpynum);
   if (!rrdpy) {
      printf("Error: could not open connection to client\n");
      printf("%s--\n%s\n", RRErrorLocation(), RRErrorString());
      return -1;
   }

   /**************************** end RRlib ****************************/


   make_window(dpy, "glxgears", 0, 0, 300, 300, &win, &ctx);
   XMapWindow(dpy, win);

   #if 0
   glXMakeCurrent(dpy, win, ctx);
   #endif
   newPbuffer(300, 300);  /***** RRlib *****/

   if (printInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }

   init();

   event_loop(dpy, win);

   glXDestroyContext(dpy, ctx);
   if(rrdpy) RRCloseDisplay(rrdpy);  /***** RRlib *****/
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);
   XCloseDisplay(serverdpy);

   return 0;
}
