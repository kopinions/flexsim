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
/*--------------------------------------------------------------------*/
/* Routing module.  Instead of implementing each in a different       */
/*     program, we just implement them as different routines.         */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/* Note on Deadlock Detection:					      */
/* Those routing protocol functions that begin with 'dd_' support     */
/* the Cycle and Deadlock Detection mechanisms.  The goal is to have  */
/* a routing function 'dd_<name>' for every touting function <name>   */
/* until the two are integrated, and Deadlock detection is performed  */
/* with a switch						      */
/*--------------------------------------------------------------------*/

static char router_rcsid[] = "$Id: route2.c,v 1.10 1994/06/07 14:58:05 dao Exp dao $";

#include <stdio.h>
#include "dat2.h"
#include "dd.h"

#define PROF     1
#define UNPROF   2

double drand48();
/* extern int QNUM; yungho : variable for number of reception queues */
extern node *nodes;
extern plink *plinks;
extern vlane *vlinks;
extern msg msgs[MAXMESSAGE];
extern circuit circs[MAXCIRC];
extern int MISROUTES, INIT;
extern int tmp;
extern int comm_mech;
extern int SLOW, SAVED_SLOW, CUMWAIT;
extern int XDISP;
extern int NODES;
extern int TOKENSPEED, HALFTOKENSPEED;

extern int dimensions, *width;
extern int protocol,virts, backtrack, vlog, node_virts, use_virts;
extern int select_fnc,RADIUS,ORDER,vary_virts,flip_dim;
abs(x) { if (x>0) return x; else return -x; }

/* newly added - for Deadlock Detection */
extern int CD;
extern int DD;

#ifdef SET_PORT
extern int useinjvcs, usedelvcs;
#endif

#ifdef PLINK
extern int plinknode;
extern int pl_injection;
extern int pl_delivery;
#endif


/*--------------------------------------------------------------------*/
/* reserve_vlink()  Reserve a vlink and update its activity counters  */
/*--------------------------------------------------------------------*/
reserve_vlink(vl,tkn)
     int vl,tkn;
{
  int circ_no;
  circ_no = msgs[tkn].circ_index;
  vlinks[vl].busy = circ_no+1;
  vlinks[vl].forward_block = 0;
  vlinks[vl].back_block = 0;
  /*  vlinks[vl].security_level = circs[circ_no].security_level;  */
  circs[circ_no].links_res++;
  circs[circ_no].path_length++;
  inc_setup(vl);
}

/*--------------------------------------------------------------------*/
/* route_to_node()                                                    */
/*--------------------------------------------------------------------*/
route_to_node(nd, tkn) {
  int i, vl,l;
  
  /* yungho : control the number of the reception queues  
  for (i=0; i<QNUM; i++) {
  */
#ifndef SET_PORT
  for (i=0; i<node_virts; i++) {
#else
  for (i=0;i<usedelvcs; i++) {
#endif
#ifdef PLINK
  for (l=0; l < pl_delivery; l++) {
#endif
#ifdef PLINK
    vl = nlink_addr(nd, i, OUT, 0, l);
#else
    vl = nlink_addr(nd, i, OUT, 0);
#endif
    if (vlinks[vl].busy == 0) {
      reserve_vlink(vl, UnPr(tkn));
      return vl;
    }
  }
#ifdef PLINK
}
#endif
  return -1;
}

int s[16],d[16],n[16];
int j, k, sign, gamma, v;
int can_wait, min_c, vl_min;

/*----- DISHA Variables ----- */
int TOKEN = 0, seize_count = 0, seize_wait_time = 0;

extern	int	WAITTIME;

/*----- DISHA Variables ----- */

/*--------------------------------------------------------------------*/
/* route() Takes a probe token, a node and the link statuses and      */
/*         decides which virtual channel to route the probe over.     */
/*         return value of -1 indicates the probe blocks.             */
/*--------------------------------------------------------------------*/
route(nd,tkn,hist) 
     int nd,tkn,hist;
{

  int result;
  /*  calc_prof_dist(nd,tkn); */
  switch(protocol) {
  case Ecube:
    if (ORDER==0)
    {
      if (CD)
        result =  dd_detXY_bidir_tor(nd,tkn);
      else
        result =  ecube_tor(nd,tkn);
    }
    else if (ORDER==1)
      result =  ecube_mesh(nd,tkn);
  break;
  case DR:
    result =  drmin(nd,tkn);
  break;
  case DP:
    MISROUTES=0;
  case DPM:
    switch(select_fnc) {
    case NORM:	if (CD)
		{
			result =  dd_dp(nd,tkn); 
			break;
		}
		else
		{
			result =  dp(nd,tkn); 
			break;
		}

    case ORD: result =  dpord(nd,tkn); break;

    case MIN_CON: result =  dpmin(nd,tkn); break;
    }
  break;
  case MBM:
    if (select_fnc == MIN_CON) 
      result =  mbmmin(nd,tkn,hist);
    else
      result =  mbm(nd,tkn,hist);
    break;
  case MBMP:
    result =  mbmp(nd,tkn,hist);
    break;
  case EPB:
    if (select_fnc == MIN_CON) 
      result =  epbmin(nd,tkn,hist);
    else
      result =  epb(nd,tkn,hist);
    break;
  case TPB:
    result =  tpb(nd,tkn,hist);
    break;
  case A1:
    result =  a1(nd,tkn,hist);
    break;
  case MBMSWF:
    result =  mbmsw(nd,tkn,hist);
    break;
  case DOR:
    if (ORDER==0)
      result =  dor2_tor(nd,tkn);
    else if (ORDER==1)
      result =  dor_tor(nd,tkn);
    else if (ORDER==2)
      result =  dor2_mesh(nd,tkn);
    else if (ORDER==3)
      result =  dor_mesh(nd,tkn);
    break;
  case NEG:
    result = negfirst(nd, tkn);
    break;
  case SBM:
    result = sbm(nd,tkn,hist);
    break;
  case DISHA_TORUS:
		if (CD)
		{
			result =  dd_disha_torus(nd,tkn); 
			break;
		}
		else
		{
    			result = disha_torus(nd,tkn);
    			break;
		}
  case DISHA_TORUS_SYNCH_TOKEN:
    result = disha_torus_synchonous_token(nd,tkn);
    break;
  case DISHA_DOR_TORUS_SYNCH_TOKEN:
    result = disha_dor_torus_synchonous_token(nd,tkn);
    break;
  case DISHA_MESH:
    result = disha_mesh(nd,tkn);
    break;
  case DISHA_MESH_ONEDB:
    result = disha_mesh_onedb(nd,tkn);
    break;
  case DUATO:
    result = duato(nd,tkn);
    break;
  case DD_MIN_ADAPTIVE_BIDIR_TORUS:
    result = dd_min_adaptive_bidir_torus(nd,tkn);
    break;
  case DD_MIN_ADAPTIVE_UNIDIR_TORUS:
    result = dd_min_adaptive_unidir_torus(nd,tkn);
    break;
  case DD_NON_MIN_ADAPTIVE_BIDIR_TORUS:
    result = dd_non_min_adaptive_bidir_torus(nd,tkn);
    break;
  case DD_NON_MIN_ADAPTIVE_UNIDIR_TORUS:
    result = dd_non_min_adaptive_unidir_torus(nd,tkn);
    break;
  case DD_DETXY_BIDIR_TOR:
    result = dd_detXY_bidir_tor(nd,tkn);
    break;
  case DD_DETXY_UNIDIR_TOR:
    result = dd_detXY_unidir_tor(nd,tkn);
    break;
  case DD_TEST_ADAPTIVE:
    result = dd_test_adaptive(nd,tkn);
    break;
  default:
    result = ecube_tor(nd,tkn);
  }
  if (result < -1) 
    return route_to_node(nd, tkn);
  else
    return result;
}

/*---------------------------------------------*/
/* attempt_link()  Try a link                  */
/* used for "Negative First" routing only      */
/*---------------------------------------------*/
attempt_link(nd,i,j,sign,v) 
     int nd,i,j,sign,v;
{
  int vl;

  if (j < dimensions) {
	
    for (k=0; k<dimensions; k++)
      n[k] = s[k];
    n[j] = (s[j] + width[j] + sign)%width[j]; 
	  
    vl = link_addr(nd,j,sign,v,0);
    if (ok_to_out(vl) && vlinks[vl].busy == 0) {
      reserve_vlink(vl,i);
      return vl;
    }
  }
  return -1;
}

/*--------------------------------------------------------------------*/
/* negfirst() Negative first routing                                 */
/*--------------------------------------------------------------------*/
negfirst(nd, tkn)
     int nd, tkn;
{
  int can_neg, mask, vl, i, sign, j;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  if (!circs[circ_no].misc2) {
    /*-----------------------------*/
    /* Profitable negative routing */
    /*-----------------------------*/
    for (v=0; v < use_virts; v++) {
      j = -1;
      while (j < dimensions) {	
	j++;
  
	while (s[j] <= d[j] && j < dimensions) {
	  j++;
	}
	vl = attempt_link(nd,i,j,-1,v);
	if (vl >= 0) return vl;
      }
    }
  }
  
  mask = 0;
  for (j=0; j<dimensions; j++) 
    if (s[j] > d[j]) { mask = 1; break; }

  if (mask == 0) {		/* Profitable ok */
    /*-----------------------------*/
    /* Profitable positive routing */
    /*-----------------------------*/
    for (v=0; v < use_virts; v++) {
      j = -1;
      while (j < dimensions) {	
	j++;
	while (s[j] >= d[j] && j < dimensions) {
	  j++;
	}
	vl = attempt_link(nd,i,j,1,v);
	if (vl >= 0) {
	  circs[circ_no].misc2 = 1;
	  return vl;
	}
      }
    }
  }
    
  if (!circs[circ_no].misc2) {
    /*-----------------------------*/
    /* Negative misrouting         */
    /*-----------------------------*/
    for (v=0; v < use_virts; v++) {
      j = -1;
      while (j < dimensions) {	
	j++;
	while ((s[j] > d[j] || s[j]==0) && j < dimensions) {
	  j++;
	}
	vl = attempt_link(nd,i,j,-1,v);
	if (vl >= 0) return vl;
      }
    }
  }
  return -1;
}

/*--------------------------------------------------------------------*/
/* ecube()   Standard fixed order dimension elimination oblivious     */
/*--------------------------------------------------------------------*/
ecube_tor(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; 
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = 0;
  while (s[j] == d[j] && j<dimensions) 
    {
      j++;
    }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];

  gamma=d[j]-s[j];
  if (gamma > 0) v=0; else v=1;
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	
    
  n[j] = (s[j] + width[j] + sign)%width[j]; 

  if ((j == flip_dim) && (vary_virts == 1)) {
    for (i=0; i<virts/2; i++) {	
      vl = link_addr(nd, j, sign, v+i*2,circs[circ_no].security_level); 
      
      if (vlinks[vl].busy == 0) {
	reserve_vlink(vl,UnPr(tkn));
	return vl;
      }
    }
  } else if ((j == (1 - flip_dim)) && (vary_virts == 1)) {
    for (i=0; i<virts/4; i++) {	
      vl = link_addr(nd, j, sign, v+i*2,circs[circ_no].security_level); 
      
      if (vlinks[vl].busy == 0) {
      reserve_vlink(vl,UnPr(tkn));
      return vl;
      }
      }
  } else {
  for (i=0; i<virts/2; i++) {	
      vl = link_addr(nd, j, sign, v+i*2,circs[circ_no].security_level); 
      
      if (vlinks[vl].busy == 0) {
	reserve_vlink(vl,UnPr(tkn));
	return vl;
	}
      }
      }
  return -1;

/*
*  The following code segment was placed by someone ???
*  it did not work --- caused link address error 
*  so the original 'ecube_tor' code was put back (above)
*
  gamma=d[j]-s[j];
  if (width[j]-abs(gamma) < abs(gamma) )
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);
  
  n[j] = (s[j] + width[j] + sign)%width[j];

  if (gamma > 0)
    {
      vl = link_addr(nd, j, sign, 0, 0);
      if (vlinks[vl].busy == 0) 
	{
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}

      vl = link_addr(nd, j, sign, 1, 0);
      if (vlinks[vl].busy == 0)
        {
          reserve_vlink(vl,UnPr(tkn));
          return vl;
        }

      return (-1);
    }
  else
    {
      vl = link_addr(nd, j, sign, 2, 0);
      if (vlinks[vl].busy == 0)
        {
          reserve_vlink(vl,UnPr(tkn));
          return vl;
        }
      
      vl = link_addr(nd, j, sign, 3, 0);
      if (vlinks[vl].busy == 0)
        {
          reserve_vlink(vl,UnPr(tkn));
          return vl;
        }
 
      return (-1);
    }
*/

}


/*------------------------------------------------------------------------*/
/* dd_detXY_bidir_tor()   Deterministic XY Routing on Bidirectional Torus */
/*------------------------------------------------------------------------*/
dd_detXY_bidir_tor(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;
  int error;
  int DeadlockIndex;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; 
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = 0;
  while (s[j] == d[j] && j<dimensions) 
    {
      j++;
    }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];

  gamma=d[j]-s[j];
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	
    
  n[j] = (s[j] + width[j] + sign)%width[j]; 

  for (i=0; i < use_virts; i++) 
  {	
    vl = link_addr(nd, j, sign, i, 0); 
      
    if (vlinks[vl].busy == 0) 
    {
      reserve_vlink(vl,UnPr(tkn));

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_detXY_bidir_tor'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_detXY_bidir_tor'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

      return vl;
    }
    else
    {
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_detXY_bidir_tor'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

    }
  }
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* if the message is in a deadlock, the break the deadlock */
	/* by setting the message's destination to the current node */
	if (IsMessageInADeadlock(circ_no, &DeadlockIndex))
	{

		/* Remove all ReqVCs and ... */
		error = RemoveAllReqVCsFromCircuit(circ_no);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_detXY_bidir_tor'\n", circ_no);
			PrintAdjacencyList();
		}

		/* Deactivate the Deadlock ... since we broke it */
		error = RetireDeadlock(DeadlockIndex);

		if (error)
		{
			printf("ERROR: from 'RetireDeadlock' in 'dd_detXY_bidir_tor'\n");
		}

		circs[circ_no].dest = nd;
		return -nd-2; 
	}

	/*--------------------------------*/

  return -1;

}



