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

// High-Performance Utility Library

#include <stdio.h>
#include "hputil.h"
#include <errno.h>
#ifdef _WIN32
 #define badarg() SetLastError(ERROR_INVALID_PARAMETER)
 #include <conio.h>
#else
 #define badarg() errno=EINVAL
 #include <sys/ipc.h>
 #include <sys/signal.h>
 #include <netdb.h>
 #include <arpa/inet.h>
 #include <netinet/tcp.h>
 #include <pthread.h>
 #ifdef linux
  #include <sys/sysinfo.h>
 #endif
 #ifdef SOLARIS
  #include <unistd.h>
 #endif
 #ifdef sgi
  #include <sys/types.h>
  #include <sys/sysmp.h>
 #endif
#endif
#ifndef INADDR_NONE
 #define INADDR_NONE ((in_addr_t) 0xffffffff)
#endif


/********************************************************************
 Error handling
 ********************************************************************/

#if defined(_WIN32)||defined(__CYGWIN__)

static char __hputil_errorstr[1024];
#define errorstrhdr "WIN32 error: "

char *hputil_strerror(void)
{
	sprintf(__hputil_errorstr, errorstrhdr);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)(&__hputil_errorstr[strlen(errorstrhdr)]), 1024-strlen(errorstrhdr)-1, NULL);
	return __hputil_errorstr;
}

#else

char *hputil_strerror(void)
{
	if(errno>=0) return strerror(errno);
	else return("errno < 0, and I don't know why");
}

#endif

static FILE *__hputil_logfile=NULL;
static int __hputil_enablelog=0;
static int __hputil_profile=0;
#ifdef _WIN32
static int __hputil_consolelog=0;
#endif

int _hpprintf (const char *format, ...);

int hputil_logto(FILE *file)
{
	if(!file)
	{
		#ifdef _WIN32
		AllocConsole();
		__hputil_consolelog=1;  hpprintf=_cprintf;
		return 0;
		#else
		badarg();  return -1;
		#endif
	}
	__hputil_logfile=file;  hpprintf=_hpprintf;
	return 0;
}

void hputil_log(int on, int profile)
{
	__hputil_enablelog=on;
	__hputil_profile=profile;
}

int _hpprintf (const char *format, ...)
{
	va_list arglist;
	if(!__hputil_enablelog) return 0;
	if(!__hputil_logfile) __hputil_logfile=stderr;
	va_start(arglist, format);
	if(__hputil_profile)
	fprintf(__hputil_logfile, "%f: ", hptime());
	vfprintf(__hputil_logfile, format, arglist);
	fflush(__hputil_logfile);
	va_end(arglist);
	return 0;
}

int (*hpprintf)(const char *, ...)=_hpprintf;


/********************************************************************
 Thread control
 ********************************************************************/

#if defined(_WIN32)||defined(__CYGWIN__)

int numprocs(void)
{
	DWORD ProcAff, SysAff, i;  int count=0;
	if(!GetProcessAffinityMask(GetCurrentProcess(), &ProcAff, &SysAff)) return(1);
	for(i=0; i<32; i++) if(ProcAff&(1<<i)) count++;
	return(count);
}

unsigned long hpthread_id(void)
{
	return GetCurrentThreadId();
}

#else

unsigned long hpthread_id(void)
{
	return (unsigned long)pthread_self();
}

#ifdef sgi
#define _SC_NPROCESSORS_CONF _SC_NPROC_CONF
#endif

int numprocs(void)
{
	long count=1;
	if((count=sysconf(_SC_NPROCESSORS_CONF))!=-1) return((int)count);
	else return(1);
}

#endif


/*******************************************************************
 High-performance timers
 *******************************************************************/

#if defined(_WIN32)||defined(__CYGWIN__)

double __hptick;

double hptime_good(void)
{
	LARGE_INTEGER Time;
	QueryPerformanceCounter(&Time);
	return((double)(Time.QuadPart)*__hptick);
}

double hptime_bad(void)
{
	return((double)GetTickCount()*__hptick);
}

void hptimer_init(void)
{
	LARGE_INTEGER Frequency;
	if(QueryPerformanceFrequency(&Frequency)!=0)
	{
		__hptick=(double)1.0/(double)(Frequency.QuadPart);
		hptime=hptime_good;		
	}
	else
	{
		__hptick=(double)0.001;
		hptime=hptime_bad;
	}
}

double hptime_notinit(void)
{
	hptimer_init();
	return hptime();
}

double (* hptime)(void)=hptime_notinit;

#else

double hptime(void)
{
	struct timeval __tv;
	gettimeofday(&__tv, (struct timezone *)NULL);
	return((double)(__tv.tv_sec)+(double)(__tv.tv_usec)*0.000001);
}

#endif


/*******************************************************************
 Network routines
 *******************************************************************/

