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

static char xdisp_rcsid[] = "$Id: xd2.c,v 1.7 1994/04/08 13:46:52 pgaughan Exp dao $";

/*------------------------------------------------------------*/
/* xdisp.c  This module implements Xwindows display of the    */
/*          routing simulation for visualization purposes.    */
/*          (this should be fun...)                           */
/*------------------------------------------------------------*/
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


#endif

/* newly added to support scaling of display and support for DISHA */
extern	int	SCALE;			
extern	int	NEW_DISHA_FEATURES;

#define max(x,y) (x > y ? (x) : (y))

#define PROFILE_LENGTH  150
#define PROFILE_HT      40

#define XTL  100
#define YTL  100
#define WD   640 * SCALE
#define HT   640 * SCALE
#define MAXX WD/NODE_SPACE
#define MAXY HT/NODE_SPACE

int NODE_MAX  =  64;
int NODE_SIZE =  60;
int LINK_SIZE =  8;
int NODE_SPACE=  80;
int BUFF_SIZE =   5;
int MAX_ROW   =   8;

/* Newly added to support visualization of DISHA */
int DISHA_TOKEN_SIZE =   12;
int DEADLOCK_BUFFER_SIZE = 16;

#define START_X 50
#define START_Y  50

#define TIMEOUT   0

#define kill_color 18

#ifndef NO_X
XFontStruct *smallfonts, *bigfonts;
Display *the_display, *display;
Window my_window, root;
GC gc, draw_gc, undraw_gc;
XGCValues values;
Pixmap big_picture;
Colormap my_colors;
Visual visual;
XSetWindowAttributes attribs;
XTextProperty main_window_name;

/* Widget Programming */
XtAppContext app_context;
Widget topLevel, hello, quit, stop, simBox, box, stats, stepButton;
Widget cpro, ppro, opro;
Widget simWindow, simHeader, sim, newsim, chart;
Widget statsWindow, statsForm, statsQuit, statsBox, statsText;
Widget maxTime, buttonBox;
#endif

int black, white;
int red[19] = {65,40,55,65,30,65,65,65,30,40,65,20,65,40,20,50, 20,40, 65};
int green[19]={45,65,55,65,65,30,65,30,65,40,50,60,20,20,65,65, 20,40,  0};
int blue[19] ={45,40,65,30,65,65,65,30,30,65,20,65,50,65,50,20, 20,40, 65};

int colors[19], pm[19];
rgb *security_rgb;
int *security_colors, security_pm;
int pixmap_height_in_pixels, pixmap_width_in_pixels;
extern int DEBUG, SECURITY_LEVELS;
extern int HALF, SIM_STATUS, RUN_TIME, STEP;
extern node *nodes;
extern plink *plinks;
extern vlane *vlinks;
extern int sign_of_link(), NODES;
extern FILE *fp;
extern int idle_pes, over_pes, msgflits;
int compro[PROFILE_LENGTH], pepro[PROFILE_LENGTH], ovpro[PROFILE_LENGTH];
int xmax, xstart, comymax, peymax, ovymax;

int max_buff, max_link, max_pl;
extern int virts, *width, sim_time, node_virts;
extern char proto_name[40];
extern char filename[];
rgb *build_colors();
void XeventLoop();
extern void xSetUpOptions();
extern int XDISP, traffic;

#ifndef NO_X
void TimePoint( w, closure, call_data )
     Widget     w;              /* unused */
     caddr_t    closure;        /* unused */
     caddr_t    call_data;      /* pointer to (double) return value */
{
  double point;

  *(double *)call_data = (double) sim_time;

}
#endif


void Sync_Xcontrol(on) 
{
#ifndef NO_X
  if (XDISP) XSynchronize(the_display, on);
#endif
}

void Sync_display()
{
#ifndef NO_X
  if (XDISP) XSync(the_display, False);
#endif
}

void mainXloop()
{ 
#ifndef NO_X

  XeventLoop();
  XtAppMainLoop(app_context); 
      
#endif
}

int last_profile_update=0;

profile_value(p, i, v, m)
     Widget p;
     int i, v, m;
{
  int y;
  Window mw;
  mw = XtWindow(p);
  XSetForeground(the_display, gc, security_colors[17]);
  y = PROFILE_HT - v*PROFILE_HT/m;
  XDrawLine(the_display, mw, gc, i, PROFILE_LENGTH, i, y);
  XSetForeground(the_display, gc, security_colors[16]);
  XDrawLine(the_display, mw, gc, i, 0, i, y-1);
}

redraw_profiles() {
  Window mw;
  int i;
  for (i=0; i<PROFILE_LENGTH; i++) {
    profile_value(cpro, i, compro[i], comymax);
    profile_value(ppro, i, pepro[i], peymax);
    profile_value(opro, i, ovpro[i], ovymax);
  }
}

