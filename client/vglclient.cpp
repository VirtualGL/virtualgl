/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011-2012, 2014 D. R. Commander
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include "VGLTransReceiver.h"
#include "vglutil.h"
#include "x11err.h"
#include <X11/Xatom.h>
#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/proc.h>
#else
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#ifdef sun
#include <procfs.h>
#endif
#endif

using namespace vglutil;
using namespace vglclient;


bool restart;
bool deadYet;
unsigned short port=0;
#ifdef USESSL
unsigned short sslPort=0;
bool doSSL=true, doNonSSL=true;
#endif
int drawMethod=RR_DRAWAUTO;
Display *maindpy=NULL;
bool detach=false, force=false, child=false;
char *logFile=NULL;

void start(char *);


extern "C" {

void handler(int type)
{
	restart=false;
	if(type==SIGHUP) restart=true;
	deadYet=true;
}


// This does nothing except prevent Xlib from exiting the program, so we can
// trap X11 errors
int xhandler(Display *dpy, XErrorEvent *xe)
{
	const char *temp=NULL;  char errmsg[257];  bool printWarnings=false;
	char *env=NULL;

	if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
		&& !strncmp(env, "1", 1)) printWarnings=true;
	if(printWarnings)
	{
		errmsg[0]=0;
		XGetErrorText(dpy, xe->error_code, errmsg, 256);
		vglout.print("X11 Error: ");
		if((temp=x11error(xe->error_code))!=NULL
			&& stricmp(temp, "Unknown error code"))
			vglout.print("%s ", temp);
		vglout.println("%s", errmsg);
	}
	return 1;
}

} // extern "C"


