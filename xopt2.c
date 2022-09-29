/*
    Flitsim : A fully adaptable X-Windows based network simulator
    Copyright (C) 1993  Patrick Gaughan, Sudha Yalamanchili, Peter Flur
                        and Binh Dao


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/*------------------------------------------------------------*/
/* xdist()  This module implements Xwindows display of the    */
/*          routing simulation for visualization purposes.    */
/*          (this should be fun...)                           */
/*------------------------------------------------------------*/

static char xdist_rcsid[] = "$Id: xopt2.c,v 1.1 1994/01/21 18:55:19 pgaughan Exp dao $";

#include <stdio.h>
#include "dat2.h"
#ifndef NO_X
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Intrinsics */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/* Widgets */
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Shell.h>
#include <X11/Xaw/StripChart.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>



#endif

#define XTL  100
#define YTL  100
#define WD   640
#define HT   640
#define MAXX WD/NODE_SPACE
#define MAXY HT/NODE_SPACE

#define NODE_SIZE   60
#define LINK_SIZE   5
#define NODE_SPACE  80
#define BUFF_SIZE   5
#define DISHA_TOKEN_SIZE 	8;

#define START_X 200
#define START_Y 200

#define TIMEOUT   1

#ifndef NO_X
extern XFontStruct *smallfonts, *bigfonts;
extern Display *the_display, *display;
extern Window my_window, root;
extern GC gc, draw_gc, undraw_gc;
extern XGCValues values;
extern Pixmap big_picture;
extern Colormap my_colors;
extern Visual visual;
extern XSetWindowAttributes attribs;
extern XTextProperty main_window_name;

/* Widget Programming */
extern XtAppContext app_context;
extern Widget topLevel, hello, quit, stop, simBox, box, stats;
extern Widget simWindow, simHeader, sim, newsim, chart;
extern Widget statsWindow, statsForm, statsQuit, statsBox, statsText;
extern Widget maxTime;


extern int DEBUG, SECURITY_LEVELS;
extern int HALF, SIM_STATUS, RUN_TIME;


Widget optionsWindow, optionsBox, Wlevel, Wproto, Wsize, WDuplex;
Widget Whalf, Wfull;


static void
Flip(widget, closure, callData)
Widget widget;
XtPointer closure, callData;
{
   Arg arg[1];
   char text[10];

   HALF = ! HALF;
   sprintf( text, "%s Duplex ", HALF ? "Half" : "Full" );
   XtVaSetValues( widget, XtNlabel, text );
}
#endif

void xSetUpOptions()
{
#ifndef NO_X
  optionsWindow = XtVaAppCreateShell
	(
	 "Options",
	 "Options Window", applicationShellWidgetClass,
	 display, 
	 XtNcolormap, my_colors,
	 NULL
	 );

  optionsBox = XtVaCreateManagedWidget
	(
	 "Form",
	 formWidgetClass,
	 optionsWindow, NULL);
      
  Wlevel = XtVaCreateManagedWidget
	(
	 "Security Levels",
	 commandWidgetClass,
	 optionsBox, 
	 NULL);
      
  Wproto = XtVaCreateManagedWidget
	(
	 "Protocol",
	 commandWidgetClass,
	 optionsBox, 
	 XtNfromVert, Wlevel,
	 NULL);
      
  Wsize = XtVaCreateManagedWidget
	(
	 "Network Size",
	 commandWidgetClass,
	 optionsBox, 
	 XtNfromVert, Wproto,
	 NULL);

/*  Whalf = XtCreatePopupShell("menu", simpleMenuWidgetClass, 
			     button, NULL,ZERO);

  sprintf(buf, "menuEntry%d", i + 1);
  (void) XtCreateManagedWidget(buf, smeBSBObjectClass, menu,
                                     NULL, ZERO); */
      
  XtRealizeWidget(optionsWindow);

  if ( DEBUG == 1 ) 
    {
      printf("in SetUp Options\n");
    }
#endif
}
