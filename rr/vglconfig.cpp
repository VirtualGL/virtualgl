/* Copyright (C)2007 Sun Microsystems, Inc.
 * Copyright (C)2009 D. R. Commander
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
#include "fakerconfig.h"
#include "rr.h"
#include "rrlog.h"
#include "rrutil.h"

Fl_Double_Window *win=NULL;
Fl_Choice *compchoice=NULL/*, *mcompchoice=NULL*/, *sampchoice=NULL,
	*stereochoice=NULL/*, *msampchoice=NULL*/, *profchoice=NULL;
Fl_Light_Button *spoilbutton=NULL, *ifbutton=NULL;
Fl_Value_Slider *qualslider=NULL/*, *mqualslider=NULL*/;
Fl_Float_Input *gammainput=NULL, *fpsinput;
/*Fl_Input *movieinput=NULL;*/
Fl_Check_Button *fpsbutton=NULL/*, *mcbutton=NULL*/;
/*Fl_Group *moviebox=NULL;*/
int ppid=-1;

#undef fconfig
FakerConfig *_fconfig=NULL;
#define fconfig (*_fconfig)


void SetComp(void)
{
	int i;
	for(i=0; i<RR_COMPRESSOPT; i++)
	{
		if(strlen(fconfig.transport)==0 && !fconfig.transvalid[_Trans[i]])
			compchoice->mode(i, FL_MENU_INACTIVE);
		else compchoice->mode(i, 0);
		if(fconfig.compress==i) compchoice->value(i);
	}
	if(!fconfig.transvalid[RRTRANS_XV])
	{
		compchoice->mode(4, FL_MENU_INACTIVE);
	}
}

void SetSamp(void)
{
	if(!sampchoice) return;
	if(_Maxsubsamp[fconfig.compress]==_Minsubsamp[fconfig.compress]
		&& strlen(fconfig.transport)==0)
		sampchoice->deactivate();
	else sampchoice->activate();
	if(strlen(fconfig.transport)==0)
	{
		if(_Minsubsamp[fconfig.compress]>0)
			sampchoice->mode(0, FL_MENU_INACTIVE);
		else sampchoice->mode(0, 0);
		if(_Minsubsamp[fconfig.compress]>1 || _Maxsubsamp[fconfig.compress]<1)
			sampchoice->mode(1, FL_MENU_INACTIVE);
		else sampchoice->mode(1, 0);
		if(_Minsubsamp[fconfig.compress]>2 || _Maxsubsamp[fconfig.compress]<2)
			sampchoice->mode(2, FL_MENU_INACTIVE);
		else sampchoice->mode(2, 0);
		if(_Minsubsamp[fconfig.compress]>4 || _Maxsubsamp[fconfig.compress]<4)
			sampchoice->mode(3, FL_MENU_INACTIVE);
		else sampchoice->mode(3, 0);
		if(_Minsubsamp[fconfig.compress]>8 || _Maxsubsamp[fconfig.compress]<8)
			sampchoice->mode(4, FL_MENU_INACTIVE);
		else sampchoice->mode(4, 0);
		if(_Minsubsamp[fconfig.compress]>16 || _Maxsubsamp[fconfig.compress]<16)
			sampchoice->mode(5, FL_MENU_INACTIVE);
		else sampchoice->mode(5, 0);
	}
	switch(fconfig.subsamp)
	{
		case 0:  sampchoice->value(0);  break;
		case 1:  sampchoice->value(1);  break;
		case 2:  sampchoice->value(2);  break;
		case 4:  sampchoice->value(3);  break;
		case 8:  sampchoice->value(4);  break;
		case 16:  sampchoice->value(5);  break;
	}
}

void SetQual(void)
{
	if(!qualslider) return;
	qualslider->value(fconfig.qual);
	if(fconfig.compress!=RRCOMP_JPEG && strlen(fconfig.transport)==0)
		qualslider->deactivate();
	else qualslider->activate();
}

void SetProf(void)
{
	if(!profchoice) return;
	if(!fconfig.transvalid[RRTRANS_VGL] || strlen(fconfig.transport)>0)
	{
		profchoice->value(3);  profchoice->deactivate();
	}
	else
	{
		profchoice->activate();
		if(fconfig.compress==RRCOMP_JPEG && fconfig.qual==30
			&& fconfig.subsamp==4) profchoice->value(0);
		else if(fconfig.compress==RRCOMP_JPEG && fconfig.qual==80
			&& fconfig.subsamp==2) profchoice->value(1);
		else if(fconfig.compress==RRCOMP_JPEG && fconfig.qual==95
			&& fconfig.subsamp==1) profchoice->value(2);
		else profchoice->value(3);
	}
}

