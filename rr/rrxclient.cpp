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
#include "hputil.h"
#include <pthread.h>
#include "rr.h"
#include "x11err.h"

RRDisplay rrdpy=NULL;
int restart=1, ssl=0;
pthread_mutex_t deadmutex;
int port=-1;
FILE *log=NULL;  char *logfile=NULL;
int service=0;

void start(int, char **);

#ifdef _WIN32
#define _SERVICENAME "RRXClient"
#define _SSLSERVICENAME "RRXClient_SSL"
#define SERVICENAME (ssl? _SSLSERVICENAME:_SERVICENAME)
#define _SERVICEFULLNAME __APPNAME" Client"
#define _SSLSERVICEFULLNAME __APPNAME" Secure Client"
#define SERVICEFULLNAME (ssl? _SSLSERVICEFULLNAME:_SERVICEFULLNAME)

SERVICE_STATUS status;
SERVICE_STATUS_HANDLE statushnd=NULL;

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
	restart=0;
	pthread_mutex_unlock(&deadmutex);
}


// This does nothing except prevent Xlib from exiting the program so we can trap X11 errors
int xhandler(Display *dpy, XErrorEvent *xe)
{
	char *temp=NULL;
	if((temp=x11error(xe->error_code))!=NULL && stricmp(temp, "Unknown error code"))
		hpprintf("X11 Error: %s\n", temp);
	return 0;
}


void usage(char *progname)
{
	hpprintf("\nUSAGE: %s [-h|-?] [-v] [-s] [-p<port>] [-d<display>]\n", progname);
	hpprintf("-h or -? = This help screen\n");
	hpprintf("-s = Use Secure Sockets Layer\n");
	hpprintf("-p = Set base port for listener (default is %d)\n", RR_DEFAULTPORT);
	hpprintf("-v = Print version\n");
	#ifdef _WIN32
	hpprintf("-install = Install RRXClient as a service\n");
	hpprintf("-remove = Remove RRXClient service\n");
	#endif
}


int main(int argc, char *argv[])
{
	int i;

	hputil_log(1, 0);
	hputil_logto(stderr);

	if(argc>1) for(i=1; i<argc; i++)
	{
		if(!strnicmp(argv[i], "-h", 2) || !stricmp(argv[i], "-?"))
			{usage(argv[0]);  exit(1);}
		if(!stricmp(argv[i], "-v"))
		{
			hpprintf("%s Display Server v%s.%s\n", __APPNAME, __MAJVER, __MINVER);
			exit(0);
		}
		if(!stricmp(argv[i], "-s")) ssl=1;
		if(argv[i][0]=='-' && toupper(argv[i][1])=='P' && strlen(argv[i])>2)
			{port=(unsigned short)atoi(&argv[i][2]);}
		if(argv[i][0]=='-' && toupper(argv[i][1])=='L' && strlen(argv[i])>2)
		{
			if((log=fopen(&argv[i][2], "wb"))!=NULL)
			{
				hputil_logto(log);  logfile=&argv[i][2];
			}
		}
		if(!stricmp(argv[i], "--service")) service=1;
	}
	if(port<0) port=ssl?RR_DEFAULTPORT+1:RR_DEFAULTPORT;

	#ifdef _WIN32
	if(argc>1) for(i=1; i<argc; i++)
	{
		if(!stricmp(argv[i], "-install")) {install_service();  exit(0);}
		if(!stricmp(argv[i], "-remove")) {remove_service();  exit(0);}
	}

	if(service)
	{
		servicetable[0].lpServiceName=SERVICENAME;
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

	return 0;
}


void start(int argc, char **argv)
{
	if(!XInitThreads()) {hpprintf("XInitThreads() failed\n");  return;}
	signal(SIGINT, handler);
	XSetErrorHandler(xhandler);

	if(pthread_mutex_init(&deadmutex, NULL)==-1) {perror("Could not create mutex");  return;}
	if(pthread_mutex_lock(&deadmutex)==-1) {perror("Could not lock mutex");  return;}

	start:
	if(!(rrdpy=RRInitDisplay(port, ssl)))
	{
		hpprintf("Could not initialize display server:\n%s\n", RRGetError());
		return;
	}
	hpprintf("Server active on port %d\n", port);
	#ifdef _WIN32
	if(service)
	{
		status.dwCurrentState=SERVICE_RUNNING;
		status.dwControlsAccepted=SERVICE_ACCEPT_STOP;
		if(!SetServiceStatus(statushnd, &status))
			EventLog("Could not set service status", 1);
	}
	#endif
	if(pthread_mutex_lock(&deadmutex)==-1) {perror("Could not lock mutex");  return;}
	if(rrdpy) {RRTerminateDisplay(rrdpy);  rrdpy=0;}
	if(restart) goto start;
}


#ifdef _WIN32
void install_service(void)
{
	SC_HANDLE servicehnd, managerhnd;
	char imagePath[512], args[512];

	if(!GetModuleFileName(NULL, imagePath, 512))
	{
		hputil_perror("Could not install service");
		exit(1);
	}
	sprintf(args, " --service -l%%systemdrive%%\\%s.log -p%d", SERVICENAME, port);
	if(ssl) strcat(args, " -s");
	strcat(imagePath, args);
	if(!(managerhnd=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))
	{
		hputil_perror("Could not open service control manager");
		exit(1);
	}
	if(!(servicehnd=CreateService(managerhnd, SERVICENAME, SERVICEFULLNAME,
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
		SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, imagePath, NULL, NULL, NULL, NULL,
		NULL)))
	{
		hputil_perror("Could not install service");
		CloseServiceHandle(managerhnd);
		exit(1);
	}
	hpprintf("Service installed successfully.\n");
	CloseServiceHandle(managerhnd);
}


void remove_service(void)
{
	SC_HANDLE servicehnd, managerhnd;

	if(!(managerhnd=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))
	{
		hputil_perror("Could not open service control manager");
		exit(1);
	}
	if(!(servicehnd=OpenService(managerhnd, SERVICENAME, SERVICE_ALL_ACCESS)))
	{
		hputil_perror("Could not remove service");
		CloseServiceHandle(managerhnd);
		exit(1);
	}
	if(ControlService(servicehnd, SERVICE_CONTROL_STOP, &status))
	{
		hpprintf("Stopping service ...\n");
		Sleep(1000);
		while(QueryServiceStatus(servicehnd, &status))
		{
			if(status.dwCurrentState==SERVICE_STOP_PENDING)
			{
				hpprintf(".");
				Sleep(1000);
			}
			else break;
			if(status.dwCurrentState==SERVICE_STOPPED) hpprintf(" Done.\n");
			else hpprintf(" Could not stop service.\n");
		}
	}
	if(!DeleteService(servicehnd))
	{
		hputil_perror("Could not remove service");
		CloseServiceHandle(managerhnd);
		exit(1);
	}
	hpprintf("Service removed successfully.\n");
	CloseServiceHandle(managerhnd);
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
		restart=0;
		pthread_mutex_unlock(&deadmutex);
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
