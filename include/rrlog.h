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

#ifndef __RRLOG_H__
#define __RRLOG_H__

#include <stdio.h>
#include <stdarg.h>
#include "rrmutex.h"

class rrlog
{
	public:

		static rrlog *instance(void)
		{
			if(instanceptr==NULL)
			{
				rrcs::safelock l(mutex);
				if(instanceptr==NULL) instanceptr=new rrlog;
			}
			return instanceptr;
		}

		void logto(FILE *log)
		{
			rrcs::safelock l(mutex);
			if(log)
			{
				if(newfile) {fclose(logfile);  newfile=false;}
				logfile=log;
			}
		}

		void logto(char *logfilename)
		{
			FILE *log=NULL;
			rrcs::safelock l(mutex);
			if(logfilename)
			{
				if(newfile) {fclose(logfile);  newfile=false;}
				if((log=fopen(logfilename, "w"))!=NULL) {logfile=log;  newfile=true;}
			}
		}

		void print(const char *format, ...)
		{
			rrcs::safelock l(mutex);
			va_list arglist;
			va_start(arglist, format);
			vfprintf(logfile, format, arglist);
			va_end(arglist);
		}

		// This class assumes that if you yell, you want it now
		void PRINT(const char *format, ...)
		{
			rrcs::safelock l(mutex);
			va_list arglist;
			va_start(arglist, format);
			vfprintf(logfile, format, arglist);
			va_end(arglist);
			flush();
		}

		void println(const char *format, ...)
		{
			rrcs::safelock l(mutex);
			va_list arglist;
			va_start(arglist, format);
			vfprintf(logfile, format, arglist);
			va_end(arglist);
			fprintf(logfile, "\n");
		}

		void PRINTLN(const char *format, ...)
		{
			rrcs::safelock l(mutex);
			va_list arglist;
			va_start(arglist, format);
			vfprintf(logfile, format, arglist);
			va_end(arglist);
			fprintf(logfile, "\n");
			flush();
		}

		void flush(void) {fflush(logfile);}

	private:

		rrlog() {logfile=stderr;  newfile=false;}
		~rrlog() {}

		static rrlog *instanceptr;
		static rrcs mutex;
		FILE *logfile;
		bool newfile;
};

#define rrout (*(rrlog::instance()))

#endif