/*--------------------------------------------------------------------------*/
/* dd_detXY_unidir_tor()   Deterministic XY Routing on Unidirectional Torus */
/*--------------------------------------------------------------------------*/
dd_detXY_unidir_tor(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;
  int error;
  int DeadlockIndex;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; 
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = 0;
  while (s[j] == d[j] && j<dimensions) 
    {
      j++;
    }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];

  /* fix the sign since we are using only one direction */
  sign = 1;	
    
  n[j] = (s[j] + width[j] + sign)%width[j]; 

  for (i=0; i < use_virts; i++) 
  {
    vl = link_addr(nd, j, sign, i, 0); 
      
    if (vlinks[vl].busy == 0) 
    {
      reserve_vlink(vl,UnPr(tkn));

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_detXY_unidir_tor'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_detXY_unidir_tor'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

      return vl;
    }
    else
    {
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_detXY_unidir_tor'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

    }
  }
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* if the message is in a deadlock, the break the deadlock */
	/* by setting the message's destination to the current node */
	if (IsMessageInADeadlock(circ_no, &DeadlockIndex))
	{

		/* Remove all ReqVCs and ... */
		error = RemoveAllReqVCsFromCircuit(circ_no);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_detXY_unidir_tor'\n", circ_no);
			PrintAdjacencyList();
		}

		/* Deactivate the Deadlock ... since we broke it */
		error = RetireDeadlock(DeadlockIndex);

		if (error)
		{
			printf("ERROR: from 'RetireDeadlock' in 'dd_detXY_unidir_tor'\n");
		}

		circs[circ_no].dest = nd;
		return -nd-2; 
	}

	/*--------------------------------*/

  return -1;

}



ecube_mesh(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = 0;
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];
    
  if ((j == flip_dim) && (vary_virts == 1)) {
    for (v=0; v<virts; v++) {
      gamma=d[j]-s[j];
      sign = sgn(gamma);	/* Decide which way to go */
      
      n[j] = (s[j] + width[j] + sign)%width[j]; 
      
      /* security level now becomes a factor in the virtual address */
      /* calculation */
      vl = link_addr(nd, j, sign, v, circs[circ_no].security_level); 
      
/*      printf("dest %d nd %d j %d sign %d\n", circs[circ_no].dest, nd, j, */
 /*      sign); */
      
      if (vlinks[vl].busy == 0) {
	reserve_vlink(vl,UnPr(tkn));
	return vl;
      }
    }
  }
  else if ((j == (1 - flip_dim)) && (vary_virts == 1)) {
    for (v=0; v<virts/2; v++) {
      gamma=d[j]-s[j];
      sign = sgn(gamma);	/* Decide which way to go */
      
      n[j] = (s[j] + width[j] + sign)%width[j]; 
      
      /* security level now becomes a factor in the virtual address */
      /* calculation */
      vl = link_addr(nd, j, sign, v,  circs[circ_no].security_level); 
      
/*      printf("dest %d nd %d j %d sign %d\n", circs[circ_no].dest, nd, j, */
 /*      sign); */
      
      if (vlinks[vl].busy == 0) {
	reserve_vlink(vl,UnPr(tkn));
	return vl;
      }
    }
  } else {
    for (v=0; v<virts; v++) {
      gamma=d[j]-s[j];
      sign = sgn(gamma);	/* Decide which way to go */
      
      n[j] = (s[j] + width[j] + sign)%width[j]; 
      
      /* security level now becomes a factor in the virtual address */
      /* calculation */
      vl = link_addr(nd, j, sign, v,  circs[circ_no].security_level); 
      
/*      printf("dest %d nd %d j %d sign %d\n", circs[circ_no].dest, nd, j, */
 /*      sign); */
      
      if (vlinks[vl].busy == 0) {
	reserve_vlink(vl,UnPr(tkn));
	return vl;
      }
    }
  }
  return -1;
}
/*-------------------------------------------------------------------*/
/* Direction Order Routing                                          */
/*-------------------------------------------------------------------*/

