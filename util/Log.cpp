// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2014, 2019, 2021 D. R. Commander
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

#include "Log.h"
#include <stdarg.h>
#include <string.h>
#include "vglutil.h"

using namespace util;


Log *Log::instance = NULL;
CriticalSection Log::mutex;


Log *Log::getInstance(void)
{
	if(instance == NULL)
	{
		CriticalSection::SafeLock l(mutex);
		if(instance == NULL) instance = new Log;
	}
	return instance;
}


void Log::logTo(FILE *logFile_)
{
	CriticalSection::SafeLock l(mutex);
	if(logFile_)
	{
		if(newFile)
		{
			fclose(logFile);  newFile = false;
		}
		logFile = logFile_;
	}
}


void Log::logTo(char *logFileName)
{
	FILE *logFile_ = NULL;
	CriticalSection::SafeLock l(mutex);
	if(logFileName)
	{
		if(newFile)
		{
			fclose(logFile);  newFile = false;
		}
		if(!stricmp(logFileName, "stdout"))
			logFile = stdout;
		else if((logFile_ = fopen(logFileName, "w")) != NULL)
		{
			logFile = logFile_;  newFile = true;
		}
	}
}


void Log::print(const char *format, ...)
{
	CriticalSection::SafeLock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	vfprintf(logFile, format, arglist);
	va_end(arglist);
}


void Log::PRINT(const char *format, ...)
{
	CriticalSection::SafeLock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	vfprintf(logFile, format, arglist);
	va_end(arglist);
	flush();
}


void Log::println(const char *format, ...)
{
	CriticalSection::SafeLock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	vfprintf(logFile, format, arglist);
	va_end(arglist);
	fprintf(logFile, "\n");
}


void Log::PRINTLN(const char *format, ...)
{
	CriticalSection::SafeLock l(mutex);
	va_list arglist;
	va_start(arglist, format);
	vfprintf(logFile, format, arglist);
	va_end(arglist);
	fprintf(logFile, "\n");
	flush();
}
