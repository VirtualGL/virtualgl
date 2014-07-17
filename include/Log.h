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

#ifndef __LOG_H__
#define __LOG_H__

#include "Mutex.h"
#include <stdio.h>


namespace vglutil
{
	class Log
	{
		public:

			static Log *getInstance(void);
			void logTo(FILE *logFile);
			void logTo(char *logFileName);
			void print(const char *format, ...);
			// This class assumes that if you yell, you want it now
			void PRINT(const char *format, ...);
			void println(const char *format, ...);
			void PRINTLN(const char *format, ...);
			void flush(void) { fflush(logFile); }
			FILE *getFile(void) { return logFile; }

		private:

			Log()
			{
				logFile=stderr;  newFile=false;
			}

			~Log() {}

			static Log *instance;
			static CriticalSection mutex;
			FILE *logFile;
			bool newFile;
	};
}


#define vglout (*(vglutil::Log::getInstance()))

#endif // __LOG_H__
