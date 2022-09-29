/*
    Flitsim : A fully adaptable X-Windows based network simulator
    Copyright (C) 1993  Patrick Gaughan, Sudha Yalamanchili, and Peter Flur

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
/* data.h   Header file for time-step simulation program                */
/*----------------------------------------------------------------------*/

static char data_rcsid[] = "$Id: data.h,v 1.14 1993/12/02 16:13:56 pgaughan Exp $";

#define HOLDER 1000000
#define MAXMSG 2048

#define L      -1
#define R       1

#define PCS     0
#define WR      1
#define APCS    2
#define AWR     3

#define NORM    0
#define MIN_CON 1
#define ORD     2

#define Ecube   0
#define DP      1
#define MBM     2
#define EPB     3
#define DR      4
#define TPB     5
#define NEG     6
#define A1      7
#define DPORD   8
#define DPMIN   9
#define MBMP   10
#define SBT    11
#define MBMSWF 14
#define DOR    15
#define DPM    16
#define DISHA_TORUS  17
#define DISHA_MESH   18
#deifne DUATO  19

#define SOUTH   1
#define WEST    2
#define EAST    3
#define NORTH   4

/*----------------------------------------------------------------------*/
/* Misc functional macros                                               */
/*----------------------------------------------------------------------*/
#define sgn(x)    (x < 0 ? -1 : (x > 0 ? 1 : 0))
#define min(x,y)  (x < y ? x : y)
#define Hist_Mask(d,s) (1 << (d*2 + (s+1)/2))

/*----------------------------------------------------------------------*/
/* The following are macros for token conversion.  Each token can be a  */
/* data, probe, tail, acknowledge token.  These macros do the raw token */
/* to specific token conversions.                                       */
/*----------------------------------------------------------------------*/
#define Pr(x)       (x + MAXMSG)
#define UnPr(x)     (x - MAXMSG)
#define TestPr(x)   (x < MAXMSG ? 0 : (x < 2*MAXMSG ? 1 : 0))

#define Tail(x)     (x + MAXMSG + MAXMSG)
#define UnTail(x)   (x - MAXMSG - MAXMSG)  
#define TestTail(x) (x < 2*MAXMSG ? 0 : (x < 3*MAXMSG ? 1 : 0))

#define Ack(x)      (x + MAXMSG + MAXMSG + MAXMSG)
#define UnAck(x)    (x - MAXMSG - MAXMSG - MAXMSG)  
#define TestAck(x)  (x < 3*MAXMSG ? 0 : ( x < MAXMSG*4 ? 1 : 0))

#define Back(x)     (x + MAXMSG + MAXMSG + MAXMSG + MAXMSG)
#define UnBack(x)   (x - MAXMSG - MAXMSG - MAXMSG - MAXMSG)  
#define TestBack(x) (x < 4*MAXMSG ? 0 : ( x < MAXMSG*5 ? 1 : 0))

#define Data(x)     (-x-1)
#define UnData(x)   (-x-1)
#define TestData(x) (x < 0 ? 1 : 0)

#define BKill(x)     (x + MAXMSG + MAXMSG + MAXMSG + MAXMSG + MAXMSG)
#define UnBKill(x)   (x - MAXMSG - MAXMSG - MAXMSG - MAXMSG - MAXMSG)  
#define TestBKill(x) (x < 5*MAXMSG ? 0 : ( x < MAXMSG*6 ? 1 : 0))

#define FKill(x)     (x + MAXMSG + MAXMSG + MAXMSG + MAXMSG + MAXMSG + MAXMSG)
#define UnFKill(x)   (x - MAXMSG - MAXMSG - MAXMSG - MAXMSG - MAXMSG - MAXMSG)
#define TestFKill(x) (x < 6*MAXMSG ? 0 : ( x < MAXMSG*7 ? 1 : 0))

#define TAck(x)     (x + 7*MAXMSG)
#define UnTAck(x)   (x - 7*MAXMSG)
#define TestTAck(x) (x < 7*MAXMSG ? 0 : ( x < MAXMSG*8 ? 1 : 0))

#define UnToken(x)  (x < 0 ? -x-1 : x % MAXMSG)

/*----------------------------------------------------------------------*/
/* Data Types                                                           */
/*----------------------------------------------------------------------*/
typedef struct vlane {
  int forward_block, back_block;
  int out_buffer[2];
  int in_buffer[16], in_head, in_tail, in_buf;
  int next_in, control_in;
  int dr, pl, next;
  int busy, hist, bcp;
  int cnt;
  int security_level;
  int x, y;
  int height, width;
} vlane;

typedef struct link {
  int used;			/* Data to be transfered? */
  int nsl, nsr, csl, csr;	/* Setups */
  int ncl, ncr, ccl, ccr;	/* Circuits */
  int nrl, nrr, crl, crr;	/* Reserved */
  int LR, Llane, Rlane, startlane;
  int Lnode, Rnode, dim;
  int lcnt, rcnt, arbcnt;
  int security_level;
  int x, y;
  int height, width;
} plink;

typedef struct node {
  int queue[8];
  int headers[5], num_head;
  int send;
  int generate;
  int next, hist;
  int cnt;
  int security_level;
  int x, y;
  int height, width;
  int msg;
} node;

typedef struct msg {
  int src, dest, misc, misc2, M;
  int routecnt, retries;
  int links_res, complete, blocked;
  int cnt;
  int t_inject, t_queue, t_intr;
  int security_level;
  int path_length;
  int wait_time;
  int token;
} msg;

typedef struct intra_list {
  int vl, n, dim, delta;
  int next;
  int security_level;
} intra_list;

int DEBUG;

typedef struct rgb {
  int red[18], green[18], blue[18];
} rgb;