void SetIF(void)
{
	if(!ifbutton) return;
	ifbutton->value(fconfig.interframe);
	if(strlen(fconfig.transport)>0 || fconfig.compress==RRCOMP_JPEG
		|| fconfig.compress==RRCOMP_RGB)
		ifbutton->activate();
	else ifbutton->deactivate();
}

void SetStereo(void)
{
	int i;
	if(!stereochoice) return;
	if(strlen(fconfig.transport)==0 && !fconfig.transvalid[RRTRANS_VGL])
		stereochoice->mode(2, FL_MENU_INACTIVE);
	else stereochoice->mode(2, 0);
	for(i=0; i<RR_STEREOOPT; i++)
		if(fconfig.stereo==i) stereochoice->value(i);
}

void SetFPS(void)
{
	char temps[20];
	snprintf(temps, 19, "%.2f", fconfig.fps);
	fpsinput->value(temps);
}

#if 0
void SetMovie(void)
{
	if(mcompchoice)
	{
		if(strlen(fconfig.moviefile)>0) mcompchoice->activate();
		else mcompchoice->deactivate();
		if(fconfig.mcompress==RRCOMP_JPEG) mcompchoice->value(0);
		else if(fconfig.mcompress==RRCOMP_RGB) mcompchoice->value(1);
	}
	if(msampchoice)
	{
		if(_Maxsubsamp[fconfig.mcompress]==_Minsubsamp[fconfig.mcompress]
			|| strlen(fconfig.moviefile)==0)
			msampchoice->deactivate();
		else msampchoice->activate();
		switch(fconfig.msubsamp)
		{
			case 0:  msampchoice->value(0);  break;
			case 1:  msampchoice->value(1);  break;
			case 2:  msampchoice->value(2);  break;
			case 4:  msampchoice->value(3);  break;
		}
	}
	if(mqualslider)
	{
		mqualslider->value(fconfig.mqual);
		if(fconfig.mcompress!=RRCOMP_JPEG || strlen(fconfig.moviefile)==0)
			mqualslider->deactivate();
		else mqualslider->activate();
	}
}
#endif


void CompCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	if(d>=0 && d<=RR_COMPRESSOPT-1 || strlen(fconfig.transport)>0)
		fconfig_setcompress(fconfig, d);
	SetSamp();
	SetQual();
	SetProf();
	SetIF();
	SetStereo();
	SetFPS();
}

void SampCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	fconfig.subsamp=d;
	SetProf();
}

void QualCB(Fl_Widget *w, void *data)
{
	Fl_Value_Slider *slider=(Fl_Value_Slider *)w;
	fconfig.qual=(int)slider->value();
	SetProf();
}

void ProfCB(Fl_Widget *w, void *data)
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
	SetComp();
	SetSamp();
	SetQual();
	SetStereo();
	SetIF();
}

void SpoilCB(Fl_Widget *w, void *data)
{
	Fl_Check_Button *check=(Fl_Check_Button *)w;
	fconfig.spoil=(check->value()!=0);
}

void GammaCB(Fl_Widget *w, void *data)
{
	Fl_Float_Input *input=(Fl_Float_Input *)w;
	fconfig_setgamma(fconfig, atof(input->value()));
	char temps[20];
	snprintf(temps, 19, "%.2f", fconfig.gamma);
	input->value(temps);
}

void IFCB(Fl_Widget *w, void *data)
{
	Fl_Check_Button *check=(Fl_Check_Button *)w;
	fconfig.interframe=(check->value()!=0);
}

void StereoCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	if(d>=0 && d<=RR_STEREOOPT-1) fconfig.stereo=d;
}

void FPSCB(Fl_Widget *w, void *data)
{
	Fl_Float_Input *input=(Fl_Float_Input *)w;
	fconfig.fps=atof(input->value());
	char temps[20];
	snprintf(temps, 19, "%.2f", fconfig.fps);
	input->value(temps);
}

#if 0
void MovieCB(Fl_Widget *w, void *data)
{
	Fl_Input *input=(Fl_Input *)w;
	strncpy(fconfig.moviefile, input->value(), MAXSTR-1);
	input->value(fconfig.moviefile);
	SetMovie();
}

void MCompCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	if(d>=RRCOMP_JPEG && d<=RRCOMP_RGB) fconfig.mcompress=d;
	SetMovie();
}

void MSampCB(Fl_Widget *w, void *data)
{
	int d=(int)((long)data);
	fconfig.msubsamp=d;
}