dor_tor(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd ==  circs[circ_no].dest) return -nd-2; /* we are there */
  get_coords(nd,s);
  get_coords( circs[circ_no].dest,d);

  j= (-1);
  do {
    j++;
    gamma = d[j]-s[j];
    if (width[j]-abs(gamma) < abs(gamma))
      sign = -sgn(gamma);
    else
      sign = sgn(gamma);
  } while (sign <=0 && (j<dimensions));

  if (sign > 0 && j<dimensions) {
    if (gamma > 0) v=0; else v=1;
    n[j] = (s[j] + width[j] + sign)%width[j];

    if ((j == flip_dim) && (vary_virts == 1)) {
      for (i=0; i<virts/2; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else if ((j == (1-flip_dim)) && (vary_virts == 1)) {
      for (i=0; i<virts/4; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      } 
    } else {
      for (i=0; i<virts/2; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    }
    return -1;
  }
  j= dimensions;
  do {
    j--;
    gamma = d[j]-s[j];
    if (width[j]-abs(gamma) < abs(gamma))
      sign = -sgn(gamma);
    else
      sign = sgn(gamma);
  } while (sign >=0 && (j>=0));
  if (sign < 0 && j>=0) {
    if (gamma > 0) v=0; else v=1;
    n[j] = (s[j] + width[j] + sign)%width[j];

    if ((j == flip_dim) && (vary_virts == 1)) {
      for (i=0; i<virts/2; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else if ((j == (1-flip_dim)) && (vary_virts == 1)) {
      for (i=0; i<virts/4; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else {
      for (i=0; i<virts/2; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    }
    return -1;
  } 
  printf("We aren't supposed to be here...\n");
}


dor2_tor(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd ==  circs[circ_no].dest) return -nd-2; /* we are there */
  get_coords(nd,s);
  get_coords( circs[circ_no].dest,d);

  j= (-1);
  do {
    j++;
    gamma = d[j]-s[j];
    if (width[j]-abs(gamma) < abs(gamma))
      sign = -sgn(gamma);
    else
      sign = sgn(gamma);
  } while (sign <=0 && (j<dimensions));

  if (sign > 0 && j<dimensions) {
    if (gamma > 0) v=0; else v=1;
    n[j] = (s[j] + width[j] + sign)%width[j];

    if ((j == flip_dim) && (vary_virts == 1)) {
      for (i=0; i < use_virts/2; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else if ((j == (1-flip_dim)) && (vary_virts == 1)) {
      for (i=0; i < use_virts/4; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else {
      for (i=0; i < use_virts/2; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    }
      return -1;
  }
  j= -1;
  do {
    j++;
    gamma = d[j]-s[j];
    if (width[j]-abs(gamma) < abs(gamma))
      sign = -sgn(gamma);
    else
      sign = sgn(gamma);
  } while (sign >=0 && (j<dimensions));
  if (sign < 0 && j>=0) {
    if (gamma > 0) v=0; else v=1;
    n[j] = (s[j] + width[j] + sign)%width[j];

    if ((j == flip_dim) && (vary_virts == 1)) {
      for (i=0; i < use_virts/2; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else if (( j == (1-flip_dim)) && (vary_virts == 1)) {
      for (i=0; i< use_virts/4; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else {
      for (i=0; i < use_virts/2; i++) {
	vl = link_addr(nd,j,sign,v+i*2,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    }
    return -1;
  } 
  printf("We aren't supposed to be here...\n");
}

dor_mesh(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd ==  circs[circ_no].dest) return -nd-2; /* we are there */
  get_coords(nd,s);
  get_coords( circs[circ_no].dest,d);

  j= (-1);
  do {
    j++;
    gamma = d[j]-s[j];
    sign = sgn(gamma);
  } while (sign <=0 && (j<dimensions));

  if (sign > 0 && j<dimensions) {
    n[j] = (s[j] + width[j] + sign)%width[j];
  
    if ((j == flip_dim) && (vary_virts == 1)) {
      for (v=0; v<virts; v++) {
	vl = link_addr(nd, j, sign, v, 0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else if ((j == (1-flip_dim)) && (vary_virts == 1)) {
      for (v=0; v<virts/2; v++) {
	vl = link_addr(nd, j, sign, v, 0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else {
      for (v=0; v<virts; v++) {
	vl = link_addr(nd, j, sign, v, 0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    }
    return -1;
  }
  j= dimensions;
  do {
    j--;
    gamma = d[j]-s[j];
    sign = sgn(gamma);
  } while (sign >=0 && (j>=0));
  if (sign < 0 && j>=0) {
    n[j] = (s[j] + width[j] + sign)%width[j];

    if ((j == flip_dim) && (vary_virts == 1)) {
      for (v=0; v<virts; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else if ((j == (1-flip_dim)) && (vary_virts == 1)) {
      for (v=0; v<virts/2; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else {
      for (v=0; v<virts; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    }
    return -1;
  } 
  printf("We aren't supposed to be here...\n");
}


dor2_mesh(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* we are there */
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j= (-1);
  do {
    j++;
    gamma = d[j]-s[j];
    sign = sgn(gamma);
  } while (sign <=0 && (j<dimensions));

  if (sign > 0 && j<dimensions) {
    n[j] = (s[j] + width[j] + sign)%width[j];

    if ((j == flip_dim) && (vary_virts == 1)) {
      for (v=0; v<virts; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else if ((j == (1-flip_dim)) && (vary_virts == 1)) {
      for (v=0; v<virts/2; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else {
      for (v=0; v<virts; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    }
    return -1;
  }
  j= -1;
  do {
    j++;
    gamma = d[j]-s[j];
    sign = sgn(gamma);
  } while (sign >=0 && (j<dimensions));
  if (sign < 0 && j>=0) {
    n[j] = (s[j] + width[j] + sign)%width[j];

    if ((j == flip_dim) && (vary_virts == 1)) {
      for (v=0; v<virts; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else if ((j == (1-flip_dim)) && (vary_virts == 1)) {
      for (v=0; v<virts/2; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    } else {
      for (v=0; v<virts; v++) {
	vl = link_addr(nd,j,sign,v,0);
	if (vlinks[vl].busy == 0) {
	  reserve_vlink(vl,UnPr(tkn));
	  return vl;
	}
      }
    }
    return -1;
  } 
  printf("We aren't supposed to be here...\n");
}

/*--------------------------------------------------------------------*/
/* vroute()   route                                                   */
/*--------------------------------------------------------------------*/
vroute(vl,i)
     int vl,i;
{
  if (vlinks[vl].busy == 0 ) {
    reserve_vlink(vl,i);
    return vl;
  }
  return -1;
}

/*--------------------------------------------------------------------*/
/* det_route()   Deterministic route                                  */
/*--------------------------------------------------------------------*/
det_route(nd,i)
{
  int vl;

  j = 0;
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }
  
  for (k=0; k<dimensions; k++)
    n[k] = s[k];
  
  gamma=d[j]-s[j];
  if (gamma > 0) v=0; else v=1;
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	/* Decide which way to go */
  
  n[j] = (s[j] + width[j] + sign)%width[j]; 
  
  vl = link_addr(nd, j, sign, v, 0);
  if (vlinks[vl].busy == (-1)) 
    if (comm_mech == AWR) backtrack=1;
  return vroute(vl,i);
}

/*--------------------------------------------------------------------*/
/* min_congest()  Minimum congestion - find the minimum congestion    */
/*                      virtual link                                  */
/*--------------------------------------------------------------------*/
min_congest(nd, dr, prof) 
     int nd, dr, prof;
{
  int i, do_it, busy, vm, free, dr_ok, vl;

	can_wait = 0;
 	min_c = 100;
  	vl_min = -1;

	/* Check all dimensions */
  	for (j=0; j<dimensions; j++) 
	{	
    		gamma=d[j]-s[j];

		/* Check all directions */
    		for (sign=(-1); sign<2; sign+=2) 
		{ 
      			if (width[j]-abs(gamma) < abs(gamma) ) 
			{
				/* Mark profitable directions */
				if (sign == -sgn(gamma)) 
	  				do_it = 1;
				else
	  				do_it = -1;
      			} 
			else 
				if (sign == sgn(gamma)) 
	  				do_it = 1;
				else
	  				do_it = -1;

			/* Profitable or not? */
      			if (prof == UNPROF)	
				do_it = -do_it;

      			if (do_it == 1) 
			{
				busy = 0;
				vm = -1;
				dr_ok = 0;
				free = 0;

				for (k=2; k < use_virts; k++) 
				{
	  				vl = link_addr(nd,j,sign,k,0);

					/* Keep track the number of busy channels */
	  				if (vlinks[vl].busy != 0) 
					{
	    					busy++;
	    					if (vlinks[vl].busy > 0) 
	      						if (vlinks[vl].dr > dr) 
							{
								if (!free) vm = vl;
								dr_ok = 1;
	      						}
					} 

					/* Even if one VC is free, then we can use this dimension */
					else 
					{
						free = 1;
	    					vm = vl;
	 				}

				}

				if (dr_ok || free) 
				{
					can_wait=1;
					if (min_c > busy) 
					{
						min_c = busy;
						vl_min = vm;
					}
				}
			}
		}
	}
}

/*--------------------------------------------------------------------*/
/* bmin_congest()  Minimum congestion - find the minimum congestion   */
/*                      virtual link (backtracking version)           */
/*--------------------------------------------------------------------*/
bmin_congest(nd, hist, prof) 
     int nd, hist, prof;
{
  int i, do_it, busy, vm, free, dr_ok, vl;

  min_c = 100;
  vl_min = -1;
  for (j=0; j<dimensions; j++) {	/* Check all dimensions */
    gamma=d[j]-s[j];
    for (sign=(-1); sign<2; sign+=2) { /* Check all directions */
      if ((hist & Hist_Mask(j, sign)) == 0) {
	if (width[j]-abs(gamma) < abs(gamma) ) {
	  if (sign == -sgn(gamma)) /* Mark profitable directions */
	    do_it = 1;
	  else
	    do_it = -1;
	} else 
	  if (sign == sgn(gamma)) 
	    do_it = 1;
	  else
	    do_it = -1;
	if (prof == UNPROF)	/* Profitable or not? */
	  do_it = -do_it;
	if (do_it == 1) {
	  busy = 0;
	  vm = -1;
	  dr_ok = 0;
	  free = 0;
	  for (k=0; k<virts; k++) {
	    vl = link_addr(nd,j,sign,k,0);
	    if (vlinks[vl].busy != 0) {
	      busy++;
	      if (vlinks[vl].busy > 0) 
		if (!free) vm = vl;
	    } else {
	      free = 1;
	      vm = vl;
	    }
	  }
	  if (free) {
	    if (min_c > busy) {
	      min_c = busy;
	      vl_min = vm;
	    }
	  }
	}
      }
    }
  }
}

/*--------------------------------------------------------------------*/
/* drmin()   Dimension Reversals - adaptive protocol (min congestion) */
/*             Dally's "fault-tolerant" protocol                      */
/*--------------------------------------------------------------------*/
drmin(nd,tkn)
     int nd,tkn;
{
  int i, channel_value, vl;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  if (circs[circ_no].misc2 < 100) {	/* We can use adaptive channels */
    min_congest(nd, circs[circ_no].misc2, PROF);
    if (can_wait) {
      if (min_c < use_virts-2) {
	vl = vroute(vl_min,i);
	if (vl != (-1)) {
	  vlinks[vl].dr = circs[circ_no].misc2;
	  channel_value = dim_of_link(vl)*4 + sign_of_link(vl) + 1;
	  if (channel_value < circs[circ_no].misc)
	    circs[circ_no].misc2++;
	  circs[circ_no].misc = channel_value;
	  return vl;
	}
      }	else
	return -1;
    }
    min_congest(nd, circs[circ_no].misc2, UNPROF);
    if (can_wait) {
      if (min_c < virts-2) {
	vl = vroute(vl_min,i);
	if (vl != (-1)) {
	  vlinks[vl].dr = circs[circ_no].misc2;
	  channel_value = dim_of_link(vl)*4 + sign_of_link(vl)+1;
	  if (channel_value < circs[circ_no].misc)
	    circs[circ_no].misc2++;
	  circs[circ_no].misc = channel_value;
	  return vl;
	}
      } else
	return -1;
    }
  } 
  circs[circ_no].misc2 = 100;	/* Have to use deterministic channels */
  return det_route(nd,i);
}

/*--------------------------------------------------------------------*/
/* duato(nd,tkn) - My adaptation                                      */
/*--------------------------------------------------------------------*/
duato(nd,tkn)
{
return (-1);
}


/*--------------------------------------------------------------------*/
/* dp()   Duato - Idle adaptive protocol (cycle through virtual links)*/
/*--------------------------------------------------------------------*/
dp(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;

  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  for (v=2; v < use_virts; v++) {	/* Adaptive subfunction */
    j = dimensions;
    while (j >= 0) {	/* Idle part */
      j--;
      while (s[j] == d[j] && j >=0) {
	j--;
      }
	
      if (j>=0) {
	for (k=0; k<dimensions; k++)
	  n[k] = s[k];
    
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	  
	n[j] = (s[j] + width[j] + sign)%width[j]; 

	vl = link_addr(nd,j,sign,v,0);
	if ( vlinks[vl].busy == 0) {
	  reserve_vlink(vl,i);
	  return vl;
	}
      }
    }
    
  }

  /* Misroute extension */
  if (!circs[circ_no].misc2) {
    if (circs[circ_no].cnt < MISROUTES) {
      for (v=2; v < use_virts-1; v++) {	/* Adaptive channels */
	for (j=0; j<dimensions; j++) {
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
	  
	  for (sign = -1; sign<=1; sign+=2) {
	    n[j] = (s[j] + width[j] + sign)%width[j]; 
	    
	    vl = link_addr(nd,j,sign,v,0);
	    if ( vlinks[vl].busy == 0) {
	      reserve_vlink(vl,i);
	      circs[circ_no].cnt++;
	      return vl;
	    }
	  }
	}
      }
    }
  }

  j = 0;			/* Ecube subfunction */
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];
    
  gamma=d[j]-s[j];
  if (gamma > 0) v=0; else v=1;
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	/* Decide which way to go */
    
  n[j] = (s[j] + width[j] + sign)%width[j]; 

  vl = link_addr(nd, j, sign, v, 0);
  if (vlinks[vl].busy == 0) {
    reserve_vlink(vl,i);
    circs[circ_no].misc2 = 1;
    return vl;
  }
  if (vlinks[vl].busy == (-1)) 
    if (comm_mech == AWR) backtrack=1;

  return -1;
}

/*----------------------------------------------------------------------------------------------*/
/* dd_dp()   Duato - Idle adaptive protocol (cycle through virtual links) w/ Deadlock Detection */
/*----------------------------------------------------------------------------------------------*/
dd_dp(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;
  int error;

  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  for (v=2; v < use_virts; v++) {	/* Adaptive subfunction */
    j = dimensions;
    while (j >= 0) {	/* Idle part */
      j--;
      while (s[j] == d[j] && j >=0) {
	j--;
      }
	
      if (j>=0) {
	for (k=0; k<dimensions; k++)
	  n[k] = s[k];
    
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	  
	n[j] = (s[j] + width[j] + sign)%width[j]; 

	vl = link_addr(nd,j,sign,v,0);
	if ( vlinks[vl].busy == 0) 
	{
	  reserve_vlink(vl,i);

		/*--------------------------------*/
		/* DD (Deadlock Detection) Code   */
		/*--------------------------------*/

		/* Remove all ReqVCs and ... */
		error = RemoveAllReqVCsFromCircuit(circ_no);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
			PrintAdjacencyList();
		}

		/* ... Add the VC to the Circuit */
		error = AddVCToCircuit(circ_no, vl);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
			PrintAdjacencyList();
		}

		/*--------------------------------*/

	  return vl;
	}

	/* else (if the vlink IS busy), then put it on the ReqVC list */
	else
	{

		/*--------------------------------*/
		/* DD (Deadlock Detection) Code   */
		/*--------------------------------*/

		/* Add ReqVC to the list */
		error = AddReqVCToCircuit(circ_no, vl);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
			PrintAdjacencyList();
		}

		/*--------------------------------*/

	 }

      }
    }
    
  }

  /* Misroute extension */
  if (!circs[circ_no].misc2) {
    if (circs[circ_no].cnt < MISROUTES) {
      for (v=2; v < use_virts; v++) {	/* Adaptive channels */
	for (j=0; j<dimensions; j++) {
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
	  
	  for (sign = -1; sign<=1; sign+=2) {
	    n[j] = (s[j] + width[j] + sign)%width[j]; 
	    
	    vl = link_addr(nd,j,sign,v,0);
	    if ( vlinks[vl].busy == 0) 
            {
	      reserve_vlink(vl,i);
	      circs[circ_no].cnt++;

		/*--------------------------------*/
		/* DD (Deadlock Detection) Code   */
		/*--------------------------------*/

		/* Remove all ReqVCs and ... */
		error = RemoveAllReqVCsFromCircuit(circ_no);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
			PrintAdjacencyList();
		}

		/* ... Add the VC to the Circuit */
		error = AddVCToCircuit(circ_no, vl);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
			PrintAdjacencyList();
		}

		/*--------------------------------*/

	      return vl;
	    }
				
	    /* else (if the vlink IS busy), then put it on the ReqVC list */
	    else
	    {

		/*--------------------------------*/
		/* DD (Deadlock Detection) Code   */
		/*--------------------------------*/

		/* Add ReqVC to the list */
		error = AddReqVCToCircuit(circ_no, vl);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
			PrintAdjacencyList();
		}

		/*--------------------------------*/

	    }
	  }
	}
      }
    }
  }

  j = 0;			/* Ecube subfunction */
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];
    
  gamma=d[j]-s[j];
  if (gamma > 0) v=0; else v=1;
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	/* Decide which way to go */
    
  n[j] = (s[j] + width[j] + sign)%width[j]; 

  vl = link_addr(nd, j, sign, v, 0);
  if (vlinks[vl].busy == 0) 
  {
    reserve_vlink(vl,i);
    circs[circ_no].misc2 = 1;

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

    return vl;
  }

  /* else (if the vlink IS busy), then put it on the ReqVC list */
  else
  {
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/
  }

  if (vlinks[vl].busy == (-1)) 
    if (comm_mech == AWR) backtrack=1;

  return -1;
}

/*--------------------------------------------------------------------*/
/* disha() - Anjan's protocol                                         */
/*--------------------------------------------------------------------*/
 
disha_torus(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;
 
  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;

/*  
  printf("Source = %d\n",circs[circ_no].src);
  printf("Destination = %d\n",circs[circ_no].dest);
  printf("Misroutes = %d\n",circs[circ_no].M);
*/

  if (nd == circs[circ_no].dest) 
    {
      if (circs[circ_no].token == 1) /* check if u r d 1 with d token */
	{
	  TOKEN = 0;
	  circs[circ_no].token = 0;
	}	
      return -nd-2; /* We are there. */
    }

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  if (circs[circ_no].token != 1)
    {
      for (v = 1; v < use_virts+1 ; v++)
	{ 
	  j = dimensions;
	  
	  while (j >= 0)                 /* Adaptive subfunction */
	    {
	      j--;
	      
	      while (s[j] == d[j] && j >=0)
		{
		  j--;
		}
	      
	      if (j >= 0) 
		{
		  for (k=0; k<dimensions; k++) n[k] = s[k];
		  
		  gamma = d[j]-s[j];
		  
		  if (width[j]-abs(gamma) < abs(gamma) )
		    sign = -sgn(gamma);
		  else
		    sign = sgn(gamma);    /* Decide which way to go */
		  
		  n[j] = (s[j] + width[j] + sign)%width[j];
		  
		  vl = link_addr(nd,j,sign,v,0);
		  
		  if ( vlinks[vl].busy == 0) 
		    {
		      reserve_vlink(vl,i);
		      circs[circ_no].wait_time = 0;		      
		      return vl;
		    }
   		}
	    } 
	}

  /* Misroute extension */

      if (!circs[circ_no].misc2)  /* Am I permitted to misroute ? */
	{
	  if (circs[circ_no].cnt < MISROUTES) 
	    {

	      for (v = 1; v < use_virts+1; v++)

		{ 
		  for (j = 0; j < dimensions; j++) 
		    {
		      for (k = 0; k < dimensions; k++) 
			{
			  n[k] = s[k];
			}
		      
		      for (sign = -1; sign <= 1; sign+= 2) 
			{
			  n[j] = (s[j] + width[j] + sign)%width[j];
			  
			  vl = link_addr(nd,j,sign,v,0);
		      
			  if ( vlinks[vl].busy == 0) 
			    {
			      reserve_vlink(vl,i);
			      circs[circ_no].cnt++;
			      circs[circ_no].wait_time = 0;
			      return vl;
			    }
			}
		    } 
		} 
	    }
	}

      if (circs[circ_no].wait_time <= WAITTIME)
	{
	  circs[circ_no].wait_time ++;
	  return -1;
	}
      else
	{
	  if (TOKEN == 1) 
	    {
	      seize_wait_time++;

	      circs[circ_no].wait_time ++;
	      return -1;
	    }
	  else
	    {
	      v = 0;

	      j = 0; 
	      while (s[j] == d[j] && j < dimensions) 
		{
		  j++;
		}
	      
	      for (k=0; k<dimensions; k++) n[k] = s[k];
	      
	      gamma = d[j]-s[j];
	      
	      if (width[j]-abs(gamma) < abs(gamma) )
		sign = -sgn(gamma);
	      else
		sign = sgn(gamma);             /* Decide which way to go */
	      
	      n[j] = (s[j] + width[j] + sign)%width[j];
	      
	      vl = link_addr(nd, j, sign, v, 0);
	      
	      if (vlinks[vl].busy == 0) 
		{ 
		  TOKEN = 1;
		  circs[circ_no].token = 1;
		  seize_count ++;

		  reserve_vlink(vl,i);
		  circs[circ_no].misc2 = 1;
		  return vl;
		}
	      else
		{
	          seize_wait_time++;

		  return -1;
		} 
	    }
	}
    }

  else
    {
      v = 0;
      j = 0;
      while (s[j] == d[j] && j < dimensions)
	  j++;
      
      for (k=0; k<dimensions; k++) n[k] = s[k];
      
      gamma = d[j]-s[j];
      
      if (width[j]-abs(gamma) < abs(gamma) )
	sign = -sgn(gamma);
      else
	sign = sgn(gamma);             /* Decide which way to go */
      
      n[j] = (s[j] + width[j] + sign)%width[j];
      
      vl = link_addr(nd, j, sign, v, 0);
      
      if (vlinks[vl].busy == 0)
	{ 

	  reserve_vlink(vl,i);
	  circs[circ_no].misc2 = 1; /* No more misrouting */
	  return vl;
	}
      else
	{
	  return -1;
	}
    }
}


/*--------------------------------------------------------------------*/
/* dd_disha_torus() - Anjan's protocol (w/ Deadlock Detection)        */
/*--------------------------------------------------------------------*/
 
dd_disha_torus(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;
  int DeadlockIndex;
  int error;

  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) 
    {
      return -nd-2; /* We are there. */
    }

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

      for (v = 0; v < use_virts+1; v++)
	{ 
	  j = dimensions;
	  
	  while (j >= 0)                 /* Adaptive subfunction */
	    {
	      j--;
	      
	      while (s[j] == d[j] && j >=0)
		{
		  j--;
		}
	      
	      if (j >= 0) 
		{
		  for (k=0; k<dimensions; k++) n[k] = s[k];
		  
		  gamma = d[j]-s[j];
		  
		  if (width[j]-abs(gamma) < abs(gamma) )
		    sign = -sgn(gamma);
		  else
		    sign = sgn(gamma);    /* Decide which way to go */
		  
		  n[j] = (s[j] + width[j] + sign)%width[j];
		  
		  vl = link_addr(nd,j,sign,v,0);
		  
		  if ( vlinks[vl].busy == 0) 
		    {
		      reserve_vlink(vl,i);
		      circs[circ_no].wait_time = 0;

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/

			/* Remove all ReqVCs and ... */
			error = RemoveAllReqVCsFromCircuit(circ_no);

			/* if there's an error, report it */
			if (error)
			{
				printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_disha_torus'\n", circ_no);
				PrintAdjacencyList();
			}

			/* ... Add the VC to the Circuit */
			error = AddVCToCircuit(circ_no, vl);

			/* if there's an error, report it */
			if (error)
			{
				printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_disha_torus'\n", circ_no, vl);
				PrintAdjacencyList();
			}

			/*--------------------------------*/
		      
		      return vl;
		    }

  		  /* else (if the vlink IS busy), then put it on the ReqVC list */
  		  else
  		  {
			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/

			/* Add ReqVC to the list */
			error = AddReqVCToCircuit(circ_no, vl);

			/* if there's an error, report it */
			if (error)
			{
				printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_disha_torus'\n", circ_no, vl);
				PrintAdjacencyList();
			}

			/*--------------------------------*/
  		  }

   		}
	    } 
	}

  /* Misroute extension */

      if (!circs[circ_no].misc2)  /* Am I permitted to misroute ? */
	{
	  if (circs[circ_no].cnt < MISROUTES) 
	    {
	      for (v = 0; v < use_virts+1; v++)
		{ 
		  for (j = 0; j < dimensions; j++) 
		    {
		      for (k = 0; k < dimensions; k++) 
			{
			  n[k] = s[k];
			}
		      
		      for (sign = -1; sign <= 1; sign+= 2) 
			{
			  n[j] = (s[j] + width[j] + sign)%width[j];
			  
			  vl = link_addr(nd,j,sign,v,0);
		      
			  if ( vlinks[vl].busy == 0) 
			    {
			      reserve_vlink(vl,i);
			      circs[circ_no].cnt++;
			      circs[circ_no].wait_time = 0;

				/*--------------------------------*/
				/* DD (Deadlock Detection) Code   */
				/*--------------------------------*/

				/* Remove all ReqVCs and ... */
				error = RemoveAllReqVCsFromCircuit(circ_no);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_disha_torus'\n", circ_no);
					PrintAdjacencyList();
				}			

				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);	

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_disha_torus'\n", circ_no, vl);
					PrintAdjacencyList();
				}

				/*--------------------------------*/

			      return vl;
			    }

			  /* else (if the vlink IS busy), then put it on the ReqVC list */
			  else
			    {
				/*--------------------------------*/
				/* DD (Deadlock Detection) Code   */
				/*--------------------------------*/
	
				/* Add ReqVC to the list */
				error = AddReqVCToCircuit(circ_no, vl);
	
				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_disha_torus'\n", circ_no, vl);
					PrintAdjacencyList();
				}

				/*--------------------------------*/
			    }


			}
		    } 
		} 
	    }
	}

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* if the message is in a deadlock, the break the deadlock */
	/* by setting the message's destination to the current node */
	if (IsMessageInADeadlock(circ_no, &DeadlockIndex))
	{

		/* Remove all ReqVCs and ... */
		error = RemoveAllReqVCsFromCircuit(circ_no);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_disha_torus'\n", circ_no);
			PrintAdjacencyList();
		}

		/* Deactivate the Deadlock ... since we broke it */
		error = RetireDeadlock(DeadlockIndex);

		if (error)
		{
			printf("ERROR: from 'RetireDeadlock' in 'dd_dp'\n");
		}

		circs[circ_no].dest = nd;
		return -nd-2; 
	}

	/*--------------------------------*/

  return -1;


}


/*------------------------------------------------------------------------------------------*/
/* disha_torus_synchonous_token() - Anjan's protocol (Sugath's Extension for synch. token)  */
/*------------------------------------------------------------------------------------------*/
 
disha_torus_synchonous_token(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;
  int token_in_range, token_range_begin, token_node, idx;

  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;

  /* if this is routed using the Deadlock Buffer (i.e. this circuit has the token */
  /* then mark the node so that we can highlight it during display                */ 
  if (circs[circ_no].token == 1)
  {
	nodes[nd].disha_deadlock_buffer_used = HIGHLIGHT_AS_USING_DBUFFER;
  }

  /* if this node is the destination */
  if (nd == circs[circ_no].dest) 
    {
      /* if this circuit has the token */
      if (circs[circ_no].token == 1) 
	{
          /* take away the token from this circuit */
	  circs[circ_no].token = 0;

          /* give the token to the current node */ 
          nodes[nd].disha_token = 1;

          /* mark the current node as the destination ... for display */ 
	  nodes[nd].disha_deadlock_buffer_used = HIGHLIGHT_AS_DESTINATION;

	  /* resume normal speed of simulation - if we are displaying simulation */
	  if (XDISP)
		SLOW = SAVED_SLOW;

	}
      /* return to indicate that we have arrived at destination */	
      return -nd-2; /* We are there. */
    }

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  /* if this circuit does not have the token */
  if (!circs[circ_no].token)
    {

      /* Try to route adaptively */
      for (v = 1; v < (use_virts+1); v++)
	{ 
	  j = dimensions;
	  
	  while (j >= 0)                 
	    {
	      j--;
	      
	      while (s[j] == d[j] && j >=0)
		{
		  j--;
		}
	      
	      if (j >= 0) 
		{
		  for (k=0; k<dimensions; k++) n[k] = s[k];
		  
		  gamma = d[j]-s[j];
		  
		  if (width[j]-abs(gamma) < abs(gamma) )
		    sign = -sgn(gamma);
		  else
		    sign = sgn(gamma);    /* Decide which way to go */
		  
		  n[j] = (s[j] + width[j] + sign)%width[j];
		  
		  vl = link_addr(nd,j,sign,v,0);
		  
		  if ( vlinks[vl].busy == 0) 
		    {
		      reserve_vlink(vl,i);

		      /* reset wait_time only if we are doing static time out */ 
		      if (!CUMWAIT)
		        circs[circ_no].wait_time = 0;
	      
		      return vl;
		    }
   		}
	    } 
	}


      /* Try to route misroute - if possible */
      /* If this is permitted to misroute    */
      if (!circs[circ_no].misc2)  
	{
          /* If number of allowed misroutes has not been exceeded */
	  if (circs[circ_no].cnt < MISROUTES) 
	    {
	      for (v = 1; v < (use_virts+1); v++)
		{ 
		  for (j = 0; j < dimensions; j++) 
		    {
		      for (k = 0; k < dimensions; k++) 
			{
			  n[k] = s[k];
			}
		      
		      for (sign = -1; sign <= 1; sign+= 2) 
			{
			  n[j] = (s[j] + width[j] + sign)%width[j];
			  
			  vl = link_addr(nd,j,sign,v,0);
		      
			  if ( vlinks[vl].busy == 0) 
			    {
			      reserve_vlink(vl,i);
			      circs[circ_no].cnt++;

		              /* reset wait_time only if we are doing static time out */ 
		              if (!CUMWAIT)
			        circs[circ_no].wait_time = 0;

			      return vl;
			    }
			}
		    } 
		} 
	    }
	}

      /* See if this circuit has timed out ... */

      /*If it has not reached timeout */
      if (circs[circ_no].wait_time <= WAITTIME)
	{
	  circs[circ_no].wait_time ++;
	  return -1;
	}

      /* else, (If it has reached timeout) */
      else
	{

	  /* Check to see if the token is close enough (within range) to grab */
	  token_in_range = 0;

	  if (HALFTOKENSPEED > nd)
		token_range_begin = NODES - (HALFTOKENSPEED - nd) + 1;
	  else
		token_range_begin = nd - HALFTOKENSPEED;
	
	  token_node = token_range_begin;

	  for (idx = 0; idx < TOKENSPEED; idx++)
	  {
		if (nodes[token_node].disha_token == 1)
		{
	  		token_in_range = 1;
			break;
		}

		token_node = (token_node+1) % NODES;
	  }


          /* if the current node is not within reach of the synchronous token  ... wait some more !*/ 
	  if (!token_in_range) 
	    {

	      seize_wait_time++;

	      circs[circ_no].wait_time ++;
	      return -1;
	    }


          /* else (if the current node is within range of the synchronous token (can grab it)... */ 
          /* then route using the deadlock buffer = vlane '0'      ... */ 
	  else
	    {
	      v = 0;
	      j = 0; 
	      while (s[j] == d[j] && j < dimensions) 
		  j++;
	      
	      for (k=0; k<dimensions; k++)
                  n[k] = s[k];
	      
	      gamma = d[j]-s[j];
	      
	      if (width[j]-abs(gamma) < abs(gamma) )
		sign = -sgn(gamma);
	      else
		sign = sgn(gamma);             /* Decide which way to go */
	      
	      n[j] = (s[j] + width[j] + sign)%width[j];
	      
	      vl = link_addr(nd, j, sign, v, 0);
	      
              /* if no one else is using the deadlock buffer */
	      if (vlinks[vl].busy == 0) 
		{ 
		  circs[circ_no].token = 1;
		  seize_count ++;
   
		  /* slow down simulation - if we are displaying */
		  if (XDISP)
		  {
			SAVED_SLOW = SLOW;
                  	SLOW = 17;
		  }

                  /* take away the token */
                  nodes[token_node].disha_token = 0;
                  nodes[token_node].disha_token_siezed = 1;

		  reserve_vlink(vl,i);
		  circs[circ_no].misc2 = 1;

                  vlinks[vl].spec_col = 1;

		  return vl;
		}

              /* else (if someone else is using the deadlock buffer) */
	      else
		{
	          seize_wait_time++;

		  return -1;
		} 
	    }
	}
    }

  /* else, (if this circuit has the token) */
  else
    {
      v = 0;
      j = 0;
      while (s[j] == d[j] && j < dimensions)
	  j++;
      
      for (k=0; k<dimensions; k++) n[k] = s[k];
      
      gamma = d[j]-s[j];
      
      if (width[j]-abs(gamma) < abs(gamma) )
	sign = -sgn(gamma);
      else
	sign = sgn(gamma);             /* Decide which way to go */
      
      n[j] = (s[j] + width[j] + sign)%width[j];
      
      vl = link_addr(nd, j, sign, v, 0);
      
      if (vlinks[vl].busy == 0)
	{ 

	  reserve_vlink(vl,i);
	  circs[circ_no].misc2 = 1; /* No more misrouting */

          vlinks[vl].spec_col = 1;

	  return vl;
	}
      else
	{
	  return -1;
	}
    }
}



/*----------------------------------------------------------------------------------------------*/
/* disha_dor_torus_synchonous_token() - Anjan's protocol (Sugath's Extension for synch. token)  */
/*----------------------------------------------------------------------------------------------*/
 
disha_dor_torus_synchonous_token(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;
  int token_in_range, token_range_begin, token_node, idx;

  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;

  /* if this is routed using the Deadlock Buffer (i.e. this circuit has the token */
  /* then mark the node so that we can highlight it during display                */ 
  if (circs[circ_no].token == 1)
  {
	nodes[nd].disha_deadlock_buffer_used = HIGHLIGHT_AS_USING_DBUFFER;
  }

  /* if this node is the destination */
  if (nd == circs[circ_no].dest) 
    {
      /* if this circuit has the token */
      if (circs[circ_no].token == 1) 
	{
          /* take away the token from this circuit */
	  circs[circ_no].token = 0;

          /* give the token to the current node */ 
          nodes[nd].disha_token = 1;

          /* mark the current node as the destination ... for display */ 
	  nodes[nd].disha_deadlock_buffer_used = HIGHLIGHT_AS_DESTINATION;

	  /* resume normal speed of simulation - if we are displaying */
	  if (XDISP)
          	SLOW = SAVED_SLOW;

	}
      /* return to indicate that we have arrived at destination */	
      return -nd-2; /* We are there. */
    }

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  /* if this circuit does not have the token */
  if (!circs[circ_no].token)
    {

      	/* Try to route deterministically */

	j = 0;
	while (s[j] == d[j] && j<dimensions) 
	{
		j++;
	}

	for (k=0; k<dimensions; k++)
		n[k] = s[k];

	gamma=d[j]-s[j];

	if (width[j]-abs(gamma) < abs(gamma) ) 
		sign = -sgn(gamma);
	else
		sign = sgn(gamma);	
    
	n[j] = (s[j] + width[j] + sign)%width[j]; 

      	for (v = 1; v < (use_virts+1); v++)
	{ 		  
		  vl = link_addr(nd, j, sign, v, 0);
		  
		  if ( vlinks[vl].busy == 0) 
		    {
		      reserve_vlink(vl,i);

		      /* reset wait_time only if we are doing static time out */ 
		      if (!CUMWAIT)
		        circs[circ_no].wait_time = 0;
	      
		      return vl;
		}
	}

      /* See if this circuit has timed out ... */

      /*If it has not reached timeout */
      if (circs[circ_no].wait_time <= WAITTIME)
	{
	  circs[circ_no].wait_time ++;
	  return -1;
	}

      /* else, (If it has reached timeout) */
      else
	{

	  /* Check to see if the token is close enough (within range) to grab */
	  token_in_range = 0;

	  if (HALFTOKENSPEED > nd)
		token_range_begin = NODES - (HALFTOKENSPEED - nd) + 1;
	  else
		token_range_begin = nd - HALFTOKENSPEED;
	
	  token_node = token_range_begin;

	  for (idx = 0; idx < TOKENSPEED; idx++)
	  {
		if (nodes[token_node].disha_token == 1)
		{
	  		token_in_range = 1;
			break;
		}

		token_node = (token_node+1) % NODES;
	  }


          /* if the current node is not within reach of the synchronous token  ... wait some more !*/ 
	  if (!token_in_range) 
	    {
	      seize_wait_time++;
		
/* 
printf("seize_wait_time= %d\n", seize_wait_time);
 */

	      circs[circ_no].wait_time ++;
	      return -1;
	    }


          /* else (if the current node is within range of the synchronous token (can grab it)... */ 
          /* then route using the deadlock buffer = vlane '0'      ... */ 
	  else
	    {
	      v = 0;
	      j = 0; 
	      while (s[j] == d[j] && j < dimensions) 
		  j++;
	      
	      for (k=0; k<dimensions; k++)
                  n[k] = s[k];
	      
	      gamma = d[j]-s[j];
	      
	      if (width[j]-abs(gamma) < abs(gamma) )
		sign = -sgn(gamma);
	      else
		sign = sgn(gamma);             /* Decide which way to go */
	      
	      n[j] = (s[j] + width[j] + sign)%width[j];
	      
	      vl = link_addr(nd, j, sign, v, 0);
	      
              /* if no one else is using the deadlock buffer */
	      if (vlinks[vl].busy == 0) 
		{ 
		  circs[circ_no].token = 1;
		  seize_count ++;
   
		  /* slow down simulation - if we are displaying */
		  if (XDISP)
		  {
			SAVED_SLOW = SLOW;
                  	SLOW = 17;
		  }

                  /* take away the token */
                  nodes[token_node].disha_token = 0;
                  nodes[token_node].disha_token_siezed = 1;

		  reserve_vlink(vl,i);
		  circs[circ_no].misc2 = 1;

                  vlinks[vl].spec_col = 1;

		  return vl;
		}

              /* else (if someone else is using the deadlock buffer) */
	      else
		{
	      	  seize_wait_time++;

/*printf("seize_wait_time= %d\n", seize_wait_time);
 */

		  return -1;
		} 
	    }
	}
    }

  /* else, (if this circuit has the token) */
  else
    {
      v = 0;
      j = 0;
      while (s[j] == d[j] && j < dimensions)
	  j++;
      
      for (k=0; k<dimensions; k++) n[k] = s[k];
      
      gamma = d[j]-s[j];
      
      if (width[j]-abs(gamma) < abs(gamma) )
	sign = -sgn(gamma);
      else
	sign = sgn(gamma);             /* Decide which way to go */
      
      n[j] = (s[j] + width[j] + sign)%width[j];
      
      vl = link_addr(nd, j, sign, v, 0);
      
      if (vlinks[vl].busy == 0)
	{ 

	  reserve_vlink(vl,i);
	  circs[circ_no].misc2 = 1; /* No more misrouting */

          vlinks[vl].spec_col = 1;

	  return vl;
	}
      else
	{
	  return -1;
	}
    }
}



/*--------------------------------------------------------------------*/
/* dd_min_adaptive_bidir_torus()   Adaptive w/ DEADLOCK DETECTION     */
/*--------------------------------------------------------------------*/
dd_min_adaptive_bidir_torus(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;
  int error;
  int DeadlockIndex;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; 
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = 0;
  while (s[j] == d[j] && j<dimensions) 
    {
      j++;
    }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];

  for (; j <dimensions; j++)
  {

    gamma=d[j]-s[j];
    if (width[j]-abs(gamma) < abs(gamma) ) 
      sign = -sgn(gamma);
    else
      sign = sgn(gamma);	
    
    n[j] = (s[j] + width[j] + sign)%width[j]; 

    for (i=0; i < virts; i++) 
    {	
      vl = link_addr(nd, j, sign, i, 0); 
      
      if (vlinks[vl].busy == 0) 
      {
        reserve_vlink(vl,UnPr(tkn));

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

        return vl;
      }
      else
      {
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

      }
    }
  }

  /*--------------------------------*/
  /* DD (Deadlock Detection) Code   */
  /*--------------------------------*/

  /* if the message is in a deadlock, the break the deadlock */
  /* by setting the message's destination to the current node */
  if (IsMessageInADeadlock(circ_no, &DeadlockIndex))
  {
    /* Remove all ReqVCs and ... */
    error = RemoveAllReqVCsFromCircuit(circ_no);

    /* if there's an error, report it */
    if (error)
    {
      printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
      PrintAdjacencyList();
    }

    /* Deactivate the Deadlock ... since we broke it */
    error = RetireDeadlock(DeadlockIndex);

    if (error)
    {
      printf("ERROR: from 'RetireDeadlock' in 'dd_dp'\n");
    }

    circs[circ_no].dest = nd;
    return -nd-2; 
  }

  /*--------------------------------*/

  return -1;
}



/*--------------------------------------------------------------------*/
/* dd_min_adaptive_unidir_torus()   Adaptive w/ DEADLOCK DETECTION    */
/*--------------------------------------------------------------------*/
dd_min_adaptive_unidir_torus(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;
  int error;
  int DeadlockIndex;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; 
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = 0;
  while (s[j] == d[j] && j<dimensions) 
    {
      j++;
    }

  /* fix the sign since we are using only one direction */
  sign = 1;

  for (k=0; k<dimensions; k++)
    n[k] = s[k];

  for (; j <dimensions; j++)
  {
    for (i=0; i < virts; i++) 
    {
      vl = link_addr(nd, j, sign, i, 0); 
      
      if (vlinks[vl].busy == 0) 
      {
        reserve_vlink(vl,UnPr(tkn));

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

        return vl;
      }
      else
      {
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

      }
    }
  }

  /*--------------------------------*/
  /* DD (Deadlock Detection) Code   */
  /*--------------------------------*/

  /* if the message is in a deadlock, the break the deadlock */
  /* by setting the message's destination to the current node */
  if (IsMessageInADeadlock(circ_no, &DeadlockIndex))
  {
    /* Remove all ReqVCs and ... */
    error = RemoveAllReqVCsFromCircuit(circ_no);

    /* if there's an error, report it */
    if (error)
    {
      printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
      PrintAdjacencyList();
    }

    /* Deactivate the Deadlock ... since we broke it */
    error = RetireDeadlock(DeadlockIndex);

    if (error)
    {
      printf("ERROR: from 'RetireDeadlock' in 'dd_dp'\n");
    }

    circs[circ_no].dest = nd;
    return -nd-2; 
  }

  /*--------------------------------*/

  return -1;
}


/*--------------------------------------------------------------------*/
/* dd_non_min_adaptive_bidir_torus()   Adaptive w/ DEADLOCK DETECTION */
/*--------------------------------------------------------------------*/
dd_non_min_adaptive_bidir_torus(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;
  int error;
  int DeadlockIndex;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; 
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = 0;
  while (s[j] == d[j] && j<dimensions) 
    {
      j++;
    }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];

  for (; j <dimensions; j++)
  {

    gamma=d[j]-s[j];
    if (width[j]-abs(gamma) < abs(gamma) ) 
      sign = -sgn(gamma);
    else
      sign = sgn(gamma);	
    
    n[j] = (s[j] + width[j] + sign)%width[j]; 

    for (i=0; i < virts; i++) 
    {	
      vl = link_addr(nd, j, sign, i, 0); 
      
      if (vlinks[vl].busy == 0) 
      {
        reserve_vlink(vl,UnPr(tkn));

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

        return vl;
      }
      else
      {
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

      }
    }
  }

  /* Try to misroute extension */
  if (!circs[circ_no].misc2)  /* Am I permitted to misroute ? */
  {
    if (circs[circ_no].cnt < MISROUTES) 
    {
      for (v = 1; v < virts; v++)
      { 
        for (j = 0; j < dimensions; j++) 
        {
          for (k = 0; k < dimensions; k++) 
            n[k] = s[k];
        }
      
        for (sign = -1; sign <= 1; sign+= 2) 
        {
          n[j] = (s[j] + width[j] + sign)%width[j];
	  
          vl = link_addr(nd,j,sign,v,0);
      
          if ( vlinks[vl].busy == 0) 
          {
            reserve_vlink(vl,i);
            circs[circ_no].cnt++;

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

            return vl;
          }
          else
          {

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

          }
        }
      } 
    } 
  }

  /*--------------------------------*/
  /* DD (Deadlock Detection) Code   */
  /*--------------------------------*/

  /* if the message is in a deadlock, the break the deadlock */
  /* by setting the message's destination to the current node */
  if (IsMessageInADeadlock(circ_no, &DeadlockIndex))
  {
    /* Remove all ReqVCs and ... */
    error = RemoveAllReqVCsFromCircuit(circ_no);

    /* if there's an error, report it */
    if (error)
    {
      printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
      PrintAdjacencyList();
    }

    /* Deactivate the Deadlock ... since we broke it */
    error = RetireDeadlock(DeadlockIndex);

    if (error)
    {
      printf("ERROR: from 'RetireDeadlock' in 'dd_dp'\n");
    }

    circs[circ_no].dest = nd;
    return -nd-2; 
  }

  /*--------------------------------*/

  return -1;
}


/*--------------------------------------------------------------------*/
/* dd_non_min_adaptive_unidir_torus()   Adaptive w/ DEADLOCK DETECTION */
/*--------------------------------------------------------------------*/
dd_non_min_adaptive_unidir_torus(nd,tkn)
     int nd,tkn;
{
  int i, vl;
  int circ_no;
  int error;
  int DeadlockIndex;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; 
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = 0;
  while (s[j] == d[j] && j<dimensions) 
    {
      j++;
    }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];

  for (; j <dimensions; j++)
  {

    /* fix the sign since we are using only one direction */
    sign = 1;
    
    n[j] = (s[j] + width[j] + sign)%width[j]; 

    for (i=0; i < virts; i++) 
    {	
      vl = link_addr(nd, j, sign, i, 0); 
      
      if (vlinks[vl].busy == 0) 
      {
        reserve_vlink(vl,UnPr(tkn));

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

        return vl;
      }
      else
      {
	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

      }
    }
  }

  /* Try to misroute extension */
  if (!circs[circ_no].misc2)  /* Am I permitted to misroute ? */
  {
    if (circs[circ_no].cnt < MISROUTES) 
    {
      for (v = 1; v < virts; v++)
      { 
        for (j = 0; j < dimensions; j++) 
        {
          for (k = 0; k < dimensions; k++) 
            n[k] = s[k];
        }

/* NEED TO CHECK THIS --- HOW CAN WE RESTRICT MISROUTING TO ONLY ONE DIRECTION ? */

        for (sign = -1; sign <= 1; sign+= 2) 
        {
          n[j] = (s[j] + width[j] + sign)%width[j];
	  
          vl = link_addr(nd,j,sign,v,0);
      
          if ( vlinks[vl].busy == 0) 
          {
            reserve_vlink(vl,i);
            circs[circ_no].cnt++;

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Remove all ReqVCs and ... */
	error = RemoveAllReqVCsFromCircuit(circ_no);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
		PrintAdjacencyList();
	}

	/* ... Add the VC to the Circuit */
	error = AddVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

            return vl;
          }
          else
          {

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/

	/* Add ReqVC to the list */
	error = AddReqVCToCircuit(circ_no, vl);

	/* if there's an error, report it */
	if (error)
	{
		printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_dp'\n", circ_no, vl);
		PrintAdjacencyList();
	}

	/*--------------------------------*/

          }
        }
      } 
    } 
  }

  /*--------------------------------*/
  /* DD (Deadlock Detection) Code   */
  /*--------------------------------*/

  /* if the message is in a deadlock, the break the deadlock */
  /* by setting the message's destination to the current node */
  if (IsMessageInADeadlock(circ_no, &DeadlockIndex))
  {
    /* Remove all ReqVCs and ... */
    error = RemoveAllReqVCsFromCircuit(circ_no);

    /* if there's an error, report it */
    if (error)
    {
      printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_dp'\n", circ_no);
      PrintAdjacencyList();
    }

    /* Deactivate the Deadlock ... since we broke it */
    error = RetireDeadlock(DeadlockIndex);

    if (error)
    {
      printf("ERROR: from 'RetireDeadlock' in 'dd_dp'\n");
    }

    circs[circ_no].dest = nd;
    return -nd-2; 
  }

  /*--------------------------------*/

  return -1;
}


/*------------------------------------------------------------------------------------------*/
/* dd_test_adaptive() - Basic test routing algorithm for Deadlock Detection                */
/*------------------------------------------------------------------------------------------*/
 
dd_test_adaptive(nd,tkn)
     int nd,tkn;
{
	int i, j, k, sign, vl, v, gamma;
	int circ_no;
 	int error;

	i = UnPr(tkn);
	backtrack=0;
	circ_no = msgs[i].circ_index;

	/* if this node is the destination */
	if (nd == circs[circ_no].dest) 
	{
		/* return to indicate that we have arrived at destination */	
		return -nd-2; /* We are there. */
	}

	get_coords(nd,s);
	get_coords(circs[circ_no].dest,d);

	/* Try to route adaptively */
	for (v = 0; v < virts; v++)
	{ 
		j = dimensions;
	  
		while (j >= 0)                 
		{
			j--;
	      
			while (s[j] == d[j] && j >=0)
			{
				j--;
			}
	      
			if (j >= 0) 
			{
				for (k=0; k<dimensions; k++) n[k] = s[k];
		  
				gamma = d[j]-s[j];
		  
				if (width[j]-abs(gamma) < abs(gamma) )
					sign = -sgn(gamma);
				else
					sign = sgn(gamma);    /* Decide which way to go */
		  
				n[j] = (s[j] + width[j] + sign)%width[j];
		  
				vl = link_addr(nd,j,sign,v,0);
		  
				/* if the vlink is not busy, then use it */
				if ( vlinks[vl].busy == 0) 
				{
					reserve_vlink(vl,i);

					/*--------------------------------*/
					/* DD (Deadlock Detection) Code   */
					/*--------------------------------*/

					/* Remove all ReqVCs and ... */
					error = RemoveAllReqVCsFromCircuit(circ_no);

					/* if there's an error, report it */
					if (error)
					{
						printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
						PrintAdjacencyList();
					}

					/* ... Add the VC to the Circuit */
					error = AddVCToCircuit(circ_no, vl);

					/* if there's an error, report it */
					if (error)
					{
						printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
						PrintAdjacencyList();
					}

					/*--------------------------------*/

					return vl;
				}

				/* else (if the vlink IS busy), then put it on the ReqVC list */
				else
				{
					/*--------------------------------*/
					/* DD (Deadlock Detection) Code   */
					/*--------------------------------*/

					/* Add ReqVC to the list */
					error = AddReqVCToCircuit(circ_no, vl);

					/* if there's an error, report it */
					if (error)
					{
						printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
						PrintAdjacencyList();
					}

					/*--------------------------------*/

				}

			}
		} 
	}

	/* if we get to here, it means that no vlink is available - all are busy */
	return -1;
}


/*--------------------------------------------------------------------*/
/* disha() - Anjan's protocol (w/ Sugath's Extensions)                */
/*--------------------------------------------------------------------*/

disha_torus2(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;
  int highlite_dur;

  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;

  if (circs[circ_no].token == 1)
  {
	nodes[nd].disha_deadlock_buffer_used = 1;
  }

  /* if it's the destination ... return success */
  if (nd == circs[circ_no].dest)
  {
    if (circs[circ_no].token == 1)
    {
	circs[circ_no].token == 0;
        nodes[nd].disha_token = 1;
    }

      return -nd-2; /* We are there. */
  }

  /* otherwise, (if it is not the destination), do the following ... */

  /* Set s = the coordinates of the current node */
  get_coords(nd,s);
  /* Set d = the coordinates of the destination node */
  get_coords(circs[circ_no].dest,d);

  /* Try to adaptively Route - if possible */
  for (v = 1; v < virts - 3; v++)
  { 
    j = dimensions;

    while (j >= 0)
    {
      j--;
	      
      while (s[j] == d[j] && j >=0)
        j--;
	      
      if (j >= 0) 
      {
        for (k=0; k<dimensions; k++) n[k] = s[k];
		  
        gamma = d[j]-s[j];
		  
        if (width[j]-abs(gamma) < abs(gamma) )
           sign = -sgn(gamma);
        else
           sign = sgn(gamma);    /* Decide which way to go */
		  
        n[j] = (s[j] + width[j] + sign)%width[j];
		  
        vl = link_addr(nd,j,sign,v,0);
		  
        if ( vlinks[vl].busy == 0) 
        {
           reserve_vlink(vl,i);
           circs[circ_no].wait_time = 0;		      
           return vl;
        }
      }
    } 
  }

  /* Try to misroute extension */
  if (!circs[circ_no].misc2)  /* Am I permitted to misroute ? */
  {
    if (circs[circ_no].cnt < MISROUTES) 
    {
      for (v = 1; v < virts - 3; v++)
      { 
        for (j = 0; j < dimensions; j++) 
        {
          for (k = 0; k < dimensions; k++) 
            n[k] = s[k];
        }
      
        for (sign = -1; sign <= 1; sign+= 2) 
        {
          n[j] = (s[j] + width[j] + sign)%width[j];
	  
          vl = link_addr(nd,j,sign,v,0);
      
          if ( vlinks[vl].busy == 0) 
          {
            reserve_vlink(vl,i);
            circs[circ_no].cnt++;
            circs[circ_no].wait_time = 0;
            return vl;
          }
        }
      } 
    } 
  }

  /* has this circuit waited for the required period */
  if (circs[circ_no].wait_time <= WAITTIME)
  {
    /*If not ... simply increment the wait count for the circuit */
    circs[circ_no].wait_time ++;
    return -1;
  }

  /* otherwise ... we must have a deadlock ... atleast in our approximation */
  else
  {
    /* do we still increment the counter ... what if we move this on the DBuffer ? */
    circs[circ_no].wait_time ++;

    /* find the VL to send it on ... using the Adaptive Function Again !!! */
    v = 0;
    j = 0;
    while (s[j] == d[j] && j < dimensions) 
      j++;

    for (k=0; k<dimensions; k++) 
      n[k] = s[k];
	      
    gamma = d[j]-s[j];
	      
    if (width[j]-abs(gamma) < abs(gamma) )
      sign = -sgn(gamma);
    else
      sign = sgn(gamma);             /* Decide which way to go */
	      
    n[j] = (s[j] + width[j] + sign)%width[j];
	      
    vl = link_addr(nd, j, sign, v, 0);

    /* If our adaptive algorithm is deterministic ... this should be busy */

    if (vlinks[vl].busy == 0 && nodes[nd].disha_token) 
    { 
      seize_count ++;

      circs[circ_no].token == 1;

      nodes[nd].disha_token = 0;
      nodes[nd].disha_token_siezed = 1;

      reserve_vlink(vl,i);
      circs[circ_no].misc2 = 1;
      return vl;
    }

    else
    {
      seize_wait_time++;

      return -1;
    }
  }

}



/* ------------------------------------------------------------------------- */
disha_mesh(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;

  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;
 
  if (nd == circs[circ_no].dest) 
    {
      if (circs[circ_no].token == 1) 
	{
	  TOKEN = 0;
	}
      return -nd-2; /* We are there. */
    }

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  if (circs[circ_no].token != 1)
    {
      for (v = 1; v < virts; v++)
	{ 
	  j = dimensions;
	  
	  while (j >= 0)                 /* Adaptive subfunction */
	    {
	      j--;
	      
	      while (s[j] == d[j] && j >=0)
		{
		  j--;
		}
	      
	      if (j >= 0) 
		{
		  for (k=0; k<dimensions; k++) n[k] = s[k];
		  
		  gamma = d[j]-s[j];
		  sign = sgn(gamma);    /* Decide which way to go */
		  
		  n[j] = (s[j] + width[j] + sign)%width[j];
		  
		  vl = link_addr(nd,j,sign,v,0);
		  
		  if ( vlinks[vl].busy == 0) 
		    {
		      reserve_vlink(vl,i);
		      return vl;
		    }
		}
	    } 
	}

  /* Misroute extension */

      if (!circs[circ_no].misc2) 
	{
	  if (circs[circ_no].cnt < MISROUTES) 
	    {
	      for (v = 1; v < virts; v++)
		{ 
		  for (j = 0; j < dimensions; j++) 
		    {
		      for (k = 0; k < dimensions; k++) 
			{
			  n[k] = s[k];
			}
		      
		      for (sign = -1; sign <= 1; sign+= 2) 
			{
			  n[j] = (s[j] + width[j] + sign)%width[j];
			  
			  vl = link_addr(nd,j,sign,v,0);
		      
			  if ( vlinks[vl].busy == 0) 
			    {
			      reserve_vlink(vl,i);
			      circs[circ_no].cnt++;
			      return vl;
			    }
			}
		    } 
		} 
	    }
	}

      if (circs[circ_no].wait_time <= WAITTIME)
	{
	  circs[circ_no].wait_time ++;
	  return -1;
	}
      else
	{
	  if (TOKEN == 1) 
	    {
	      seize_wait_time++;

	      circs[circ_no].wait_time ++;
	      return -1;
	    }
	  else
	    {
	      TOKEN = 1;
              seize_count ++;

	      v = 0;
	      circs[circ_no].token = 1;
	      
	      j = 0; 
	      
	      while (s[j] == d[j] && j < dimensions) 
		{
		  j++;
		}
	      
	      for (k=0; k<dimensions; k++) n[k] = s[k];
	      
	      gamma = d[j]-s[j];
	      sign = sgn(gamma);             /* Decide which way to go */
	      
	      n[j] = (s[j] + width[j] + sign)%width[j];
	      
	      vl = link_addr(nd, j, sign, v, 0);
	      
	      if (vlinks[vl].busy == 0) 
		{ 

		  reserve_vlink(vl,i);
		  circs[circ_no].misc2 = 1;
		  return vl;
		}
	      else
		{
		  seize_wait_time++;

		  return -1;
		} 

	    }
	}
    }

  else
    {
      v = 0;
      j = 0;
      
      while (s[j] == d[j] && j < dimensions)
	{
	  j++;
	}
      
      for (k=0; k<dimensions; k++) n[k] = s[k];
      
      gamma = d[j]-s[j];
      sign = sgn(gamma);             /* Decide which way to go */
      
      n[j] = (s[j] + width[j] + sign)%width[j];
      
      vl = link_addr(nd, j, sign, v, 0);
      
      if (vlinks[vl].busy == 0)
	{ 

	  reserve_vlink(vl,i);
	  circs[circ_no].misc2 = 1;
	  return vl;
	}
      else
	{
	  return -1;
	}
    }
}

/* ------------------------------------------------------------------------- */


disha_mesh_onedb(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;
  int next_node;
  int vl1, vl2, vl3, vl4;
  int error;

  i = UnPr(tkn);
  backtrack=0;
  circ_no = msgs[i].circ_index;
 
  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */
 
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);
 

  for (v = 1; v < virts; v++)
  {
    j = dimensions;
      
    while (j >= 0)                 /* Adaptive subfunction */
    {
      j--;
          
      while (s[j] == d[j] && j >=0)
      {
        j--;
      }
          
      if (j >= 0)
      {
        for (k=0; k<dimensions; k++) n[k] = s[k];
              
        gamma = d[j]-s[j];
        sign = sgn(gamma);    /* Decide which way to go */
              
        n[j] = (s[j] + width[j] + sign)%width[j];
              
        vl = link_addr(nd,j,sign,v,0);
              
        if ( vlinks[vl].busy == 0)
        {
          reserve_vlink(vl,i);

	/*--------------------------------*/
	/* DD (Deadlock Detection) Code   */
	/*--------------------------------*/
	if (CD)
	{
		/* Remove all ReqVCs and ... */
		error = RemoveAllReqVCsFromCircuit(circ_no);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
			PrintAdjacencyList();
		}

		/* ... Add the VC to the Circuit */
		error = AddVCToCircuit(circ_no, vl);

		/* if there's an error, report it */
		if (error)
		{
			printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
			PrintAdjacencyList();
		}
	}
	/*--------------------------------*/

          return vl;
        }
      }
    }
  }

  if (s[1] < d[1])
  {
    if (s[1]%2 == 0)
    {
      if (d[0] > s[0])
      {
        n[1] = s[1];
        n[0] = s[0] + 1;
        next_node = get_addr(n);

/*     
              vl1 = link_addr(next_node,0,-1,0,0);
              vl2 = link_addr(next_node,0,1,0,0);
              vl3 = link_addr(next_node,1,-1,0,0);
              vl4 = link_addr(next_node,1,1,0,0);
              
              if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                {
                  n[1] = s[1] + 1;
                  n[0] = s[0];
                  next_node = get_addr(n);
                  
                  vl1 = link_addr(next_node,0,-1,0,0);
                  vl2 = link_addr(next_node,0,1,0,0);
                  vl3 = link_addr(next_node,1,-1,0,0);
                  vl4 = link_addr(next_node,1,1,0,0);
                  
                  if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                    {     
                      return -1;
                    }

                  else
                    {
                      v = 0;
                      vl = link_addr(nd,1,1,v,0);
                      if ( vlinks[vl].busy == 0)
                      {
                          reserve_vlink(vl,i);
                          return vl;
                      }
                      return -1;
                    }
            }
          
          if (d[0] <= s[0]) 
            {
              n[1] = s[1] + 1;
              n[0] = s[0];
              next_node = get_addr(n);
      
              vl1 = link_addr(next_node,0,-1,0,0);
              vl2 = link_addr(next_node,0,1,0,0);
              vl3 = link_addr(next_node,1,-1,0,0);
              vl4 = link_addr(next_node,1,1,0,0);
              
              if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                {
                  return -1;
                }
              
              else 
*/

                {
                  v = 0;
                  vl = link_addr(nd,1,1,v,0);
                  if ( vlinks[vl].busy == 0)
                    {
                      reserve_vlink(vl,i);

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* Remove all ReqVCs and ... */
				error = RemoveAllReqVCsFromCircuit(circ_no);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
					PrintAdjacencyList();
				}

				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}
			}
			/*--------------------------------*/

                      return vl;
                    }

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* else (if the vlink IS busy), then put it on the ReqVC list */

				/* Add ReqVC to the list */
				error = AddReqVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}

			}
			/*--------------------------------*/

                  return -1;
                }
            }
        }
      
      if (s[1] % 2 == 1) 
        {
          if (d[0] < s[0])
            {
              n[1] = s[1];
              n[0] = s[0] - 1;
              next_node = get_addr(n);
/*      
              vl1 = link_addr(next_node,0,-1,0,0);
              vl2 = link_addr(next_node,0,1,0,0);
              vl3 = link_addr(next_node,1,-1,0,0);
              vl4 = link_addr(next_node,1,1,0,0);
              
              if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                {
                  n[1] = s[1] + 1;
                  n[0] = s[0];
                  next_node = get_addr(n);
                  
                  vl1 = link_addr(next_node,0,-1,0,0);
                  vl2 = link_addr(next_node,0,1,0,0);
                  vl3 = link_addr(next_node,1,-1,0,0);
                  vl4 = link_addr(next_node,1,1,0,0);
                  
                  if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                    {
                      return -1;
                    }

                  else
                    {
                      v = 0;
                      vl = link_addr(nd,1,1,v,0);
                      if ( vlinks[vl].busy == 0)
                        {
                          reserve_vlink(vl,i);
                          return vl;
                        }
                      return -1;
                    }
                }
              
              else 
*/
                {
                  v = 0;
                  vl = link_addr(nd,0,-1,v,0);
                  if ( vlinks[vl].busy == 0)
                    {
                      reserve_vlink(vl,i);

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* Remove all ReqVCs and ... */
				error = RemoveAllReqVCsFromCircuit(circ_no);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
					PrintAdjacencyList();
				}

				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}
			}
			/*--------------------------------*/

                      return vl;
                    }

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* else (if the vlink IS busy), then put it on the ReqVC list */

				/* Add ReqVC to the list */
				error = AddReqVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}

			}
			/*--------------------------------*/

                  return -1;
                }
            }
          
          if (d[0] >= s[0]) 
            {
              n[1] = s[1] + 1;
              n[0] = s[0];
              next_node = get_addr(n);
/*      
              vl1 = link_addr(next_node,0,-1,0,0);
              vl2 = link_addr(next_node,0,1,0,0);
              vl3 = link_addr(next_node,1,-1,0,0);
              vl4 = link_addr(next_node,1,1,0,0);
              
              if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                {
                  return -1;
                }
              
              else 
*/
                {
                  v = 0;
                  vl = link_addr(nd,1,1,v,0);
                  if ( vlinks[vl].busy == 0)
                    {
                      reserve_vlink(vl,i);

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* Remove all ReqVCs and ... */
				error = RemoveAllReqVCsFromCircuit(circ_no);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
					PrintAdjacencyList();
				}

				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}
			}
			/*--------------------------------*/

                      return vl;
                    }

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* else (if the vlink IS busy), then put it on the ReqVC list */

				/* Add ReqVC to the list */
				error = AddReqVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}

			}
			/*--------------------------------*/

                  return -1;
                }
            }
        }
    }

  if (s[1] == d[1])
    {
      if (s[1]%2 == 0)
        {
          if (d[0] > s[0])
            {
              n[1] = s[1];
              n[0] = s[0] + 1;
              next_node = get_addr(n);
/*      
              vl1 = link_addr(next_node,0,-1,0,0);
              vl2 = link_addr(next_node,0,1,0,0);
              vl3 = link_addr(next_node,1,-1,0,0);
              vl4 = link_addr(next_node,1,1,0,0);
              
              if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                {
                  return -1;
                }
              
              else 
*/
                {
                  v = 0;
                  vl = link_addr(nd,0,1,v,0);
                  if ( vlinks[vl].busy == 0)
                    {
                      reserve_vlink(vl,i);

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* Remove all ReqVCs and ... */
				error = RemoveAllReqVCsFromCircuit(circ_no);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
					PrintAdjacencyList();
				}

				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}
			}
			/*--------------------------------*/

                      return vl;
                    }

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* else (if the vlink IS busy), then put it on the ReqVC list */

				/* Add ReqVC to the list */
				error = AddReqVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}

			}
			/*--------------------------------*/

                  return -1;
                }
            }
          
          if (d[0] <= s[0]) 
            {
              if (s[1] != 0)
                {
                  n[1] = s[1] - 1;
                  n[0] = s[0];
                  next_node = get_addr(n);
/*                
                  vl1 = link_addr(next_node,0,-1,0,0);
                  vl2 = link_addr(next_node,0,1,0,0);
                  vl3 = link_addr(next_node,1,-1,0,0);
                  vl4 = link_addr(next_node,1,1,0,0);
                  
                  if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) ||(vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                    {
                      return -1;
                    }
                  
                  else 
*/
                    {
                      v = 0;
                      vl = link_addr(nd,1,-1,v,0);
                      if ( vlinks[vl].busy == 0)
                        {
                          reserve_vlink(vl,i);

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* Remove all ReqVCs and ... */
				error = RemoveAllReqVCsFromCircuit(circ_no);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
					PrintAdjacencyList();
				}

				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}
			}
			/*--------------------------------*/

                          return vl;
                        }

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* else (if the vlink IS busy), then put it on the ReqVC list */

				/* Add ReqVC to the list */
				error = AddReqVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}

			}
			/*--------------------------------*/

                      return -1;
                    }
                }

              if (s[1] == 0) 
                {
                  return -1;
                }
            }
        }
      
      if (s[1]%2 == 1) 
        {
          if (d[0] < s[0])
            {
              n[1] = s[1];
              n[0] = s[0] - 1;
              next_node = get_addr(n);
/*            
              vl1 = link_addr(next_node,0,-1,0,0);
              vl2 = link_addr(next_node,0,1,0,0);
              vl3 = link_addr(next_node,1,-1,0,0);
              vl4 = link_addr(next_node,1,1,0,0);
              
              if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                {
                  return -1;
                }
              
              else 
*/
                {
                  v = 0;
                  vl = link_addr(nd,0,-1,v,0);
                  if ( vlinks[vl].busy == 0)
                    {
                      reserve_vlink(vl,i);

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* Remove all ReqVCs and ... */
				error = RemoveAllReqVCsFromCircuit(circ_no);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
					PrintAdjacencyList();
				}

				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}
			}
			/*--------------------------------*/

                      return vl;
                    }

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* else (if the vlink IS busy), then put it on the ReqVC list */

				/* Add ReqVC to the list */
				error = AddReqVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}

			}
			/*--------------------------------*/

                  return -1;
                }
            }
          
          if (d[0] >= s[0]) 
            {
              n[1] = s[1] - 1;
              n[0] = s[0];
              next_node = get_addr(n);
/*            
              vl1 = link_addr(next_node,0,-1,0,0);
              vl2 = link_addr(next_node,0,1,0,0);
              vl3 = link_addr(next_node,1,-1,0,0);
              vl4 = link_addr(next_node,1,1,0,0);
              
              if ((vlinks[vl1].busy == 1) || (vlinks[vl2].busy == 1) || (vlinks[vl3].busy == 1) || (vlinks[vl4].busy == 1))
                {
                  return -1;
                }
              
              else 
*/
                {
                  v = 0;
                  vl = link_addr(nd,1,-1,v,0);
                  if (vlinks[vl].busy == 0)
                    {
                      reserve_vlink(vl,i);
 
			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* Remove all ReqVCs and ... */
				error = RemoveAllReqVCsFromCircuit(circ_no);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveAllReqVCsFromCircuit' - circ_no: %d in 'dd_basic_adaptive'\n", circ_no);
					PrintAdjacencyList();
				}

				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}
			}
			/*--------------------------------*/

                     return vl;
                    }

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			if (CD)
			{
				/* else (if the vlink IS busy), then put it on the ReqVC list */

				/* Add ReqVC to the list */
				error = AddReqVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddReqVCToCircuit' - circ_no: %d, vl: %d in 'dd_basic_adaptive'\n", circ_no, vl);
					PrintAdjacencyList();
				}

			}
			/*--------------------------------*/

                  return -1;
                } 
            }
        }
    }
 
  if (s[1] > d[1]) 
    {
      return -1;
    }
}


