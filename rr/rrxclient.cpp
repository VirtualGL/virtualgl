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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include "rrdisplayserver.h"
#include "x11err.h"
#include "xdk-sym.h"

rrdisplayserver *rrdpy=NULL;
bool restart=true, quiet=false;
rrevent death;
int port=-1;
#ifdef USESSL
rrdisplayserver *rrssldpy=NULL;
int sslport=-1;
bool ssl=true, nonssl=true;
#endif
int service=0;
#ifdef SUNOGL
#include <GL/glx.h>
int drawmethod=RR_DRAWOGL;
#else
int drawmethod=RR_DRAWX11;
#endif

void start(int, char **);

#ifdef _WIN32
#define SERVICENAME "VGLClient"
#define SERVICEFULLNAME __APPNAME" Client"

SERVICE_STATUS status;
SERVICE_STATUS_HANDLE statushnd=0;

void install_service(void);
void remove_service(void);
void WINAPI control_service(DWORD);
void WINAPI start_service(DWORD, LPTSTR *);
void EventLog(char *, int);

SERVICE_TABLE_ENTRY servicetable[] =
{
	{NULL, (LPSERVICE_MAIN_FUNCTION)start_service},
	{NULL, NULL}
};
#endif


void handler(int type)
{
	restart=false;
	death.signal();
}


// This does nothing except prevent Xlib from exiting the program so we can trap X11 errors
int xhandler(Display *dpy, XErrorEvent *xe)
{
	const char *temp=NULL;  char errmsg[257];
	errmsg[0]=0;
	XGetErrorText(dpy, xe->error_code, errmsg, 256);
	rrout.print("X11 Error: ");
	if((temp=x11error(xe->error_code))!=NULL && stricmp(temp, "Unknown error code"))
		rrout.print("%s ", temp);
	rrout.println("%s", errmsg);
	return 1;
}


void usage(char *progname)
{
	fprintf(stderr, "\nUSAGE: %s [-h|-?] [-port <port>] [-l <file>] [-v]", progname);
	#ifdef USESSL
	fprintf(stderr, "\n       [-sslport <port>] [-sslonly] [-nossl]");
	#endif
	fprintf(stderr, " [-x] [-gl]");
	fprintf(stderr, "\n\n-h or -? = This help screen\n");
	fprintf(stderr, "-port = TCP port to use for unencrypted connections (default = %d)\n", RR_DEFAULTPORT);
	#ifdef USESSL
	fprintf(stderr, "-sslport = TCP port to use for encrypted connections (default = %d)\n", RR_DEFAULTSSLPORT);
	fprintf(stderr, "-sslonly = Only allow encrypted connections\n");
	fprintf(stderr, "-nossl = Only allow unencrypted connections\n");
	#endif
	fprintf(stderr, "-l = Redirect all output to <file>\n");
	fprintf(stderr, "-v = Display version information\n");
	fprintf(stderr, "-x = Use X11 drawing  %s\n", drawmethod==RR_DRAWX11?"(default)":" ");
	fprintf(stderr, "-gl = Use OpenGL drawing  %s\n", drawmethod==RR_DRAWOGL?"(default)":" ");
	#ifdef _WIN32
	fprintf(stderr, "-install = Install %s client as a service\n", __APPNAME);
	fprintf(stderr, "-remove = Remove %s client service\n", __APPNAME);
	#endif
}


int main(int argc, char *argv[])
{
	int i;  bool printversion=false;

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
		if(!stricmp(argv[i], "-port"))
		{
			if(i<argc-1) port=(unsigned short)atoi(argv[++i]);
		}
		if(!stricmp(argv[i], "-l"))
		{
			if(i<argc-1) rrout.logto(argv[++i]);
		}
		if(!stricmp(argv[i], "--service")) service=1;
		if(!stricmp(argv[i], "-x")) drawmethod=RR_DRAWX11;
		if(!stricmp(argv[i], "-gl")) drawmethod=RR_DRAWOGL;
	}
	if(port<0) port=RR_DEFAULTPORT;
	#ifdef USESSL
	if(sslport<0) sslport=RR_DEFAULTSSLPORT;
	#endif

	#ifdef _WIN32
	if(argc>1) for(i=1; i<argc; i++)
	{
		if(!stricmp(argv[i], "-install")) {install_service();  exit(0);}
		if(!stricmp(argv[i], "-remove")) {remove_service();  exit(0);}
	}
	#endif

	rrout.println("\n%s Client v%s (Build %s)", __APPNAME, __VERSION, __BUILD);
	if(printversion) return 0;

	#ifdef _WIN32
	if(service)
	{
		servicetable[0].lpServiceName=(char *)SERVICENAME;
		if(!StartServiceCtrlDispatcher(servicetable))
			EventLog("Could not start service", 1);
		exit(0);
	}
	#else
	if(service)
	{
		int err=fork();
		if(err<0) exit(1);
		if(err>0) exit(0);
	}
	#endif

	start(argc, argv);

	}
	catch(rrerror &e)
	{
		rrout.println("%s--\n%s", e.getMethod(), e.getMessage());  exit(1);
	}
	#ifdef XDK
	__vgl_unloadsymbols();
	#endif
	return 0;
}


