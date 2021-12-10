// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011-2012, 2014, 2017-2019, 2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

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

using namespace util;
using namespace client;


bool restart;
bool deadYet;
unsigned short port = 0;
bool ipv6 = false;
int drawMethod = RR_DRAWAUTO;
Display *maindpy = NULL;
bool detach = false, force = false, child = false;
char *logFile = NULL;

int start(char *);


extern "C" {

void handler(int type)
{
	restart = false;
	if(type == SIGHUP) restart = true;
	deadYet = true;
}


// This does nothing except prevent Xlib from exiting the program, so we can
// trap X11 errors
int xhandler(Display *dpy, XErrorEvent *xe)
{
	const char *temp = NULL;  char errmsg[257];  bool printWarnings = false;
	char *env = NULL;

	if((env = getenv("VGL_VERBOSE")) != NULL && strlen(env) > 0
		&& !strncmp(env, "1", 1)) printWarnings = true;
	if(printWarnings)
	{
		errmsg[0] = 0;
		XGetErrorText(dpy, xe->error_code, errmsg, 256);
		vglout.print("X11 Error: ");
		if((temp = x11error(xe->error_code)) != NULL
			&& stricmp(temp, "Unknown error code"))
			vglout.print("%s ", temp);
		vglout.println("%s", errmsg);
	}
	return 1;
}

}  // extern "C"