/*--------------------------------------------------------------------*/
/* dpord() Duato - Idle adaptive protocol (cycle through plinks)      */
/*--------------------------------------------------------------------*/
dpord(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  backtrack=0;
  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  j = dimensions;		/* Adaptive subfunction */
  while (j >= 0) {		/* Idle part */
    j--;
    while (s[j] == d[j] && j >=0) {
      j--;
    }
	
    if (j>=0) {
      for (v=2; v<virts; v++) {	
	for (k=0; k<dimensions; k++)
	  n[k] = s[k];
    
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	
	n[j] = (s[j] + width[j] + sign)%width[j]; 
	
	vl = link_addr(nd,j,sign,v, 0);
	if ( vlinks[vl].busy == 0) {
	  reserve_vlink(vl,i);
	  return vl;
	}
      }
    }
    
  }

  /* Misroute extension */
  if (circs[circ_no].cnt < MISROUTES) {
    for (j=0; j<dimensions; j++) {
      for (v=2; v<virts; v++) {	/* Adaptive channels */
	for (k=0; k<dimensions; k++)
	  n[k] = s[k];

	for (sign = -1; sign<=1; sign+=2) {
	  n[j] = (s[j] + width[j] + sign)%width[j]; 

	  vl = link_addr(nd,j,sign,v,0);
	  if ( vlinks[vl].busy == 0) {
	    reserve_vlink(vl,i);
	    circs[circ_no].cnt++;
	    return vl;
	  }
	}
      }
    }
  }


  j = 0;			/* Ecube subfunction */
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];
    
  gamma=d[j]-s[j];
  if (gamma > 0) v=0; else v=1;
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	/* Decide which way to go */
    
  n[j] = (s[j] + width[j] + sign)%width[j]; 

  vl = link_addr(nd, j, sign, v, 0);
  if (vlinks[vl].busy == 0) {
    reserve_vlink(vl,i);
    circs[circ_no].cnt = MISROUTES;
    return vl;
  }
  if (vlinks[vl].busy == (-1)) 
    if (comm_mech == AWR) backtrack=1;
  return -1;
}