void MQualCB(Fl_Widget *w, void *data)
{
	Fl_Value_Slider *slider=(Fl_Value_Slider *)w;
	fconfig.mqual=(int)slider->value();
}
#endif


Fl_Menu_Item compmenu[]=
{
	{"None (X11 Transport)", 0, CompCB, (void *)RRCOMP_PROXY},
	{"JPEG (VGL Transport)", 0, CompCB, (void *)RRCOMP_JPEG},
	{"RGB (VGL Transport)", 0, CompCB, (void *)RRCOMP_RGB},
	{"YUV (XV Transport)", 0, CompCB, (void *)RRCOMP_XV},
	{"YUV (VGL Transport)", 0, CompCB, (void *)RRCOMP_YUV},
	{0, 0, 0, 0}
};

Fl_Menu_Item ctcompmenu[]=
{
	{"0", 0, CompCB, (void *)0},
	{"1", 0, CompCB, (void *)1},
	{"2", 0, CompCB, (void *)2},
	{"3", 0, CompCB, (void *)3},
	{"4", 0, CompCB, (void *)4},
	{"5", 0, CompCB, (void *)5},
	{"6", 0, CompCB, (void *)6},
	{"7", 0, CompCB, (void *)7},
	{"8", 0, CompCB, (void *)8},
	{"9", 0, CompCB, (void *)9},
	{"10", 0, CompCB, (void *)10},
	{0, 0, 0, 0}
};

Fl_Menu_Item sampmenu[]=
{
	{"Grayscale", 0, SampCB, (void *)0},
	{"1X", 0, SampCB, (void *)1},
	{"2X", 0, SampCB, (void *)2},
	{"4X", 0, SampCB, (void *)4},
	{"8X", 0, SampCB, (void *)8},
	{"16X", 0, SampCB, (void *)16},
	{0, 0, 0, 0}
};

Fl_Menu_Item profmenu[]=
{
	{"Low Qual (Low-Bandwidth Network)", 0, ProfCB, (void *)0},
	{"Medium Qual", 0, ProfCB, (void *)1},
	{"High Qual (High-Bandwidth Network)", 0, ProfCB, (void *)2},
	{"Custom", 0, 0, 0},
	{0, 0, 0, 0}
};

Fl_Menu_Item stereomenu[]=
{
	{"Send Left Eye Only", 0, StereoCB, (void *)RRSTEREO_LEYE},
	{"Send Right Eye Only", 0, StereoCB, (void *)RRSTEREO_REYE},
	{"Quad-Buffered (if available)", 0, StereoCB, (void *)RRSTEREO_QUADBUF},
	{"Anaglyphic (Red/Cyan)", 0, StereoCB, (void *)RRSTEREO_REDCYAN},
	{0, 0, 0, 0}
};

#if 0
Fl_Menu_Item mcompmenu[]=
{
	{"JPEG", 0, MCompCB, (void *)RRCOMP_JPEG},
	{"RGB", 0, MCompCB, (void *)RRCOMP_RGB},
	{0, 0, 0, 0}
};

Fl_Menu_Item msampmenu[]=
{
	{"Grayscale", 0, MSampCB, (void *)0},
	{"1X", 0, MSampCB, (void *)1},
	{"2X", 0, MSampCB, (void *)2},
	{"4X", 0, MSampCB, (void *)4},
	{0, 0, 0, 0}
};
#endif