void start(int argc, char **argv)
{
	if(!XInitThreads()) {rrout.println("XInitThreads() failed");  return;}
	#ifdef SUNOGL
	if(!glXInitThreadsSUN()) _throw("glXInitThreadsSUN() failed");
	#endif

	signal(SIGINT, handler);
	XSetErrorHandler(xhandler);

	try {

	death.wait();

	start:

	#ifdef USESSL
	if(ssl)
	{
		if(!(rrssldpy=new rrdisplayserver(sslport, true, drawmethod)))
			_throw("Could not initialize listener");
		rrout.println("Listening for SSL connections on port %d", sslport);
	}
	if(nonssl)
	#endif
	{
		if(!(rrdpy=new rrdisplayserver(port, false, drawmethod)))
			_throw("Could not initialize listener");
		rrout.println("Listening for unencrypted connections on port %d", port);
	}
	#ifdef _WIN32
	if(service)
	{
		status.dwCurrentState=SERVICE_RUNNING;
		status.dwControlsAccepted=SERVICE_ACCEPT_STOP;
		if(!SetServiceStatus(statushnd, &status))
			EventLog("Could not set service status", 1);
	}
	#endif
	death.wait();
	if(rrdpy) {delete rrdpy;  rrdpy=NULL;}
	#ifdef USESSL
	if(rrssldpy) {delete rrssldpy;  rrssldpy=NULL;}
	#endif
	if(restart) goto start;

	} catch(rrerror &e)
	{
		rrout.println("%s--\n%s", e.getMethod(), e.getMessage());
	}
}


#ifdef _WIN32
void install_service(void)
{
	SC_HANDLE servicehnd, managerhnd=NULL;
	char imagePath[512], args[512];

	try {

	if(!GetModuleFileName(NULL, imagePath, 512))
		_throww32();
	sprintf(args, " --service -l %%systemdrive%%\\%s.log -p %d", SERVICENAME, port);
	strcat(imagePath, args);
	if(!(managerhnd=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))
		_throww32();
	if(!(servicehnd=CreateService(managerhnd, SERVICENAME, SERVICEFULLNAME,
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
		SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, imagePath, NULL, NULL, NULL, NULL,
		NULL)))
		_throww32();
	if(!quiet) MessageBox(NULL, "Service Installed Successfully", "Success", MB_OK);

	} catch (rrerror &e)
	{
		if(!quiet) MessageBox(NULL, e.getMessage(), "Service Install Error", MB_OK|MB_ICONERROR);
		exit(1);
	}

	if(managerhnd) CloseServiceHandle(managerhnd);
}


void remove_service(void)
{
	SC_HANDLE servicehnd, managerhnd=NULL;

	try {

	if(!(managerhnd=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))
		_throww32();

	if(!(servicehnd=OpenService(managerhnd, SERVICENAME, SERVICE_ALL_ACCESS)))
		_throww32();
	if(ControlService(servicehnd, SERVICE_CONTROL_STOP, &status))
	{
		rrout.println("Stopping service ...");
		Sleep(1000);
		while(QueryServiceStatus(servicehnd, &status))
		{
			if(status.dwCurrentState==SERVICE_STOP_PENDING)
			{
				rrout.print(".");
				Sleep(1000);
			}
			else break;
			if(status.dwCurrentState==SERVICE_STOPPED) rrout.println(" Done.");
			else rrout.println(" Could not stop service.");
		}
	}
	if(!DeleteService(servicehnd))
		_throww32();
	if(!quiet) MessageBox(NULL, "Service Removed Successfully", "Success", MB_OK);

	} catch (rrerror &e)
	{
		if(!quiet) MessageBox(NULL, e.getMessage(), "Service Install Error", MB_OK|MB_ICONERROR);
		exit(1);
	}

	if(managerhnd) CloseServiceHandle(managerhnd);
}


void WINAPI start_service(DWORD dwArgc, LPTSTR *lpszArgv)
{
	memset(&status, 0, sizeof(status));
	status.dwServiceType=SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState=SERVICE_START_PENDING;
	status.dwControlsAccepted=0;

	if(!(statushnd=RegisterServiceCtrlHandler(SERVICENAME, control_service)))
		goto finally;
	if(!SetServiceStatus(statushnd, &status))
		EventLog("Could not set service status", 1);
	start(dwArgc, lpszArgv);

	finally:
	if(statushnd)
	{
		status.dwCurrentState=SERVICE_STOPPED;
		status.dwControlsAccepted=0;
		SetServiceStatus(statushnd, &status);
	}
}


void WINAPI control_service(DWORD code)
{
	if(code==SERVICE_CONTROL_STOP)
	{
		status.dwCurrentState=SERVICE_STOP_PENDING;
		status.dwControlsAccepted=0;
		if(!SetServiceStatus(statushnd, &status))
			EventLog("Could not set service status", 1);
		restart=false;
		death.signal();
	}
}


void EventLog(char *message, int error)
{
	HANDLE eventsource;
	char *strings[1];
	if(!(eventsource=RegisterEventSource(NULL, SERVICENAME))) return;
	strings[0]=message;
	ReportEvent(eventsource, error? EVENTLOG_ERROR_TYPE:EVENTLOG_INFORMATION_TYPE,
		0, 0, NULL, 1, 0, (LPCTSTR*)strings, NULL);
	DeregisterEventSource(eventsource);
}

#endif