void killproc(bool userOnly)
{
	#ifdef __APPLE__

	unsigned char *buf = NULL;

	if(!userOnly && getuid() != 0) THROW("Only root can do that");

	try
	{
		int mib_all[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
		int mib_user[4] = { CTL_KERN, KERN_PROC, KERN_PROC_UID, getuid() };
		size_t len = 0;

		TRY_UNIX(sysctl(userOnly ? mib_user : mib_all, 4, NULL, &len, NULL, 0));
		if(len < sizeof(kinfo_proc)) THROW("Process table is empty");
		buf = new unsigned char[len];
		TRY_UNIX(sysctl(userOnly ? mib_user : mib_all, 4, buf, &len, NULL, 0));
		int nprocs = len / sizeof(kinfo_proc);
		kinfo_proc *kp = (kinfo_proc *)buf;

		for(int i = 0; i < nprocs; i++)
		{
			int pid = kp[i].kp_proc.p_pid;
			if(!strcmp(kp[i].kp_proc.p_comm, "vglclient") && pid != getpid())
			{
				vglout.println("Terminating vglclient process %d", pid);
				kill(pid, SIGTERM);
			}
		}
		delete [] buf;  buf = NULL;
	}
	catch(...)
	{
		delete [] buf;
		throw;
	}

	#else

	DIR *procdir = NULL;  struct dirent *dent = NULL;  int fd = -1;

	if(!userOnly && getuid() != 0) THROW("Only root can do that");

	try
	{
		if(!(procdir = opendir("/proc"))) THROW_UNIX();
		while((dent = readdir(procdir)) != NULL)
		{
			if(dent->d_name[0] >= '1' && dent->d_name[0] <= '9')
			{
				char temps[1024];  struct stat fsbuf;
				int pid = atoi(dent->d_name);

				if(pid == getpid()) continue;
				#ifdef sun
				sprintf(temps, "/proc/%s/psinfo", dent->d_name);
				#else
				sprintf(temps, "/proc/%s/stat", dent->d_name);
				#endif
				if((fd = open(temps, O_RDONLY)) == -1) continue;
				if(fstat(fd, &fsbuf) != -1 && (fsbuf.st_uid == getuid() || !userOnly))
				{
					int bytes = 0;

					#ifdef sun

					psinfo_t psinfo;
					if((bytes = read(fd, &psinfo, sizeof(psinfo_t))) == sizeof(psinfo_t)
						&& psinfo.pr_nlwp != 0)
					{
						if(!strcmp(psinfo.pr_fname, "vglclient"))
						{
							vglout.println("Terminating vglclient process %d", pid);
							kill(pid, SIGTERM);
						}
					}

					#else

					char *ptr = NULL;
					if((bytes = read(fd, temps, 1023)) > 0 && bytes < 1024)
					{
						temps[bytes] = '\0';
						if((ptr = strchr(temps, '(')) != NULL)
						{
							ptr++;  char *ptr2 = strchr(ptr, ')');
							if(ptr2)
							{
								*ptr2 = '\0';
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
				close(fd);  fd = -1;
			}
		}
		closedir(procdir);  procdir = NULL;
	}
	catch(...)
	{
		if(fd) close(fd);
		if(procdir) closedir(procdir);
		throw;
	}

	#endif
}


void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-display <d> = The X display to which to draw the rendered frames received from\n");
	fprintf(stderr, "               the VirtualGL Faker\n");
	fprintf(stderr, "               (default: read from the DISPLAY environment variable)\n");
	fprintf(stderr, "-port <p> = TCP port to use for connections from the VirtualGL Faker\n");
	fprintf(stderr, "            (default: automatically select a free port)\n");
	fprintf(stderr, "-ipv6 = Use IPv6 sockets\n");
	fprintf(stderr, "-detach = Detach from console (used by vglconnect)\n");
	fprintf(stderr, "-force = Force the VirtualGL Client to run, even if there is already another\n");
	fprintf(stderr, "         instance running on the same X display (use with caution)\n");
	fprintf(stderr, "-kill = Kill all detached VirtualGL Client processes running under this user ID\n");
	fprintf(stderr, "-l = Redirect all output to <file>\n");
	fprintf(stderr, "-v = Display version information\n");
	fprintf(stderr, "-x = Use X11 drawing (default)\n");
	fprintf(stderr, "-gl = Use OpenGL drawing\n\n");
	exit(1);
}


void daemonize(void)
{
	int err;
	if(getppid() == 1) return;
	err = fork();
	if(err < 0) exit(-1);
	if(err > 0) exit(0);
	child = true;
	setsid();
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
}


unsigned short instanceCheck(Display *dpy)
{
	Atom atom = None;  unsigned short p = 0;
	if((atom = XInternAtom(dpy, "_VGLCLIENT_PORT", True)) != None)
	{
		unsigned char *prop = NULL;  unsigned long n = 0, bytesLeft = 0;
		int actualFormat = 0;  Atom actualType = None;
		if(XGetWindowProperty(dpy, DefaultRootWindow(dpy), atom, 0, 1, False,
				XA_INTEGER, &actualType, &actualFormat, &n, &bytesLeft,
				&prop) == Success
			&& n >= 1 && actualFormat == 16 && actualType == XA_INTEGER && prop)
			p = *(unsigned short *)prop;
		if(prop) XFree(prop);
		if(p != 0)
		{
			vglout.println("vglclient is already running on this X display and accepting connections on");
			vglout.println("   port %d.", p);
		}
	}
	return p;
}


void getEnvironment(void)
{
	char *env = NULL;  int temp;
	if((env = getenv("VGLCLIENT_DRAWMODE")) != NULL && strlen(env) > 0)
	{
		if(!strnicmp(env, "o", 1)) drawMethod = RR_DRAWOGL;
		else if(!strncmp(env, "x", 1)) drawMethod = RR_DRAWX11;
	}
	if((env = getenv("VGLCLIENT_LOG")) != NULL && strlen(env) > 0)
	{
		logFile = env;
		FILE *f = fopen(logFile, "a");
		if(!f)
		{
			vglout.println("Could not open log file %s", logFile);
			THROW_UNIX();
		}
		else fclose(f);
	}
	if((env = getenv("VGLCLIENT_PORT")) != NULL && strlen(env) > 0
		&& (temp = atoi(env)) > 0 && temp < 65536)
		port = (unsigned short)temp;
	if((env = getenv("VGLCLIENT_IPV6")) != NULL && strlen(env) > 0
		&& (temp = atoi(env)) == 1)
		ipv6 = true;
}


int main(int argc, char *argv[])
{
	int i;  bool printVersion = false;
	char *displayname = NULL;

	try
	{
		getEnvironment();

		if(argc > 1) for(i = 1; i < argc; i++)
		{
			if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
			else if(!stricmp(argv[i], "-ipv6")) ipv6 = true;
			else if(!stricmp(argv[i], "-v")) printVersion = true;
			else if(!stricmp(argv[i], "-force")) force = true;
			else if(!stricmp(argv[i], "-detach")) detach = true;
			else if(!stricmp(argv[i], "-kill")) { killproc(true);  return 0; }
			else if(!stricmp(argv[i], "-killall")) { killproc(false);  return 0; }
			else if(!stricmp(argv[i], "-port") && i < argc - 1)
			{
				port = (unsigned short)atoi(argv[++i]);
			}
			else if(!stricmp(argv[i], "-l") && i < argc - 1)
			{
				logFile = argv[++i];
				FILE *f = fopen(logFile, "a");
				if(!f)
				{
					vglout.println("Could not open log file %s", logFile);
					THROW_UNIX();
				}
				else fclose(f);
			}
			else if(!stricmp(argv[i], "-x")) drawMethod = RR_DRAWX11;
			else if(!stricmp(argv[i], "-gl")) drawMethod = RR_DRAWOGL;
			else if(!stricmp(argv[i], "-display") && i < argc - 1)
			{
				displayname = argv[++i];
			}
			else usage(argv);
		}

		if(!child)
		{
			vglout.println("\n%s Client %d-bit v%s (Build %s)", __APPNAME,
				(int)sizeof(size_t) * 8, __VERSION, __BUILD);
			if(printVersion) return 0;
			if(detach) daemonize();
		}

		if(start(displayname) < 0) return -1;
	}
	catch(std::exception &e)
	{
		vglout.println("%s-- %s", GET_METHOD(e), e.what());  exit(1);
	}

	return 0;
}


int start(char *displayname)
{
	VGLTransReceiver *receiver = NULL;
	Atom portAtom = None;  unsigned short actualPort = 0;
	bool newListener = false;
	int retval = 0;

	if(!XInitThreads()) { vglout.println("XInitThreads() failed");  return -1; }

	signal(SIGINT, handler);
	signal(SIGTERM, handler);
	signal(SIGHUP, handler);
	XSetErrorHandler(xhandler);

	start:

	try
	{
		restart = false;

		if((maindpy = XOpenDisplay(displayname)) == NULL)
			THROW("Could not open display");

		if(!force) actualPort = instanceCheck(maindpy);
		if(actualPort == 0)
		{
			receiver = new VGLTransReceiver(ipv6, drawMethod);
			if(port == 0)
			{
				bool success = false;  unsigned short i = RR_DEFAULTPORT;
				do
				{
					try
					{
						receiver->listen(i);
						success = true;
					}
					catch(...)
					{
						success = false;  if(i == 0) throw;
					}
					i++;
					if(i > 4299) i = 4200;
					if(i == RR_DEFAULTPORT) i = 0;
				} while(!success);
			}
			else receiver->listen(port);
			vglout.println("Listening for connections on port %d%s",
				actualPort = receiver->getPort(), ipv6 ? " [IPv6 enabled]" : "");
			if((portAtom = XInternAtom(maindpy, "_VGLCLIENT_PORT", False)) == None)
				THROW("Could not get _VGLCLIENT_PORT atom");
			XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
				portAtom, XA_INTEGER, 16, PropModeReplace,
				(unsigned char *)&actualPort, 1);
			newListener = true;
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
			if(maindpy) { XCloseDisplay(maindpy);  maindpy = NULL; }
			return 0;
		}

		restart = true;
		while(!deadYet)
		{
			if(XPending(maindpy) > 0) {}
			usleep(100000);
		}

	}
	catch(std::exception &e)
	{
		vglout.println("%s-- %s", GET_METHOD(e), e.what());
		retval = -1;
	}

	delete receiver;  receiver = NULL;
	if(maindpy && portAtom != None)
	{
		XDeleteProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
			portAtom);
		portAtom = None;
	}
	if(maindpy) { XCloseDisplay(maindpy);  maindpy = NULL; }

	if(restart)
	{
		deadYet = restart = false;  actualPort = 0;
		goto start;
	}

	return retval;
}
