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

#include "dat2.h"
#include <math.h>
#include <stdio.h>

double drand48 ();

/* ----- Network state variables ---------  */
extern node *nodes;		/* These variables hold the network state */

/* ------ Message variables ---------  */
extern msg msgs[];	/* Misc information necessary for each msg */
extern int dimensions, *width, numvlinks, numnvlinks, numplinks, *part,
  *cum_part;
extern int PL, MSGL, inject_rate, DIST, RUN_TIME, HALF, DEMAND, PER, M, MAP;
extern int NODES, virts, node_virts, comm_mech, FAULTS, ABORT, NO_CTS;
extern int BUFFERS, SECURITY_LEVELS;
extern int nvlog, vlog, dimlog, nlog, protocol, backtrack;
extern int HSPOTS, *hsnodes, HSPERCENT;
extern int route_delay, select_fnc, data_delay;
extern int link_time, ack_time, link_arb_time;
extern char proto_name[];
extern int RADIUS, RAD, ORDER, RETRY;
extern int MESH, SRC_QUEUE, PROBE_LEAD;
extern int *dyn_fault_times, num_dyn_faults, dyn_cnt, physical_faults;
extern int traffic;

extern int HYBRID, LMSGL, LMSGPCT;

extern int sim_time;
extern float t_next;

extern int t1[16], t2[16], n[16];

extern int min_event_time, activity;

#ifdef SET_PORT
extern int useinjvcs, usedelvcs;
#endif

#ifdef PLINK
extern int plinknode;
extern int pl_injection;
extern int pl_delivery;
#endif

/*--------------------------------------------------------------*/
/* Synthetic traffic generator for flitsim                      */
/*--------------------------------------------------------------*/
/* exp_dist()  Get an exponentially distributed random value    */
/*--------------------------------------------------------------*/
float exp_dist (rate)
     float rate;
{
  float y;
  y = drand48 ();
  if (y == 1.0 || rate == 0.0)
    return 1.0e16;
  return (-log (1 - y) / rate);
}

/*-----------------------------------------------------------*/
/* Perfect shuffle                                           */
/*-----------------------------------------------------------*/
int perf_shuffle(i)
     int i;
{
  int n,k;

  n = i % 2;
  k = (n*NODES)/2 + i/2;
  return k;
}

/*-----------------------------------------------------------*/
/* flip bits                                                 */
/*-----------------------------------------------------------*/

int flip_bit(i)
     int i;
{
  int j,k,bit;
 
  k = 0;
  for (j=0; j<nlog; j++) {
    bit = i%2;
    i = i/2;
    if (bit == 0) {
      bit = 1 << j;
      k += bit;
    }
  }
  return k;
}

/*-----------------------------------------------------------*/
/* hot spot                                                  */
/*-----------------------------------------------------------*/

int hot_spot(i)
     int i;
{
  int bit,k,j;

  k = 0;
  for (j=0; j<nlog/2; j++) {
    bit = i%2;
    i = i/2;
    if (bit == 1) {
      bit = 1 << (nlog/2 + j);
      k += bit;
    }
  }
  
  for (j=nlog/2; j<nlog; j++) {
    bit = i%2;
    i = i/2;
    if (bit == 1) {
      bit = 1 << (j - nlog/2);
      k += bit;
    }
  }
  return k;
}

/*-----------------------------------------------------------*/
/* Bit reversal                                              */
/*-----------------------------------------------------------*/
int bit_reversal (i)
     int i;
{
  int j, k, bit;
  k = 0;
  for (j = 0; j < nlog; j++)
    {
      bit = i % 2;
      i = i / 2;
      k = k * 2;
      k |= bit;
    }
  return k;
}

/*-----------------------------------------------------------*/
/* Transpose                                                 */
/*-----------------------------------------------------------*/
int transpose (i)
     int i;
{
  int j, k;
  
  get_coords (i, t1);
  for (j = 0; j < dimensions - 1; j += 2)
    {
      t2[j] = t1[j + 1];
      t2[j + 1] = t1[j];
    }
  if (j < dimensions)
    t2[j] = t1[j];
  k = get_addr (t2);
  return k;
}

