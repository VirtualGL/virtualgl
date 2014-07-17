/* Copyright (C)2007 Sun Microsystems, Inc.
 * Copyright (C)2009, 2012, 2014 D. R. Commander
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

#include <sys/types.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Spinner.H>
#include <FL/x.H>
#include "fakerconfig.h"
#include "rr.h"
#include "Error.h"
#include "Log.h"
#include "vglutil.h"

using namespace vglutil;


Fl_Double_Window *win=NULL;
Fl_Choice *compChoice=NULL, *sampChoice=NULL, *stereoChoice=NULL,
	*profChoice=NULL;
Fl_Light_Button *spoilButton=NULL, *ifButton=NULL;
Fl_Value_Slider *qualSlider=NULL;
Fl_Float_Input *gammaInput=NULL, *fpsInput;
Fl_Check_Button *fpsButton=NULL;
int ppid=-1;

#undef fconfig
FakerConfig *_fconfig=NULL;
#define fconfig (*_fconfig)


// Set functions

void setComp(void)
{
	int i;

	for(i=0; i<RR_COMPRESSOPT; i++)
	{
		if(strlen(fconfig.transport)==0 && !fconfig.transvalid[_Trans[i]])
			compChoice->mode(i, FL_MENU_INACTIVE);
		else compChoice->mode(i, 0);
		if(fconfig.compress==i) compChoice->value(i);
	}
	if(!fconfig.transvalid[RRTRANS_XV])
	{
		compChoice->mode(4, FL_MENU_INACTIVE);
	}
}

void setSamp(void)
{
	if(!sampChoice) return;
	if(_Maxsubsamp[fconfig.compress]==_Minsubsamp[fconfig.compress]
		&& strlen(fconfig.transport)==0)
		sampChoice->deactivate();
	else sampChoice->activate();
	if(strlen(fconfig.transport)==0)
	{
		if(_Minsubsamp[fconfig.compress]>0)
			sampChoice->mode(0, FL_MENU_INACTIVE);
		else sampChoice->mode(0, 0);
		if(_Minsubsamp[fconfig.compress]>1 || _Maxsubsamp[fconfig.compress]<1)
			sampChoice->mode(1, FL_MENU_INACTIVE);
		else sampChoice->mode(1, 0);
		if(_Minsubsamp[fconfig.compress]>2 || _Maxsubsamp[fconfig.compress]<2)
			sampChoice->mode(2, FL_MENU_INACTIVE);
		else sampChoice->mode(2, 0);
		if(_Minsubsamp[fconfig.compress]>4 || _Maxsubsamp[fconfig.compress]<4)
			sampChoice->mode(3, FL_MENU_INACTIVE);
		else sampChoice->mode(3, 0);
		if(_Minsubsamp[fconfig.compress]>8 || _Maxsubsamp[fconfig.compress]<8)
			sampChoice->mode(4, FL_MENU_INACTIVE);
		else sampChoice->mode(4, 0);
		if(_Minsubsamp[fconfig.compress]>16 || _Maxsubsamp[fconfig.compress]<16)
			sampChoice->mode(5, FL_MENU_INACTIVE);
		else sampChoice->mode(5, 0);
	}
	switch(fconfig.subsamp)
	{
		case 0:  sampChoice->value(0);  break;
		case 1:  sampChoice->value(1);  break;
		case 2:  sampChoice->value(2);  break;
		case 4:  sampChoice->value(3);  break;
		case 8:  sampChoice->value(4);  break;
		case 16:  sampChoice->value(5);  break;
	}
}

void setQual(void)
{
	if(!qualSlider) return;
	qualSlider->value(fconfig.qual);
	if(fconfig.compress!=RRCOMP_JPEG && strlen(fconfig.transport)==0)
		qualSlider->deactivate();
	else qualSlider->activate();
}

void setProf(void)
{
	if(!profChoice) return;
	if(!fconfig.transvalid[RRTRANS_VGL] || strlen(fconfig.transport)>0)
	{
		profChoice->value(3);  profChoice->deactivate();
	}
	else
	{
		profChoice->activate();
		if(fconfig.compress==RRCOMP_JPEG && fconfig.qual==30
			&& fconfig.subsamp==4) profChoice->value(0);
		else if(fconfig.compress==RRCOMP_JPEG && fconfig.qual==80
			&& fconfig.subsamp==2) profChoice->value(1);
		else if(fconfig.compress==RRCOMP_JPEG && fconfig.qual==95
			&& fconfig.subsamp==1) profChoice->value(2);
		else profChoice->value(3);
	}
}

void setIF(void)
{
	if(!ifButton) return;
	ifButton->value(fconfig.interframe);
	if(strlen(fconfig.transport)>0 || fconfig.compress==RRCOMP_JPEG
		|| fconfig.compress==RRCOMP_RGB)
		ifButton->activate();
	else ifButton->deactivate();
}

void setStereo(void)
{
	int i;
	if(!stereoChoice) return;
	if(strlen(fconfig.transport)==0 && !fconfig.transvalid[RRTRANS_VGL])
		stereoChoice->mode(2, FL_MENU_INACTIVE);
	else stereoChoice->mode(2, 0);
	for(i=0; i<RR_STEREOOPT; i++)
		if(fconfig.stereo==i) stereoChoice->value(i);
}

void setFPS(void)
{
	char temps[20];
	snprintf(temps, 19, "%.2f", fconfig.fps);
	fpsInput->value(temps);
}


// Callbacks

void compCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	if((d>=0 && d<=RR_COMPRESSOPT-1) || strlen(fconfig.transport)>0)
		fconfig_setcompress(fconfig, d);
	setSamp();
	setQual();
	setProf();
	setIF();
	setStereo();
	setFPS();
}

void sampCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	fconfig.subsamp=d;
	setProf();
}

void qualCB(Fl_Widget *w, void *data)
{
	Fl_Value_Slider *slider=(Fl_Value_Slider *)w;
	fconfig.qual=(int)slider->value();
	setProf();
}

void profCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	if(!fconfig.transvalid[RRTRANS_VGL]) return;
	switch(d)
	{
		case 0:
			fconfig_setcompress(fconfig, RRCOMP_JPEG);
			fconfig.qual=30;  fconfig.subsamp=4;
			break;
		case 1:
			fconfig_setcompress(fconfig, RRCOMP_JPEG);
			fconfig.qual=80;  fconfig.subsamp=2;
			break;
		case 2:
			fconfig_setcompress(fconfig, RRCOMP_JPEG);
			fconfig.qual=95;  fconfig.subsamp=1;
			break;
	}
	setComp();
	setSamp();
	setQual();
	setStereo();
	setIF();
}

void spoilCB(Fl_Widget *w, void *data)
{
	Fl_Check_Button *check=(Fl_Check_Button *)w;
	fconfig.spoil=(check->value()!=0);
}

void gammaCB(Fl_Widget *w, void *data)
{
	Fl_Float_Input *input=(Fl_Float_Input *)w;
	fconfig_setgamma(fconfig, atof(input->value()));
	char temps[20];
	snprintf(temps, 19, "%.2f", fconfig.gamma);
	input->value(temps);
}

void ifCB(Fl_Widget *w, void *data)
{
	Fl_Check_Button *check=(Fl_Check_Button *)w;
	fconfig.interframe=(check->value()!=0);
}

void stereoCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	if(d>=0 && d<=RR_STEREOOPT-1) fconfig.stereo=d;
}

void fpsCB(Fl_Widget *w, void *data)
{
	Fl_Float_Input *input=(Fl_Float_Input *)w;
	fconfig.fps=atof(input->value());
	char temps[20];
	snprintf(temps, 19, "%.2f", fconfig.fps);
	input->value(temps);
}


// Menus

Fl_Menu_Item compMenu[]=
{
	{"None (X11 Transport)", 0, compCB, (void *)RRCOMP_PROXY},
	{"JPEG (VGL Transport)", 0, compCB, (void *)RRCOMP_JPEG},
	{"RGB (VGL Transport)", 0, compCB, (void *)RRCOMP_RGB},
	{"YUV (XV Transport)", 0, compCB, (void *)RRCOMP_XV},
	{"YUV (VGL Transport)", 0, compCB, (void *)RRCOMP_YUV},
	{0, 0, 0, 0}
};

Fl_Menu_Item pluginCompMenu[]=
{
	{"0", 0, compCB, (void *)0},
	{"1", 0, compCB, (void *)1},
	{"2", 0, compCB, (void *)2},
	{"3", 0, compCB, (void *)3},
	{"4", 0, compCB, (void *)4},
	{"5", 0, compCB, (void *)5},
	{"6", 0, compCB, (void *)6},
	{"7", 0, compCB, (void *)7},
	{"8", 0, compCB, (void *)8},
	{"9", 0, compCB, (void *)9},
	{"10", 0, compCB, (void *)10},
	{0, 0, 0, 0}
};

Fl_Menu_Item sampMenu[]=
{
	{"Grayscale", 0, sampCB, (void *)0},
	{"1X", 0, sampCB, (void *)1},
	{"2X", 0, sampCB, (void *)2},
	{"4X", 0, sampCB, (void *)4},
	{"8X", 0, sampCB, (void *)8},
	{"16X", 0, sampCB, (void *)16},
	{0, 0, 0, 0}
};

Fl_Menu_Item profMenu[]=
{
	{"Low Qual (Low-Bandwidth Network)", 0, profCB, (void *)0},
	{"Medium Qual", 0, profCB, (void *)1},
	{"High Qual (High-Bandwidth Network)", 0, profCB, (void *)2},
	{"Custom", 0, 0, 0},
	{0, 0, 0, 0}
};

Fl_Menu_Item stereoMenu[]=
{
	{"Send Left Eye Only", 0, stereoCB, (void *)RRSTEREO_LEYE},
	{"Send Right Eye Only", 0, stereoCB, (void *)RRSTEREO_REYE},
	{"Quad-Buffered (if available)", 0, stereoCB, (void *)RRSTEREO_QUADBUF},
	{"Anaglyphic (Red/Cyan)", 0, stereoCB, (void *)RRSTEREO_REDCYAN},
	{"Anaglyphic (Green/Magenta)", 0, stereoCB, (void *)RRSTEREO_GREENMAGENTA},
	{"Anaglyphic (Blue/Yellow)", 0, stereoCB, (void *)RRSTEREO_BLUEYELLOW},
	{"Passive (Interleaved)", 0, stereoCB, (void *)RRSTEREO_INTERLEAVED},
	{"Passive (Top/Bottom)", 0, stereoCB, (void *)RRSTEREO_TOPBOTTOM},
	{"Passive (Side-by-Side)", 0, stereoCB, (void *)RRSTEREO_SIDEBYSIDE},
	{0, 0, 0, 0}
};


void init(int argc, char **argv)
{
	char temps[20];

	_newcheck(win=new Fl_Double_Window(485, 340, "VirtualGL Configuration"));

	if(strlen(fconfig.transport)>0)
	{
		_newcheck(compChoice=new Fl_Choice(211, 10, 225, 25,
			"Image Compression: "));
		compChoice->menu(pluginCompMenu);
	}
	else
	{
		_newcheck(compChoice=new Fl_Choice(227, 10, 225, 25,
			"Image Compression (Transport): "));
		compChoice->menu(compMenu);
	}
	setComp();

	_newcheck(sampChoice=new Fl_Choice(210, 45, 100, 25,
		"Chrominance Subsampling: "));
	sampChoice->menu(sampMenu);
	setSamp();

	if(strlen(fconfig.transport)>0)
	{
		_newcheck(qualSlider=new Fl_Value_Slider(30, 80, 335, 25,
			"Image Quality"));
	}
	else
	{
		_newcheck(qualSlider=new Fl_Value_Slider(30, 80, 335, 25,
			"JPEG Image Quality"));
	}
	qualSlider->callback(qualCB, 0);
	qualSlider->type(1);
	qualSlider->minimum(1);
	qualSlider->maximum(100);
	qualSlider->step(1);
	qualSlider->slider_size(0.01);
	setQual();

	_newcheck(profChoice=new Fl_Choice(157, 130, 280, 24,
		"Connection Profile: "));
	profChoice->menu(profMenu);
	profChoice->mode(3, FL_MENU_INACTIVE);
	setProf();

	_newcheck(gammaInput=new Fl_Float_Input(315, 165, 85, 25,
		"Gamma Correction Factor (1.0=no correction): "));
	gammaInput->callback(gammaCB, 0);
	snprintf(temps, 19, "%.2f", fconfig.gamma);
	gammaInput->value(temps);

	_newcheck(spoilButton=new Fl_Light_Button(10, 200, 345, 25,
		"Frame Spoiling (Do Not Use When Benchmarking)"));
	spoilButton->callback(spoilCB, 0);
	spoilButton->value(fconfig.spoil);

	_newcheck(ifButton=new Fl_Light_Button(10, 235, 185, 25,
		"Inter-Frame Comparison"));
	ifButton->callback(ifCB, 0);
	setIF();

	_newcheck(stereoChoice=new Fl_Choice(232, 270, 220, 25,
		"Stereographic Rendering Method: "));
	stereoChoice->menu(stereoMenu);
	setStereo();

	_newcheck(fpsInput=new Fl_Float_Input(240, 305, 85, 25,
		"Limit Frames/second (0.0=no limit): "));
	fpsInput->callback(fpsCB, 0);
	setFPS();

	win->end();
  win->show(argc, argv);
}


void checkParentPID(void *data)
{
	if(kill(ppid, 0)==-1)
	{
		delete win;  win=NULL;
	}
}


void usage(char *programName)
{
	vglout.print("\nUSAGE: %s [-display <d>] -shmid <s> [-ppid <p>]\n\n",
		programName);
	vglout.print("<d> = X display to which to display the GUI (default: read from DISPLAY\n");
	vglout.print("      environment variable)\n");
	vglout.print("<s> = Shared memory segment ID (reported by VirtualGL when the\n");
	vglout.print("      environment variable VGL_VERBOSE is set to 1)\n");
	vglout.print("<p> = Parent process ID.  VGL Config will exit when this process\n");
	vglout.print("      terminates.\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int status=0, shmid=-1;  bool test=false;
	char *darg[2]={ NULL, NULL };

	try
	{
		for(int i=0; i<argc; i++)
		{
			if(!stricmp(argv[i], "-display") && i<argc-1)
			{
				darg[0]=argv[i++];  darg[1]=argv[i];
			}
			if(!stricmp(argv[i], "-shmid") && i<argc-1)
			{
				int temp=atoi(argv[++i]);  if(temp>-1) shmid=temp;
			}
			if(!stricmp(argv[i], "-test")) test=true;
			if(!stricmp(argv[i], "-ppid") && i<argc-1)
			{
				int temp=atoi(argv[++i]);  if(temp>0) ppid=temp;
			}
		}
		if(darg[0] && darg[1]) { argv[1]=darg[0];  argv[2]=darg[1];  argc=3; }
		else argc=1;

		if(test)
		{
			if(!(_fconfig=fconfig_instance()))
				_throw("Could not allocate FakerConfig");
			fl_open_display();
			fconfig_setdefaultsfromdpy(fl_display);
			fconfig_print(fconfig);
		}
		else
		{
			if(shmid==-1) usage(argv[0]);
			if((_fconfig=(FakerConfig *)shmat(shmid, 0, 0))==(FakerConfig *)-1)
				_throwunix();
			if(!_fconfig)
				_throw("Could not attach to config structure in shared memory");
		}

		init(argc, argv);
		if(ppid>0) Fl::add_check(checkParentPID);
		status=Fl::run();
	}
	catch(Error &e)
	{
		vglout.print("Error in vglconfig--\n%s\n", e.getMessage());
		status=-1;
	}
	if(_fconfig)
	{
		if(test) fconfig_deleteinstance();
		else shmdt((char *)_fconfig);
	}
	return status;
}
