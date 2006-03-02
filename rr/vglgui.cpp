/* Copyright (C)2006 Sun Microsystems, Inc.
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

#include "vglgui.h"
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include "fakerconfig.h"
#include "rr.h"

extern FakerConfig fconfig;

static const char *fallback_resources[]=
{
	"*quallabel.label: JPEG Quality",
	"*qualslider.length: 100",
	"*qualslider.width: 130",
	"*qualslider.orientation: horizontal",
	"*qualslider.translations: #override\\n\
		<Btn1Down>: StartScroll(Continuous) MoveThumb() NotifyThumb()\\n\
		<Btn1Motion>: MoveThumb() NotifyThumb()\\n\
		<Btn3Down>: StartScroll(Continuous) MoveThumb() NotifyThumb()\\n\
		<Btn3Motion>: MoveThumb() NotifyThumb()",

	"*qualtext.label: 000",

	"*subsamplabel.label: JPEG Subsampling\\n[4:1:1 = fastest]\\n[None = best quality]",
	"*subsamp411.label: 4:1:1",
	"*subsamp422.label: 4:2:2",
	"*subsamp444.label: None",

	"*spoil.label: Frame Spoiling: XXX",

	"*dialog.title: VirtualGL Configuration",
	"*dialog*background: grey",
	"*dialog.buttonForm.Label.borderWidth: 0",

	"*dialog*quitbutton.label: Close dialog",
	"*dialog*lobutton.label: Qual Preset: Broadband/T1",
	"*dialog*hibutton.label: Qual Preset: LAN (default)",

  NULL
};

vglgui *vglgui::_Instanceptr=NULL;
rrcs vglgui::_Instancemutex, vglgui::_Popupmutex;

static void Die(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	vglgui *that=vglgui::instance();
	if(that) that->destroy();
}

static XtActionsRec actions[]=
{
	{(char *)"Die", Die}
};

void vglgui::destroy(void)
{
	rrcs::safelock l(_Popupmutex);
	if(_toplevel)
	{
		XtUnrealizeWidget(_toplevel);
		XtDestroyWidget(_toplevel);
		_toplevel=_qualtext=_qualslider=_subsamp411=_subsamp422=_subsamp444=0;
	}
	if(_appctx)
	{
		XtAppSetExitFlag(_appctx);
	}
	if(_t) {_t->stop();  delete _t;  _t=NULL;}
	if(_appctx)
	{
		XtDestroyApplicationContext(_appctx);  _appctx=0;
	}
	if(_dpy)
	{
		XtCloseDisplay(_dpy);  _dpy=NULL;
	}
}

void vglgui::popup(Display *dpy)
{
	int argc=1;  char *argv[2]={(char *)"VirtualGL", NULL};

	if(!dpy) _throw("Invalid argument");
	rrcs::safelock l(_Popupmutex);
	if(_toplevel) return;
	XtToolkitInitialize();
	errifnot(_appctx=XtCreateApplicationContext());
	XtAppSetFallbackResources(_appctx, (char **)fallback_resources);
	errifnot(_dpy=XtOpenDisplay(_appctx, DisplayString(dpy), "VirtualGL",
		"dialog", NULL, 0, &argc, argv));
	errifnot(_toplevel=XtVaAppCreateShell("VirtualGL", "dialog",
		applicationShellWidgetClass, _dpy, argv, &argc, fallback_resources,
		XtNborderWidth, 0, NULL));
	Widget buttonForm=XtVaCreateManagedWidget("buttonForm", formWidgetClass,
		_toplevel, NULL);
	errifnot(buttonForm);

	Widget quitbutton=XtVaCreateManagedWidget("quitbutton", commandWidgetClass,
		buttonForm, NULL);
	errifnot(quitbutton);
	XtVaSetValues(quitbutton, XtNleft, XawChainLeft, XtNright, XawChainRight,
		NULL);
	XtAddCallback(quitbutton, XtNcallback, quitProc, this);

	errifnot(_spoil=XtCreateManagedWidget("spoil", toggleWidgetClass, buttonForm,
		NULL, 0));
	XtVaSetValues(_spoil, XtNfromVert, quitbutton, XtNleft, XawChainLeft,
		XtNright, XawChainRight, NULL);
	XtAddCallback(_spoil, XtNcallback, spoilProc, this);

	if(fconfig.compress!=RRCOMP_NONE)
	{
		Widget lobutton=XtVaCreateManagedWidget("lobutton", commandWidgetClass,
			buttonForm, NULL);
		errifnot(lobutton);
		XtVaSetValues(lobutton, XtNfromVert, _spoil, XtNleft, XawChainLeft,
			XtNright, XawChainRight, NULL);
		XtAddCallback(lobutton, XtNcallback, loQualProc, this);

		Widget hibutton=XtVaCreateManagedWidget("hibutton", commandWidgetClass,
			buttonForm, NULL);
		errifnot(hibutton);
		XtVaSetValues(hibutton, XtNfromVert, lobutton, XtNleft, XawChainLeft,
			XtNright, XawChainRight, NULL);
		XtAddCallback(hibutton, XtNcallback, hiQualProc, this);

		Widget quallabel=XtCreateManagedWidget("quallabel", labelWidgetClass,
			buttonForm, NULL, 0);
		errifnot(quallabel);
		XtVaSetValues(quallabel, XtNfromVert, hibutton, XtNleft, XawChainLeft,
			XtNright, XawChainRight, NULL);

		errifnot(_qualslider=XtCreateManagedWidget("qualslider", scrollbarWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_qualslider, XtNfromVert, quallabel, XtNleft, XawChainLeft,
			NULL);
		XtAddCallback(_qualslider, XtNscrollProc, qualScrollProc, this);
		XtAddCallback(_qualslider, XtNjumpProc, qualJumpProc, this);

		errifnot(_qualtext=XtCreateManagedWidget("qualtext", labelWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_qualtext, XtNfromVert, quallabel, XtNfromHoriz, _qualslider,
			XtNright, XawChainRight, NULL);

		Widget subsamplabel=XtCreateManagedWidget("subsamplabel", labelWidgetClass,
			buttonForm, NULL, 0);
		errifnot(subsamplabel);
		XtVaSetValues(subsamplabel, XtNfromVert, _qualslider, XtNleft, XawChainLeft,
			XtNright, XawChainRight, NULL);

		errifnot(_subsamp411=XtCreateManagedWidget("subsamp411", toggleWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_subsamp411, XtNfromVert, subsamplabel, XtNleft, XawChainLeft,
			NULL);
		XtAddCallback(_subsamp411, XtNcallback, subsamp411Proc, this);

		errifnot(_subsamp422=XtCreateManagedWidget("subsamp422", toggleWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_subsamp422, XtNfromVert, subsamplabel, XtNfromHoriz,
			_subsamp411, XtNradioGroup, _subsamp411, NULL);
		XtAddCallback(_subsamp422, XtNcallback, subsamp422Proc, this);

		errifnot(_subsamp444=XtCreateManagedWidget("subsamp444", toggleWidgetClass,
			buttonForm, NULL, 0));
		XtVaSetValues(_subsamp444, XtNfromVert, subsamplabel, XtNfromHoriz,
			_subsamp422, XtNradioGroup, _subsamp411, NULL);
		XtAddCallback(_subsamp444, XtNcallback, subsamp444Proc, this);
	}
	XtAppAddActions(_appctx, actions, XtNumber(actions));

	XtRealizeWidget(_toplevel);

	Atom deleteatom=XInternAtom(_dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(_dpy, XtWindow(_toplevel), &deleteatom, 1);
	XtOverrideTranslations(_toplevel, XtParseTranslationTable ("<Message>WM_PROTOCOLS: Die()"));
	UpdateQual();
	errifnot(_t=new Thread(this));
	_t->start();
}

void vglgui::UpdateQual(void)
{
	char text[10];
	if(_qualslider)
	{
	  float f=(float)fconfig.currentqual/100.;
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
		sprintf(text, "%d", fconfig.currentqual);
		XtVaSetValues(_qualtext, XtNlabel, text, NULL);
	}
	if(_subsamp411 && _subsamp422 && _subsamp444)
	{
		if(fconfig.currentsubsamp==RR_444)
		{
			XtVaSetValues(_subsamp411, XtNstate, 0, NULL);
			XtVaSetValues(_subsamp422, XtNstate, 0, NULL);
			XtVaSetValues(_subsamp444, XtNstate, 1, NULL);
		}
		else if(fconfig.currentsubsamp==RR_411)
		{
			XtVaSetValues(_subsamp411, XtNstate, 1, NULL);
			XtVaSetValues(_subsamp422, XtNstate, 0, NULL);
			XtVaSetValues(_subsamp444, XtNstate, 0, NULL);
		}
		else if(fconfig.currentsubsamp==RR_422)
		{
			XtVaSetValues(_subsamp411, XtNstate, 0, NULL);
			XtVaSetValues(_subsamp422, XtNstate, 1, NULL);
			XtVaSetValues(_subsamp444, XtNstate, 0, NULL);
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

#define newqual(q) {  \
	char *env=NULL, temps[15];  \
	fconfig.currentqual=q;  \
	fconfig.sanitycheck();  \
	if((env=getenv("VGL_QUAL"))!=NULL && strlen(env)>0)  \
	{  \
		sprintf(temps, "VGL_QUAL=%d", fconfig.currentqual);  \
		putenv(temps);  \
	}  \
	if((env=getenv("RRQUAL"))!=NULL && strlen(env)>0)  \
	{  \
		sprintf(temps, "RRQUAL=%d", fconfig.currentqual);  \
		putenv(temps);  \
	}}

#define newsubsamp(s) {  \
	char *env=NULL, temps[18];  \
	fconfig.currentsubsamp=s;  \
	fconfig.sanitycheck();  \
	if((env=getenv("VGL_SUBSAMP"))!=NULL && strlen(env)>0)  \
	{  \
		sprintf(temps, "VGL_SUBSAMP=%d", fconfig.currentsubsamp==RR_411? 411:  \
			fconfig.currentsubsamp==RR_422? 422:444);  \
		putenv(temps);  \
	}  \
	if((env=getenv("RRSUBSAMP"))!=NULL && strlen(env)>0)  \
	{  \
		sprintf(temps, "RRSUBSAMP=%d", fconfig.currentsubsamp==RR_411? 411:  \
			fconfig.currentsubsamp==RR_422? 422:444);  \
		putenv(temps);  \
	}}

void vglgui::qualScrollProc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	float	size, val;  int qual;  long pos=(long)p;
	XtVaGetValues(w, XtNshown, &size, XtNtopOfThumb, &val, 0);
	if(pos<0) val-=.1;  else val+=.1;
	if(val>1.0) val=1.0;  if(val<0.0) val=0.0;
	qual=(int)(val*100.);  if(qual<1) qual=1;  if(qual>100) qual=100;
	newqual(qual);
	if(that) that->UpdateQual();
}

void vglgui::qualJumpProc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	float val=*(float *)p;  int qual;
	qual=(int)(val*100.);  if(qual<1) qual=1;  if(qual>100) qual=100;
	newqual(qual);
	if(that) that->UpdateQual();
}

void vglgui::loQualProc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	newsubsamp(RR_411);
	newqual(30);
	if(that) that->UpdateQual();
}

void vglgui::hiQualProc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	newsubsamp(RR_444);
	newqual(95);
	if(that) that->UpdateQual();
}

void vglgui::quitProc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	if(that) that->destroy();
}

void vglgui::subsamp411Proc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	if((long)p==1) newsubsamp(RR_411);
	if(that) that->UpdateQual();
}

void vglgui::subsamp422Proc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	if((long)p==1) newsubsamp(RR_422);
	if(that) that->UpdateQual();
}

void vglgui::subsamp444Proc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	if((long)p==1) newsubsamp(RR_444);
	if(that) that->UpdateQual();
}

void vglgui::spoilProc(Widget w, XtPointer client, XtPointer p)
{
	vglgui *that=(vglgui *)client;
	if((long)p==1 || (long)p==0)
	{
		int spoil=(int)((long)p);
		char *env=NULL, temps[15];
		if((env=getenv("VGL_SPOIL"))!=NULL && strlen(env)>0)
		{
			sprintf(temps, "VGL_SPOIL=%.1d", spoil);
			putenv(temps);
		}
		if((env=getenv("RRSPOIL"))!=NULL && strlen(env)>0)
		{
			sprintf(temps, "RRSPOIL=%.1d", spoil);
			putenv(temps);
		}
		fconfig.spoil=spoil? true:false;
		fconfig.sanitycheck();
	}
	if(that) that->UpdateQual();
}
