include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

all:

##########################################################################
else
ifeq ($(platform), osx)
##########################################################################

all:

##########################################################################
else
ifeq ($(platform), osxx86)
##########################################################################

all:

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/libfltk_static.a

OBJS = $(ODIR)/Fl.o \
       $(ODIR)/Fl_Adjuster.o \
       $(ODIR)/Fl_Bitmap.o \
       $(ODIR)/Fl_Browser.o \
       $(ODIR)/Fl_Browser_.o \
       $(ODIR)/Fl_Browser_load.o \
       $(ODIR)/Fl_Box.o \
       $(ODIR)/Fl_Button.o \
       $(ODIR)/Fl_Chart.o \
       $(ODIR)/Fl_Check_Browser.o \
       $(ODIR)/Fl_Check_Button.o \
       $(ODIR)/Fl_Choice.o \
       $(ODIR)/Fl_Clock.o \
       $(ODIR)/Fl_Color_Chooser.o \
       $(ODIR)/Fl_Counter.o \
       $(ODIR)/Fl_Dial.o \
       $(ODIR)/Fl_Double_Window.o \
       $(ODIR)/Fl_File_Browser.o \
       $(ODIR)/Fl_File_Chooser.o \
       $(ODIR)/Fl_File_Chooser2.o \
       $(ODIR)/Fl_File_Icon.o \
       $(ODIR)/Fl_File_Input.o \
       $(ODIR)/Fl_Group.o \
       $(ODIR)/Fl_Help_View.o \
       $(ODIR)/Fl_Image.o \
       $(ODIR)/Fl_Input.o \
       $(ODIR)/Fl_Input_.o \
       $(ODIR)/Fl_Light_Button.o \
       $(ODIR)/Fl_Menu.o \
       $(ODIR)/Fl_Menu_.o \
       $(ODIR)/Fl_Menu_Bar.o \
       $(ODIR)/Fl_Sys_Menu_Bar.o \
       $(ODIR)/Fl_Menu_Button.o \
       $(ODIR)/Fl_Menu_Window.o \
       $(ODIR)/Fl_Menu_add.o \
       $(ODIR)/Fl_Menu_global.o \
       $(ODIR)/Fl_Multi_Label.o \
       $(ODIR)/Fl_Overlay_Window.o \
       $(ODIR)/Fl_Pack.o \
       $(ODIR)/Fl_Pixmap.o \
       $(ODIR)/Fl_Positioner.o \
       $(ODIR)/Fl_Preferences.o \
       $(ODIR)/Fl_Progress.o \
       $(ODIR)/Fl_Repeat_Button.o \
       $(ODIR)/Fl_Return_Button.o \
       $(ODIR)/Fl_Roller.o \
       $(ODIR)/Fl_Round_Button.o \
       $(ODIR)/Fl_Scroll.o \
       $(ODIR)/Fl_Scrollbar.o \
       $(ODIR)/Fl_Shared_Image.o \
       $(ODIR)/Fl_Single_Window.o \
       $(ODIR)/Fl_Slider.o \
       $(ODIR)/Fl_Tabs.o \
       $(ODIR)/Fl_Text_Buffer.o \
       $(ODIR)/Fl_Text_Display.o \
       $(ODIR)/Fl_Text_Editor.o \
       $(ODIR)/Fl_Tile.o \
       $(ODIR)/Fl_Tiled_Image.o \
       $(ODIR)/Fl_Tooltip.o \
       $(ODIR)/Fl_Valuator.o \
       $(ODIR)/Fl_Value_Input.o \
       $(ODIR)/Fl_Value_Output.o \
       $(ODIR)/Fl_Value_Slider.o \
       $(ODIR)/Fl_Widget.o \
       $(ODIR)/Fl_Window.o \
       $(ODIR)/Fl_Window_fullscreen.o \
       $(ODIR)/Fl_Window_hotspot.o \
       $(ODIR)/Fl_Window_iconize.o \
       $(ODIR)/Fl_Wizard.o \
       $(ODIR)/Fl_XBM_Image.o \
       $(ODIR)/Fl_XPM_Image.o \
       $(ODIR)/Fl_abort.o \
       $(ODIR)/Fl_add_idle.o \
       $(ODIR)/Fl_arg.o \
       $(ODIR)/Fl_compose.o \
       $(ODIR)/Fl_display.o \
       $(ODIR)/Fl_get_key.o \
       $(ODIR)/Fl_get_system_colors.o \
       $(ODIR)/Fl_grab.o \
       $(ODIR)/Fl_lock.o \
       $(ODIR)/Fl_own_colormap.o \
       $(ODIR)/Fl_visual.o \
       $(ODIR)/Fl_x.o \
       $(ODIR)/filename_absolute.o \
       $(ODIR)/filename_expand.o \
       $(ODIR)/filename_ext.o \
       $(ODIR)/filename_isdir.o \
       $(ODIR)/filename_list.o \
       $(ODIR)/filename_match.o \
       $(ODIR)/filename_setext.o \
       $(ODIR)/fl_arc.o \
       $(ODIR)/fl_arci.o \
       $(ODIR)/fl_ask.o \
       $(ODIR)/fl_boxtype.o \
       $(ODIR)/fl_color.o \
       $(ODIR)/fl_cursor.o \
       $(ODIR)/fl_curve.o \
       $(ODIR)/fl_diamond_box.o \
       $(ODIR)/fl_dnd.o \
       $(ODIR)/fl_draw.o \
       $(ODIR)/fl_draw_image.o \
       $(ODIR)/fl_draw_pixmap.o \
       $(ODIR)/fl_engraved_label.o \
       $(ODIR)/fl_file_dir.o \
       $(ODIR)/fl_font.o \
       $(ODIR)/fl_labeltype.o \
       $(ODIR)/fl_line_style.o \
       $(ODIR)/fl_oval_box.o \
       $(ODIR)/fl_overlay.o \
       $(ODIR)/fl_overlay_visual.o \
       $(ODIR)/fl_plastic.o \
       $(ODIR)/fl_read_image.o \
       $(ODIR)/fl_rect.o \
       $(ODIR)/fl_round_box.o \
       $(ODIR)/fl_rounded_box.o \
       $(ODIR)/fl_set_font.o \
       $(ODIR)/fl_set_fonts.o \
       $(ODIR)/fl_scroll_area.o \
       $(ODIR)/fl_shadow_box.o \
       $(ODIR)/fl_shortcut.o \
       $(ODIR)/fl_show_colormap.o \
       $(ODIR)/fl_symbols.o \
       $(ODIR)/fl_vertex.o \
       $(ODIR)/flstring.o \
       $(ODIR)/screen_xywh.o

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

-include $(OBJS:.o=.d)

$(LDIR)/libfltk_static.a: $(OBJS)
	$(AR) $@ $^

##########################################################################
endif
endif
endif
##########################################################################
