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

// Compilation notes:
// Sun:  cc yourcode.c -lhputil -lrt -lpthread -lsocket -lxnet
// Linux:  cc yourcode.c -D_FILE_OFFSET_BITS=64 -lhputil -lpthread
// SGI:  cc yourcode.c -lhputil -lpthread
// Windows:  cl yourcode.c -link hputil.lib ws2_32.lib

#ifndef __HPUTIL_H__
#define __HPUTIL_H__


#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
	#include <windows.h>
	#define sleep(t) Sleep((t)*1000)
	#define usleep(t) Sleep((t)/1000)
#else
  #ifdef sun
   #ifndef SOLARIS
    #define SOLARIS
   #endif
  #endif
	#ifdef SOLARIS
		#define __EXTENSIONS__
		#include <strings.h>
	#endif
	#include <stdarg.h>
	#include <unistd.h>
	#include <sys/time.h>
	#include <netinet/in.h>
  #include <sys/socket.h>
	#define stricmp strcasecmp
	#define strnicmp strncasecmp
#endif

#ifdef __CYGWIN__
#include <windows.h>
#undef _WIN32
#undef WIN32
#endif

#ifndef min
 #define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
 #define max(a,b) ((a)>(b)?(a):(b))
#endif
#define pow2(i) (1<<(i))
#define isPow2(x) (((x)&(x-1))==0)

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 Error handling
 *******************************************************************/

/*
   Returns a pointer to a string describing why the last command failed.
   All functions in this library return -1 for error and 0 for success,
   unless otherwise specified.  On Unix, this function calls strerror(errno)
   and returns that error message.  On Windows, it calls GetLastError() and
   then FormatMessage().
*/

char *hputil_strerror(void);

/*
   These routines provide a flexible logging mechanism that can be
   dynamically adjusted at run time.
*/

/*
   int hputil_logto(FILE *file)
   file:  stream pointer to which to send output of subsequent hpprintf
      calls.  hpprintf outputs to stderr unless you redirect it using this
      function
*/
int hputil_logto(FILE *file);

/*
   void hputil_log(int on, int profile)
   on:  1 = enable logging, 0 = disable logging (hpprintf becomes a no op)
      Logging is disabled by default
   profile:  1 = prepend timing information to every hpprintf output, 0 = don't
*/
void hputil_log(int on, int profile);

/*
   void hpprintf(const char *format, ...)
   Works just like printf, except that the output goes to the stream specified
   in hputil_logto() (or stderr if no stream has been specified.)  If
   logging has not been turned on with hputil_log(), this function has no
   effect
*/
extern int (*hpprintf)(const char *, ...);

/*
   Works the same as the Posix perror() function but prints the error
   string from this library
*/

#define hputil_perror(c) hpprintf("%s\n%s\n", c, hputil_strerror())

/*
   int numprocs(void)
   Returns the number of CPU's active in the system
*/
int numprocs(void);

/*
   unsigned long hpthread_id(void)
   Returns the ID of the calling thread
*/
unsigned long hpthread_id(void);


/*******************************************************************
 High-performance timers
 *******************************************************************/

/*
   Call hptimer_init() once at the beginning of your code, then use hptime()
   to clock the beginning and end of a routine you wish to time.  The
   difference in these two values is the amount of time the routine took to
   run (in seconds.)  This routine has microsecond resolution on Unix and
   sub-microsecond resolution on Windows.

   hptimer_init();
   double tStart=hptime();
   {some_code;}
   printf("Code took %f seconds to run", hptime()-tStart);
*/

#if defined(_WIN32)||defined(__CYGWIN__)

	void hptimer_init(void);
	extern double (* hptime)(void);

#else

	#define hptimer_init() {}
	double hptime(void);

#endif


/*******************************************************************
 Network routines
 *******************************************************************/

#ifndef _WIN32
 #define SOCKET_ERROR -1
 #define INVALID_SOCKET -1
 int closesocket(int);
#endif

#define byteswap(i) ( \
	(((i) & 0xff000000) >> 24) | \
	(((i) & 0x00ff0000) >>  8) | \
	(((i) & 0x0000ff00) <<  8) | \
	(((i) & 0x000000ff) << 24) )

#define byteswap16(i) ( \
	(((i) & 0xff00) >> 8) | \
	(((i) & 0x00ff) << 8) )

int littleendian(void);
int hpnet_init(void);
int hpnet_createServer(unsigned short port, int maxConnections, int socket_type);
int hpnet_killClient(int socket_descr);
int hpnet_waitForConnection(int socket_descr, struct sockaddr_in *clientaddr);
int hpnet_connect(char *servername, unsigned short port, struct sockaddr_in *servaddr, int socket_type);
int hpnet_clientSyncBufsize(int socket_descr, struct sockaddr_in *servaddr);
int hpnet_serverSyncBufsize(int socket_descr, struct sockaddr_in *cliaddr);
int hpnet_send(int socket_descr, char *buf, int len, struct sockaddr_in *remoteaddr);
int hpnet_recv(int socket_descr, char *buf, int len, struct sockaddr_in *remoteaddr);
int hpnet_term(void);
char *hpnet_strerror(void);


#ifdef __cplusplus
}
#endif


#endif