/*--------------------------------------------------------------------*/
/* dpmin()   Duato - Idle adaptive protocol (min congestion)          */
/*--------------------------------------------------------------------*/
dpmin(nd,tkn)
     int nd,tkn;
{
  int i,vl;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  backtrack=0;
  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  min_congest(nd, -1, PROF);
  if (min_c < use_virts-2) {
    return vroute(vl_min,i);
  }
  
  if (circs[circ_no].cnt < MISROUTES) {
    min_congest(nd, -1, UNPROF);
    if (min_c < use_virts-2) {
      circs[circ_no].cnt++;
      return vroute(vl_min,i);
    }
  }
  
  vl =det_route(nd,i);
  if (vl != -1)
    circs[circ_no].cnt = MISROUTES;
  return vl;
}


int kcnt[MAXMESSAGE], ck[MAXMESSAGE][2], dmask[MAXMESSAGE];
/*--------------------------------------------------------------------*/
/* a1()   A1 (Shin and Chen)                                          */
/*--------------------------------------------------------------------*/
a1(nd,tkn)
     int nd,tkn;
{
  int i, j, k, sign, vl, v, gamma, cnt, found, l;
  int circ_no;
  
  backtrack = 1;
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */
  if (circs[circ_no].misc == 2) return -1; /* Backtrack to source */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  if (circs[circ_no].misc == 0) {	/* Initialize routing info */
    circs[circ_no].misc = 1;
    circs[circ_no].misc2 = nd;
    kcnt[i]=0;
    cnt=0;
    for (j=0; j<dimensions; j++) {
      if (s[j] != d[j]) {
	ck[i][cnt]=j;
	cnt++;
      }
    }
    kcnt[i]=cnt;
    dmask[i]=0;
  }
  
  for (v=0; v<virts; v++) 
    for (l=0; l<kcnt[i]; l++) {	/* Route profitably */
      j = ck[i][l];
      if (j < dimensions) {
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	
	for (k=0; k<dimensions; k++)
	  n[k] = s[k];
	
	n[j] = (s[j] + width[j] + sign)%width[j]; 
	
	vl = link_addr(nd,j,sign,v, 0);
	if ( vlinks[vl].busy == 0) {
	  reserve_vlink(vl,i);
	  circs[circ_no].misc2 = nd;
	  if (d[j] == n[j]) {
	    kcnt[i]--;
	    for (k=l; k<kcnt[i]; k++)
	      ck[i][k] = ck[i][k+1];
	  }
	  return vl;
	}
      }
    }
  /* Time to misroute */
  for (v=0; v<virts; v++) {
    cnt = 1;
    for (j=dimensions-1; j>=0; j--) {
      if ((cnt & dmask[i]) == 0) {
	for (sign=(-1); sign<2; sign+=2) {
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
	  
	  n[j] = (s[j] + width[j] + sign)%width[j]; 
	  if (circs[circ_no].misc2 != get_addr(n)) {
	    vl = link_addr(nd,j,sign,v, 0);
	    if ( vlinks[vl].busy == 0) {
	      reserve_vlink(vl,i);
	      circs[circ_no].misc2 = nd;
	      dmask[i] |= cnt;
	      found = 0;
	      for (k=0; k<kcnt[i]; k++)
		if (ck[i][k] == j) found = 1; 
	      if (!found) ck[i][kcnt[i]] = j;
	      kcnt[i]++;
	      return vl;
	    }
	  }
	}
      }
      cnt = cnt*2;
    }
  }
  return -1;
}


