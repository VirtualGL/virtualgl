/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2011-2012 D. R. Commander
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
#include "vgltransreceiver.h"
#include "rrutil.h"
#include "x11err.h"
#include "xdk-sym.h"
#include <X11/Xatom.h>
#ifdef _MSC_VER
#define snprintf _snprintf
#endif
#ifdef _WIN32
#include <psapi.h>
#else
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
#endif


bool restart;
bool deadyet;
unsigned short port=0;
#ifdef USESSL
unsigned short sslport=0;
bool ssl=true, nonssl=true;
#endif
int drawmethod=RR_DRAWAUTO;
Display *maindpy=NULL;
bool detach=false, force=false, child=false;
char *logfile=NULL;

void start(char *);


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

#ifdef _WIN32
BOOL consolehandler(DWORD type)
{ 
	switch(type)
	{
		case CTRL_C_EVENT: 
		case CTRL_CLOSE_EVENT: 
		case CTRL_BREAK_EVENT: 
		case CTRL_LOGOFF_EVENT: 
		case CTRL_SHUTDOWN_EVENT: 
			restart=false;  deadyet=true;  return TRUE;
		default: 
			return FALSE; 
  } 
} 
#endif
 
} // extern "C"


void killproc(bool useronly)
{
	#ifdef _WIN32
	DWORD pid[1024], bytes;
	int nps=0, nmod=0, i, j;
	char filename[MAX_PATH], *ptr;
	HMODULE mod[1024];  HANDLE ph=NULL, token=0;

	if(!useronly)
	{
		tryw32(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
			&token));
		errifnot(token);
		TOKEN_PRIVILEGES tp;  LUID luid;
		if(!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) _throww32();
		tp.PrivilegeCount=1;
		tp.Privileges[0].Luid=luid;
		tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
		if(!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
			NULL, NULL)) _throww32();
	}

	try
	{
		tryw32(EnumProcesses(pid, 1024, &bytes));
		nps=bytes/sizeof(DWORD);
		if(nps) for(i=0; i<nps; i++)
		{
			if(pid[i]==GetCurrentProcessId()) continue;
			if(!(ph=OpenProcess(PROCESS_TERMINATE|PROCESS_QUERY_INFORMATION
				|PROCESS_VM_READ, FALSE, pid[i]))) continue;
			if(EnumProcessModules(ph, mod, 1024, &bytes))
			{
				nmod=bytes/sizeof(DWORD);
				if(nmod) for(j=0; j<nmod; j++)
				{
					if(GetModuleFileNameEx(ph, mod[j], filename, MAX_PATH))
					{
						ptr=strrchr(filename, '\\');
						if(ptr) ptr=&ptr[1];  else ptr=filename;
						if(strlen(ptr) && !stricmp(ptr, "vglclient.exe"))
						{
							rrout.println("Terminating vglclient process %d", pid[i]);
							TerminateProcess(ph, 0);  WaitForSingleObject(ph, INFINITE);
						}
					}
				}
			}
			CloseHandle(ph);  ph=NULL;
		}
		if(token) {CloseHandle(token);  token=0;}
	}
	catch(...)
	{
		if(ph) CloseHandle(ph);
		if(token) CloseHandle(token);
		throw;
	}
	#else
	#ifdef __APPLE__
	unsigned char *buf=NULL;

	if(!useronly && getuid()!=0) _throw("Only root can do that");

	try
	{
		int mib_all[4]={CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
		int mib_user[4]={CTL_KERN, KERN_PROC, KERN_PROC_UID, getuid()};
		size_t len=0;

		tryunix(sysctl(useronly? mib_user:mib_all, 4, NULL, &len, NULL, 0));
		if(len<sizeof(kinfo_proc)) _throw("Process table is empty");
		errifnot(buf=new unsigned char[len]);
		tryunix(sysctl(useronly? mib_user:mib_all, 4, buf, &len, NULL, 0));
		int nprocs=len/sizeof(kinfo_proc);
		kinfo_proc *kp=(kinfo_proc *)buf;

		for(int i=0; i<nprocs; i++)
		{
			int pid=kp[i].kp_proc.p_pid;
			if(!strcmp(kp[i].kp_proc.p_comm, "vglclient")
				&& pid!=getpid())
			{
				rrout.println("Terminating vglclient process %d", pid);
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

	if(!useronly && getuid()!=0) _throw("Only root can do that");

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
				if(fstat(fd, &fsbuf)!=-1 && (fsbuf.st_uid==getuid() || !useronly))
				{
					int bytes=0;
					#ifdef sun
					psinfo_t psinfo;
					if((bytes=read(fd, &psinfo, sizeof(psinfo_t)))==sizeof(psinfo_t)
						&& psinfo.pr_nlwp!=0)
					{
						if(!strcmp(psinfo.pr_fname, "vglclient"))
						{
							rrout.println("Terminating vglclient process %d", pid);
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
									rrout.println("Terminating vglclient process %d", pid);
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


#ifdef _WIN32

void daemonize(void)
{
	STARTUPINFO si={0};  PROCESS_INFORMATION pi;
	char cl[MAX_PATH+1];
	snprintf(cl, MAX_PATH, "%s --child", GetCommandLine());
	if(!CreateProcess(NULL, cl, NULL, NULL, FALSE, 0, NULL, NULL,
		&si, &pi)) _throww32();
}

#else

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
		unsigned char *prop=NULL;  unsigned long n=0, bytesleft=0;
		int actualformat=0;  Atom actualtype=None;
		if(XGetWindowProperty(dpy, DefaultRootWindow(dpy), atom, 0, 1, False,
			XA_INTEGER, &actualtype, &actualformat, &n, &bytesleft,
			&prop)==Success && n>=1 && actualformat==16
			&& actualtype==XA_INTEGER && prop)
			p=*(unsigned short *)prop;
		if(prop) XFree(prop);
		if(p!=0)
		{
			rrout.println("vglclient is already running on this X display and accepting unencrypted");
			rrout.println("   connections on port %d.", p);
		}
	}
	return p;
}


void getenvironment(void)
{
	char *env=NULL;  int temp;
	if((env=getenv("VGLCLIENT_DRAWMODE"))!=NULL && strlen(env)>0)
	{
		if(!strnicmp(env, "o", 1)) drawmethod=RR_DRAWOGL;
		else if(!strncmp(env, "x", 1)) drawmethod=RR_DRAWX11;
	}
	#ifdef USESSL
	if((env=getenv("VGLCLIENT_LISTEN"))!=NULL && strlen(env)>0)
	{
		if(!strncmp(env, "n", 1)) {ssl=false;  nonssl=true;}
		else if(!strncmp(env, "s", 1)) {ssl=true;  nonssl=false;}
	}
	#endif
	if((env=getenv("VGLCLIENT_LOG"))!=NULL && strlen(env)>0)
	{
		logfile=env;
		FILE *f=fopen(logfile, "a");
		if(!f)
		{
			rrout.println("Could not open log file %s", logfile);  _throwunix();
		}
		else fclose(f);
	}
	if((env=getenv("VGLCLIENT_PORT"))!=NULL && strlen(env)>0
		&& (temp=atoi(env))>0 && temp<65536)
		port=(unsigned short)temp;
	#ifdef USESSL
	if((env=getenv("VGLCLIENT_SSLPORT"))!=NULL && strlen(env)>0
		&& (temp=atoi(env))>0 && temp<65536)
		sslport=(unsigned short)temp;
	#endif
}


int main(int argc, char *argv[])
{
	int i;  bool printversion=false;
	char *displayname=NULL;

	try {

	getenvironment();

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
		if(!stricmp(argv[i], "-kill")) {killproc(true);  return 0;}
		if(!stricmp(argv[i], "-killall")) {killproc(false);  return 0;}
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
				if(!f)
				{
					rrout.println("Could not open log file %s", logfile);  _throwunix();
				}
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
		if(!stricmp(argv[i], "--child")) {child=true;  detach=false;}
		#endif
	}

	if(!child)
	{
		rrout.println("\n%s Client %d-bit v%s (Build %s)", __APPNAME,
			(int)sizeof(size_t)*8, __VERSION, __BUILD);
		if(printversion) return 0;
		if(detach) daemonize();
	}

	#ifdef _WIN32
	if(!detach) start(displayname);
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
	vgltransreceiver *vglrecv=NULL;
  Atom port_atom=None;  unsigned short actualport=0;
	#ifdef USESSL
	vgltransreceiver *vglsslrecv=NULL;
  Atom sslport_atom=None;  unsigned short actualsslport=0;
	#endif
	bool newlistener=false;

	if(!XInitThreads()) {rrout.println("XInitThreads() failed");  return;}

	signal(SIGINT, handler);
	signal(SIGTERM, handler);
	#ifdef _WIN32
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)consolehandler, TRUE);
	#else
	signal(SIGHUP, handler);
	#endif
	XSetErrorHandler(xhandler);

	start:

	try
	{
		restart=false;

		if((maindpy=XOpenDisplay(displayname))==NULL)
			_throw("Could not open display");

		#ifdef USESSL
		if(ssl)
		{
			if(!force) actualsslport=instancecheckssl(maindpy);
			if(actualsslport==0)
			{
				if(!(vglsslrecv=new vgltransreceiver(true, drawmethod)))
					_throw("Could not initialize SSL listener");
				if(sslport==0)
				{
					bool success=false;  unsigned short i=RR_DEFAULTSSLPORT;
					do
					{
						try
						{
							vglsslrecv->listen(i);
							success=true;
						}
						catch(...) {success=false;  if(i==0) throw;}
						i++;  if(i>4299) i=4200;  if(i==RR_DEFAULTPORT) i=0;
					} while(!success);
				}
				else vglsslrecv->listen(sslport);
				rrout.println("Listening for SSL connections on port %d",
					actualsslport=vglsslrecv->port());
				if((sslport_atom=XInternAtom(maindpy, "_VGLCLIENT_SSLPORT",
					False))==None)
					_throw("Could not get _VGLCLIENT_SSLPORT atom");
				XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
					sslport_atom, XA_INTEGER, 16, PropModeReplace,
					(unsigned char *)&actualsslport, 1);
				newlistener=true;
			}
		}
		if(nonssl)
		#endif
		{
			if(!force) actualport=instancecheck(maindpy);
			if(actualport==0)
			{
				if(!(vglrecv=new vgltransreceiver(false, drawmethod)))
					_throw("Could not initialize listener");
				if(port==0)
				{
					bool success=false;  unsigned short i=RR_DEFAULTPORT;
					do
					{
						try
						{
							vglrecv->listen(i);
							success=true;
						}
						catch(...) {success=false;  if(i==0) throw;}
						i++;  if(i==RR_DEFAULTSSLPORT) i++;
						if(i>4299) i=4200;  if(i==RR_DEFAULTPORT) i=0;
					} while(!success);
				}
				else vglrecv->listen(port);
				rrout.println("Listening for unencrypted connections on port %d",
					actualport=vglrecv->port());
				if((port_atom=XInternAtom(maindpy, "_VGLCLIENT_PORT", False))==None)
					_throw("Could not get _VGLCLIENT_PORT atom");
				XChangeProperty(maindpy, RootWindow(maindpy, DefaultScreen(maindpy)),
					port_atom, XA_INTEGER, 16, PropModeReplace,
					(unsigned char *)&actualport, 1);
				newlistener=true;
			}
		}

		if(logfile && newlistener)
		{
			rrout.println("Redirecting output to %s", logfile);
			rrout.logto(logfile);
		}

		if(child)
		{
			printf("%d\n", actualport);
			#ifdef _WIN32
			FreeConsole();
			#endif
			fclose(stdin);  fclose(stdout);  fclose(stderr);
		}

		if(!newlistener)
		{
			if(maindpy) {XCloseDisplay(maindpy);  maindpy=NULL;}
			return;
		}

		restart=true;
		while(!deadyet)
		{
			if(XPending(maindpy)>0) {}
			usleep(100000);
		}

	}
	catch(rrerror &e)
	{
		rrout.println("%s-- %s", e.getMethod(), e.getMessage());
	}

	if(vglrecv) {delete vglrecv;  vglrecv=NULL;}
	#ifdef USESSL
	if(vglsslrecv) {delete vglsslrecv;  vglsslrecv=NULL;}
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
		deadyet=restart=false;  actualport=0;
		#ifdef USESSL
		actualsslport=0;
		#endif
		goto start;
	}
}
