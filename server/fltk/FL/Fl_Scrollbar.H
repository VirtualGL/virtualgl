//
// "$Id: Fl_Scrollbar.H 6042 2008-02-25 13:00:53Z matt $"
//
// Scroll bar header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Scrollbar_H
#define Fl_Scrollbar_H

#include "Fl_Slider.H"

class FL_EXPORT Fl_Scrollbar : public Fl_Slider {

  int linesize_;
  int pushed_;
  static void timeout_cb(void*);
  void increment_cb();
protected:
  void draw();

public:

  Fl_Scrollbar(int x,int y,int w,int h, const char *l = 0);
  ~Fl_Scrollbar();
  int handle(int);

  int value() {return int(Fl_Slider::value());}
  int value(int p, int s, int top, int total) {
    return scrollvalue(p, s, top, total);
  }
  int linesize() const {return linesize_;}
  void linesize(int i) {linesize_ = i;}

};

#endif

//
// End of "$Id: Fl_Scrollbar.H 6042 2008-02-25 13:00:53Z matt $".
//