/*--------------------------------------------------------------------*/
/* epb()   Profitable Backtracking                                    */
/*--------------------------------------------------------------------*/
epb(nd,tkn,hist)
     int nd,tkn,hist;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;

  backtrack = 1;
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  for (v=0; v<virts; v++) {
    j = -1;
    while (j < dimensions) {	/* Profitable part */
      j++;
      while (s[j] == d[j] && j < dimensions) {
	j++;
      }
      
      if (j < dimensions) {
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	
	if ((hist & Hist_Mask(j, sign)) == 0) {
	  
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
    
	  n[j] = (s[j] + width[j] + sign)%width[j]; 
	  
	  vl = link_addr(nd,j,sign,v, 0);
	  if ( vlinks[vl].busy == 0) {
	    reserve_vlink(vl,i);
	    return vl;
	  }
	}
      }
      
    }
  }
  return -1;
}

/*--------------------------------------------------------------------*/
/* epbmin()   EPB (min congestion)                                    */
/*--------------------------------------------------------------------*/
epbmin(nd,tkn,hist)
     int nd,hist,tkn;
{
  int i;
  int circ_no;

  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  bmin_congest(nd, hist, PROF);
  if (min_c < virts) 
    return vroute(vl_min,i);
  
  return -1;
}

/*--------------------------------------------------------------------*/
/* mbmmin()   MB-m (min congestion)                                   */
/*--------------------------------------------------------------------*/
mbmmin(nd,tkn,hist)
     int nd,hist,tkn;
{
  int i;
  int circ_no;
  
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  bmin_congest(nd, hist, PROF);
  if (min_c < virts) 
    return vroute(vl_min,i);
  
  if (circs[circ_no].cnt < MISROUTES) {
    bmin_congest(nd, hist, UNPROF);
    if (min_c < virts) {
      circs[circ_no].cnt++;
      return vroute(vl_min,i);
    }
  }
  return -1;
}