void init(int argc, char **argv)
{
	char temps[20];

	errifnot(win=new Fl_Double_Window(485, 340, "VirtualGL Configuration"));

	if(strlen(fconfig.transport)>0)
	{
		errifnot(compchoice=new Fl_Choice(211, 10, 225, 25,
			"Image Compression: "));
		compchoice->menu(ctcompmenu);
	}
	else
	{
		errifnot(compchoice=new Fl_Choice(227, 10, 225, 25,
			"Image Compression (Transport): "));
		compchoice->menu(compmenu);
	}
	SetComp();

	errifnot(sampchoice=new Fl_Choice(210, 45, 100, 25,
		"Chrominance Subsampling: "));
	sampchoice->menu(sampmenu);
	SetSamp();

	if(strlen(fconfig.transport)>0)
	{
		errifnot(qualslider=new Fl_Value_Slider(30, 80, 335, 25,
			"Image Quality"));
	}
	else
	{
		errifnot(qualslider=new Fl_Value_Slider(30, 80, 335, 25,
			"JPEG Image Quality"));
	}
	qualslider->callback(QualCB, 0);
	qualslider->type(1);
	qualslider->minimum(1);
	qualslider->maximum(100);
	qualslider->step(1);
	qualslider->slider_size(0.01);
	SetQual();

	errifnot(profchoice=new Fl_Choice(157, 130, 280, 24,
		"Connection Profile: "));
	profchoice->menu(profmenu);
	profchoice->mode(3, FL_MENU_INACTIVE);
	SetProf();

	errifnot(gammainput=new Fl_Float_Input(315, 165, 85, 25,
		"Gamma Correction Factor (1.0=no correction): "));
	gammainput->callback(GammaCB, 0);
	snprintf(temps, 19, "%.2f", fconfig.gamma);
	gammainput->value(temps);

	errifnot(spoilbutton=new Fl_Light_Button(10, 200, 345, 25,
		"Frame Spoiling (Do Not Use When Benchmarking)"));
	spoilbutton->callback(SpoilCB, 0);
	spoilbutton->value(fconfig.spoil);

	errifnot(ifbutton=new Fl_Light_Button(10, 235, 185, 25,
		"Inter-Frame Comparison"));
	ifbutton->callback(IFCB, 0);
	SetIF();

	errifnot(stereochoice=new Fl_Choice(232, 270, 220, 25,
		"Stereographic Rendering Method: "));
	stereochoice->menu(stereomenu);
	SetStereo();

	errifnot(fpsinput=new Fl_Float_Input(240, 305, 85, 25,
		"Limit Frames/second (0.0=no limit): "));
	fpsinput->callback(FPSCB, 0);
	SetFPS();

	#if 0
	errifnot(moviebox=new Fl_Group(10, 430, 465, 175, "Movie Recorder"));
	moviebox->box(FL_ENGRAVED_BOX);

	errifnot(movieinput=new Fl_Input(252, 445, 210, 25,
		"Movie File (leave blank to disable): "));
	movieinput->callback(MovieCB, 0);
	movieinput->value(fconfig.moviefile);

	errifnot(mcompchoice=new Fl_Choice(198, 480, 225, 25,
		"Image Compression Type: "));
	mcompchoice->menu(mcompmenu);

	errifnot(msampchoice=new Fl_Choice(216, 515, 100, 25,
		"Chrominance Subsampling: "));
	msampchoice->menu(msampmenu);

	errifnot(mqualslider=new Fl_Value_Slider(35, 550, 335, 25,
		"JPEG Image Quality"));
	mqualslider->type(1);
  mqualslider->minimum(1);
 	mqualslider->maximum(100);
	mqualslider->step(1);
	mqualslider->slider_size(0.01);

	SetMovie();
	#endif

	win->end();
  win->show(argc, argv);
}


void checkparentpid(void *data)
{
	if(kill(ppid, 0)==-1)
	{
		delete win;  win=NULL;
	}
}


#define usage() {\
	rrout.print("USAGE: %s [-display <d>] -shmid <s> [-ppid <p>]\n\n", argv[0]); \
	rrout.print("<d> = X display to which to display the GUI (default: read from DISPLAY\n"); \
	rrout.print("      environment variable)\n"); \
	rrout.print("<s> = Shared memory segment ID (reported by VirtualGL when the\n"); \
	rrout.print("      environment variable VGL_VERBOSE is set to 1)\n"); \
	rrout.print("<p> = Parent process ID.  VGL Config will exit when this process\n"); \
	rrout.print("      terminates.\n"); \
	return -1;}


int main(int argc, char **argv)
{
	int status=0, shmid=-1;  bool test=false;
	char *darg[2]={NULL, NULL};
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
		if(darg[0] && darg[1]) {argv[1]=darg[0];  argv[2]=darg[1];  argc=3;}
		else argc=1;

		if(test)
		{
			if(!(_fconfig=fconfig_instance()))
				_throw("Could not allocate FakerConfig");
			fconfig_print(fconfig);
		}
		else
		{
			if(shmid==-1) usage();
			if((_fconfig=(FakerConfig *)shmat(shmid, 0, 0))==(FakerConfig *)-1)
				_throwunix();
			if(!_fconfig)
				_throw("Could not attach to config structure in shared memory");
		}

		init(argc, argv);
		if(ppid>0) Fl::add_check(checkparentpid);		
		status=Fl::run();
	}
	catch(rrerror &e)
	{
		rrout.print("Error in vglconfig--\n%s\n", e.getMessage());
		status=-1;
	}
	if(_fconfig)
	{
		if(test) fconfig_deleteinstance();
		else shmdt((char *)_fconfig);
	}
	return status;
}