update_profiles() {
  int i, tmp, j, k;
  if (sim_time - xstart > xmax) {
    tmp = sim_time - 15000;
    for (i=0; i<PROFILE_LENGTH; i++) {
      j = (tmp - xstart)*PROFILE_LENGTH/xmax + i;
      if (j < PROFILE_LENGTH) {
	compro[i] = compro[j];
	pepro[i] = pepro[j];
	ovpro[i] = ovpro[j];
      } else {
	compro[i] = 0;
	pepro[i] = 0;
	ovpro[i] = 0;
      }
    }
    xstart = tmp;
    redraw_profiles();
  }
  k = max(last_profile_update - xstart, 0);
  for (i=PROFILE_LENGTH*k/xmax;
       i<PROFILE_LENGTH*(sim_time - xstart)/xmax+1; 
       i++) {
    compro[i] = max(msgflits, compro[i]);
    if (msgflits > comymax) comymax = msgflits;
    pepro[i] = max(NODES-idle_pes, pepro[i]);
    ovpro[i] = max(over_pes, ovpro[i]);
    profile_value(cpro, i, compro[i], comymax);
    profile_value(ppro, i, pepro[i], peymax);
    profile_value(opro, i, ovpro[i], ovymax);
  }
  last_profile_update = sim_time;
}

void XeventLoop()
{ 
#ifndef NO_X
  char str[80];

  if ((sim_time % 10 == 0) && SIM_STATUS)
    {
      sprintf(str,
	      "%s Time=%d", 
	      proto_name, sim_time);
      XtVaSetValues(simHeader, 
		    XtNlabel, str, NULL);
    }	    

  event_loop();

  if (traffic == TRACEDRIVEN) 
    update_profiles();
  XtAppAddTimeOut(XtWidgetToApplicationContext(topLevel), TIMEOUT,
		  XeventLoop, NULL);
      
#endif
}

/*
 * quit button callback function
 */
