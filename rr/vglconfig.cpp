/* Copyright (C)2007 Sun Microsystems, Inc.
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

#include <sys/types.h>
#include <sys/shm.h>
#include <X11/IntrinsicP.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#define __FAKERCONFIG_STATICDEF__
#include "fakerconfig.h"
#include "rr.h"
#include "rrlog.h"

XtAppContext _appctx=0;
Widget _toplevel=0, _qualtext=0, _qualslider=0, _subsampgray=0, _subsamp4x=0,
	_subsamp2x=0, _subsamp1x=0, _spoil=0;
Display *_dpy=NULL;

#undef fconfig
FakerConfig *_fconfig=NULL;
#define fconfig (*_fconfig)

String qualslider_translations = "#override\n\
		<Btn1Down>: StartScroll(Continuous) MoveThumb() NotifyThumb()\n\
		<Btn1Motion>: MoveThumb() NotifyThumb()\n\
		<Btn3Down>: StartScroll(Continuous) MoveThumb() NotifyThumb()\n\
		<Btn3Motion>: MoveThumb() NotifyThumb()";

void mainloop(void)
{
	while(!XtAppGetExitFlag(_appctx))
	{
		XEvent e;
		XtAppNextEvent(_appctx, &e);
		if(e.type==ClientMessage
			&& e.xclient.message_type==XInternAtom(_dpy, "WM_PROTOCOLS", False)
			&& (Atom)e.xclient.data.l[0]==XInternAtom(_dpy, "WM_DELETE_WINDOW", False))
			XtAppSetExitFlag(_appctx);
		else
			XtDispatchEvent(&e);
	}
}

void Die(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	XtAppContext appctx=XtWidgetToApplicationContext(w);
	if(appctx) XtAppSetExitFlag(appctx);
}

XtActionsRec actions[]=
{
	{(char *)"Die", Die}
};

void destroy(void)
{
	if(_toplevel)
	{
		XtUnrealizeWidget(_toplevel);
		XtDestroyWidget(_toplevel);
		_toplevel=_qualtext=_qualslider=_subsampgray=_subsamp4x=_subsamp2x
			=_subsamp1x=0;
	}
	if(_dpy)
	{
		XtCloseDisplay(_dpy);  _dpy=NULL;
	}
	if(_appctx)
	{
		XtDestroyApplicationContext(_appctx);  _appctx=0;
	}
}

void UpdateQual(void)
{
	char text[10];
	if(_qualslider)
	{
	  float f=(float)fconfig.qual/100.;
		XtArgVal *p=(XtArgVal *)&f;  Arg args[1];
		if(f>0.99) f=0.99;
		#ifdef sun
		XawScrollbarSetThumb(_qualslider, f, 0.0);
		#else
		if(sizeof(float)>sizeof(XtArgVal))
			XtSetArg(args[0], XtNtopOfThumb, &f);
		else
			XtSetArg(args[0], XtNtopOfThumb, *p);
		XtSetValues(_qualslider, args, 1);
		#endif
	}
	if(_qualtext)
	{
		sprintf(text, "%d", (int)fconfig.qual);
		XtVaSetValues(_qualtext, XtNlabel, text, NULL);
	}
	if(_subsamp4x && _subsamp2x && _subsamp1x)
	{
		if(fconfig.subsamp==1)
		{
			XtVaSetValues(_subsamp4x, XtNstate, 0, NULL);
			XtVaSetValues(_subsamp2x, XtNstate, 0, NULL);
			XtVaSetValues(_subsamp1x, XtNstate, 1, NULL);
		}
		else if(fconfig.subsamp==4)
		{
			XtVaSetValues(_subsamp4x, XtNstate, 1, NULL);
			XtVaSetValues(_subsamp2x, XtNstate, 0, NULL);
			XtVaSetValues(_subsamp1x, XtNstate, 0, NULL);
		}
		else if(fconfig.subsamp==2)
		{
			XtVaSetValues(_subsamp4x, XtNstate, 0, NULL);
			XtVaSetValues(_subsamp2x, XtNstate, 1, NULL);
			XtVaSetValues(_subsamp1x, XtNstate, 0, NULL);
		}
	}
	if(fconfig.spoil)
	{
		XtVaSetValues(_spoil, XtNlabel, "Frame Spoiling: ON ", NULL);
		XtVaSetValues(_spoil, XtNstate, 1, NULL);
	}
	else
	{
		XtVaSetValues(_spoil, XtNlabel, "Frame Spoiling: off", NULL);
		XtVaSetValues(_spoil, XtNstate, 0, NULL);
	}
}

void qualScrollProc(Widget w, XtPointer client, XtPointer p)
{
	float	size, val;  int qual;  long pos=(long)p;
	XtVaGetValues(w, XtNshown, &size, XtNtopOfThumb, &val, NULL);
	if(pos<0) val-=.1;  else val+=.1;
	if(val>1.0) val=1.0;  if(val<0.0) val=0.0;
	qual=(int)(val*100.);  if(qual<1) qual=1;  if(qual>100) qual=100;
	fconfig.qual=qual;
	UpdateQual();
}

void qualJumpProc(Widget w, XtPointer client, XtPointer p)
{
	float val=*(float *)p;  int qual;
	qual=(int)(val*100.);  if(qual<1) qual=1;  if(qual>100) qual=100;
	fconfig.qual=qual;
	UpdateQual();
}

void loQualProc(Widget w, XtPointer client, XtPointer p)
{
	fconfig.subsamp=4;
	fconfig.qual=30;
	UpdateQual();
}

void hiQualProc(Widget w, XtPointer client, XtPointer p)
{
	fconfig.subsamp=1;
	fconfig.qual=95;
	UpdateQual();
}

void quitProc(Widget w, XtPointer client, XtPointer p)
{
	XtAppContext appctx=XtWidgetToApplicationContext(w);
	if(appctx) XtAppSetExitFlag(appctx);
}

void subsamp4xProc(Widget w, XtPointer client, XtPointer p)
{
	if((long)p==1) fconfig.subsamp=4;
	UpdateQual();
}

void subsamp2xProc(Widget w, XtPointer client, XtPointer p)
{
	if((long)p==1) fconfig.subsamp=2;
	UpdateQual();
}

void subsamp1xProc(Widget w, XtPointer client, XtPointer p)
{
	if((long)p==1) fconfig.subsamp=1;
	UpdateQual();
}

void spoilProc(Widget w, XtPointer client, XtPointer p)
{
	if((long)p==1 || (long)p==0)
	{
		int spoil=(int)((long)p);
		fconfig.spoil=spoil? true:false;
	}
	UpdateQual();
}

void init(int argc, char **argv)
{
	XtToolkitThreadInitialize();
	XtToolkitInitialize();
	errifnot(_appctx=XtCreateApplicationContext());
	errifnot(_dpy=XtOpenDisplay(_appctx, NULL, "VirtualGL", "dialog", NULL, 0,
		&argc, argv));
	errifnot(_toplevel=XtVaAppCreateShell("VirtualGL", "dialog",
		applicationShellWidgetClass, _dpy, XtNborderWidth, 0, 
		XtNtitle, "VirtualGL Configuration", NULL));
	Widget buttonForm=XtVaCreateManagedWidget("buttonForm", formWidgetClass,
		_toplevel, NULL);
	errifnot(buttonForm);

	Widget quitbutton=XtVaCreateManagedWidget("quitbutton", commandWidgetClass,
		buttonForm, NULL);
	errifnot(quitbutton);
	XtVaSetValues(quitbutton, XtNleft, XawChainLeft, XtNright, XawChainRight,
		XtNlabel, "Close dialog", NULL);
	XtAddCallback(quitbutton, XtNcallback, quitProc, NULL);

	errifnot(_spoil=XtCreateManagedWidget("spoil", toggleWidgetClass, buttonForm,
		NULL, 0));
	XtVaSetValues(_spoil, XtNfromVert, quitbutton, XtNleft, XawChainLeft,
		XtNright, XawChainRight, XtNlabel, "Frame Spoiling: XXX", NULL);
	XtAddCallback(_spoil, XtNcallback, spoilProc, NULL);

	if(fconfig.compress==RRCOMP_JPEG)
	{
		Widget lobutton=XtVaCreateManagedWidget("lobutton", commandWidgetClass,
			buttonForm, NULL);
		errifnot(lobutton);
		XtVaSetValues(lobutton, XtNfromVert, _spoil, XtNleft, XawChainLeft,
			XtNright, XawChainRight, XtNlabel, "Qual Preset: Broadband/T1", NULL);
		XtAddCallback(lobutton, XtNcallback, loQualProc, NULL);

		Widget hibutton=XtVaCreateManagedWidget("hibutton", commandWidgetClass,
			buttonForm, NULL);
		errifnot(hibutton);
		XtVaSetValues(hibutton, XtNfromVert, lobutton, XtNleft, XawChainLeft,
			XtNright, XawChainRight, XtNlabel, "Qual Preset: LAN (default)", NULL);
		XtAddCallback(hibutton, XtNcallback, hiQualProc, NULL);

		Widget quallabel=XtCreateManagedWidget("quallabel", labelWidgetClass,
			buttonForm, NULL, 0);
		errifnot(quallabel);
		XtVaSetValues(quallabel, XtNfromVert, hibutton, XtNleft, XawChainLeft,
			XtNright, XawChainRight, XtNlabel, "JPEG Quality", XtNborderWidth, 0,
			NULL);

		errifnot(_qualslider=XtCreateManagedWidget("qualslider", scrollbarWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_qualslider, XtNfromVert, quallabel, XtNleft, XawChainLeft,
			XtNlength, 100, XtNwidth, 130, XtNorientation, XtorientHorizontal,
			XtNtranslations, XtParseTranslationTable(qualslider_translations), 
			XtNheight, 17, NULL);
		XtAddCallback(_qualslider, XtNscrollProc, qualScrollProc, NULL);
		XtAddCallback(_qualslider, XtNjumpProc, qualJumpProc, NULL);

		errifnot(_qualtext=XtCreateManagedWidget("qualtext", labelWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_qualtext, XtNfromVert, quallabel, XtNfromHoriz, _qualslider,
			XtNright, XawChainRight, XtNlabel, "000", XtNborderWidth, 0, NULL);

		Widget subsamplabel=XtCreateManagedWidget("subsamplabel", labelWidgetClass,
			buttonForm, NULL, 0);
		errifnot(subsamplabel);
		XtVaSetValues(subsamplabel, XtNfromVert, _qualslider, XtNleft, XawChainLeft,
			XtNright, XawChainRight,
			XtNlabel, "JPEG Subsampling\n[4x = fastest]\n[None = best quality]",
			XtNborderWidth, 0, NULL);

		errifnot(_subsamp4x=XtCreateManagedWidget("subsamp4x", toggleWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_subsamp4x, XtNfromVert, subsamplabel, XtNleft, XawChainLeft,
			XtNlabel, "4x", NULL);
		XtAddCallback(_subsamp4x, XtNcallback, subsamp4xProc, NULL);

		errifnot(_subsamp2x=XtCreateManagedWidget("subsamp2x", toggleWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_subsamp2x, XtNfromVert, subsamplabel, XtNfromHoriz,
			_subsamp4x, XtNradioGroup, _subsamp4x, XtNlabel, "2x", NULL);
		XtAddCallback(_subsamp2x, XtNcallback, subsamp2xProc, NULL);

		errifnot(_subsamp1x=XtCreateManagedWidget("subsamp1x", toggleWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_subsamp1x, XtNfromVert, subsamplabel, XtNfromHoriz,
			_subsamp2x, XtNradioGroup, _subsamp4x, XtNlabel, "None", NULL);
		XtAddCallback(_subsamp1x, XtNcallback, subsamp1xProc, NULL);
	}
	XtAppAddActions(_appctx, actions, XtNumber(actions));

	XtRealizeWidget(_toplevel);

	Atom deleteatom=XInternAtom(_dpy, "WM_DELETE_WINDOW", False);
	if(!XSetWMProtocols(_dpy, XtWindow(_toplevel), &deleteatom, 1))
		_throw("Could not set WM protocols");
	XtOverrideTranslations(_toplevel, XtParseTranslationTable ("<Message>WM_PROTOCOLS: Die()"));
	UpdateQual();
}




#define usage() {\
	rrout.print("USAGE: %s [-display <d>] -shmid <id>\n\n", argv[0]); \
	rrout.print("<d> = X display to which to display the GUI (default: read from DISPLAY\n"); \
	rrout.print("      environment variable)\n"); \
	rrout.print("<id> = Shared memory segment ID (reported by VirtualGL when the\n"); \
	rrout.print("       environment variable VGL_VERBOSE is set to 1)\n"); \
	return -1;}


int main(int argc, char **argv)
{
	int status=0, shmid=-1;
	try
	{
		if(argc<3) usage();
		for(int i=0; i<argc; i++)
		{
			if(!stricmp(argv[i], "-shmid") && i<argc-1)
			{
				int temp=atoi(argv[++i]);  if(temp>-1) shmid=temp;
			}
		}
		if(shmid==-1) usage();

		if((_fconfig=(FakerConfig *)shmat(shmid, 0, 0))==(FakerConfig *)-1)
			_throwunix();
		if(!_fconfig) _throw("Could not attached to config structure in shared memory");

		init(argc, argv);
		mainloop();
	}
	catch(rrerror &e)
	{
		rrout.print("Error in vglconfig--\n%s\n", e.getMessage());
		status=-1;
	}
	destroy();
	if(_fconfig) shmdt((char *)_fconfig);
	return status;
}
