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

#include <X11/IntrinsicP.h>
#include "rrmutex.h"
#include "rrthread.h"
#include "rrlog.h"

class vglgui : public Runnable
{
	public:

		static vglgui *instance(void)
		{
			if(_Instanceptr==NULL)
			{
				rrcs::safelock l(_Instancemutex);
				if(_Instanceptr==NULL) _Instanceptr=new vglgui;
			}
			return _Instanceptr;
		}

		void init(void);
		void destroy(void);
		void popup(Display *dpy);

		void run(void)
		{
			try
			{
				init();
				if(_appctx) XtAppMainLoop(_appctx);
				destroy();
			}
			catch(rrerror &e)
			{
				rrout.println("Configuration dialog error--\n%s", e.getMessage());
			}
			_t->detach();  delete _t;  _t=NULL;
		}

	private:

		vglgui(void): _appctx(0), _toplevel(0), _qualtext(0),	_qualslider(0),
			_subsamp411(0), _subsamp422(0), _subsamp444(0), _spoil(0), _t(NULL),
			_dpy(NULL)
		{
		}

		virtual ~vglgui(void) {}
		void UpdateQual(void);
		static void qualScrollProc(Widget, XtPointer, XtPointer);
		static void qualJumpProc(Widget, XtPointer, XtPointer);
		static void subsamp411Proc(Widget, XtPointer, XtPointer);
		static void subsamp422Proc(Widget, XtPointer, XtPointer);
		static void subsamp444Proc(Widget, XtPointer, XtPointer);
		static void loQualProc(Widget, XtPointer, XtPointer);
		static void hiQualProc(Widget, XtPointer, XtPointer);
		static void quitProc(Widget, XtPointer, XtPointer);
		static void spoilProc(Widget, XtPointer, XtPointer);

		XtAppContext _appctx;
		Widget _toplevel, _qualtext, _qualslider, _subsamp411, _subsamp422,
			_subsamp444, _spoil;
		static vglgui *_Instanceptr;
		static rrcs _Instancemutex;
		static rrcs _Popupmutex;
		Thread *_t;
		Display *_dpy;
};

#define vglpopup(dpy) ((*(vglgui::instance())).popup(dpy))