/*ARGSUSED*/
#ifndef NO_X
void Quit(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{ 
  report_stats();
/*  unlink(filename); */
  exit(0); 
}

/*
 * quit button callback function
 */
/*ARGSUSED*/
void statsQuitAction(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{ 
  XtDestroyWidget(statsWindow); 
}

/*
 * stats button callback function
 * report current stats and continue
 */
/*ARGSUSED*/
void Stats(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{ 

  int x, y;
  int realX, realY;

  Xreport_stats();

  if ( statsWindow != NULL ) 
    {
      XtVaGetValues(statsWindow,
		    XtNx, &x,
		    XtNy, &y,
		    NULL);
      XtTranslateCoords(statsWindow,
			x, y,
			&realX, &realY);

      XtDestroyWidget(statsWindow);
    }

  statsWindow = XtVaAppCreateShell
    (
     "Simulation Statistics",
     "Stats Window", applicationShellWidgetClass,
     display, 
     NULL
     );

/*  
  if ( x || y )
    {
      XtVaSetValues ( statsWindow,
		    XtNx, x,
		    XtNy, y,
		    NULL);
    }
*/
  
  statsBox = XtVaCreateManagedWidget
    (
     "Form",
     formWidgetClass,
     statsWindow, NULL);
  
  statsQuit = XtVaCreateManagedWidget
    (
     "Remove Statistics",
     commandWidgetClass,
     statsBox,
     NULL);
  
/*  statsText = XtVaCreateManagedWidget
    (
     "Statistics",
     asciiTextWidgetClass,
     statsBox,
     XtNfromVert, statsQuit,
     XtNtype, XawAsciiFile,
     XtNstring, filename,
     XtNheight, 400,
     XtNwidth, 500,
     XtNscrollVertical,  XawtextScrollWhenNeeded,
     XtNscrollHorizontal, XawtextScrollWhenNeeded,
     NULL); */
  
  XtAddCallback(statsQuit, XtNcallback, statsQuitAction, 0);
  
  
  XtRealizeWidget(statsWindow); 
}

/*
 * Pause/Restart button callback function
 */
/*ARGSUSED*/
void Stop(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{ 
  char str[80];
  XFontStruct *buttonfonts;
  
#define RESTART "Re-start"
#define PAUSE "Re-Stop"

  SIM_STATUS = (SIM_STATUS ? 0 : 1);

  XtVaGetValues(w, 
		XtNfont, &buttonfonts,
		NULL);


  if ( !SIM_STATUS ) 
    {
      sprintf(str,
	      "%s Protocol stopped at Time=%d", 
	      proto_name, sim_time);
    }
  else
    {
      sprintf(str,
	      "%s Time=%d", 
	      proto_name, sim_time);
    }
  
  XtVaSetValues(simHeader, 
		    XtNlabel, str, NULL);

  if ( !SIM_STATUS ) 
    {
      sprintf(str, "%s",
	      RESTART);
    }
  else
    {
      sprintf(str, "%s",
	      PAUSE);
    }
  
  XtVaSetValues(stop, 
		    XtNlabel, str, NULL);

}

/*
 * stepSimulation button callback function
 */
/*ARGSUSED*/
void Step(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{ 
  char str[80];
  if ( !SIM_STATUS ) 
    {
      ++STEP; 
      sprintf(str,
	      "%s Protocol stopped at Time=%d", 
	      proto_name, sim_time);
      XtVaSetValues(simHeader, 
		    XtNlabel, str, NULL);
    }
  

}
#endif


/*------------------------------------------------------------*/
/* XtInit()  - Sets up the X Intrinsics for Control of the   */
/*             main loop                                     */
/*------------------------------------------------------------*/
XtInit (argc, argv, rows, columns)
     int argc, rows, columns;
     char **argv;
{
#ifndef NO_X
  XEvent nextEvent;
  char str[80];
  Dimension x_loc, y_loc, h, w, xth, xtW, xtH;
  int blank, numcells,i;

  /* there are some problems (bugs) w/ proper scaling !! */
  if (SCALE > 1)
  {
    NODE_MAX  =  128 / pwr2(SCALE);
    NODE_SIZE =  NODE_SIZE * SCALE;
    LINK_SIZE =  LINK_SIZE * SCALE;
    NODE_SPACE=  NODE_SPACE * SCALE;
    BUFF_SIZE =   BUFF_SIZE * SCALE;
    /* Newly added to support visualization of DISHA */
    DISHA_TOKEN_SIZE =  DISHA_TOKEN_SIZE * SCALE;
    MAX_ROW   =   32 / pwr2(SCALE);
  }

  MAX_ROW = 8;
  if (rows > 8 || columns > 8) {
    NODE_MAX   =  NODE_MAX*4;
    NODE_SIZE  =  NODE_SIZE/2;
    LINK_SIZE  =  LINK_SIZE/2;
    NODE_SPACE =  NODE_SPACE/2;
    BUFF_SIZE  =  BUFF_SIZE/2;

 /* Newly added to support visualization of DISHA */
    DISHA_TOKEN_SIZE =  DISHA_TOKEN_SIZE/2;
 /* note: we dont scale down the 'DEADLOCK_BUFFER_SIZE' */

   MAX_ROW    =  16;
    if (rows > 16)
      rows = 16;
    if (columns > 16) 
      columns = 16;
  }

  pixmap_width_in_pixels = WD * rows/MAX_ROW;
  pixmap_height_in_pixels = HT * columns/MAX_ROW;
  
  blank = 1;

  XtToolkitInitialize();
  app_context = XtCreateApplicationContext();

  display = XtOpenDisplay(app_context, NULL,
			  argv[0], "Simulation",
			  NULL, 0, &argc, argv);

  if (display == NULL)
	printf("display = NULL\n");

  white = XWhitePixel(display, 0);
  black = XBlackPixel(display, 0);

  numcells = SECURITY_LEVELS * 19;
  security_colors = (int *) calloc(numcells, sizeof(int));

  if ( security_colors ) 
    {
      
      security_rgb = build_colors(SECURITY_LEVELS);
      
      my_colors = DefaultColormap(display, 0);

      XAllocColorCells(display, my_colors, False, security_pm, 0, 
		       security_colors, numcells);

      set_colors(numcells);
      
      topLevel = XtVaAppCreateShell
	(
	 "Routing Simulation",
	 "Simulation", applicationShellWidgetClass,
	 display,
	 XtNx, START_X,
	 XtNy, START_Y,
	 NULL
	 );
      
      XtVaGetValues(topLevel,XtNheight, &xtH, XtNwidth, &xtW, NULL);
      
      XtTranslateCoords(topLevel, (Position) 0, (Position) 0,
			&x_loc, &y_loc);

      box = XtVaCreateManagedWidget
	(
	 "Form",
	 formWidgetClass,
	 topLevel, NULL);
      
      quit = XtVaCreateManagedWidget
	(
	 "Quit",
	 commandWidgetClass,
	 box, 
	 NULL);
      
      XtAddCallback(quit, XtNcallback, Quit, 0);
      
      stats = XtVaCreateManagedWidget
	(
	 "Statistics",
	 commandWidgetClass,
	 box, 
	 XtNfromHoriz, 
	 quit,
	 NULL);
      
      XtAddCallback(stats, XtNcallback, Stats, 0);
      
      stop = XtVaCreateManagedWidget
	(
	 "Stop Simulation",
	 commandWidgetClass,
	 box,
	 XtNfromHoriz, 
	 stats,
	 NULL);
      
      XtAddCallback(stop, XtNcallback, Stop, 0);

      stepButton = XtVaCreateManagedWidget
	(
	 "Step Simulation",
	 commandWidgetClass,
	 box,
	 XtNfromHoriz, 
	 stop,
	 NULL);
      
      XtAddCallback(stepButton, XtNcallback, Step, 0);

      if (traffic == TRACEDRIVEN) {
	cpro = XtVaCreateManagedWidget(
	  "Comm profile",
	  formWidgetClass,
	  box,
	  XtNwidth,
	  PROFILE_LENGTH,
	  XtNheight,
	  PROFILE_HT,
	  XtNfromVert, quit,
	  XtNbackground, security_colors[16],
	  NULL);
	ppro = XtVaCreateManagedWidget(
	  "PE profile",
	  formWidgetClass,
	  box,
	  XtNwidth,
	  PROFILE_LENGTH,
	  XtNheight,
	  PROFILE_HT,
	  XtNfromVert, quit,
	  XtNfromHoriz, cpro,
	  XtNbackground, security_colors[16],
	  NULL);
	opro = XtVaCreateManagedWidget(
	  "Overhead profile",
	  formWidgetClass,
	  box,
	  XtNwidth,
	  PROFILE_LENGTH,
	  XtNheight,
	  PROFILE_HT,
	  XtNfromVert, quit,
	  XtNfromHoriz, ppro,
	  XtNbackground, security_colors[16],
	  NULL);
	xmax = 20000;
	comymax = NODES;
	peymax = NODES;
	ovymax = NODES;
	for (i=0; i<100; i++) {
	  compro[i]=0;
	  pepro[i]=0;
	  ovpro[i]=0;
	}
      } else 
	cpro = quit;
  
      simHeader = XtVaCreateManagedWidget
	(
	 "SimHeader",
	 labelWidgetClass,
	 box,
	 XtNwidth,
	 pixmap_width_in_pixels,
	 XtNfromVert, cpro,
	 NULL);
      
      sim = XtVaCreateManagedWidget
	(
	 "Shell",
	 formWidgetClass,
	 box,
	 XtNwidth,
	 pixmap_width_in_pixels,
	 XtNheight,
	 pixmap_height_in_pixels,
	 XtNfromVert, simHeader,
	 XtNbackground, security_colors[16],
	 NULL);
      

      XtRealizeWidget(topLevel);

      /*
      xSetUpOptions();
      */
      }
  else
    {
      printf("Unable to allocate pixel maps!\n");
      exit(0);
    }

#endif
}

/*------------------------------------------------------------*/
/* xinit()  Open the display and window                       */
/*------------------------------------------------------------*/
xinit(rows, columns) 
  int rows, columns;
{
#ifndef NO_X
  char str[80];

  the_display = XtDisplay (sim);

  if (the_display == NULL) {
    printf("Open display failed!\n");
    return 0;
  }

/*  XSynchronize(the_display,True);   */

  root = NULL;

/*
  gc = XDefaultGC(the_display,0);
*/

  values.foreground = 0;
  values.background = 1;
  values.dashes = 1;
  values.dash_offset = 0;
  values.line_style = LineSolid;

  gc = XCreateGC(XtDisplay(sim), XtWindow(sim),
		      GCForeground | GCBackground | GCDashOffset | 
		      GCDashList | GCLineStyle, &values);

  my_window = XtWindow(sim);

  smallfonts=XLoadQueryFont(the_display,"5x8");
  bigfonts=XLoadQueryFont(the_display,"6x10");

  attribs.backing_store = Always;
  XChangeWindowAttributes(the_display, my_window, CWBackingStore, &attribs);

  XSetForeground(the_display, gc, security_colors[17]);
  XSetBackground(the_display, gc, security_colors[16]);

#endif
}


/*--------------------------------------------------------------*/
/* xclose()     Dest the window and close up.                   */
/*--------------------------------------------------------------*/
xclose() {
#ifndef NO_X
  XDestroyWindow(the_display, my_window);
  XCloseDisplay(the_display);
#endif
}

/*--------------------------------------------------------------*/
/* set_colors()     Setup my color map entries                  */
/*--------------------------------------------------------------*/
set_colors(numcells) 
int numcells;
{
#ifndef NO_X
  int i,j;
  XColor color;

/*
  XAllocColorCells(display, my_colors, False, pm, 0, colors, 18);
*/
/*
  if (SECURITY_LEVELS==1) {
    for (j=0; j<18; j++) {
      color.pixel = security_colors[j];
      color.red = red[j]*1000;
      color.green = green[j]*1000;
      color.blue = blue[j]*1000;
      color.flags = 15;
      XStoreColor(display, my_colors, &color);
    }
*/
  for (i=0; i<SECURITY_LEVELS; i++) {
    for (j=0; j<19; j++) {
      color.pixel = security_colors[i*19+j];
      color.red = security_rgb[i].red[j]*1000;
      color.green = security_rgb[i].green[j]*1000;
      color.blue = security_rgb[i].blue[j]*1000;
      color.flags = 15;
      XStoreColor(display, my_colors, &color);
    }
  }

#endif
}

/*--------------------------------------------------------------*/
/* xevents()    */
/*--------------------------------------------------------------*/
xevents(time) 
int time;
{
#ifndef NO_X
  int i;
  XEvent Event,nextEvent;
  register int Pressed=False;
  register int inWindow=False;
  char buffer[10];
  register char *keyChars = buffer;
  register int keyDown = False;
  int nbytes;
  Bool eventHappened;
  char str[80];

/*  while ( XtAppPending (app_context) != 0 ) {
    XtAppNextEvent(app_context, nextEvent);
    XtDispatchEvent(nextEvent);
  }
*/

  i = XEventsQueued(the_display,QueuedAfterFlush);


#endif
}

/*--------------------------------------------------------------*/
/* draw_buff(x,y,c)  Draw a buffer at location (x,y)            */
/*--------------------------------------------------------------*/
draw_buff(x,y,c) 
     int x,y,c;
{
#ifndef NO_X
  XSetForeground(the_display, gc, c);
  XDrawRectangle(the_display, my_window, gc, 
		 x+1, y+1, BUFF_SIZE-1, BUFF_SIZE-1);
#endif
}

/*--------------------------------------------------------------*/
/* fill_buff(x,y,c)  Fill a buffer at location (x,y)            */
/*--------------------------------------------------------------*/
fill_buff(x, y, c) 
     int x,y,c;
{
#ifndef NO_X
  int save;
  if (c == 0) {
    XClearArea(the_display, my_window,
		   x+2, y+2, BUFF_SIZE-2, BUFF_SIZE-2, False);
  } else {
    XSetForeground(the_display, gc, c);
    XFillRectangle(the_display, my_window, gc, 
		   x+2, y+2, BUFF_SIZE-2, BUFF_SIZE-2);
  }
#endif
}


/*--------------------------------------------------------------*/
/* draw_disha_token(x,y,c)  Draw a disha token at location (x,y)*/
/*--------------------------------------------------------------*/
draw_disha_token(x,y,c) 
     int x,y,c;
{
#ifndef NO_X
  XSetForeground(the_display, gc, c);
  XDrawRectangle(the_display, my_window, gc, 
		 x+1, y+1, DISHA_TOKEN_SIZE-1, DISHA_TOKEN_SIZE-1);
#endif
}

/*--------------------------------------------------------------*/
/* fill_disha_token(x,y,c)  Fill a disha token at location (x,y)*/
/*--------------------------------------------------------------*/
fill_disha_token(x, y, c) 
     int x,y,c;
{
#ifndef NO_X
  int save;
  if (c == 0) {
    XClearArea(the_display, my_window,
		   x+2, y+2, DISHA_TOKEN_SIZE-2, DISHA_TOKEN_SIZE-2, False);
  } else {
    XSetForeground(the_display, gc, c);
    XFillRectangle(the_display, my_window, gc, 
		   x+2, y+2, DISHA_TOKEN_SIZE-2, DISHA_TOKEN_SIZE-2);
  }
#endif
}


/*--------------------------------------------------------------*/
/* draw_link(vl)  Draw link vlinks[vl].pl                       */
/*--------------------------------------------------------------*/
draw_link(vl, drawit) 
int vl, drawit;
{
  int tkn,c,pl;
  int direction, up, level;

#ifndef NO_X
  pl = vlinks[vl].pl;
  if (pl < NODES) return;

  tkn = vlinks[vl].busy;
  level = vlinks[vl].security_level;

  direction = (sign_of_link(vl) == 1) ? 1 : 0;
  up = pl % 2;

/*
  if (tkn>0 && drawit) 
    c = colors[tkn%16];
  else
    c = colors[17];
*/

  if ( DEBUG >= 2 ) {
    printf("Security level = %d\n", level);
  }

  if (tkn== (-2))
    c = kill_color;
  else if (tkn>0 && drawit) 
  { 
    if (vlinks[vl].spec_col)
        c = security_colors[13];
    else
    	c = security_colors[level*19 + tkn%16];
  }
  else
    c = security_colors[17];


  if ( pl <= max_link )
    {

      if ( plinks[pl].x + plinks[pl].width > 0 &&
	  plinks[pl].y + plinks[pl].height >0 ) 
	{
	  if ( HALF ) 
	    { /* half duplex - fill the whole link because it's got */
	      /* the whole channel */
	      
	      XSetForeground(the_display, gc, c);
	      if (tkn==0 || !drawit) 
		{
		  XClearArea(the_display, my_window, 
			     plinks[pl].x+1,
			     plinks[pl].y+1,
			     plinks[pl].width-1,
			     plinks[pl].height-1, False);
		  XDrawRectangle(the_display, my_window, gc, 
				 plinks[pl].x,
				 plinks[pl].y,
				 plinks[pl].width,
				 plinks[pl].height);
		}
	      else
		{
		  XFillRectangle(the_display, my_window, gc, 
				 plinks[pl].x+1,
				 plinks[pl].y+1,
				 plinks[pl].width-1,
				 plinks[pl].height-1);
		}
	    }

	  else
	    { /* full duplex - determine which half of the link */
	      
	      XSetForeground(the_display, gc, c);
	      /*
		XDrawRectangle(the_display, my_window, gc, 
		plinks[pl].x,
		plinks[pl].y,
		plinks[pl].width,
		plinks[pl].height);
		*/
	      
	      
	      if (tkn==0 || !drawit) 
		{
		  XClearArea(the_display, my_window,
			     plinks[pl].x+1 + up * (!direction) *
			     (plinks[pl].width/2), 
			     plinks[pl].y+1 + (!up) * (direction) *
			     (plinks[pl].height/2), 
			     plinks[pl].width / (1+up),
			     plinks[pl].height / (2-up),
			     False);
		  XDrawRectangle(the_display, my_window, gc, 
				 plinks[pl].x,
				 plinks[pl].y,
				 plinks[pl].width,
				 plinks[pl].height);
		}
	      else
		{
		  XFillRectangle(the_display, my_window, gc, 
				 plinks[pl].x+1 + up * (!direction) *
				 (plinks[pl].width/2), 
				 plinks[pl].y+1 + (!up) * (direction)
				 * (plinks[pl].height/2), 
				 plinks[pl].width / (1+up),
				 plinks[pl].height / (2-up));
		}
	    }
	}
    }
#endif
}

/*--------------------------------------------------------------*/
/* mark_node(n, activity)  Mark a nodes activity                */
/*--------------------------------------------------------------*/
mark_node(n, activity) {
#ifndef NO_X
  int c;
  int tmp, tmp2;
  int x, y;

  if (!XDISP) return;
  if (n >= NODE_MAX) return;

  switch (activity) {
  case 'E': 
    c = security_colors[7]; break;
  case 'O': 
    c = security_colors[3]; break;
  case 'S':
    c = security_colors[1]; break;
  case 'R':
    c = security_colors[9]; break;
  }
  XSetForeground(the_display, gc, c);
  tmp = NODE_SIZE/4;
  tmp2 = (3*NODE_SIZE)/4;
  x = nodes[n].x;
  y = nodes[n].y;
  XFillRectangle(the_display, my_window, gc, x+tmp, y+tmp-2, NODE_SIZE/2,
		 4);
  XFillRectangle(the_display, my_window, gc, x+tmp, y+tmp2-1, NODE_SIZE/2,
		 4);
  XFillRectangle(the_display, my_window, gc, x+tmp-2, y+tmp-2, 4, 
		 NODE_SIZE/2+5);
  XFillRectangle(the_display, my_window, gc, x+tmp2-2, y+tmp-2, 4, 
		 NODE_SIZE/2+5);
#endif
}

/*--------------------------------------------------------------*/
/* thread_node(n, thread)  Mark a nodes with a thread           */
/*--------------------------------------------------------------*/
thread_node(n, thread) {
#ifndef NO_X
  int c;
  int tmp;
  int x, y;

  if (!XDISP) return;
  if (n >= NODE_MAX) return;
  if (thread<0) 
    c = security_colors[16];
  else
    c = security_colors[thread%16];
  XSetForeground(the_display, gc, c);

  tmp = BUFF_SIZE;
  x = nodes[n].x;
  y = nodes[n].y;
  XFillRectangle(the_display, my_window, gc, x+NODE_SIZE/2-tmp+1, 
		 y+NODE_SIZE/2-tmp+1, tmp*2, tmp*2);
#endif
}

/*--------------------------------------------------------------*/
/* draw_node(x,y)  Draw a node at location (x,y)                */
/*--------------------------------------------------------------*/
draw_node(x,y,node_num) 
     int x,y,node_num;
{
#ifndef NO_X
  int i;
  char str[10];

  XDrawRectangle(the_display, my_window, gc, x, y, NODE_SIZE,
		 NODE_SIZE);
  nodes[node_num].x = x;
  nodes[node_num].y = y;
  nodes[node_num].width = NODE_SIZE;
  nodes[node_num].height = NODE_SIZE;
  max_buff = (NODE_SIZE/BUFF_SIZE/2) - 1;
  if (max_buff > node_virts) max_buff = node_virts;
  for (i=0; i<max_buff; i++) {
    frame_buff(node_num, -1, 0, i, 0, 0, 0);
    frame_buff(node_num, -1, 0, i, 1, 0, 0);
  }
  max_buff = (NODE_SIZE/BUFF_SIZE/2) - 1;
  if (max_buff > virts) max_buff = virts;
  for (i = 0; i<max_buff; i++) {
    /* Top side */
    draw_buff(x + NODE_SIZE/2 - (i+1)*BUFF_SIZE, y, security_colors[17]);
    draw_buff(x + NODE_SIZE/2 + i*BUFF_SIZE, y, security_colors[17]);
    /* Bottom side */
    draw_buff(x + NODE_SIZE/2 - (i+1)*BUFF_SIZE, 
	      y + NODE_SIZE - BUFF_SIZE-1, security_colors[17]);
    draw_buff(x + NODE_SIZE/2 + i*BUFF_SIZE, 
	      y + NODE_SIZE - BUFF_SIZE-1, security_colors[17]);
    /* Left side */
    draw_buff(x, y + NODE_SIZE/2 - (i+1)*BUFF_SIZE, security_colors[17]);
    draw_buff(x, y + NODE_SIZE/2 + i*BUFF_SIZE, security_colors[17]);
    /* Right side */
    draw_buff(x + NODE_SIZE - BUFF_SIZE -1, 
	      y + NODE_SIZE/2 - (i+1)*BUFF_SIZE, security_colors[17]);
    draw_buff(x + NODE_SIZE - BUFF_SIZE -1, 
	      y + NODE_SIZE/2 + i*BUFF_SIZE, security_colors[17]);
  }

  /* Draw a disha token slot (for each node) */

  if (NEW_DISHA_FEATURES)
  {
  	draw_disha_token(x + NODE_SIZE/2 - DISHA_TOKEN_SIZE/2 - 1, 
	      	y + NODE_SIZE/2 - DISHA_TOKEN_SIZE/2 - 1, security_colors[17]);
  }

  /* Draw links */


  /* Up */
  XDrawRectangle(the_display, my_window, gc, 
		 x + (NODE_SIZE-LINK_SIZE)/2, 
		 y - NODE_SPACE + NODE_SIZE, 
		 LINK_SIZE,
		 NODE_SPACE-NODE_SIZE);


  /* Down */
  plinks[node_num*2+1+NODES].x = x+(NODE_SIZE-LINK_SIZE)/2;
  plinks[node_num*2+1+NODES].y = y+(NODE_SIZE);
  plinks[node_num*2+1+NODES].width = LINK_SIZE;
  plinks[node_num*2+1+NODES].height = NODE_SPACE-NODE_SIZE;

  XDrawRectangle(the_display, my_window, gc, 
		 plinks[node_num*2+1+NODES].x,
		 plinks[node_num*2+1+NODES].y,
		 plinks[node_num*2+1+NODES].width,
		 plinks[node_num*2+1+NODES].height);

/*  if ( !HALF ) {
    XDrawRectangle(the_display, my_window, gc, 
		   plinks[node_num*2+1+NODES].x,
		   plinks[node_num*2+1+NODES].y,
		   plinks[node_num*2+1+NODES].width/2,
		   plinks[node_num*2+1+NODES].height);
  }*/


  /* Left */
  XDrawRectangle(the_display, my_window, gc, 
		 x-(NODE_SPACE-NODE_SIZE),
		 y + (NODE_SIZE-LINK_SIZE)/2, 
		 NODE_SPACE-NODE_SIZE,
		 LINK_SIZE);



  /* Right */
  plinks[node_num*2+NODES].x = x+NODE_SIZE;
  plinks[node_num*2+NODES].y = y+(NODE_SIZE-LINK_SIZE)/2;
  plinks[node_num*2+NODES].width = NODE_SPACE-NODE_SIZE;
  plinks[node_num*2+NODES].height = LINK_SIZE;
  XDrawRectangle(the_display, my_window, gc, 
		 plinks[node_num*2+NODES].x,
		 plinks[node_num*2+NODES].y,
		 plinks[node_num*2+NODES].width,
		 plinks[node_num*2+NODES].height);

/*  if ( !HALF ) {
    XDrawRectangle(the_display, my_window, gc, 
		   plinks[node_num*2+NODES].x,
		   plinks[node_num*2+NODES].y,
		   plinks[node_num*2+NODES].width,
		   plinks[node_num*2+NODES].height/2);
  }*/

  if ( DEBUG >= 1 ) {
    /* if requested, label link numbers for quick reference */

    XSetFont(the_display,gc,smallfonts->fid);

    /* right */
    sprintf(str,"%d", node_num*2);
    XDrawString(the_display, my_window, gc, 
		x + NODE_SIZE+3, 
		y + (NODE_SIZE-LINK_SIZE)/2, 
		str,
		strlen(str));
    
    /* down */
    sprintf(str,"%d", node_num*2+1);
    XDrawString(the_display, my_window, gc, 
		x + NODE_SIZE/2+2, 
		y + NODE_SIZE+10, 
		str,
		strlen(str));
  }

#endif
}



/*--------------------------------------------------------------*/
/* highlight_node(x,y)  frame a node at location (x,y)                */
/*--------------------------------------------------------------*/
highlight_node(node_num, highlight) 
     int node_num, highlight;
{
#ifndef NO_X
  int x, y, c;
  int thickness = 5;

  if (!XDISP) return;

  x = nodes[node_num].x;
  y = nodes[node_num].y;

  if (highlight == 1)
    c = security_colors[10];
  else if (highlight == 2)
    c = security_colors[13];
  else if (highlight == 3)
    c = security_colors[4];
  else
    c = 0;

  if (c)
  {
  XSetForeground(the_display, gc, c);
  XFillRectangle(the_display, my_window, gc, x-thickness, y-thickness, NODE_SIZE+(2 * thickness), 
		thickness);
 
  XSetForeground(the_display, gc, c);
  XFillRectangle(the_display, my_window, gc, x-thickness, y+NODE_SIZE+1, NODE_SIZE+(2 * thickness), 
		thickness);

  XSetForeground(the_display, gc, c);
  XFillRectangle(the_display, my_window, gc, x-thickness, y-thickness, thickness, 
		NODE_SIZE+(2 * thickness));

  XSetForeground(the_display, gc, c);
  XFillRectangle(the_display, my_window, gc, x+NODE_SIZE+1, y-thickness, thickness, 
		NODE_SIZE+(2 * thickness));

  }
  else
  {
    XClearArea(the_display, my_window,
		   x-thickness, y-thickness, NODE_SIZE+(2 * thickness), thickness, True);
    XClearArea(the_display, my_window,
		   x-thickness, y+NODE_SIZE+1, NODE_SIZE+(2 * thickness), thickness, True);

    XClearArea(the_display, my_window,
		   x-thickness, y-thickness, thickness, NODE_SIZE+(2 * thickness), True);
    XClearArea(the_display, my_window,
		   x+NODE_SIZE+1, y-thickness, thickness, NODE_SIZE+(2 * thickness), True);

  }

#endif
}

/*--------------------------------------------------------------*/
/* draw_network()  Draw the network                             */
/*--------------------------------------------------------------*/
draw_network(rows, columns) 
  int rows, columns;
{
#ifndef NO_X
  int x,y;
  int i,j, strwidth;
  char str[20];
  int node_num;

  i = 0;
  j = 0;
 
  if ( DEBUG >= 2 ) {
    printf ("Drawing %d by %d...\n", rows, columns);
  }

  for (x = (NODE_SPACE-NODE_SIZE)/2; x < WD-NODE_SIZE; x += NODE_SPACE) {
    for (y = (NODE_SPACE-NODE_SIZE)/2; y < WD-NODE_SIZE; y += NODE_SPACE) {
      if ( i<rows && j<columns ) {
	node_num = (i) + (j) * (rows);
        draw_node(x,y,node_num);
        if ( DEBUG >= 1 ) {
	  XSetFont(the_display,gc,bigfonts->fid);
	  sprintf(str,"(%d,%d)", i, j);
	  strwidth = XTextWidth(bigfonts, str, strlen(str));
	  XDrawString(the_display, my_window, gc, 
		      x + (NODE_SIZE-strwidth)/2 , 
		      y + NODE_SIZE * 4 / 10, 
		      str,
		      strlen(str));
	  XSetFont(the_display,gc,smallfonts->fid);
	  sprintf(str,"#%d", node_num);
	  strwidth = XTextWidth(smallfonts, str, strlen(str));
	  XDrawString(the_display, my_window, gc, 
		      x + (NODE_SIZE-strwidth)/2 , 
		      y + NODE_SIZE * 8 / 10, 
		      str,
		      strlen(str));
	}
      }
      if ( DEBUG >= 2 ) {
        printf("Drawing node (%d,%d)\n",i,j);
      } 
      j++;
    }
    i++;
    j = 0;
  }
  max_link = node_num*2+1+NODES;

#endif
}


/*-----------------------------------------------------------*/
/* mark_disha_token(nd, dim, sign, v, tkn)                          */
/*-----------------------------------------------------------*/
mark_disha_token(nd, fill)
     int nd, fill;
{
#ifndef NO_X
  int x,y,c;

  if (fill)
    c = security_colors[10];
  else
    c = security_colors[16];

  x = nodes[nd].x + NODE_SIZE/2 - DISHA_TOKEN_SIZE/2 - 1;
  y = nodes[nd].y + NODE_SIZE/2 - DISHA_TOKEN_SIZE/2 - 1;
    
  fill_disha_token(x,y,c);
}
#endif


/*-----------------------------------------------------------*/
/* mark_buff(nd, dim, sign, v, tkn)                          */
/*-----------------------------------------------------------*/
mark_buff(nd, dim, sign, v, in, tkn, level)
     int nd, dim, sign, v, in, tkn, level;
{
#ifndef NO_X
  int i,j,x,y,c;

  if (tkn== (-2))
    c = security_colors[kill_color];
  else  if (tkn>0) 
    {
      c = security_colors[level*19 + tkn%16]; 
      /*      c = security_colors[tkn%16];  */
    }
  else
    {
      c = security_colors[16];
    }
  
  i = nd%width[0];
  j = nd/width[0];
  if (i < MAXX && j < MAXY) {
    x = (NODE_SPACE-NODE_SIZE)/2 + i*NODE_SPACE;
    y = (NODE_SPACE-NODE_SIZE)/2 + j*NODE_SPACE;
    if (dim==(-1)) {		/* This node's buffer */
      x = x + NODE_SIZE/2 - (node_virts/2 - v)*BUFF_SIZE;
      y = y + NODE_SIZE/2 - BUFF_SIZE/2 + (in*2 - 1) * BUFF_SIZE*2;
    } else {
      if (v>max_buff) return;
      if (dim == 0) {
	y = y + NODE_SIZE/2;
	if ((in && sign>0) || (!in && sign<0)) 
	  y = y-BUFF_SIZE*(v+1);
	else
	  y = y+BUFF_SIZE*v;
	if (sign > 0) 
	  x = x + NODE_SIZE - BUFF_SIZE -1;
      } else {
	x = x + NODE_SIZE/2;
	if ((in && sign>0) || (!in && sign<0)) 
	  x = x-BUFF_SIZE*(v+1);
	else
	  x = x+BUFF_SIZE*v;
	if (sign > 0) 
	  y = y + NODE_SIZE - BUFF_SIZE -1;
      }
    }
    fill_buff(x,y,c);
  }
#endif
}

/*-----------------------------------------------------------*/
/* frame_buff(nd, dim, sign, v, tkn)                         */
/*-----------------------------------------------------------*/
frame_buff(nd, dim, sign, v, in, tkn, level)
     int nd, dim, sign, in, v, level;
{
#ifndef NO_X
  int i,j,x,y, c;

  if (tkn == (-2))
    c = security_colors[kill_color];
  else
    if (tkn>0) 
      {
	c = security_colors[level*19 + tkn%16];
	/*      c = security_colors[tkn%16]; */
      }
    else
      {
	c = security_colors[17];
      }


  i = nd%width[0];
  j = nd/width[0];
  if (i < MAXX && j < MAXY) {
    x = (NODE_SPACE-NODE_SIZE)/2 + i*NODE_SPACE;
    y = (NODE_SPACE-NODE_SIZE)/2 + j*NODE_SPACE;
    if (dim== (-1)) {		/* This node's buffer */
      x = x + NODE_SIZE/2 - (node_virts/2 - v)*BUFF_SIZE;
      y = y + NODE_SIZE/2 - BUFF_SIZE/2 + (in*2 - 1)*BUFF_SIZE*2;
    } else {
      if (v>max_buff) return;
      if (dim == 0) {
	y = y + NODE_SIZE/2;
	if ((in && sign>0) || (!in && sign<0)) 
	  y = y-BUFF_SIZE*(v+1);
	else
	  y = y+BUFF_SIZE*v;
	if (sign > 0) 
	  x = x + NODE_SIZE - BUFF_SIZE -1;
      } else {
	x = x + NODE_SIZE/2;
	if ((in && sign>0) || (!in && sign<0)) 
	  x = x-BUFF_SIZE*(v+1);
	else
	  x = x+BUFF_SIZE*v;
	if (sign > 0) 
	  y = y + NODE_SIZE - BUFF_SIZE -1;
      }
    }
    draw_buff(x,y,c);
  }
#endif
}


draw_color_bar() {
#ifndef NO_X
  int i,j, cnt;
  j = 50;
  cnt=0;
  while (cnt<256) {
    for (i=0; i<WD/2/BUFF_SIZE; i++) {
      fill_buff(i*BUFF_SIZE*2+BUFF_SIZE,j, cnt%256);
      cnt++;
    }
    j += BUFF_SIZE;
    if (cnt%5 == 0) 
      XDrawLine(the_display, my_window, gc, i*BUFF_SIZE*2 + 1.5*BUFF_SIZE, j,
		i*BUFF_SIZE*2 + 1.5*BUFF_SIZE, j-5);
  }
#endif
}




