/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2013-2014 D. R. Commander
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
	static long long usrJifs[MAXCPUS+1], niceJifs[MAXCPUS+1], sysJifs[MAXCPUS+1],
		totalJifs[MAXCPUS+1];
	static double minPercent[MAXCPUS+1], maxPercent[MAXCPUS+1],
		grandTotalPercent[MAXCPUS+1];
	static long long iter=0;
	double usrPercent, nicePercent, sysPercent, totalPercent;
	long long usrJif, niceJif, sysJif, idleJif, ioJif, hiqJif, siqJif,
		totalJif;
	static int first=1;
	int i, j;

	FILE *procFile;
	char temps[255], temps2[255];
	if(!(procFile=fopen("/proc/stat", "r")))
	{
		printf("Could not open /proc/stat\n");
		return;
	}

	for(i=0; i<MAXCPUS+1; i++)
	{
		if(!fgets(temps, 254, procFile)) continue;
		ioJif=hiqJif=siqJif=0;
		if((j=sscanf(temps, "%s %lld %lld %lld %lld %lld %lld %lld", temps2,
			&usrJif, &niceJif, &sysJif, &idleJif, &ioJif, &hiqJif, &siqJif))<5
			|| strncasecmp(temps, "cpu", 3))
			break;
		totalJif=usrJif+niceJif+sysJif+idleJif+ioJif+hiqJif+siqJif;
		if(!first)
		{
			usrPercent=(double)(usrJif-usrJifs[i])
				/(double)(totalJif-totalJifs[i])*100.;
			nicePercent=(double)(niceJif-niceJifs[i])
				/(double)(totalJif-totalJifs[i])*100.;
			sysPercent=(double)(sysJif+ioJif+hiqJif+siqJif-sysJifs[i])
				/(double)(totalJif-totalJifs[i])*100.;
			totalPercent=usrPercent+nicePercent+sysPercent;
			if(totalPercent>maxPercent[i]) maxPercent[i]=totalPercent;
			if(totalPercent<minPercent[i]) minPercent[i]=totalPercent;
			grandTotalPercent[i]+=totalPercent;
			printf("%s: %5.1f (Usr=%5.1f Nice=%5.1f Sys=%5.1f) / Min=%5.1f Max=%5.1f Avg=%5.1f\n",
				i==0? "ALL ":temps2, totalPercent, usrPercent, nicePercent, sysPercent,
				minPercent[i], maxPercent[i], grandTotalPercent[i]/(double)iter);
		}
		else
		{
			minPercent[i]=100.;  maxPercent[i]=0.;  grandTotalPercent[i]=0.;
		}
		usrJifs[i]=usrJif;
		niceJifs[i]=niceJif;
		sysJifs[i]=sysJif+ioJif+hiqJif+siqJif;
		totalJifs[i]=totalJif;
	}
	if(first) first=0;
	else printf("\n");
	iter++;
	fclose(procFile);
}


int main(int argc, char **argv)
{
	while(1)
	{
		collect();
		sleep(2);
	}
}
