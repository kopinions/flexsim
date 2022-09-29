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

static char data_rcsid[] = "$Id: dat2.h,v 1.5 1994/04/08 13:48:13 pgaughan Exp $";

#define SYNTHETIC    0
#define TRACEDRIVEN  1

#define HOLDER       1000000
#define MAXCIRC      8192
#define MAXMESSAGE   10240

#define L      -1
#define R       1

#define PCS     0
#define WR      1
#define APCS    2
#define AWR     3
#define RECON   4

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
#define SBM    12
#define MBMSWF 14
#define DOR    15
#define DPM    16
#define DISHA_TORUS  17
#define DISHA_MESH   18
#define DISHA_MESH_ONEDB   19
#define DUATO  20
#define DISHA_TORUS_SYNCH_TOKEN 21
#define DISHA_DOR_TORUS_SYNCH_TOKEN 22
#define DD_MIN_ADAPTIVE_BIDIR_TORUS 23
#define DD_MIN_ADAPTIVE_UNIDIR_TORUS 24
#define DD_NON_MIN_ADAPTIVE_BIDIR_TORUS 25
#define DD_NON_MIN_ADAPTIVE_UNIDIR_TORUS 26
#define DD_DETXY_BIDIR_TOR 27
#define DD_DETXY_UNIDIR_TOR 28
#define DD_TEST_ADAPTIVE 29

/* To support display of DISHA Toen Capture */
#define HIGHLIGHT_AS_USING_DBUFFER 1
#define HIGHLIGHT_AS_DESTINATION   2
#define HIGHLIGHT_AS_UNUSED        3



#define SOUTH   1
#define WEST    2
#define EAST    3
#define NORTH   4

#define INVALID      (-1)

#define NODE_TO_LINK   1
#define LINK_TO_LINK   2
#define LINK_TO_NODE   3

#define IN        1
#define OUT       0

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
#define Pr(x)       (x + MAXMESSAGE)
#define UnPr(x)     (x - MAXMESSAGE)
#define TestPr(x)   (x < MAXMESSAGE ? 0 : (x < 2*MAXMESSAGE ? 1 : 0))

#define Tail(x)     (x + MAXMESSAGE + MAXMESSAGE)
#define UnTail(x)   (x - MAXMESSAGE - MAXMESSAGE)  
#define TestTail(x) (x < 2*MAXMESSAGE ? 0 : (x < 3*MAXMESSAGE ? 1 : 0))

#define Ack(x)      (x + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE)
#define UnAck(x)    (x - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE)  
#define TestAck(x)  (x < 3*MAXMESSAGE ? 0 : ( x < MAXMESSAGE*4 ? 1 : 0))

#define Back(x)     (x + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE)
#define UnBack(x)   (x - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE)  
#define TestBack(x) (x < 4*MAXMESSAGE ? 0 : ( x < MAXMESSAGE*5 ? 1 : 0))

#define Data(x)     (-x-1)
#define UnData(x)   (-x-1)
#define TestData(x) (x < 0 ? 1 : 0)

#define BKill(x)     (x + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE)
#define UnBKill(x)   (x - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE)  
#define TestBKill(x) (x < 5*MAXMESSAGE ? 0 : ( x < MAXMESSAGE*6 ? 1 : 0))

#define FKill(x)     (x + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE + MAXMESSAGE)
#define UnFKill(x)   (x - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE - MAXMESSAGE)
#define TestFKill(x) (x < 6*MAXMESSAGE ? 0 : ( x < MAXMESSAGE*7 ? 1 : 0))

#define TAck(x)     (x + 7*MAXMESSAGE)
#define UnTAck(x)   (x - 7*MAXMESSAGE)
#define TestTAck(x) (x < 7*MAXMESSAGE ? 0 : ( x < MAXMESSAGE*8 ? 1 : 0))

#define UnToken(x)  (x < 0 ? -x-1 : x % MAXMESSAGE)

/*----------------------------------------------------------------------*/
/* Data Types                                                           */
/*----------------------------------------------------------------------*/
typedef struct vlane {
  int forward_block, back_block;
  int out_buffer;
  int *in_buffer, in_head, in_tail, in_buf;
  int next_in;
  int dr, pl, next;
  int busy, hist;
  int cnt, acks;
  int security_level;
  int x, y;
  int height, width;
  int data;
  int spec_col;
} vlane;

typedef struct cvlane {		/* Control lane */
  int out_buffer;
  int in_buffer;
  int next_in;
  int next;
} cvlane;

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
  int probe_register;
} plink;

typedef struct node {
  int *queue;
  int headers[4], num_head;
  int *send;
  int *generate, *consume;
  int cnt, idle, over;
  int security_level;
  int x, y;
  int height, width;
  int disha_token;
  int disha_token_siezed;
  int disha_deadlock_buffer_used;
} node;

typedef struct circ_s {
  int msgtype, src, dest;
  int acked;
  int misc, misc2;
  int routecnt, retries;
  int links_res, complete, blocked;
  int cnt;
  int security_level;
  int path_length;
  int times_bt, pl;
  int inject_virt;
  int wait_time;
  int token;
  int M;
} circuit;

typedef struct message {
  int circ_index;
  int t_inject, t_queue, t_intr;
  int msglen;
  int preroute;
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








