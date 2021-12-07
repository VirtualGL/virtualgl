/*
  Compile with:
  gcc testgl5.c -o testgl5 -I/home/X11R6/include -L/home/X11R6/lib -lGL

  where /home/X11R6 is replaced with your X11R6 prefix.

  If rendering contexts are working correctly, this program should
  produce a window with 2 green rectangle and 2 magenta rectangles.
*/

#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void configure(unsigned int width, unsigned int height);
void drawBuffer(void);

int gWidth, gHeight;

Display *xDisplay;
GLXFBConfig glxFBConfig;
GLXWindow glxWindow;
GLXPbuffer glxPbuffer = 0;
GLXContext glxContext;
GLXContext glxPbufferContext;
int interactive = 0, reproducebug = 0;

void configure(unsigned int width, unsigned int height)
{
  int pbuffer_attrib[] = {
    GLX_PBUFFER_WIDTH, -1,
    GLX_PBUFFER_HEIGHT, -1,
    GLX_PRESERVED_CONTENTS, True,
    GLX_LARGEST_PBUFFER, False,
    None, None,
  };

   printf("Reconfigure event\n");

   gWidth = width;
   gHeight = height;

   if (glxPbufferContext) {
     glXDestroyContext(xDisplay, glxPbufferContext);
   }

   if (glxPbuffer) {
     glXDestroyPbuffer(xDisplay, glxPbuffer);
   }

   pbuffer_attrib[1] = width;
   pbuffer_attrib[3] = height;
   glxPbuffer = glXCreatePbuffer(xDisplay, glxFBConfig, pbuffer_attrib);

   if (!glxPbuffer) {
     printf("Unable to allocate pbuffer\n");
     exit(-1);
   }

   glxPbufferContext = glXCreateNewContext(xDisplay, glxFBConfig, GLX_RGBA_TYPE, NULL, True);
   if (!glxPbufferContext) {
     printf("Unable to allocate new pbuffer context\n");
   }

   drawBuffer();
}

