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

/*----------------------------------------------------------------------*/
/* Time-step simulator                                                  */
/*                                                                      */
/*   This module presents the main routines for a general time-step     */
/*   routing simulator.  It is made up of network resources ---         */
/*   nodes and links.  Nodes generate messages to other nodes and       */
/*   act as the sources and sinks of flits.  Links contain the          */
/*   resources required for virtual channels over a half-duplex link    */
/*----------------------------------------------------------------------*/

#include <stdio.h>
int DEBUG =1;
#include "dat2.h"
#include "dd.h"

extern int RUN_TIME, INIT, SIM_STATUS, STEP;
extern int sim_time, HALF, XDISP, *width;
extern int traffic;
extern long seed;
extern int sim_time;
extern char trace_file_name[];
void event_loop();
char filename[30];

/* newly added - for testing */
extern int TEST;

/* newly added - for Deadlock Detection */
extern int CD;
extern int DD;
extern int DDTEST;

/*----------------------------------------------------------------------*/
/* main()  Contains the outermost loop of the simulation.               */
/*----------------------------------------------------------------------*/
void usage (argc, argv)
     int argc;
     char **argv;
{
  printf("Usage: %s\n\n\tOptions:\n", argv[0]);
  printf("\tD=<dimensions> (default=2)\n");
  printf("\tHALF=n  (0=full duplex, 1=half duplex, default=1) \n");
  printf("\tX=n (0=X-Windows off 1 = on, default=0)\n");
  printf("\tDEMAND=<demand> Indicates whether to use demand-driven\n");
  printf("\t                 or time-slice multiplexing.  (default=1)\n");
  printf("\tSIZE=c1c2...cn size of network in powers of 2. 33 gives\n");
  printf("\t                 an 8x8 network, default = 44. (NOTE: number\n");
  printf("\t                 of dimensions must be specified before this\n");
  printf("\t                 parameter if different than the default.)\n");
  printf("\tDEBUG=n set debug level (default=0)\n");
  printf("\tBUFFERS=n set buffer depth of each input virtual channel\n");
  printf("            (must be > 1, default=8)\n");
  printf("\tINJECT=n set inject rate per inject epoch (default=1)\n");
  printf("\tPER=n set inject epoch.  The simulation will inject on\n");
  printf("\t          average INJECT messages per node every PER\n");
  printf("\t          network cycles (default=1000)\n");
  printf("\tSECURITY_LEVELS=n set number of security level\n");
  printf("\tRDELAY= routing header intra-node delay in network\n");
  printf("\t           cycles (default=1)\n");
  printf("\tADELAY= acknowledgement flit intra-node delay in network\n");
  printf("\t           cycles (default=1)\n");
  printf("\tDDELAY= data flit intra-node delay in network\n");
  printf("\t           cycles (default=1)\n");
  printf("\tHSPOTS=n  set the number of network hot spots (default=0).\n");
  printf("\tHSPLACE=c1c2..cn  set the location of a hotspot.\n");
  printf("\t           A=10, Z=36, a=37, z=73\n");
  printf("\t           (may be used repeatedly.  Otherwise hot spots\n");
  printf("\t           are randomly placed.)\n");
  printf("\tM=n  set the number misroutes allowed by MB-m routing\n");
  printf("\t           protocol. (default=3)\n");
  printf("\tNO_CTS=n  0=CTS lookahead used, 1=CTS lookahead not used.\n");
  printf("\t           (default=0)\n");
  printf("\tMAP=n  0=No congestion map generated, 1=Generate congestion\n");
  printf("\t           map file. (default=0)\n");
  printf("\tSLOW=n  1=Place one second delays between flit movement stages\n");
  printf("\t           (Helps in debugging. default=0)\n");
  printf("\tFAULTS=n  Number of static virtual channel faults to place\n");
  printf("\t           randomly in the network (default=0)\n");
  printf("\tPFAULTS=n Number of static physical link faults to place\n");
  printf("\tTRANS=n   1=display transient statistics (default=1)\n");
  printf("\tCOMM=<communication mechanism>\n");
  printf("\t\tW = wormhole routing (WR)\n");
  printf("\t\tP = pipelined circuit-switching (PCS)\n");
  printf("\t\tAP = acknowledged PCS\n");
  printf("\t\tAx = acknowledged WR\n");
  printf("\t\tR = recon routing\n");
  printf("\t\tThis is set correctly by PROTO. WR is not compatible\n");
  printf("\t\twith some routing protocols.\n");
  printf("\tSCOUT=n  number of acks the routing header sends back to the\n");
  printf("\t\t dest before the data flits are sent.  Recon routing.\n");
  printf("\tSRCQ=n  number of injection queues (default=8)\n");
  printf("\tTRACE=trace_fn  Turns on trace driven simulation.  Specifies\n");
  printf("\t\t trace file.\n");
  printf("\tPROTO=<routing protocol>\n");
  printf("\t\tE = Ecube (defaults to WR communication mechanism)\n");
  printf("\t\tD = Duato (psuedo-min congestion selection function) (WR)\n");
  printf("\t\tM = Duato with m misroutes (AWR)\n");
  printf("\t\tO = Duato (Dimension Order selection function) (WR)\n");
  printf("\t\tC = Duato (Minimum congestion selection function) (WR)\n");
  printf("\t\tN = Negative First (WR)\n");
  printf("\t\tR = Dimension Reversal (WR)\n");
  printf("\t\tL = Misrouting Backtracking-m (PCS)\n");
  printf("\t\tP = MB-m + (PCS)\n");
  printf("\t\tA = A1 (PCS)\n");
  printf("\t\tT = Two-phase Backtracking (PCS)\n");
  printf("\t\tB = Exhaustive Profitable Backtracking (PCS)\n");
  printf("\t\tF = MB-m SW (PCS)\n");
  printf("\t\tI = Directional Ordered Routing (WR)\n");
  printf("\tSELECT=<selection function>\n");
  printf("\t\tN = Normal (psuedo-minimum congestion)\n");
  printf("\t\tM = Minimum congestion\n");
  printf("\t\tO = Dimension Order\n");
  printf("\tORDER=specify order of traversal for dor and mesh or toroid\n");
  printf("\t\t0 = DOR using y+x+y-x- (toroid)\n");
  printf("\t\t1 = DOR using y+x+x-y- (toroid)\n");
  printf("\t\t2 = DOR using y+x+y-x- (mesh)\n");
  printf("\t\t3 = DOR using y+x+x-y- (mesh)\n");
  printf("\t\t0 = Ecube (toroid)\n");
  printf("\t\t1 = Ecube (mesh)\n");
  printf("\tVARY=0/1 Turn off/on halving of virtual channels in different\n");
  printf("\t\t dimensions.  0 = off, 1 = on\n");
  printf("\tMSGL=<message length in flits>\n");
  printf("\tDUR=<duration of simulation in network clock cycles>\n");
  printf("\tVIRTS=<number of virtual channels per physical link direction>\n");
  printf("\tRADIUS=Size of exhaustive search radius for MB-m SW\n");
  printf("\tDIST=<distance/communication pattern>\n");
  printf("\t\tDIST=0 Random destinations selected from entire network\n");
  printf("\t\tDIST=-100 Bit reversal message pattern\n");
  printf("\t\tDIST=-900 Transpose message pattern\n");
  printf("\t\tDIST=-999 Perfect shuffle message pattern\n");
  printf("\t\tDIST=-1000 Flip bits message pattern\n");
  printf("\t\tDIST=-1001 Hot spot message pattern\n");
  printf("\t\tDIST>0 Random destinations selected from box DISTx...xDIST\n");
  printf("\t\t    centered on source node.\n");
  printf("\t\tDIST<0 Random destinations selected such that the distance\n");
  printf("\t\t    from source to destination is exactly |DIST|.\n");
  printf("\t\t(default = 4)\n");
  printf("\tDIST=<distance/communication pattern>\n");
  printf("\tDYN=n  Number of dynamic virtual channel faults\n");
  printf("\tPDYN=n Number of dynamic physical channel faults\n");
  exit(1);
}

