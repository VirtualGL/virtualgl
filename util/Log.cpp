/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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

#include "Log.h"
#include <stdarg.h>

using namespace vglutil;


Log *Log::instancePtr=NULL;
CS Log::mutex;


Log *Log::instance(void)
{
	if(instancePtr==NULL)
	{
		CS::SafeLock l(mutex);
		if(instancePtr==NULL) instancePtr=new Log;
	}
	return instancePtr;
}


void Log::logTo(FILE *logFile_)
{
	CS::SafeLock l(mutex);
	if(logFile_)
	{
		if(newFile)
		{
			fclose(logFile);  newFile=false;
		}
		logFile=logFile_;
	}
}


void Log::logTo(char *logFileName)
{
	FILE *logFile_=NULL;
	CS::SafeLock l(mutex);
	if(logFileName)
	{
		if(newFile)
		{
			fclose(logFile);  newFile=false;
		}
		if((logFile_=fopen(logFileName, "w"))!=NULL)
		{
			logFile=logFile_;  newFile=true;
		}
	}
}


void Log::print(const char *format, ...)
{
	CS::SafeLock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	vfprintf(logFile, format, arglist);
	va_end(arglist);
}


void Log::PRINT(const char *format, ...)
{
	CS::SafeLock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	vfprintf(logFile, format, arglist);
	va_end(arglist);
	flush();
}


void Log::println(const char *format, ...)
{
	CS::SafeLock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	vfprintf(logFile, format, arglist);
	va_end(arglist);
	fprintf(logFile, "\n");
}


void Log::PRINTLN(const char *format, ...)
{
	CS::SafeLock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	vfprintf(logFile, format, arglist);
	va_end(arglist);
	fprintf(logFile, "\n");
	flush();
}
