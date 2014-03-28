//
// "$Id: Fl_Repeat_Button.H,v 1.1 2007-09-01 23:40:02 dcommander Exp $"
//
// Repeat button header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#ifndef Fl_Repeat_Button_H
#define Fl_Repeat_Button_H
#include "Fl.H"
#include "Fl_Button.H"

class FL_EXPORT Fl_Repeat_Button : public Fl_Button {
  static void repeat_callback(void *);
public:
  int handle(int);
  Fl_Repeat_Button(int X,int Y,int W,int H,const char *l=0)
    : Fl_Button(X,Y,W,H,l) {}
  void deactivate() {
    Fl::remove_timeout(repeat_callback,this);
    Fl_Button::deactivate();
  }
};

#endif

//
// End of "$Id: Fl_Repeat_Button.H,v 1.1 2007-09-01 23:40:02 dcommander Exp $".
//
