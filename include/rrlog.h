/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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
			if(_instanceptr==NULL)
			{
				rrcs::safelock l(_mutex);
				if(_instanceptr==NULL) _instanceptr=new rrlog;
			}
			return _instanceptr;
		}

		void logto(FILE *log)
		{
			rrcs::safelock l(_mutex);
			if(log)
			{
				if(_newfile) {fclose(_logfile);  _newfile=false;}
				_logfile=log;
			}
		}

		void logto(char *logfilename)
		{
			FILE *log=NULL;
			rrcs::safelock l(_mutex);
			if(logfilename)
			{
				if(_newfile) {fclose(_logfile);  _newfile=false;}
				if((log=fopen(logfilename, "w"))!=NULL) {_logfile=log;  _newfile=true;}
			}
		}

		void print(const char *format, ...)
		{
			rrcs::safelock l(_mutex);
			va_list arglist;
			va_start(arglist, format);
			vfprintf(_logfile, format, arglist);
			va_end(arglist);
		}

		// This class assumes that if you yell, you want it now
		void PRINT(const char *format, ...)
		{
			rrcs::safelock l(_mutex);
			va_list arglist;
			va_start(arglist, format);
			vfprintf(_logfile, format, arglist);
			va_end(arglist);
			flush();
		}

		void println(const char *format, ...)
		{
			rrcs::safelock l(_mutex);
			va_list arglist;
			va_start(arglist, format);
			vfprintf(_logfile, format, arglist);
			va_end(arglist);
			fprintf(_logfile, "\n");
		}

		void PRINTLN(const char *format, ...)
		{
			rrcs::safelock l(_mutex);
			va_list arglist;
			va_start(arglist, format);
			vfprintf(_logfile, format, arglist);
			va_end(arglist);
			fprintf(_logfile, "\n");
			flush();
		}

		void flush(void) {fflush(_logfile);}

		FILE *getfile(void) {return _logfile;}

	private:

		rrlog() {_logfile=stderr;  _newfile=false;}
		~rrlog() {}

		static rrlog *_instanceptr;
		static rrcs _mutex;
		FILE *_logfile;
		bool _newfile;
};

#define rrout (*(rrlog::instance()))

#endif