extern int dimensions;
extern node *nodes;

main (argc, argv)
     int argc;
     char **argv;
{

/*  Don't buffer the output - to prevent loss of intermediate data during failure */
  setbuf(stdout, NULL);
 
  if ( argc == 1 ) 
    {
      usage(argc, argv);
    }
/*  sprintf(filename, "%s", mktemp("/tmp/simXXXXXX")); */

  seed=0;
  random_seed (seed);
  parse_command_line (argc, argv, 1);

  create_network ();
 
  /*--------------------------------*/
  /* DD (Deadlock Detection) Code   */
  /*--------------------------------*/
  if (DD || CD)
  {
    InitCircList();
    InitVCList();
    InitCycleList();
    InitDeadlockList();
#ifdef HASH
    InitHashTable();
#endif
  }

  /* for testing/obtaining status ofnodes */
  if (TEST)
  { 
	print_nodes();
  	print_plinks();
  }

#ifndef NO_X
  if ( XDISP ) {
    if (dimensions == 1) {
      XtInit (argc, argv, width[0], 0);
      xinit (width[0], 0); 
      draw_network (width[0], 0); 

    } else {
      XtInit (argc, argv, width[0], width[1]);
      xinit (width[0], width[1]); 
      draw_network (width[0], width[1]); 
    }
  }
#endif

  do_faults();
  if (traffic == TRACEDRIVEN)
    init_trace(trace_file_name);

  sim_time = -1;
  init_stats ();
  INIT = 0;
  SIM_STATUS = 1;
  STEP = 0;

  sim_time = 0;
  nodes[0].disha_token = 1;


  if ( !XDISP )
    {
      while (1)
	event_loop();
    }
  else
    {
      /* transfer control to the X intrinsics */
#ifndef NO_X
      mainXloop();
#endif

      /* if no x is defined, run the main loop; this should never
	 be run if X is defined in the makefile */
      while (1)
	event_loop();
    }


}