int littleendian(void)
{
	unsigned int value=1;
	unsigned char *ptr=(unsigned char *)(&value);
	if(ptr[0]==1 && ptr[3]==0) return 1;
	else return 0;
}

#ifndef _WIN32
int closesocket(int s)
{
	int socket_type, socket_type_size=sizeof(socket_type);
	if(getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&socket_type, &socket_type_size)==SOCKET_ERROR)
		return SOCKET_ERROR;
	if(socket_type==SOCK_STREAM)
	{
		shutdown(s, 2);
	}
	if(close(s)==SOCKET_ERROR) return SOCKET_ERROR;
	return 0;
}
#endif

int hpnet_createServer(unsigned short port, int maxconn, int type)
{
	int sd, m=1;  struct sockaddr_in myaddr;
	if(maxconn<0 || (type!=SOCK_DGRAM && type!=SOCK_STREAM)) {badarg();  return SOCKET_ERROR;}

	if((sd=socket(AF_INET, type, type==SOCK_DGRAM?IPPROTO_UDP:IPPROTO_TCP))==INVALID_SOCKET)
		return SOCKET_ERROR;
	if(type==SOCK_STREAM)
	{
		if(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char*)&m, sizeof(int))==SOCKET_ERROR)
			{closesocket(sd);  return SOCKET_ERROR;}
	}
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&m, sizeof(int))==SOCKET_ERROR)
		{closesocket(sd);  return SOCKET_ERROR;}
	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family=AF_INET;
	myaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	myaddr.sin_port=htons(port);
	if(bind(sd, (struct sockaddr *)&myaddr, sizeof(myaddr))==SOCKET_ERROR)
		{closesocket(sd);  return SOCKET_ERROR;}
	if(type==SOCK_STREAM)
	{
		if(listen(sd, maxconn)==SOCKET_ERROR)
			{closesocket(sd);  return SOCKET_ERROR;}
	}
	return sd;
}

int hpnet_waitForConnection(int sd, struct sockaddr_in *remoteaddr)
{
	int sd_client, m=1;  int addrlen;
	int socket_type, socket_type_size=sizeof(socket_type);
	if(!remoteaddr) {badarg();  return SOCKET_ERROR;}
	addrlen=sizeof(*remoteaddr);

	if(getsockopt(sd, SOL_SOCKET, SO_TYPE, (char *)&socket_type, &socket_type_size)==SOCKET_ERROR)
		return SOCKET_ERROR;
	if(socket_type==SOCK_STREAM)
	{
		if((sd_client=accept(sd, (struct sockaddr *)remoteaddr, &addrlen))==INVALID_SOCKET)
			return SOCKET_ERROR;
		if(setsockopt(sd_client, IPPROTO_TCP, TCP_NODELAY, (char*)&m, sizeof(int))==SOCKET_ERROR)
			{closesocket(sd_client);  return SOCKET_ERROR;}
		return sd_client;
	}
	else return SOCKET_ERROR;
}

int hpnet_killClient(int sd)
{
	int socket_type, socket_type_size=sizeof(socket_type);
	if(getsockopt(sd, SOL_SOCKET, SO_TYPE, (char *)&socket_type, &socket_type_size)==SOCKET_ERROR)
		return SOCKET_ERROR;
	if(socket_type==SOCK_DGRAM) return 0;
	else if(socket_type==SOCK_STREAM)
	{
		if(closesocket(sd)==SOCKET_ERROR) return SOCKET_ERROR;
		return 0;
	}
	else return SOCKET_ERROR;
}

int hpnet_connect(char *servername, unsigned short port, struct sockaddr_in *servaddr, int type)
{
	int sd, m=1;  struct hostent *hent;
	if(servername==NULL || servaddr==NULL || (type!=SOCK_DGRAM && type!=SOCK_STREAM))
		{badarg();  return SOCKET_ERROR;}

	memset(servaddr, 0, sizeof(*servaddr));
	servaddr->sin_family=AF_INET;
	servaddr->sin_addr.s_addr=inet_addr(servername);
	servaddr->sin_port=htons(port);
	if(servaddr->sin_addr.s_addr==INADDR_NONE)
	{
		if((hent=gethostbyname(servername))==0) return SOCKET_ERROR;
		memcpy(&(servaddr->sin_addr), hent->h_addr_list[0], hent->h_length);
	}
	if((sd=socket(AF_INET, type, type==SOCK_DGRAM?IPPROTO_UDP:IPPROTO_TCP))==SOCKET_ERROR)
		return SOCKET_ERROR;
	if(type==SOCK_STREAM)
	{
		if(connect(sd, (struct sockaddr *)servaddr, sizeof(*servaddr))==SOCKET_ERROR)
			{closesocket(sd);  return SOCKET_ERROR;}
		if(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char*)&m, sizeof(int))==SOCKET_ERROR)
			{closesocket(sd);  return SOCKET_ERROR;}
	}
	return sd;
}