void killproc(bool userOnly)
{
	#ifdef __APPLE__

	unsigned char *buf=NULL;

	if(!userOnly && getuid()!=0) _throw("Only root can do that");

	try
	{
		int mib_all[4]={CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
		int mib_user[4]={CTL_KERN, KERN_PROC, KERN_PROC_UID, getuid()};
		size_t len=0;

		_unix(sysctl(userOnly? mib_user:mib_all, 4, NULL, &len, NULL, 0));
		if(len<sizeof(kinfo_proc)) _throw("Process table is empty");
		_newcheck(buf=new unsigned char[len]);
		_unix(sysctl(userOnly? mib_user:mib_all, 4, buf, &len, NULL, 0));
		int nprocs=len/sizeof(kinfo_proc);
		kinfo_proc *kp=(kinfo_proc *)buf;

		for(int i=0; i<nprocs; i++)
		{
			int pid=kp[i].kp_proc.p_pid;
			if(!strcmp(kp[i].kp_proc.p_comm, "vglclient")
				&& pid!=getpid())
			{
				vglout.println("Terminating vglclient process %d", pid);
				kill(pid, SIGTERM);
			}
		}
		free(buf);  buf=NULL;
	}
	catch (...)
	{
		if(buf) free(buf);
		throw;
	}

	#else

	DIR *procdir=NULL;  struct dirent *dent=NULL;  int fd=-1;

	if(!userOnly && getuid()!=0) _throw("Only root can do that");

	try
	{
		if(!(procdir=opendir("/proc"))) _throwunix();
		while((dent=readdir(procdir))!=NULL)
		{
			if(dent->d_name[0]>='1' && dent->d_name[0]<='9')
			{
				char temps[1024];  struct stat fsbuf;
				int pid=atoi(dent->d_name);

				if(pid==getpid()) continue;
				#ifdef sun
				sprintf(temps, "/proc/%s/psinfo", dent->d_name);
				#else
				sprintf(temps, "/proc/%s/stat", dent->d_name);
				#endif
				if((fd=open(temps, O_RDONLY))==-1) continue;
				if(fstat(fd, &fsbuf)!=-1 && (fsbuf.st_uid==getuid() || !userOnly))
				{
					int bytes=0;

					#ifdef sun

					psinfo_t psinfo;
					if((bytes=read(fd, &psinfo, sizeof(psinfo_t)))==sizeof(psinfo_t)
						&& psinfo.pr_nlwp!=0)
					{
						if(!strcmp(psinfo.pr_fname, "vglclient"))
						{
							vglout.println("Terminating vglclient process %d", pid);
							kill(pid, SIGTERM);
						}
					}

					#else

					char *ptr=NULL;
					if((bytes=read(fd, temps, 1023))>0 && bytes<1024)
					{
						temps[bytes]='\0';
						if((ptr=strchr(temps, '('))!=NULL)
						{
							ptr++;  char *ptr2=strchr(ptr, ')');
							if(ptr2)
							{
								*ptr2='\0';
								if(!strcmp(ptr, "vglclient"))
								{
									vglout.println("Terminating vglclient process %d", pid);
									kill(pid, SIGTERM);
								}
							}
						}
					}

					#endif
				}
				close(fd);  fd=-1;
			}
		}
		closedir(procdir);  procdir=NULL;
	}
	catch(...)
	{
		if(fd) close(fd);  if(procdir) closedir(procdir);  throw;
	}

	#endif
}


void usage(char *progname)
{
	fprintf(stderr, "\nUSAGE: %s [-h|-?] [-display <name>]\n", progname);
	fprintf(stderr, "       [-port <p>] ");
	#ifdef USESSL
	fprintf(stderr, "[-sslport <port>] [-sslonly] [-nossl]\n       ");
	#endif
	fprintf(stderr, "[-l <file>] [-detach] [-force] [-v] [-x] [-gl]\n");
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
	fprintf(stderr, "-kill = Kill all detached VGLclient processes running under this user ID\n");
	fprintf(stderr, "-l = Redirect all output to <file>\n");
	fprintf(stderr, "-v = Display version information\n");
	fprintf(stderr, "-x = Use X11 drawing (default)\n");
	fprintf(stderr, "-gl = Use OpenGL drawing\n");
}


void daemonize(void)
{
	int err;
	if(getppid()==1) return;
	err=fork();
	if(err<0) exit(-1);
	if(err>0) exit(0);
	child=true;
	setsid();
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
}


#ifdef USESSL

unsigned short instanceCheckSSL(Display *dpy)
{
	Atom atom=None;  unsigned short p=0;
	if((atom=XInternAtom(dpy, "_VGLCLIENT_SSLPORT", True))!=None)
	{
		unsigned char *prop=NULL;  unsigned long n=0, bytesLeft=0;
		int actualFormat=0;  Atom actualType=None;
		if(XGetWindowProperty(dpy, DefaultRootWindow(dpy), atom, 0, 1, False,
				XA_INTEGER, &actualType, &actualFormat, &n, &bytesLeft,
				&prop)==Success
			&& n>=1 && actualFormat==16 && actualType==XA_INTEGER && prop)
			p=*(unsigned short *)prop;
		if(prop) XFree(prop);
		if(p!=0)
		{
			vglout.println("vglclient is already running on this X display and accepting SSL");
			vglout.println("   connections on port %d.", p);
		}
	}
	return p;
}

#endif


unsigned short instanceCheck(Display *dpy)
{
	Atom atom=None;  unsigned short p=0;
	if((atom=XInternAtom(dpy, "_VGLCLIENT_PORT", True))!=None)
	{
		unsigned char *prop=NULL;  unsigned long n=0, bytesLeft=0;
		int actualFormat=0;  Atom actualType=None;
		if(XGetWindowProperty(dpy, DefaultRootWindow(dpy), atom, 0, 1, False,
				XA_INTEGER, &actualType, &actualFormat, &n, &bytesLeft, &prop)==Success
			&& n>=1 && actualFormat==16 && actualType==XA_INTEGER && prop)
			p=*(unsigned short *)prop;
		if(prop) XFree(prop);
		if(p!=0)
		{
			vglout.println("vglclient is already running on this X display and accepting unencrypted");
			vglout.println("   connections on port %d.", p);
		}
	}
	return p;
}


void getEnvironment(void)
{
	char *env=NULL;  int temp;
	if((env=getenv("VGLCLIENT_DRAWMODE"))!=NULL && strlen(env)>0)
	{
		if(!strnicmp(env, "o", 1)) drawMethod=RR_DRAWOGL;
		else if(!strncmp(env, "x", 1)) drawMethod=RR_DRAWX11;
	}
	#ifdef USESSL
	if((env=getenv("VGLCLIENT_LISTEN"))!=NULL && strlen(env)>0)
	{
		if(!strncmp(env, "n", 1)) { doSSL=false;  doNonSSL=true; }
		else if(!strncmp(env, "s", 1)) { doSSL=true;  doNonSSL=false; }
	}
	#endif
	if((env=getenv("VGLCLIENT_LOG"))!=NULL && strlen(env)>0)
	{
		logFile=env;
		FILE *f=fopen(logFile, "a");
		if(!f)
		{
			vglout.println("Could not open log file %s", logFile);
			_throwunix();
		}
		else fclose(f);
	}
	if((env=getenv("VGLCLIENT_PORT"))!=NULL && strlen(env)>0
		&& (temp=atoi(env))>0 && temp<65536)
		port=(unsigned short)temp;
	#ifdef USESSL
	if((env=getenv("VGLCLIENT_SSLPORT"))!=NULL && strlen(env)>0
		&& (temp=atoi(env))>0 && temp<65536)
		sslPort=(unsigned short)temp;
	#endif
}


int main(int argc, char *argv[])
{
	int i;  bool printVersion=false;
	char *displayname=NULL;

	try
	{
		getEnvironment();

		if(argc>1) for(i=1; i<argc; i++)
		{
			if(!strnicmp(argv[i], "-h", 2) || !stricmp(argv[i], "-?"))
			{
				usage(argv[0]);  exit(1);
			}
			#ifdef USESSL
			if(!stricmp(argv[i], "-sslonly")) { doSSL=true;  doNonSSL=false; }
			if(!stricmp(argv[i], "-nossl")) { doSSL=false;  doNonSSL=true; }
			if(!stricmp(argv[i], "-sslPort"))
			{
				if(i<argc-1) sslPort=(unsigned short)atoi(argv[++i]);
			}
			#endif
			if(!stricmp(argv[i], "-v")) printVersion=true;
			if(!stricmp(argv[i], "-force")) force=true;
			if(!stricmp(argv[i], "-detach")) detach=true;
			if(!stricmp(argv[i], "-kill")) { killproc(true);  return 0; }
			if(!stricmp(argv[i], "-killall")) { killproc(false);  return 0; }
			if(!stricmp(argv[i], "-port"))
			{
				if(i<argc-1) port=(unsigned short)atoi(argv[++i]);
			}
			if(!stricmp(argv[i], "-l"))
			{
				if(i<argc-1)
				{
					logFile=argv[++i];
					FILE *f=fopen(logFile, "a");
					if(!f)
					{
						vglout.println("Could not open log file %s", logFile);
						_throwunix();
					}
					else fclose(f);
				}
			}
			if(!stricmp(argv[i], "-x")) drawMethod=RR_DRAWX11;
			if(!stricmp(argv[i], "-gl")) drawMethod=RR_DRAWOGL;
			if(!stricmp(argv[i], "-display"))
			{
				if(i<argc-1) displayname=argv[++i];
			}
		}

		if(!child)
		{
			vglout.println("\n%s Client %d-bit v%s (Build %s)", __APPNAME,
				(int)sizeof(size_t)*8, __VERSION, __BUILD);
			if(printVersion) return 0;
			if(detach) daemonize();
		}

		start(displayname);
	}
	catch(Error &e)
	{
		vglout.println("%s-- %s", e.getMethod(), e.getMessage());  exit(1);
	}

	return 0;
}


void start(char *displayname)
{
	VGLTransReceiver *receiver=NULL;
	Atom portAtom=None;  unsigned short actualPort=0;
	#ifdef USESSL
	VGLTransReceiver *sslReceiver=NULL;
	Atom sslPortAtom=None;  unsigned short actualSSLPort=0;
	#endif
	bool newListener=false;

	if(!XInitThreads()) { vglout.println("XInitThreads() failed");  return; }

	signal(SIGINT, handler);
	signal(SIGTERM, handler);
	signal(SIGHUP, handler);
	XSetErrorHandler(xhandler);

	start:

	try
	{
		restart=false;

		if((maindpy=XOpenDisplay(displayname))==NULL)
			_throw("Could not open display");

		#ifdef USESSL
		if(doSSL)
		{
			if(!force) actualSSLPort=instanceCheckSSL(maindpy);
			if(actualSSLPort==0)
			{
				_newcheck(sslReceiver=new VGLTransReceiver(true, drawMethod));
				if(sslPort==0)
				{
					bool success=false;  unsigned short i=RR_DEFAULTSSLPORT;
					do
					{
						try
						{
							sslReceiver->listen(i);
							success=true;
						}
						catch(...)
						{
							success=false;  if(i==0) throw;
						}
						i++;
						if(i>4299) i=4200;  if(i==RR_DEFAULTPORT) i=0;
					} while(!success);
				}
				else sslReceiver->listen(sslPort);
				vglout.println("Listening for SSL connections on port %d",
					actualSSLPort=sslReceiver->getPort());
				if((sslPortAtom=XInternAtom(maindpy, "_VGLCLIENT_SSLPORT",
					False))==None)
					_throw("Could not get _VGLCLIENT_SSLPORT atom");
				XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
					sslPortAtom, XA_INTEGER, 16, PropModeReplace,
					(unsigned char *)&actualSSLPort, 1);
				newListener=true;
			}
		}
		if(doNonSSL)
		#endif
		{
			if(!force) actualPort=instanceCheck(maindpy);
			if(actualPort==0)
			{
				_newcheck(receiver=new VGLTransReceiver(false, drawMethod));
				if(port==0)
				{
					bool success=false;  unsigned short i=RR_DEFAULTPORT;
					do
					{
						try
						{
							receiver->listen(i);
							success=true;
						}
						catch(...)
						{
							success=false;  if(i==0) throw;
						}
						i++;  if(i==RR_DEFAULTSSLPORT) i++;
						if(i>4299) i=4200;  if(i==RR_DEFAULTPORT) i=0;
					} while(!success);
				}
				else receiver->listen(port);
				vglout.println("Listening for unencrypted connections on port %d",
					actualPort=receiver->getPort());
				if((portAtom=XInternAtom(maindpy, "_VGLCLIENT_PORT", False))==None)
					_throw("Could not get _VGLCLIENT_PORT atom");
				XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
					portAtom, XA_INTEGER, 16, PropModeReplace,
					(unsigned char *)&actualPort, 1);
				newListener=true;
			}
		}

		if(logFile && newListener)
		{
			vglout.println("Redirecting output to %s", logFile);
			vglout.logTo(logFile);
		}

		if(child)
		{
			printf("%d\n", actualPort);
			fclose(stdin);  fclose(stdout);  fclose(stderr);
		}

		if(!newListener)
		{
			if(maindpy) { XCloseDisplay(maindpy);  maindpy=NULL; }
			return;
		}

		restart=true;
		while(!deadYet)
		{
			if(XPending(maindpy)>0) {}
			usleep(100000);
		}

	}
	catch(Error &e)
	{
		vglout.println("%s-- %s", e.getMethod(), e.getMessage());
	}

	if(receiver) { delete receiver;  receiver=NULL; }
	#ifdef USESSL
	if(sslReceiver) { delete sslReceiver;  sslReceiver=NULL; }
	if(maindpy && sslPortAtom!=None)
	{
		XDeleteProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			sslPortAtom);
		sslPortAtom=None;
	}
	#endif
	if(maindpy && portAtom!=None)
	{
		XDeleteProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			portAtom);
		portAtom=None;
	}
	if(maindpy) { XCloseDisplay(maindpy);  maindpy=NULL; }

	if(restart)
	{
		deadYet=restart=false;  actualPort=0;
		#ifdef USESSL
		actualSSLPort=0;
		#endif
		goto start;
	}
}