void event_loop()
{

  /* MAIN EVENT LOOP */

  /* non intrinsic event handling - obsolete */
  /* 
  if ( XDISP ) 
  {
    xevents(sim_time);
  }
  */

  if ( SIM_STATUS || STEP) 
  {
    if ( STEP ) --STEP;

    /* check stats every 500 time units --- this should be a configurable parameter */
    if ((sim_time + 1) % 500 == 0)
      check_stats (sim_time);

    /* inject new message */
    inject_new ();

    /* update the network --- by performing network activities */
    update_network (); 

    /* if traffic is synthetic ... */
    if (traffic == SYNTHETIC) 
    {
      /* if simulation time is greater than 9999, then reset ... why ? */
      if (sim_time >= 9999 && INIT==0)
	init_stats ();

      /* if we've reached the specified "DUR" then stop the simulation */
      if ( sim_time >= RUN_TIME ) 
      {

	if (DDTEST >= PRINT_AT_END)
	{
		/* Print data structures based on if Cycle Deadlock and/or Deadlock is ON  */
		if (CD || DD)  
	  		PrintAdjacencyList();

		if (CD)  
          		PrintCycleList();

		if (DD)  
          		PrintDeadlockList();
	}

	report_stats ();
	exit(0);
      }
    } 

    /* if traffic is from trace ... */
    else
    {
	if (trace_done()) 
	{
		report_stats();
		exit(0);
	}
    }
	
    /* increment simulation time */
    sim_time++;
  }

}

#include <sys/time.h>

random_seed (seed)
     long seed;
{
  struct timeval tp;

  if (seed == 0L) {
    gettimeofday (&tp, NULL);
    seed = tp.tv_sec * 1000000 + tp.tv_usec;
  }
  printf ("seed = %d\n", seed);
  srand48 (seed);
}