int hpnet_clientSyncBufsize(int s, struct sockaddr_in *servaddr)
{
	int clisndbufsize, clircvbufsize, clibufsize, bufsize;
	int srvbufsize, sizeofint=sizeof(int);
	if(getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&clisndbufsize, &sizeofint)==SOCKET_ERROR)
		return SOCKET_ERROR;
	if(getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&clircvbufsize, &sizeofint)==SOCKET_ERROR)
		return SOCKET_ERROR;
	clibufsize=min(clisndbufsize, clircvbufsize);
	bufsize=clibufsize;
	if(!littleendian()) clibufsize=byteswap(clibufsize);
	if(hpnet_send(s, (char *)&clibufsize, sizeof(int), servaddr)!=sizeof(int))
		return SOCKET_ERROR;
	if(hpnet_recv(s, (char *)&srvbufsize, sizeof(int), servaddr)!=sizeof(int))
		return SOCKET_ERROR;
	if(!littleendian()) srvbufsize=byteswap(srvbufsize);
	return min(bufsize, srvbufsize);
}

int hpnet_serverSyncBufsize(int s, struct sockaddr_in *servaddr)
{
	int srvsndbufsize, srvrcvbufsize, srvbufsize, bufsize;
	int clibufsize, sizeofint=sizeof(int);
	if(getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&srvsndbufsize, &sizeofint)==SOCKET_ERROR)
		return SOCKET_ERROR;
	if(getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&srvrcvbufsize, &sizeofint)==SOCKET_ERROR)
		return SOCKET_ERROR;
	srvbufsize=min(srvsndbufsize, srvrcvbufsize);
	bufsize=srvbufsize;
	if(hpnet_recv(s, (char *)&clibufsize, sizeof(int), servaddr)!=sizeof(int))
		return SOCKET_ERROR;
	if(!littleendian()) clibufsize=byteswap(clibufsize);
	if(!littleendian()) srvbufsize=byteswap(srvbufsize);
	if(hpnet_send(s, (char *)&srvbufsize, sizeof(int), servaddr)!=sizeof(int))
		return SOCKET_ERROR;
	return min(clibufsize, bufsize);
}

int hpnet_send(int s, char *buf, int len, struct sockaddr_in *sin)
{
	int bytessent=0, retval, sendsize;
	int socket_type, typesize=sizeof(int);
	if(getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&socket_type, &typesize)==SOCKET_ERROR)
		return SOCKET_ERROR;
	while(bytessent<len)
	{
		sendsize=len-bytessent;
		if(socket_type==SOCK_DGRAM) retval=sendto(s, &buf[bytessent], sendsize, 0, (struct sockaddr *)sin, sizeof(*sin));
		else if(socket_type==SOCK_STREAM) retval=send(s, &buf[bytessent], sendsize, 0);
		else return SOCKET_ERROR;
		if(retval==SOCKET_ERROR) return SOCKET_ERROR;
		if(retval==0) return bytessent;
		bytessent+=retval;
	}
	return bytessent;
}

int hpnet_recv(int s, char *buf, int len, struct sockaddr_in *sin)
{
	int bytesrecd=0, retval, structlen=sizeof(struct sockaddr_in), readsize;
	int socket_type, typesize=sizeof(int);
	if(getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&socket_type, &typesize)==SOCKET_ERROR)
		return SOCKET_ERROR;
	while(bytesrecd<len)
	{
		readsize=len-bytesrecd;
		if(socket_type==SOCK_DGRAM) retval=recvfrom(s, &buf[bytesrecd], readsize, 0, (struct sockaddr *)sin, &structlen);
		else if(socket_type==SOCK_STREAM) retval=recv(s, &buf[bytesrecd], readsize, 0);
		else return SOCKET_ERROR;
		if(retval==SOCKET_ERROR) return SOCKET_ERROR;
		if(retval==0) return bytesrecd;
		bytesrecd+=retval;
	}
	return bytesrecd;
}

int hpnet_init(void)
{
	#ifdef _WIN32
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0
	|| LOBYTE(wsaData.wVersion)!=2
	|| HIBYTE(wsaData.wVersion)!=2)
		return SOCKET_ERROR;
	#else
	if(signal(SIGPIPE, SIG_IGN)==SIG_ERR) return SOCKET_ERROR;
	#endif
	return 0;
}

int hpnet_term(void)
{
	#ifdef _WIN32
	if(WSACleanup()==SOCKET_ERROR) return(SOCKET_ERROR);
	#endif
	return 0;
}

#define wserrstrhdr "WINSOCK error: "

char *hpnet_strerror(void)
{
	#ifdef _WIN32
	sprintf(__hputil_errorstr, wserrstrhdr);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)(&__hputil_errorstr[strlen(wserrstrhdr)]), 1024-strlen(wserrstrhdr)-1, NULL);
	return __hputil_errorstr;
	#else
	return strerror(errno);
	#endif
}
