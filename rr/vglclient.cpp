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
#include <fcntl.h>
#include "rrdisplayserver.h"
#include "x11err.h"
#include "xdk-sym.h"
#include <X11/Xatom.h>
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

bool restart=true;
bool deadyet;
unsigned short port=0;
#ifdef USESSL
unsigned short sslport=0;
bool ssl=true, nonssl=true;
#endif
int drawmethod=RR_DRAWAUTO;
Display *maindpy=NULL;
bool compat=false, detach=false, force=false;
char *logfile=NULL;

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
	#ifndef _WIN32
	if(type==SIGHUP) restart=true;
	#endif
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
	fprintf(stderr, "[-l <file>] [-detach] [-force] [-old] [-v] [-x] [-gl]\n");
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
	fprintf(stderr, "-detach = Detach from console (used by vglconnect)\n");
	fprintf(stderr, "-force = Force VGLclient to run, even if there is already another instance\n");
	fprintf(stderr, "         running on the same X display (use with caution)\n");
	fprintf(stderr, "-l = Redirect all output to <file>\n");
	fprintf(stderr, "-old = equivalent to '-port 4242 -sslport 4243'.  Use this when talking to\n");
	fprintf(stderr, "       2.0 or earlier VirtualGL servers\n");
	fprintf(stderr, "-v = Display version information\n");
	fprintf(stderr, "-x = Use X11 drawing (default for x86 systems)\n");
	fprintf(stderr, "-gl = Use OpenGL drawing (default for Sparc systems with 3D accelerators)\n");
}

#ifdef _WIN32
bool child=false;

void daemonize(void)
{
	STARTUPINFO si={0};  PROCESS_INFORMATION pi;
	char cl[MAX_PATH+1];
	snprintf(cl, MAX_PATH, "%s -port %d -sslport %d -child", GetCommandLine(),
		port, sslport);
	if(!CreateProcess(NULL, cl, NULL, NULL, FALSE, 0, NULL, NULL,
		&si, &pi)) _throww32();
}

#else

void daemonize(void)
{
	int err;
	if(getppid()==1) return;
	fclose(stdin);  fclose(stdout);  fclose(stderr);
	err=fork();
	if(err<0) exit(-1);
	if(err>0) exit(0);
	setsid();
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
}

#endif


#ifdef USESSL
unsigned short instancecheckssl(Display *dpy)
{
	Atom atom=None;  unsigned short p=0;
	if((atom=XInternAtom(dpy, "_VGLCLIENT_SSLPORT", True))!=None)
	{
		unsigned short *prop=NULL;  unsigned long n=0, bytesleft=0;
		int actualformat=0;  Atom actualtype=None;
		if(XGetWindowProperty(dpy, DefaultRootWindow(dpy), atom, 0, 1, False,
			XA_INTEGER, &actualtype, &actualformat, &n, &bytesleft,
			(unsigned char **)&prop)==Success && n>=1 && actualformat==16
			&& actualtype==XA_INTEGER && prop)
			p=*prop;
		if(prop) XFree(prop);
		if(p!=0)
		{
			rrout.println("vglclient is already running on this X display and accepting SSL");
			rrout.println("   connections on port %d.", p);
		}
	}
	return p;
}
#endif