/*--------------------------------------------------------------------*/
/* tpb()   Two Phase Backtracking                                     */
/*--------------------------------------------------------------------*/
tpb(nd,tkn,hist)
     int nd,tkn,hist;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;

  backtrack = 1;
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  for (v=0; v<virts; v++) {
    j = -1;
    while (j < dimensions) {	/* Profitable part */
      j++;
      while (s[j] == d[j] && j < dimensions) {
	j++;
      }
      
      if (j < dimensions) {
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	
	if ((hist & Hist_Mask(j, sign)) == 0) {
	  
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
    
	  n[j] = (s[j] + width[j] + sign)%width[j]; 
	  
	  vl = link_addr(nd,j,sign,v, 0);
	  if ( vlinks[vl].busy == 0) {
	    reserve_vlink(vl,i);
	    return vl;
	  }
	}
      }
      
    }
  }
  if (distance(nd,circs[circ_no].dest) <= 2) {
    for (v=0; v<virts; v++) {
      for (j=0; j<dimensions; j++) { /* Unprofitable part */
	gamma=d[j]-s[j];
	for (sign = -1; sign<2; sign+=2) {
	  if ((hist & Hist_Mask(j, sign)) == 0) {
	    for (k=0; k<dimensions; k++)
	      n[k] = s[k];
	    
	    n[j] = (s[j] + width[j] + sign)%width[j]; 
	    
	    vl = link_addr(nd,j,sign,v,0);
	    if ( vlinks[vl].busy == 0) {
	      reserve_vlink(vl,i);
	      return vl;
	    }
	  }
	}
      }
    }
  }
  return -1;
}

/*--------------------------------------------------------------------*/
/* mbm()   Misrouting Backtracking-m                                  */
/*--------------------------------------------------------------------*/
mbm(nd,tkn,hist)
     int nd,tkn,hist;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;

  backtrack = 1;
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  for (v=0; v<virts; v++) {
    j = -1;
    while (j < dimensions) {	/* Profitable part */
      j++;
      while (s[j] == d[j] && j < dimensions) {
	j++;
      }
      
      if (j < dimensions) {
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	
	if ((hist & Hist_Mask(j, sign)) == 0) {
	  
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
    
	  n[j] = (s[j] + width[j] + sign)%width[j]; 
	  
	  vl = link_addr(nd,j,sign,v, 0);
	  if ( vlinks[vl].busy == 0) {
	    reserve_vlink(vl,i);
	    return vl;
	  }
	}
      }
      
    }
  }
  if (circs[circ_no].cnt < MISROUTES) {
    for (v=0; v<virts; v++) {	/* Misrouting part */
      for (j=0; j<dimensions; j++) {
	for (sign = -1; sign < 2; sign+=2) {
	  if ((hist & Hist_Mask(j, sign)) == 0) {
	    
	    for (k=0; k<dimensions; k++)
	      n[k] = s[k];
	    
	    n[j] = (s[j] + width[j] + sign)%width[j]; 
	    
	    vl = link_addr(nd,j,sign,v, 0);
	    if ( vlinks[vl].busy == 0) {
	      reserve_vlink(vl,i);
	      circs[circ_no].cnt++;
	      return vl;
	    }
	  }
	}
      }
    }
  }
  return -1;
}