/*---------------------------------------------------------------*/
/* distance()  Calculate the minimum distance between two nodes. */
/*---------------------------------------------------------------*/
distance (n1, n2)
     int n1, n2;
{
  int j, sum, diff;
  
  get_coords (n1, t1);
  get_coords (n2, t2);
  sum = 0;
  for (j = 0; j < dimensions; j++)
    {
      if (t1[j] != t2[j])
	{
	  diff = abs (t1[j] - t2[j]);
	  sum += min (abs (width[j] - diff), diff);
	}
    }
  return sum;
}

/*---------------------------------------------------------------*/
/* meshdist()  Calculate the mesh distance between two nodes.    */
/*---------------------------------------------------------------*/
meshdist(n1,n2)
     int n1,n2;
{
  int j,sum,diff;
  
  get_coords(n1,t1);
  get_coords(n2,t2);
  sum=0;
  for (j=0; j<dimensions; j++) {
    if (t1[j]!=t2[j]) {
      diff = abs(t1[j]-t2[j]);
      sum += diff;
    }
  }
  return sum;
}

/*---------------------------------------------------------------*/
/* maxdist()  Calculate the L-infinity norm between two nodes.   */
/*---------------------------------------------------------------*/
maxdist(n1,n2)
     int n1,n2;
{
  int j,sum,diff;
  
  get_coords(n1,t1);
  get_coords(n2,t2);
  sum=0;
  for (j=0; j<dimensions; j++) {
    if (t1[j]!=t2[j]) {
      diff = abs(t1[j]-t2[j]);
      if (sum<diff) sum = diff;
    }
  }
  return sum;
}

/*---------------------------------------------------------------*/
/* get_node_at_dist()  Given a node and a distance, find another */
/*                     node in the network at that distance from */
/*                     the first node.                           */
/*---------------------------------------------------------------*/
get_node_at_dist (nd, dist)
     int nd, dist;
{
  int j, dt, i, sum, cnode;
  
  sum = 0;
  get_coords (nd, n);
  for (j = 0; j < dimensions - 1; j++)
    {
      dt = (int) (drand48 ()* (2 * (dist - sum) + 1)) - (dist - sum);
      n[j] = (n[j] + dt + width[j]) % width[j];
      sum += abs (dt);
    }
  dt = ((int) (drand48 ()* 2) * 2 - 1) * (dist - sum);
  n[dimensions - 1] = (n[dimensions - 1] + dt + width[dimensions - 1])
    % width[dimensions - 1];
  cnode = get_addr (n);
  return cnode;
}

/*---------------------------------------------------------------*/
/* get_node_in_box()  Given a node and a distance, find another  */
/*                    node in a hyper box where each component   */
/*                    differs by less than the distance          */
/*---------------------------------------------------------------*/
get_node_in_box (nd, dist)
     int nd, dist;
{
  int j, sign, cnode;
  
  get_coords (nd, n);
  for (j = 0; j < dimensions; j++)
    {
      n[j] = (n[j] + (int) (drand48 ()* (2 * dist + 1)) - dist + width[j])
	% width[j];
    }
  cnode = get_addr (n);
  return cnode;
}

/*----------------------------------------------------------------------*/
/* find_nearest_hotspot()   Find the nearest hotspot for a destination  */
/*----------------------------------------------------------------------*/
find_nearest_hotspot (n)
     int n;
{
  int dist, d, i, j;
  dist = 100;
  for (j = 0; j < HSPOTS; j++)
    {
      d = distance (n, hsnodes[j]);
      if (d < dist)
	{
	  dist = d;
	  i = hsnodes[j];
	}
    }
  return i;
}