unsigned short instancecheck(Display *dpy)
{
	Atom atom=None;  unsigned short p=0;
	if((atom=XInternAtom(dpy, "_VGLCLIENT_PORT", True))!=None)
	{
		unsigned short *prop=NULL;  unsigned long n=0, bytesleft=0;
		int actualformat=0;  Atom actualtype=None;
		if(XGetWindowProperty(dpy, DefaultRootWindow(dpy), atom, 0, 1, False,
			XA_INTEGER, &actualtype, &actualformat, &n, &bytesleft,
			(unsigned char **)&prop)==Success && n>=1 && actualformat==16
			&& actualtype==XA_INTEGER && prop)
			p=*prop;
		if(prop) XFree(prop);
		if(p!=0)
		{
			rrout.println("vglclient is already running on this X display and accepting unencrypted");
			rrout.println("   connections on port %d.", p);
		}
	}
	return p;
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
		if(!stricmp(argv[i], "-v")) printversion=true;
		if(!stricmp(argv[i], "-force")) force=true;
		if(!stricmp(argv[i], "-detach")) detach=true;
		if(!stricmp(argv[i], "-old")) compat=true;
		if(!stricmp(argv[i], "-port"))
		{
			if(i<argc-1) port=(unsigned short)atoi(argv[++i]);
		}
		if(!stricmp(argv[i], "-l"))
		{
			if(i<argc-1)
			{
				logfile=argv[++i];
				FILE *f=fopen(logfile, "a");
				if(!f) _throwunix();
				else fclose(f);
			}
		}
		if(!stricmp(argv[i], "-x")) drawmethod=RR_DRAWX11;
		if(!stricmp(argv[i], "-gl")) drawmethod=RR_DRAWOGL;
		if(!stricmp(argv[i], "-display"))
		{
			if(i<argc-1) displayname=argv[++i];
		}
		#ifdef _WIN32
		if(!stricmp(argv[i], "-child")) {child=true;  detach=false;}
		#endif
	}

	#ifdef _WIN32
	if(!child) {
	#endif

	rrout.println("\n%s Client v%s (Build %s)", __APPNAME, __VERSION, __BUILD);
	if(printversion) return 0;

	if(!force)
	{
		Display *dpy=NULL;
		if((dpy=XOpenDisplay(displayname))==NULL)
			_throw("Could not open display");
		unsigned short actualport=instancecheck(dpy);
		#ifdef USESSL
		unsigned short actualsslport=instancecheckssl(dpy);
		#endif
		XCloseDisplay(dpy);

		if(actualport!=0
		#ifdef USESSL
		&& actualsslport!=0
		#endif
		)
		{
			if(detach) printf("%d\n", actualport);
			return 0;
		}
	}

	if(logfile)	rrout.println("Redirecting output to %s", logfile);

	#ifdef USESSL
	if(sslport==0)
	{
		rrsocket sd(true);
		sslport=sd.listen(0, true);
		sd.close();
	}
	#endif
	if(port==0)
	{
		rrsocket sd(false);
		port=sd.listen(0, true);
		sd.close();
	}

	if(detach)
	{
		printf("%d\n", port);
		daemonize();
	}


	#ifdef _WIN32
	}
	if(!detach)
	{
		if(child) FreeConsole();
		start(displayname);
	}
	#else
	start(displayname);
	#endif

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
	rrdisplayserver *rrdpy=NULL;
  Atom port_atom=None;  unsigned short actualport=0;
	#ifdef USESSL
	rrdisplayserver *rrssldpy=NULL;
  Atom sslport_atom=None;  unsigned short actualsslport=0;
	#endif

	if(logfile) rrout.logto(logfile);

	if(!XInitThreads()) {rrout.println("XInitThreads() failed");  return;}
	#ifdef SUNOGL
	if(!_glXInitThreadsSUN()) _throw("glXInitThreadsSUN() failed");
	#endif

	signal(SIGINT, handler);
	signal(SIGTERM, handler);
	#ifndef _WIN32
	signal(SIGHUP, handler);
	#endif
	XSetErrorHandler(xhandler);

	start:

	try {

	if((maindpy=XOpenDisplay(displayname))==NULL)
		_throw("Could not open display");

	#ifdef USESSL
	if(ssl)
	{
		if(!(rrssldpy=new rrdisplayserver(compat? RR_DEFAULTSSLPORT:sslport,
			true, drawmethod)))
			_throw("Could not initialize SSL listener");
		rrout.println("Listening for SSL connections on port %d",
			actualsslport=rrssldpy->port());
		if((sslport_atom=XInternAtom(maindpy, "_VGLCLIENT_SSLPORT", False))==None)
			_throw("Could not get _VGLCLIENT_SSLPORT atom");
		XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			sslport_atom, XA_INTEGER, 16, PropModeReplace,
			(unsigned char *)&actualsslport, 1);
	}
	if(nonssl)
	#endif
	{
		if(!(rrdpy=new rrdisplayserver(compat? RR_DEFAULTPORT:port,
			false, drawmethod)))
			_throw("Could not initialize listener");
		rrout.println("Listening for unencrypted connections on port %d",
			actualport=rrdpy->port());
		if((port_atom=XInternAtom(maindpy, "_VGLCLIENT_PORT", False))==None)
			_throw("Could not get _VGLCLIENT_PORT atom");
		XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			port_atom, XA_INTEGER, 16, PropModeReplace,
			(unsigned char *)&actualport, 1);
	}
	XSync(maindpy, False);
	rrout.flush();

	while(!deadyet)
	{
		if(XPending(maindpy)>0) {}
		usleep(100000);
	}

	} catch(rrerror &e)
	{
		rrout.println("%s-- %s", e.getMethod(), e.getMessage());
	}

	if(rrdpy) {delete rrdpy;  rrdpy=NULL;}
	#ifdef USESSL
	if(rrssldpy) {delete rrssldpy;  rrssldpy=NULL;}
	if(maindpy && sslport_atom!=None)
	{
		XDeleteProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			sslport_atom);
		sslport_atom=None;
	}
	#endif
	if(maindpy && port_atom!=None)
	{
		XDeleteProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			port_atom);
		port_atom=None;
	}
	if(maindpy) {XCloseDisplay(maindpy);  maindpy=NULL;}

	if(restart)
	{
		deadyet=restart=false;  goto start;
	}
}
