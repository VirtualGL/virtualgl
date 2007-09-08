/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include "rrdisplayserver.h"
#include "x11err.h"
#include "xdk-sym.h"
#include <X11/Xatom.h>
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

rrdisplayserver *rrdpy=NULL;
bool restart=true, quiet=false;
bool deadyet;
unsigned short port=0;
#ifdef USESSL
rrdisplayserver *rrssldpy=NULL;
unsigned short sslport=0;
bool ssl=true, nonssl=true;
#endif
int drawmethod=RR_DRAWAUTO;
Display *maindpy=NULL;
bool compat=false;

void start(char *);

#ifdef SUNOGL
#include "dlfcn.h"
int _glXInitThreadsSUN(void)
{
	int retval=1;
	typedef int (*_glXInitThreadsSUNType)(void);
	_glXInitThreadsSUNType __glXInitThreadsSUN=NULL;
	if((__glXInitThreadsSUN=
		(_glXInitThreadsSUNType)dlsym(RTLD_NEXT, "glXInitThreadsSUN"))!=NULL)
		retval=__glXInitThreadsSUN();
	return retval;
}
#endif

extern "C" {
void handler(int type)
{
	restart=false;
	deadyet=true;
}


// This does nothing except prevent Xlib from exiting the program so we can trap X11 errors
int xhandler(Display *dpy, XErrorEvent *xe)
{
	const char *temp=NULL;  char errmsg[257];  bool printwarnings=false;
	char *env=NULL;
	if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
		&& !strncmp(env, "1", 1)) printwarnings=true;
	if(printwarnings)
	{
		errmsg[0]=0;
		XGetErrorText(dpy, xe->error_code, errmsg, 256);
		rrout.print("X11 Error: ");
		if((temp=x11error(xe->error_code))!=NULL && stricmp(temp, "Unknown error code"))
			rrout.print("%s ", temp);
		rrout.println("%s", errmsg);
	}
	return 1;
}
} // extern "C"


void usage(char *progname)
{
	fprintf(stderr, "\nUSAGE: %s [-h|-?] [-display <name>]\n", progname);
	fprintf(stderr, "       [-port <p>] ");
	#ifdef USESSL
	fprintf(stderr, "[-sslport <port>] [-sslonly] [-nossl]\n       ");
	#endif
	fprintf(stderr, "[-l <file>] [-old] [-v] [-x] [-gl]\n");
	fprintf(stderr, "\n-h or -? = This help screen\n");
	fprintf(stderr, "-display = The X display to which to draw the images received from the\n");
	fprintf(stderr, "           VirtualGL server (default = read from the DISPLAY environment\n");
	fprintf(stderr, "           variable)\n");
	fprintf(stderr, "-port = TCP port to use for unencrypted connections from the VirtualGL server\n");
	fprintf(stderr, "        (default = automatically select a free port)\n");
	#ifdef USESSL
	fprintf(stderr, "-sslport = TCP port to use for encrypted connections from the VirtualGL server\n");
	fprintf(stderr, "           (default = automatically select a free port)\n");
	fprintf(stderr, "-sslonly = Only allow encrypted connections\n");
	fprintf(stderr, "-nossl = Only allow unencrypted connections\n");
	#endif
	fprintf(stderr, "-l = Redirect all output to <file>\n");
	fprintf(stderr, "-old = equivalent to '-port 4242 -sslport 4243'.  Use this when talking to\n");
	fprintf(stderr, "       2.0 or earlier VirtualGL servers\n");
	fprintf(stderr, "-v = Display version information\n");
	fprintf(stderr, "-x = Use X11 drawing (default for x86 systems)\n");
	fprintf(stderr, "-gl = Use OpenGL drawing (default for Sparc systems with 3D accelerators)\n");
}


int main(int argc, char *argv[])
{
	int i;  bool printversion=false;
	char *displayname=NULL;

	try {

	if(argc>1) for(i=1; i<argc; i++)
	{
		if(!strnicmp(argv[i], "-h", 2) || !stricmp(argv[i], "-?"))
			{usage(argv[0]);  exit(1);}
		#ifdef USESSL
		if(!stricmp(argv[i], "-sslonly")) {ssl=true;  nonssl=false;}
		if(!stricmp(argv[i], "-nossl")) {ssl=false;  nonssl=true;}
		if(!stricmp(argv[i], "-sslport"))
		{
			if(i<argc-1) sslport=(unsigned short)atoi(argv[++i]);
		}
		#endif
		if(!stricmp(argv[i], "-q")) quiet=true;
		if(!stricmp(argv[i], "-v")) printversion=true;
		if(!stricmp(argv[i], "-old")) compat=true;
		if(!stricmp(argv[i], "-port"))
		{
			if(i<argc-1) port=(unsigned short)atoi(argv[++i]);
		}
		if(!stricmp(argv[i], "-l"))
		{
			if(i<argc-1) rrout.logto(argv[++i]);
		}
		if(!stricmp(argv[i], "-x")) drawmethod=RR_DRAWX11;
		if(!stricmp(argv[i], "-gl")) drawmethod=RR_DRAWOGL;
		if(!stricmp(argv[i], "-display"))
		{
			if(i<argc-1) displayname=argv[++i];
		}
	}

	rrout.println("\n%s Client v%s (Build %s)", __APPNAME, __VERSION, __BUILD);
	if(printversion) return 0;

	start(displayname);

	}
	catch(rrerror &e)
	{
		rrout.println("%s-- %s", e.getMethod(), e.getMessage());  exit(1);
	}
	#ifdef XDK
	__vgl_unloadsymbols();
	#endif
	return 0;
}