void drawBuffer(void)
{
  printf("Entered drawBuffer: %d %d\n", gWidth, gHeight);

  /* Clear the GLXWindow */

  printf("Setting context: %p 0x%lx 0x%lx %p\n", xDisplay, glxWindow, glxWindow, glxContext);
  glXMakeContextCurrent(xDisplay, glxWindow, glxWindow, glxContext);

  glDrawBuffer(GL_FRONT_AND_BACK);

  glViewport( 0, 0, gWidth, gHeight );
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho(0, gWidth, 0, gHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(1.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glDrawBuffer(GL_BACK);

  glViewport( 0, 0, gWidth, gHeight );
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho(0, gWidth, 0, gHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(1.0, 0.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glFlush();

  if (interactive) {
    printf("Red | Red\n");
    printf("Red | Red\n");
    printf("Press any key ...\n");  getchar();
  }

  printf("Setting context: %p 0x%lx 0x%lx %p\n", xDisplay, glxPbuffer, glxPbuffer, glxContext);
  glXMakeContextCurrent(xDisplay, glxPbuffer, glxPbuffer, glxContext);

  glDrawBuffer(GL_FRONT_AND_BACK);

  glViewport( 0, 0, gWidth, gHeight );
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho(0, gWidth, 0, gHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0.0, 1.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glDrawBuffer(GL_BACK);

  glViewport( 0, 0, gWidth, gHeight );
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho(0, gWidth, 0, gHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0.0, 0.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glFlush();

  if (interactive) {
    printf("Red | Red\n");
    printf("Red | Red\n");
    printf("Press any key ...\n");  getchar();
  }

  printf("Setting context: %p 0x%lx 0x%lx %p\n", xDisplay, glxWindow, glxWindow, glxContext);
  glXMakeContextCurrent(xDisplay, glxWindow, glxWindow, glxContext);

  glDrawBuffer(GL_FRONT);
  glReadBuffer(GL_BACK);

  glRasterPos2i(0, 0);
  glCopyPixels(0, 0, 250, 100, GL_COLOR);
  glFlush();

  if (interactive) {
    printf("Red    | Red\n");
    printf("Purple | Red\n");
    printf("Press any key ...\n");  getchar();
  }

  printf("Setting context: %p 0x%lx 0x%lx %p\n", xDisplay, glxWindow, glxPbuffer, glxContext);
  glXMakeContextCurrent(xDisplay, glxWindow, glxPbuffer, glxContext);

  glDrawBuffer(GL_FRONT);
  glReadBuffer(GL_FRONT);

  glRasterPos2i(0, 100);
  glCopyPixels(0, 100, 250, 100, GL_COLOR);
  glFlush();


  if (interactive) {
    printf("Green  | Red\n");
    printf("Purple | Red\n");
    printf("Press any key ...\n");  getchar();
  }

  printf("Setting context: %p 0x%lx 0x%lx %p\n", xDisplay, glxWindow, glxWindow, glxPbufferContext);
  /* ********************************************************************* */
  /* BUG IS EXHIBITED IN THE FOLLOWING LINE                                */
  if (reproducebug)
    glXMakeContextCurrent(xDisplay, glxWindow, glxWindow, glxPbufferContext);

  /* Uncomment following line to show correct behavior                     */
  else
    glXMakeContextCurrent(xDisplay, glxWindow, glxWindow, glxContext);


  glDrawBuffer(GL_FRONT);
  glReadBuffer(GL_BACK);

  glRasterPos2i(250, 0);
  glCopyPixels(250, 0, 250, 100, GL_COLOR);
  glFlush();

  if (interactive) {
    printf("Green  | Red\n");
    printf("Purple | Purple\n");
    printf("Press any key ...\n");  getchar();
  }

  printf("Setting context: %p 0x%lx 0x%lx %p\n", xDisplay, glxWindow, glxPbuffer, glxPbufferContext);
  /* ********************************************************************* */
  /* BUG IS EXHIBITED IN THE FOLLOWING LINE                                */
  if (reproducebug)
    glXMakeContextCurrent(xDisplay, glxWindow, glxPbuffer, glxPbufferContext);

  /* Uncomment following line to show correct behavior                     */
  else
    glXMakeContextCurrent(xDisplay, glxWindow, glxPbuffer, glxContext);


  glDrawBuffer(GL_FRONT);
  glReadBuffer(GL_FRONT);

  glRasterPos2i(250, 100);
  glCopyPixels(250, 100, 250, 100, GL_COLOR);
  glFlush();




  if (interactive) {
    printf("Green  | Green\n");
    printf("Purple | Purple\n");
    printf("Press any key ...\n");  getchar();
  }

  printf("Setting context: %p 0x%lx 0x%lx %p\n", xDisplay, glxWindow, glxWindow, glxContext);
  glXMakeContextCurrent(xDisplay, glxWindow, glxWindow, glxContext);

  glDrawBuffer(GL_FRONT_AND_BACK);

  glViewport( 0, 0, gWidth, gHeight );
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho(0, gWidth, 0, gHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glColor4d(0.0, 0.0, 0.0, 1.0);

  glBegin(GL_LINES);
  glVertex2i(250, 0);
  glVertex2i(250, 200);
  glVertex2i(0, 100);
  glVertex2i(500, 100);
  glEnd();

  glFlush();

  if (interactive) {
    printf("Green  | Green\n");
    printf("Purple | Purple\n");
    printf("Press any key ...\n");  getchar();
  }

}

/*
  Stuff below here is intended to be GLX boilerplate.
*/

static void event_loop( Display *dpy )
{
  XEvent eventHolder;
  XEvent *event = &eventHolder;

   while (1) {
      XNextEvent( dpy, event );

      switch (event->type) {
      case ConfigureNotify:
        configure( event->xconfigure.width, event->xconfigure.height );
        break;
      default:
        break;
      }
   }
}

static Window make_rgb_db_window( Display *dpy,
                                  unsigned int width, unsigned int height )
{
  GLXFBConfig *glxFBConfigs = NULL;

  int glxNumFBConfigs;

  int fbAttribs[] = {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_ALPHA_SIZE, 1,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PBUFFER_BIT,
    GLX_DOUBLEBUFFER, True,
    None, None,
  };

  XVisualInfo *visinfo;

  int scrnum;
  XSetWindowAttributes attr;
  unsigned long mask;
  Window root;
  Window xwin;

  scrnum = DefaultScreen( dpy );

  glxFBConfigs = glXChooseFBConfig(dpy, scrnum, fbAttribs, &glxNumFBConfigs);

  if (0 == glxNumFBConfigs ||
      !glxFBConfigs) {
    printf("Error: No valid FBConfigs found\n");
    exit(1);
  }

  glxFBConfig = glxFBConfigs[0];

  visinfo = glXGetVisualFromFBConfig(dpy, glxFBConfig);
  if (!visinfo) {
    printf("Error: couldn't get visual corresponding to FBConfig\n");
    exit(1);
  }

  root = RootWindow( dpy, scrnum );

  /* window attributes */
  attr.background_pixel = 0;
  attr.border_pixel = 0;
  attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
  attr.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonMotionMask;
  mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

  xwin = XCreateWindow( dpy, root, 0, 0, width, height,
                        0, visinfo->depth, InputOutput,
                        visinfo->visual, mask, &attr );
  if (!xwin) {
    printf("Error: Unable to create X Window\n");
    exit(-1);
  }

  glxWindow = glXCreateWindow(dpy, glxFBConfig, xwin, NULL);
  if (!glxWindow) {
    printf("Error: Unable to create GLX Window.\n");
    exit(-1);
  }

  glxContext = glXCreateNewContext(dpy, glxFBConfig, GLX_RGBA_TYPE, NULL, True);
  if (!glxContext) {
    printf("Error: glXCreateNewContext failed\n");
    exit(1);
  }

  return xwin;
}


int main( int argc, char *argv[] )
{
   Display *dpy;
   Window win;
   int i;

   if (argc > 1) {
     for (i = 0; i < argc; i++) {
       if (!strcmp(argv[i], "-i")) interactive = 1;
       if (!strcmp(argv[i], "-bug")) reproducebug = 1;
     }
   }

   dpy = XOpenDisplay(NULL);
   xDisplay = dpy;

   win = make_rgb_db_window( dpy, 500, 200 );

   XMapWindow( dpy, win );

   event_loop( dpy );
   return 0;
}