/*----------------------------------------------------------------------*/
/* inject_spec()  Put specified message into the network                */
/*----------------------------------------------------------------------*/
inject_spec (src, dest, msglen, lev, msgtype)
     int src, dest, msglen, lev, msgtype;
{
  int i, j, k, num, cnt, rem;
  
  /* Calculate misc statistics */
  
  for (i=0; i<node_virts; i++) {
    if (nodes[src].send[i]==0) break;
  }
  if (nodes[src].send[i])
    {
      for (j = 0; j < SRC_QUEUE; j++)
	if (nodes[src].queue[j] == (-1))
	  {
	    nodes[src].queue[j] = sim_time;
	    break;
	  }
    }
  else
    {
      j = find_circ ();
      k = find_msg ();
      if (j < MAXCIRC && k < MAXMESSAGE)
	{
	  create_new_circ (j, src, dest, i, sim_time, lev, msgtype);
	  create_new_msg (k, j, msglen, sim_time, 0);
	}
      else
	printf ("Message or circuit overflow. \n");
    }
}


/*------------------------------------------------------------------------*/
/* inject_synth()  Put a new synthetically generated message into network */
/*------------------------------------------------------------------------*/
inject_synth ()
{
  float t;
  int src;
  int i, j, k, l, num, rem, o;
  
  if (inject_rate <= 0)
    {			/* Saturation load */
      num = NODES;
    }
  else
    {
      num = 0;
      while (t_next <= sim_time)
	{
	  activity++;
	  num++;
	  t = exp_dist ((float) inject_rate * NODES / PER);
	  /*    printf("t=%f\n",t); */
	  t_next += t;
	}
      min_event_time = t_next;
    }
  
  for (k = 0; k < num; k++)
    {
      if (inject_rate <= 0)
	src = k;
      else
	src = (int) (drand48 ()* NODES) % NODES;
#ifndef SET_PORT
      for (i=0; i<node_virts; i++) 
	{
#else
      for (i=0; i <useinjvcs;i++)
	{
#endif
#ifdef PLINK
      for (o =0;o<pl_injection;o++)
        {
#endif
	if (nodes[src].send[i+o*node_virts] == 0)
	  break;
	}
#ifdef PLINK
       if (nodes[src].send[i+o*node_virts] == 0)
	  break;
	}
#endif
      
      if (nodes[src].send[i+o*node_virts])
	{
	  for (j = 0; j < SRC_QUEUE; j++)
	    if (nodes[src].queue[j] == (-1))
	      {
		nodes[src].queue[j] = sim_time;
		break;
	      }
	}
      else
	{
	  j = find_circ ();
	  l = find_msg ();
	  if (j < MAXCIRC && l < MAXMESSAGE)
	    {
	      create_new_circ (j, src, src, i+o*node_virts, sim_time, 
			       uniform_dist(SECURITY_LEVELS), src);
	
	      if (HYBRID && (drand48 ()* 100 < LMSGPCT))
	        create_new_msg (l, j, LMSGL, sim_time, 0);

	      else	
	        create_new_msg (l, j, MSGL, sim_time, 0);
	    }
	  else
	    printf ("Message or circuit overflow. \n");
	}
    }
}

generate_dest(src) {
  int dest;

  dest = src;
  if (HSPOTS && (drand48 ()* 100 < HSPERCENT))
    {
      dest = find_nearest_hotspot (src);
    }
  else
    while (dest == src)
      {
	if (DIST == (-100))
	  {		/* Bit reversal pattern */
	    dest = bit_reversal (src);
	    return dest;
	  }
	else if (DIST == (-900))
	  {		/* Transpose pattern */
	    dest = transpose (src);
	    return dest;
	  } 
	else if (DIST==(-999)) 
	  {  /* Perfect shuffle */
	    dest = perf_shuffle(src);
	    return dest;
	  } 
	else if (DIST==(-1000)) 
	  { /* flip bits */
	    dest = flip_bit(src);
	    return dest;
	  } 
	else if (DIST==(-1001)) { /* hot spot */
	  dest = hot_spot(src);
	  return dest;
	}
	else if (DIST == 0)
	  dest = (int) (drand48 ()* NODES) % NODES;
	else if (DIST < 0)
	  {
	    dest = get_node_at_dist (src, -DIST);
	    if (distance (src, dest) != -DIST)
	      printf ("Error! bad distance\n");
	  }
	else
	  dest = get_node_in_box (src, DIST);
	if (MESH) {
	  if (DIST < 0 && meshdist(src,dest) > -DIST) dest=src;
	  if(DIST > 0 && maxdist(src,dest) > DIST) dest=src;
	}
      }
  return dest;
}







