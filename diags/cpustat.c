/* Copyright (C)2004 Landmark Graphics
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAXCPUS 4

void collect(void)
{
	static long long usrjif[MAXCPUS+1], nicejif[MAXCPUS+1], sysjif[MAXCPUS+1],
		totaljif[MAXCPUS+1];
	static double min[MAXCPUS+1], max[MAXCPUS+1], grandtotal[MAXCPUS+1];
	static long long iter=0;
	double usr, nice, sys, total;
	long long _usrjif, _nicejif, _sysjif, _idlejif, _totaljif;
	static int first=1;
	int i, j;

	FILE *procfile;
	char temps[255], temps2[255];
	if(!(procfile=fopen("/proc/stat", "r")))
	{
		printf("Could not open /proc/stat\n");
		return;
	}

	for(i=0; i<MAXCPUS+1; i++)
	{
		if(!fgets(temps, 254, procfile)) continue;
		if((j=sscanf(temps, "%s %lld %lld %lld %lld", temps2, &_usrjif, &_nicejif,
			&_sysjif, &_idlejif))!=5 || strncasecmp(temps, "cpu", 3))
			break;
		_totaljif=_usrjif+_nicejif+_sysjif+_idlejif;
		if(!first)
		{
			usr=(double)(_usrjif-usrjif[i])/(double)(_totaljif-totaljif[i])*100.;
			nice=(double)(_nicejif-nicejif[i])/(double)(_totaljif-totaljif[i])*100.;
			sys=(double)(_sysjif-sysjif[i])/(double)(_totaljif-totaljif[i])*100.;
			total=usr+nice+sys;
			if(total>max[i]) max[i]=total;
			if(total<min[i]) min[i]=total;
			grandtotal[i]+=total;
			printf("%s: %5.1f (Usr=%5.1f Nice=%5.1f Sys=%5.1f) / Min=%5.1f Max=%5.1f Avg=%5.1f\n",
				i==0?"ALL ":temps2, total, usr, nice, sys, min[i], max[i],
				grandtotal[i]/(double)iter);
		}
		else
		{
			min[i]=100.;  max[i]=0.;  grandtotal[i]=0.;
		}
		usrjif[i]=_usrjif;
		nicejif[i]=_nicejif;
		sysjif[i]=_sysjif;
		totaljif[i]=_totaljif;
	}
	if(first) first=0;
	else printf("\n");
	iter++;
	fclose(procfile);
}
		

int main(int argc, char **argv)
{
	while(1)
	{
		collect();
		sleep(2);
	}
}