/*--------------------------------------------------------------------*/
/* mbm+()   Misrouting Backtracking m+                                */
/*--------------------------------------------------------------------*/
mbmp(nd,tkn,hist)
     int nd,tkn,hist;
{
  int i, j, k, sign, vl, v, gamma;
  int circ_no;

  backtrack = 1;
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We are there. */

  if (looped(nd,i) ) return -1; /* We have visited this node in */
				  /* our path already */
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

  for (v=0; v<virts; v++) {
    j = -1;
    while (j < dimensions) {	/* Profitable part */
      j++;
      while (s[j] == d[j] && j < dimensions) {
	j++;
      }
      
      if (j < dimensions) {
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	
	if ((hist & Hist_Mask(j, sign)) == 0) {
	  
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
    
	  n[j] = (s[j] + width[j] + sign)%width[j]; 
	  
	  vl = link_addr(nd,j,sign,v, 0);
	  if ( vlinks[vl].busy == 0) {
	    reserve_vlink(vl,i);
	    return vl;
	  }
	}
      }
      
    }
  }
  if (circs[circ_no].cnt < MISROUTES) {
    for (v=0; v<virts; v++) {	/* Misrouting part */
      for (j=0; j<dimensions; j++) {
	for (sign = -1; sign < 2; sign+=2) {
	  if ((hist & Hist_Mask(j, sign)) == 0) {
	    
	    for (k=0; k<dimensions; k++)
	      n[k] = s[k];
	    
	    n[j] = (s[j] + width[j] + sign)%width[j]; 
	    
	    vl = link_addr(nd,j,sign,v, 0);
	    if ( vlinks[vl].busy == 0) {
	      reserve_vlink(vl,i);
	      circs[circ_no].cnt++;
	      return vl;
	    }
	  }
	}
      }
    }
  }
  return -1;
}


/*--------------------------------------------------------------------*/
/* looped()   Check for loop completion at current node               */
/*--------------------------------------------------------------------*/
looped(n,tkn)
     int n,tkn;
{
  int j, i, v, vl;

  for (j=0; j<dimensions; j++) { /* Check all virtual links out of node */
    for (i=(-1); i<2; i+=2) {
      for (v=0; v<virts; v++) {
	vl = link_addr(n,j,i,v, 0);
	if (vlinks[vl].busy - 1 == tkn) return 1;
      }
    }
  }
  return 0;
}

/*------------------------------------------------------------------*/
/* mbm-sw                                                           */
/*------------------------------------------------------------------*/
mbmsw(nd,tkn,hist)
int nd,tkn,hist;
{
  int i,j,k,sign,vl,v,gamma;
  int dist_old, dist_new;
  int circ_no;

  backtrack = 1;
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We got to dest */

  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);


  if (distance(nd,circs[circ_no].dest) <= RADIUS)
    {
      for(v=0; v<virts; v++)
	{
	  j = -1;
	  while (j < dimensions) 
	    {	/* Profitable part */
	      j++;
	      while (s[j] == d[j] && j < dimensions) 
		{
		  j++;
		}
	      
	      if (j < dimensions) 
		{
		  gamma=d[j]-s[j];
		  if (width[j]-abs(gamma) < abs(gamma) )
		    sign = -sgn(gamma);
		  else
		    sign = sgn(gamma);	/* Decide which way to go */
		  
		  if ((hist & Hist_Mask(j, sign)) == 0) 
		    {
		      for (k=0; k<dimensions; k++)
			n[k] = s[k];
		      n[j] = (s[j] + width[j] + sign)%width[j]; 
		      vl = link_addr(nd,j,sign,v,0);
		      if ( vlinks[vl].busy == 0) 
			{
			  reserve_vlink(vl,i);
			  return vl;
			}
		    }
		}
	    }
	}
      for (v=0; v<virts; v++) 
	{	/* Misrouting part */
	  for (j=0; j<dimensions; j++) 
	    {
	      for (sign = -1; sign < 2; sign+=2) 
		{
		  if ((hist & Hist_Mask(j, sign)) == 0) 
		    {
		      
		      for (k=0; k<dimensions; k++)
			n[k] = s[k];
		      
		      n[j] = (s[j] + width[j] + sign)%width[j]; 
		      
		      vl = link_addr(nd,j,sign,v,0);
		      if ( vlinks[vl].busy == 0) 
			{
			  reserve_vlink(vl,i);
			  if (protocol == MBMSWF) {};
			  circs[circ_no].cnt++;
			  return vl;
			}
		    }
		}
	    }
	}
    } 
  else if (distance (nd,circs[circ_no].dest) > RADIUS)
    {
      for (v=0; v<virts; v++) 
	{
	  j = -1;
	  while (j < dimensions) 
	    {	/* Profitable part */
	      j++;
	      while (s[j] == d[j] && j < dimensions) 
		{
		  j++;
		}
	      
	      if (j < dimensions) 
		{
		  gamma=d[j]-s[j];
		  if (width[j]-abs(gamma) < abs(gamma) )
		    sign = -sgn(gamma);
		  else
		    sign = sgn(gamma);	/* Decide which way to go */
		  
		  if ((hist & Hist_Mask(j, sign)) == 0) 
		    {
		      
		      for (k=0; k<dimensions; k++)
			n[k] = s[k];
		      
		      n[j] = (s[j] + width[j] + sign)%width[j]; 
		      
		      vl = link_addr(nd,j,sign,v,0);
		      if ( vlinks[vl].busy == 0) 
			{
			  reserve_vlink(vl,i);
			  return vl;
			}
		    }
		}
	      
	    }
	}
      if (circs[circ_no].cnt < MISROUTES) 
	{
	  for (v=0; v<virts; v++) {	/* Misrouting part */
	    for (j=0; j<dimensions; j++) 
	      {
		for (sign = -1; sign < 2; sign+=2) 
		  {
		    if ((hist & Hist_Mask(j, sign)) == 0) 
		      {
			
			for (k=0; k<dimensions; k++)
			  n[k] = s[k];
			
			n[j] = (s[j] + width[j] + sign)%width[j]; 
			
			vl = link_addr(nd,j,sign,v,0);
			if ( vlinks[vl].busy == 0) 
			  {
			    reserve_vlink(vl,i);
			    circs[circ_no].cnt++;
			    return vl;
			  }
		      }
		  }
	      }
	  }
	}
    }
  return -1;
}

/*-------------------------------------------------------------------*/
/* sbm                                                               */
/* sm-m is a marriage of scouting with mb-m.  It does the following  */
/*    things.  If there are no faults in the system, it follows the  */
/*    routing algorithm dp.  If it encounters faults, then it does a */
/*    whole bunch of different things.  A probe can misroute and/or  */
/*    backtrack.  It can misroute even after it has taken a determ   */
/*    route.  Misrouting is preferred over backtracking.  Normally,  */
/*    scouting distance is set at 1, but it is increased to 3 or 4   */
/*    when it hits a fault (4 if it hits a faulty router).  If the   */
/*    destination is faulty, then the entire path is torn down.  We  */
/*    have a bit to denote if we have taken a determ path.  This bit */
/*    will allow us to misroute on busy channels, other wise, we do  */
/*    not.                                                           */
/*-------------------------------------------------------------------*/
sbm(nd, tkn, hist)
     int nd, tkn, hist;
{
  int i,j,k,sign,vl,v,gamma;
  int circ_no;

  backtrack = 0;
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;
  
  if (nd == circs[circ_no].dest) return -nd-2; /* We got there */
  
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);

				/* route over adaptive profitable links */
  for (v=2; v<virts; v++) {	/* Adaptive subfunction */
    j = dimensions;
    while (j >= 0) {	/* Idle part */
      j--;
      while (s[j] == d[j] && j >= 0) {
	j--;
      }
      
      if (j>=0) {
	for (k=0; k<dimensions; k++)
	  n[k] = s[k];
	
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma))
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma); 	/* Decide which way to go */
	
	if ((hist & Hist_Mask(j,sign)) == 0) {
	  n[j] = (s[j] + width[j] + sign)%width[j]; 
	  
	  vl = link_addr(nd,j,sign,v,0);
	  if ( vlinks[vl].busy == 0) {
	    reserve_vlink(vl,i);
	    return vl;
	  }
	}
      }
    }
  }
  
  j = 0;
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }
  for (k=0; k<dimensions; k++)
    n[k] = s[k];
  
  gamma=d[j]-s[j];
  if (gamma > 0) 
    v=0; 
  else 
    v=1; 
  if (width[j]-abs(gamma) < abs(gamma)) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma); 		/* Decide which way to go */
  
  vl = link_addr(nd,j,sign,v,0);
  
				/* Misroute extension */
  if (vlinks[vl].busy == (-1)) { /* MR here due to faulty link */
    i = UnPr(tkn);
    backtrack = 1;
    if (circs[circ_no].cnt < MISROUTES) {
      for (v=2; v<virts; v++) {	/* Adaptive channels */
	for (j=0; j<dimensions; j++) {
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
	  
	  for (sign = -1; sign<=1; sign+=2) {
	    if ((hist & Hist_Mask(j,sign)) == 0) {
	      n[j] = (s[j] + width[j] + sign)%width[j]; 
	      
	      vl = link_addr(nd,j,sign,v,0);
	      if (vlinks[vl].busy == 0) {
		reserve_vlink(vl,i);
		circs[circ_no].cnt++;
		circs[circ_no].misc2 = 1; /* misrouted due to fault */
		return vl;
	      }
	    }
	  }
	}
      }
    }
  }

  j = 0;
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }

  for (k=0; k<dimensions; k++)
    n[k] = s[k];
  
  gamma=d[j]-s[j];
  if (gamma > 0) 
    v=0; 
  else 
    v=1;
  if (width[j]-abs(gamma) < abs(gamma)) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	/* Decide which way to go */

  vl = link_addr(nd,j,sign,v,0);

  if (circs[circ_no].misc2) {	/* we can only MR if we had already MR */
                                /* due to faulty link */
    i = UnPr(tkn);
    backtrack = 1;
    if (circs[circ_no].cnt < MISROUTES) {
      for (v=2; v<virts; v++) {	/* Adaptive channels */
	for (j=0; j<dimensions; j++) {
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
	  
	  for (sign = -1; sign<=1; sign+=2) {
	    if ((hist & Hist_Mask(j,sign)) == 0) {
	      n[j] = (s[j] + width[j] + sign)%width[j]; 
	      
	      vl = link_addr(nd,j,sign,v,0);
	      if ( vlinks[vl].busy == 0) {
		reserve_vlink(vl,i);
		circs[circ_no].cnt++;
		return vl;
	      }
	    }
	  }
	}
      }
    }
  }
				/* We cannot route profitably, and we */
				/* have not hit a faulty link, now we */
				/* try to route deterministically */
  j = 0;			/* Ecube subfunction */
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }
  
  for (k=0; k<dimensions; k++)
    n[k] = s[k];
  
  gamma=d[j]-s[j];
  if (gamma > 0) v=0; else v=1;
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	/* Decide which way to go */
  
  if ((hist & Hist_Mask(j,sign)) == 0) { 
    n[j] = (s[j] + width[j] + sign)%width[j]; 
  }
  vl = link_addr(nd, j, sign, v, 0);
  if (vlinks[vl].busy == 0) {
    reserve_vlink(vl,i);
    return vl;
  }
  backtrack = 1;		/* if we cannot do any of that, we */
				/* must try to backtrack */
  return -1;
}

sbm_old(nd, tkn, hist)
     int nd, tkn, hist;
{
  int i,j,k,sign,vl,v,gamma;
  int circ_no;

  backtrack = 0;
  i = UnPr(tkn);
  circ_no = msgs[i].circ_index;

  if (nd == circs[circ_no].dest) return -nd-2; /* We got there */
  
  get_coords(nd,s);
  get_coords(circs[circ_no].dest,d);
  
  for (v=2; v<virts; v++) {	/* Adaptive subfunction */
    j = dimensions;
    while (j >= 0) {	/* Idle part */
      j--;
      while (s[j] == d[j] && j >=0) {
	j--;
      }
      
      if (j>=0) {
	for (k=0; k<dimensions; k++)
	  n[k] = s[k];
	
	gamma=d[j]-s[j];
	if (width[j]-abs(gamma) < abs(gamma) )
	  sign = -sgn(gamma);
	else
	  sign = sgn(gamma);	/* Decide which way to go */
	
	if ((hist & Hist_Mask(j,sign)) == 0) {
	  n[j] = (s[j] + width[j] + sign)%width[j]; 
	  
	  vl = link_addr(nd,j,sign,v,0);
	  if ( vlinks[vl].busy == 0) {
	    reserve_vlink(vl,i);
	    return vl;
	  }
	}
      }
    }
  }

  j = 0;			/* Misroute if appropriate */
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }
  
  for (k=0; k<dimensions; k++)
    n[k] = s[k];
  
  gamma=d[j]-s[j];
  if (gamma > 0) v=0; else v=1;
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	/* Decide which way to go */

  vl = link_addr(nd,j,sign,v,0);
  if (vlinks[vl].busy == (-1)) {
    i = UnPr(tkn);
/*    msgs[i].probe_lead = 4; */
    backtrack=1;
    if (circs[circ_no].cnt < MISROUTES) {
      for (v=2; v<virts; v++) 
	for (j=0; j<dimensions; j++) {
	  for (k=0; k<dimensions; k++)
	    n[k] = s[k];
	  
	  for (sign=-1; sign<=1; sign+=2) {
	    if ((hist & Hist_Mask(j,sign)) == 0) {
	      n[j] = (s[j] + width[j] + sign)%width[j];
	      
	      vl = link_addr(nd,j,sign,v,0);
	      if (vlinks[vl].busy == 0) {
		reserve_vlink(vl,i);
		circs[circ_no].cnt++;
		return vl;
	      }
	    }
	  }
	}
    }
  }

  j = 0;
  while (s[j] == d[j] && j<dimensions) {
    j++;
  }
  
  for (k=0; k<dimensions; k++)
    n[k] = s[k];
  gamma=d[j]-s[j];
  if (gamma > 0) v=0; else v=1;
  if (width[j]-abs(gamma) < abs(gamma) ) 
    sign = -sgn(gamma);
  else
    sign = sgn(gamma);	/* Decide which way to go */

  vl = link_addr(nd,j,sign,v,0);
  if ((hist & Hist_Mask(j,sign)) == 0) {
    n[j] = (s[j] + width[j] + sign)%width[j]; 
    
    if (vlinks[vl].busy == 0) {
      reserve_vlink(vl,i);
      circs[circ_no].misc2 = 1;
      return vl;
    }
  } else backtrack=1;
  if (circs[circ_no].misc2) 
    backtrack=0;
  return -1;
}
