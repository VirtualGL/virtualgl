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

#include "rrconn.h"

void *rraconn::dispatcher(void *param)
{
	rraconn *rrc=(rraconn *)param;
	try
	{
		while(!rrc->deadyet) rrc->dispatch();
	}
	catch(rrerror &e)
	{
		if(!rrc->deadyet) rrc->lasterror=e;
	}
	return 0;
}