void start(char *displayname)
{
	if(!XInitThreads()) {rrout.println("XInitThreads() failed");  return;}
	#ifdef SUNOGL
	if(!_glXInitThreadsSUN()) _throw("glXInitThreadsSUN() failed");
	#endif

	signal(SIGINT, handler);
	XSetErrorHandler(xhandler);

	try {

	start:
	if((maindpy=XOpenDisplay(displayname))==NULL)
		_throw("Could not open display");

  Atom port_atom=None;
	#ifdef USESSL
	Atom sslport_atom=None;
	if(ssl)
	{
		int oldsslport=-1;
		if((sslport_atom=XInternAtom(maindpy, "_VGLCLIENT_SSLPORT", True))!=None)
		{
			unsigned short *prop=NULL;  unsigned long n=0, bytesleft=0;
			int actualformat=0;  Atom actualtype=None;
			if(XGetWindowProperty(maindpy, DefaultRootWindow(maindpy),
				sslport_atom, 0, 1, False, XA_INTEGER, &actualtype, &actualformat, &n,
				&bytesleft, (unsigned char **)&prop)==Success && n>=1
				&& actualformat==16 && actualtype==XA_INTEGER && prop)
				oldsslport=*prop;
			if(prop) XFree(prop);
		}
		if(oldsslport<0)
		{
			if(!(rrssldpy=new rrdisplayserver(compat? RR_DEFAULTSSLPORT:sslport,
				true, drawmethod)))
				_throw("Could not initialize SSL listener");
			rrout.println("Listening for SSL connections on port %d",
				sslport=rrssldpy->port());
			if((sslport_atom=XInternAtom(maindpy, "_VGLCLIENT_SSLPORT", False))==None)
				_throw("Could not get _VGLCLIENT_SSLPORT atom");
			XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
				sslport_atom, XA_INTEGER, 16, PropModeReplace, (unsigned char *)&sslport,
				1);
		}
		else
		{
			rrout.println("Another VGL client instance is already accepting SSL connections for this");
			rrout.println("   X display on port %d", oldsslport);
			sslport_atom=None;
		}
	}
	if(nonssl)
	#endif
	{
		int oldport=-1;
		if((port_atom=XInternAtom(maindpy, "_VGLCLIENT_PORT", True))!=None)
		{
			unsigned short *prop=NULL;  unsigned long n=0, bytesleft=0;
			int actualformat=0;  Atom actualtype=None;
			if(XGetWindowProperty(maindpy, DefaultRootWindow(maindpy),
				port_atom, 0, 1, False, XA_INTEGER, &actualtype, &actualformat, &n,
				&bytesleft, (unsigned char **)&prop)==Success && n>=1
				&& actualformat==16 && actualtype==XA_INTEGER && prop)
				oldport=*prop;
			if(prop) XFree(prop);
		}
		if(oldport<0)
		{
			if(!(rrdpy=new rrdisplayserver(compat? RR_DEFAULTPORT:port,
				false, drawmethod)))
				_throw("Could not initialize listener");
			rrout.println("Listening for unencrypted connections on port %d",
				port=rrdpy->port());
			if((port_atom=XInternAtom(maindpy, "_VGLCLIENT_PORT", False))==None)
				_throw("Could not get _VGLCLIENT_PORT atom");
			XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
				port_atom, XA_INTEGER, 16, PropModeReplace, (unsigned char *)&port,
				1);
		}
		else
		{
			rrout.println("Another VGL client instance is already accepting unencrypted connections for");
			rrout.println("   this X display on port %d", oldport);
			port_atom=None;
		}
	}
	XSync(maindpy, False);
	rrout.flush();
	if(port_atom==None && sslport_atom==None) {deadyet=true;  restart=false;}

	while(!deadyet)
	{
		if(XPending(maindpy)>0) {}
		usleep(100000);
	}

	if(rrdpy) {delete rrdpy;  rrdpy=NULL;}
	#ifdef USESSL
	if(rrssldpy) {delete rrssldpy;  rrssldpy=NULL;}
	if(maindpy && sslport_atom!=None)
		XDeleteProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			sslport_atom);
	#endif
	if(maindpy && port_atom!=None)
		XDeleteProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			port_atom);
	if(maindpy) {XCloseDisplay(maindpy);  maindpy=NULL;}
	if(restart) goto start;

	} catch(rrerror &e)
	{
		rrout.println("%s-- %s", e.getMethod(), e.getMessage());
	}
}
