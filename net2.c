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

   /*------------------------------------------------------------------*/
   /* Network module                                                   */
   /*------------------------------------------------------------------*/

static char network_rcsid[] = "$Id: net2.c,v 1.18 1994/06/20 19:09:45 dao Exp dao $";

#include <stdio.h>
#include <math.h>
#include "dat2.h"
#include "dd.h"

#define delay(x,y) x++; if (x < y) return; else x=0
#define route_done(x)  (x< -1)
#define route_fail(x)  (x == (-1))
#define route_success(x)  (x >= 0)

double drand48 ();

/*******************************************************/
/* Global Variables                                    */
/*******************************************************/
long seed;

/* int QNUM;  yungho : variable for the number of reception queues */

extern char filename[];
int XDISP, SLOW, TRANSIENT, SIM_STATUS, STEP;

int tmp;

int *complement;
/* ----- Network state variables ---------  */
node *nodes;			/* These variables hold the network state */
plink *plinks;
vlane *vlinks;
cvlane *corrvlinks, *compvlinks;

/* ------ Message variables ---------  */
msg msgs[MAXMESSAGE];		/* Misc information necessary for each msg */
circuit circs[MAXCIRC];		/* Misc information for each circuit */

float t_next;			/* Next message injection time */

/* ------ Network configurations variables ---------  */
int dimensions, *width, numvlinks, numnvlinks, numplinks, *part, *cum_part;
int PL, MSGL, inject_rate, DIST, RUN_TIME, HALF, DEMAND, PER, MISROUTES, MAP;
int NODES, virts, node_virts, comm_mech, FAULTS, ABORT, NO_CTS, use_virts;
int BUFFERS, SECURITY_LEVELS;
int nvlog, vlog, dimlog, nlog, protocol, backtrack;
int HSPOTS, *hsnodes, HSPERCENT;
int route_delay, select_fnc, data_delay;
int link_time, ack_time, link_arb_time;
char proto_name[40], trace_file_name[40];
int RADIUS, RAD, ORDER=0, RETRY=10;
int MESH, SRC_QUEUE, PROBE_LEAD;
int *dyn_fault_times, num_dyn_faults, dyn_cnt, physical_faults;
int traffic, vary_virts=0, flip_dim=0;
int min_event_time=0, activity;
int FLIT_SIZE, EXECUTE_CYCLES;
int idle_pes, cum_idle_pes, over_pes, cum_over_pes, old_idle, old_over;
int RCV_OVER=100, RCV_OVER_PER=4; /* Default software overhead values */
int SEND_OVER=100, SEND_OVER_PER=4, THREAD_OVER=20;
int PRE_OVER=100, REL_OVER = 10;
int PROFILE = 0, msgflits = 0, oldflits=0;
int blocked = 0, old_blocked = 0;
int temp_tot_done;
float throughput;
FILE *commprofile, *peprofile, *overprofile, *blkprofile;
char THREAD_MAP[40];

/* newly added to support hybrid traffic */
int HYBRID, LMSGL, LMSGPCT;

/* newly added to support scaling of display */
int SCALE = 1;
int TEST = 0;
/* newly added to support scaling of display */
int NEW_DISHA_FEATURES = 0;
int CUMWAIT = 0;
int DEADLOCK_BUFFER_UTILIZED = 0;
int SAVED_SLOW;
int WAITTIME = 25;
extern int seize_count;
extern int seize_wait_time;

/* newly added to support changing speeds of Disha Synchronous Token */
int TOKENSPEED = 1;
int HALFTOKENSPEED = 0;

/* newly added to support changing resolution of statistical checkpoints */
int SFREQ = 0;

/* newly added to support Deadlock Detection */
int CD = 0;
int DD = 0;
int CDFREQ = 25;
int DDFREQ = 100;
int DDTEST = 0;

/* ------ Simulation status variables and statistics ------- */
int lat_hist[10000], su_hist[500], path_hist[50], mis_hist[10];
int sim_time, t_init, INIT;
int injects, setups, tot_done, tot_tacks;
int lat_ave, su_ave, q_ave;
int tacklat_ave, path_length_ave;
int misroutes_ave;
float lat_sqave, su_sqave, q_sqave, tacklat_sqave;
int con_circs, con_su, con_tacks;
int ave_circs, ave_su, ave_tacks;
int t_service, t_served, t_head, heads;
int backs_to_src, flits_delivered;
int rdist[33], sdist[33];
int cdist[33];
int rhdist[33], rfcdist[33], rhcdist[33];
int blk_ave, p_blk, tot_traversed;
float P_U, P_B;
int paths_checked;
int nu_c, nu_s, nu, nu_B;
int src_kill_informs, src_kill_inform_time, retry_failures;
int control_collide, cum_control_collide, control_moves;

/* newly added to support hybrid traffic */
int lng_tot_done;
int lng_lat_ave;
int lng_temp_tot_done;

/* ------ Misc scratch variables ------ */
int t1[16], t2[16], n[16];

#ifdef SET_PORT
int useinjvcs, usedelvcs ;
#endif

#ifdef PLINK
int plinknode;
int pl_injection;
int pl_delivery;
#endif


/*******************************************************/
/* Misc test routines - newly added                    */
/*******************************************************/
/*------------------------------------------------------------------*/
/* For printing out status of all nodes			             */
/*------------------------------------------------------------------*/

void	print_nodes()
{
	int	i, j;

	printf("NODES:\n");
	printf("======\n");

	for (i = 0; i < NODES; i++)
	{	
		printf("[%d].queue:      ", i); 	
		for (j= 0; j < SRC_QUEUE; j++)
			printf("[%d]: %d\t", j, nodes[i].queue[j]);

		printf("\n[%d].headers:  ", i);
		for (j= 0; j < 4; j++)
			printf("[%d]: %d\t", j, nodes[i].headers[j]);

		printf("\n[%d].num_head: %d\n", i, nodes[i].num_head);

		printf("[%d].send:       ", i);
		for (j= 0; j < node_virts; j++)
			printf("[%d]: %d\t", j, nodes[i].send[j]);

		printf("\n[%d].generate: ", i);
		for (j= 0; j < node_virts; j++)
			printf("[%d]: %d\t", j, nodes[i].generate[j]);

		printf("\n[%d].consume:  ", i);
		for (j= 0; j < node_virts; j++)
			printf("[%d]: %d\t", j, nodes[i].consume[j]);

		printf("\n");
		printf("[%d].cnt: %d\n", i, nodes[i].cnt);
		printf("[%d].idle: %d\n", i, nodes[i].idle);
		printf("[%d].over: %d\n", i, nodes[i].over);
		printf("[%d].security_level: %d\n", i, nodes[i].security_level);
		printf("[%d].x: %d\n", i, nodes[i].x);
		printf("[%d].y: %d\n", i, nodes[i].y);
		printf("[%d].height: %d\n", i, nodes[i].height);
		printf("[%d].width: %d\n", i, nodes[i].width);
		printf("\n");
	}
 	printf("\n\n");
}

/*------------------------------------------------------------------*/
/* For printing out status of all nodes			             */
/*------------------------------------------------------------------*/

void	print_nodes2()
{
	int	i, j;

	printf("NODES:\n");
	printf("======\n");

  	printf("%6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s\n", 
	  "node",
	  "n_head",
	  "cnt",
	  "idle",
	  "over", 
	  "s_lev", 
	  "x", 
	  "y", 
	  "height",
	  "width", 
	  "dtok", 
	  "dtok_s", 
	  "ddbu"
          );

  
	for (i = 0; i < NODES; i++)
	{	

		printf("%6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d\n", 
		 i,
		nodes[i].num_head,
		nodes[i].cnt,
		nodes[i].idle,
		nodes[i].over,
		nodes[i].security_level,
		nodes[i].x,
		nodes[i].y,
		nodes[i].height,
		nodes[i].width,
		nodes[i].disha_token,
		nodes[i].disha_token_siezed,
		nodes[i].disha_deadlock_buffer_used
		);
	}
 	printf("\n\n");
}


/*------------------------------------------------------------------*/
/* For printing out status of all plinks			             */
/*------------------------------------------------------------------*/

void	print_plinks()
{
	int	i;

	printf("PLINKS:\n");
	printf("======\n");

	for (i = 0; i < numplinks+NODES*plinknode; i++)
	{		
		printf("[%d].used: %d\n", i, plinks[i].used);
		printf("[%d].Lnode: %d\n", i, plinks[i].Lnode);
		printf("[%d].Rnode: %d\n", i, plinks[i].Rnode);
		printf("[%d].security_level: %d\n", i, plinks[i].security_level);
		printf("[%d].LR: %d\n", i, plinks[i].LR);
		printf("[%d].startlane: %d\n", i, plinks[i].startlane);
		printf("[%d].Llane: %d\n", i, plinks[i].Llane);
		printf("[%d].Rlane: %d\n", i, plinks[i].Rlane);
		printf("[%d].arbcnt: %d\n", i, plinks[i].arbcnt);
		printf("[%d].lcnt: %d\n", i, plinks[i].lcnt);
		printf("[%d].rcnt: %d\n", i, plinks[i].rcnt);
		printf("[%d].probe_register: %d\n", i, plinks[i].probe_register);
		printf("\n");

       }
 	printf("\n\n");
 }


/*------------------------------------------------------------------*/
/* For printing out status of vlinks			             */
/*------------------------------------------------------------------*/

void	print_used_plinks()
{
	int	i;

	printf("PLINKS:sim_time = %d\n", sim_time);
	printf("==================\n");

  	printf("%6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s\n", 
	  "plink",
	  "used",
	  "Lnode",
	  "Rnode", 
	  "s_lev", 
	  "LR", 
	  "startl", 
	  "Llane",
	  "Rlane", 
	  "arbcnt", 
	  "lcnt", 
	  "rcnt", 
	  "pr_reg"
          );

	for (i = 0; i < numplinks+NODES*plinknode; i++)
	{		
		if (plinks[i].used)
		{

  	printf("%6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d\n", 
		 i,
		 plinks[i].used,
		 plinks[i].Lnode,
		 plinks[i].Rnode,
		 plinks[i].security_level,
		 plinks[i].LR,
		 plinks[i].startlane,
		 plinks[i].Llane,
		 plinks[i].Rlane,
		 plinks[i].arbcnt,
		 plinks[i].lcnt,
		 plinks[i].rcnt,
		 plinks[i].probe_register
		 );
		}
       }
 	printf("\n\n");

 }



/*------------------------------------------------------------------*/
/* For printing out status of Circuits			             */
/*------------------------------------------------------------------*/

void	print_circs()
{
	int	i;

	printf("CIRCS: sim_time = %d\n", sim_time);
	printf("==================\n");

 	printf("%6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s\n", 
	  "circ",
	  "msgtyp",
	  "src",
	  "dest",
	  "acked", 
	  "misc", 
	  "misc2", 
	  "rtecnt", 
	  "retrys",
	  "lnks_r", 
	  "compl.", 
	  "blck'd", 
	  "cnt", 
	  "seclev",
	  "p_len", 
	  "tim_bt", 
	  "pl",
	  "inject_vir",
	  "wait_t", 
	  "token",
	  "M"
          );

	for (i = 0; i < MAXCIRC; i++)
	{		
		if (circs[i].src != (-1))
		{
  		printf("%6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d\n", 
		 		i,
				circs[i].msgtype,
				circs[i].src,
				circs[i].dest,
				circs[i].acked,
				circs[i].misc,
				circs[i].misc2,
				circs[i].routecnt,
				circs[i].retries,
				circs[i].links_res,
				circs[i].complete,
				circs[i].blocked,
				circs[i].cnt,
				circs[i].security_level,
				circs[i].path_length,
				circs[i].times_bt,
				circs[i].pl,
				circs[i].inject_virt,
  				circs[i].wait_time,
  				circs[i].token,
   				circs[i].M
				);
		}
       }
 	printf("\n\n");

}


/*------------------------------------------------------------------*/
/* For printing out status of Messages			             */
/*------------------------------------------------------------------*/

void	print_msgs()
{
	int	i;

	printf("MSGS:sim_time = %d\n", sim_time);
	printf("==================\n");

	printf("%6s %6s %6s %6s %6s %6s %6s\n", 
	  "msg",
	  "c_idx",
	  "t_inj",
	  "t_que",
	  "t_intr", 
	  "msglen", 
	  "prerte"
          );

	for (i = 0; i < MAXMESSAGE; i++)
	{		
		if (msgs[i].circ_index != (-1))
		{


  			printf("%6d %6d %6d %6d %6d %6d %6d\n", 
		 		i,
				msgs[i].circ_index,
				msgs[i].t_inject,
				msgs[i].t_queue,
				msgs[i].t_intr,
				msgs[i].msglen,
				msgs[i].preroute
				);
		}
       }
 	printf("\n\n");
}


/*------------------------------------------------------------------*/
/* For printing out status of vlinks			             */
/*------------------------------------------------------------------*/

void	print_vlinks()
{
	int	i;

	printf("VLINKS:sim_time = %d\n", sim_time);
	printf("==================\n");

  	printf("%6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %12s %6s %6s %6s %6s %6s %6s %6s\n", 
	  "vlink",
	  "f_blk",
	  "b_blk", 
	  "o_buf", 
	  "in_hd", 
	  "in_tl", 
	  "in_buf",
	  "nxt_in", 
	  "dr", 
	  "pl", 
	  "next", 
	  "busy", 
	  "hist", 
	  "cnt", 
	  "s_lev", 
	  "x", 
	  "y", 
	  "height", 
	  "width",	  
	  "data"
          );


	for (i = 0; i < (numvlinks+ numnvlinks); i++)
	{		
		if (vlinks[i].busy)
		{

  	printf("%6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d %12d %6d %6d %6d %6d %6d %6d %6d\n", 
		 i,
		 vlinks[i].forward_block,
		 vlinks[i].back_block,
		 vlinks[i].out_buffer,
		 vlinks[i].in_head,
		 vlinks[i].in_tail,
		 vlinks[i].in_buf,
		 vlinks[i].next_in,
		 vlinks[i].dr,
		 vlinks[i].pl,
		 vlinks[i].next,
		 vlinks[i].busy,
		 vlinks[i].hist,	 
		 vlinks[i].cnt,
		 vlinks[i].security_level,
		 vlinks[i].x,
		 vlinks[i].y,
		 vlinks[i].height,
		 vlinks[i].width,
		 vlinks[i].data
		 );
		}
       }
 	printf("\n\n");
}



/*------------------------------------------------------------------*/
/* For printing out status of corrvlinks			             */
/*------------------------------------------------------------------*/

void	print_corrvlinks()
{
	int	i;

	printf("CORRVLINKS:sim_time = %d\n", sim_time);
	printf("===++++===============\n");

  	printf("%6s %6s %6s %6s %6s\n", 
	  "corrvl",
	  "O_buff",
	  "i_buff", 
	  "nxt_in", 
	  "next"
          );


	for (i = 0; i < (numvlinks+ numnvlinks); i++)
	{		
		if (corrvlinks[i].out_buffer || corrvlinks[i].in_buffer)
		{

  	printf("%6d %6d %6d %6d %6d\n", 
		 i,
		 corrvlinks[i].out_buffer,
		 corrvlinks[i].in_buffer,
		 corrvlinks[i].next_in,
		 corrvlinks[i].next
		 );
		}
       }
 	printf("\n\n");


 }


/*------------------------------------------------------------------*/
/* For printing out status of compvlinks			             */
/*------------------------------------------------------------------*/

void	print_compvlinks()
{
	int	i;

	printf("COMPVLINKS:sim_time = %d\n", sim_time);
	printf("===++++===============\n");

  	printf("%6s %6s %6s %6s %6s\n", 
	  "corrvl",
	  "O_buff",
	  "i_buff", 
	  "nxt_in", 
	  "next"
          );


	for (i = 0; i < (numvlinks+ numnvlinks); i++)
	{		
		if (compvlinks[i].out_buffer || compvlinks[i].in_buffer)
		{

  	printf("%6d %6d %6d %6d %6d\n", 
		 i,
		 compvlinks[i].out_buffer,
		 compvlinks[i].in_buffer,
		 compvlinks[i].next_in,
		 compvlinks[i].next
		 );
		}
       }

 	printf("\n\n");

 }


/*******************************************************/
/* Misc utility routines                               */
/*******************************************************/

/*-----------------------------------------------------*/
/* plink_addr(n, d, sign)  Give a physical link addr   */
/*-----------------------------------------------------*/
#ifdef PLINK
#define plink_addr(n, d) ((n << (dimlog-1)) + d + NODES*plinknode)
#else
#define plink_addr(n, d) ((n << (dimlog-1)) + d + NODES)
#endif


/*--------------------------*/
/* delete_header()          */
/*--------------------------*/
delete_header(n,i)
int n, i;
{
  int j;

  for (j=i; j<nodes[n].num_head-1; j++) {
    nodes[n].headers[j] = nodes[n].headers[j+1];
  }
  nodes[n].headers[nodes[n].num_head-1] = (-1);
  nodes[n].num_head--;
}

/*--------------------------*/
/* find_header()            */
/*--------------------------*/
find_header(n,v)
int n, v;
{
  int j;

  for (j=0; j<nodes[n].num_head; j++) 
    if (nodes[n].headers[j] == v) return 1;

  return 0;
}

/*--------------------------*/
/* add_header()             */
/*--------------------------*/
add_header(n,v)
int n,v;
{
  int j;

  for (j=0; j<4; j++) 
    if (nodes[n].headers[j] == (-1)) {
      nodes[n].headers[j] = v;
      nodes[n].num_head++;
      return;
    }
}

/*--------------------------------*/
/* max(x,y)  Integer maximum      */
/*--------------------------------*/
max(x,y) {
  if (x>y) return x;
  return y;
}

/*--------------------------------*/
/* report_evap()                  */
/*--------------------------------*/
report_evap(type,where)
     char *type, *where;
{
  printf("Flit evap on %s at %s\n",type,where);
}

/*--------------------------------*/
/* handle_msg_done_at_node(n,v)   */
/*--------------------------------*/
handle_msg_done_at_node(n,v) {
  int i,j, k;

  nodes[n].send[v] = 0;
  /* Message is done. */
  for (j = 0; j < SRC_QUEUE; j++)
    if (nodes[n].queue[j] != (-1))
      {
	i = find_msg ();
	k = find_circ ();

	if (i < MAXMESSAGE && k < MAXCIRC)
	  {
	    if (traffic == SYNTHETIC) {
	      create_new_circ (k, n, n, v, nodes[n].queue[j], 
			       uniform_dist(SECURITY_LEVELS), n);
	      create_new_msg (i, k, MSGL, sim_time, 0);
	    }
	    nodes[n].queue[j] = (-1);
	  }
	else
	  printf ("Message or circuit overflow. \n");
	break;
      }
}

/*--------------------------------*/
/* flush_control_flits(vl,tkn)    */
/*--------------------------------*/
flush_control_flits(vl,tkn)
{
  int ovl, i, j, k, cnt, pl;

  ovl = complement[vl];
  pl = vlinks[ovl].pl;

  if (compvlinks[ovl].out_buffer)
      plinks[pl].used--;
  compvlinks[ovl].out_buffer=0;
  compvlinks[ovl].in_buffer=0;
  compvlinks[ovl].next_in=0;

  if (corrvlinks[vl].out_buffer)
    plinks[pl].used--;
  corrvlinks[vl].out_buffer = 0;
  corrvlinks[vl].in_buffer = 0;
  corrvlinks[vl].next_in = 0;
    
  i = next_in_flit(ovl);
  mark_tkn_buff(ovl, i, IN);
}

/*-----------------------------*/
/* flush_out_data_flits(vl,tkn)    */
/*-----------------------------*/
flush_out_data_flits(vl,tkn)
{
  int i;

  if (UnData(vlinks[vl].out_buffer) == tkn || 
      UnTail(vlinks[vl].out_buffer) == tkn) {
    vlinks[vl].out_buffer = 0;
    vlinks[vl].data = 0;
    plinks[vlinks[vl].pl].used--;
  }
  if (XDISP) {
    mark_tkn_buff(vl, 0, OUT);
  }
}
      

/*-----------------------------*/
/* flush_in_data_flits(vl,tkn) */
/*-----------------------------*/
flush_in_data_flits(vl,tkn)
{
  int i;
  int head, data;

  data = vlinks[vl].in_buffer[vlinks[vl].in_head];
  while ((UnData(data) == tkn || UnTail(data) == tkn) &&
	 vlinks[vl].in_buf>0) {
    vlinks[vl].in_buffer[vlinks[vl].in_head] = 0;
    vlinks[vl].in_head = (vlinks[vl].in_head + 1) % BUFFERS;
    vlinks[vl].in_buf--;
    vlinks[vl].data = 0;
    data = vlinks[vl].in_buffer[vlinks[vl].in_head];
  }
  vlinks[vl].next_in = 0;
  if (XDISP) {
    i = next_in_flit(vl);
    mark_tkn_buff(vl, i, IN);
  }
}
      

/*--------------------------------*/
/* inc_setup(tmp)                 */
/*--------------------------------*/
inc_setup(tmp) {
  if (tmp >= 0) 
    if (sign_of_link (tmp) == L)
      {
	plinks[vlinks[tmp].pl].nrl++;
	plinks[vlinks[tmp].pl].nsl++;
	if (plinks[vlinks[tmp].pl].nrl > 32 ||
	    plinks[vlinks[tmp].pl].nsl > 32)
	  printf("Setups greater than 32 on link %d\n",vlinks[tmp].pl);
      }
    else
      {
	plinks[vlinks[tmp].pl].nrr++;
	plinks[vlinks[tmp].pl].nsr++;
	if (plinks[vlinks[tmp].pl].nrr > 32 ||
	    plinks[vlinks[tmp].pl].nsr > 32)
	  printf("Setups greater than 32 on link %d\n",vlinks[tmp].pl);
      }
}

/*--------------------------------*/
/* dec_setup(tmp)                 */
/*--------------------------------*/
dec_setup(tmp) {
  if (tmp >= 0) 
    if (sign_of_link (tmp) == L)
      {
	plinks[vlinks[tmp].pl].nrl--;
	plinks[vlinks[tmp].pl].nsl--;
	if (plinks[vlinks[tmp].pl].nrl < 0 ||
	    plinks[vlinks[tmp].pl].nsl < 0)
	  printf("Setups less than zero on link %d\n",vlinks[tmp].pl);
      }
    else
      {
	plinks[vlinks[tmp].pl].nrr--;
	plinks[vlinks[tmp].pl].nsr--;
	if (plinks[vlinks[tmp].pl].nrr < 0 ||
	    plinks[vlinks[tmp].pl].nsr < 0)
	  printf("Setups less than zero on link %d\n",vlinks[tmp].pl);
      }
}

/*--------------------------------*/
/* dec_circuits(tmp)              */
/*--------------------------------*/
dec_circuits(tmp) {
  if (tmp >=0)
    if (sign_of_link (tmp) == L)
      {
	plinks[vlinks[tmp].pl].nrl--;
	plinks[vlinks[tmp].pl].ncl--;
	if (plinks[vlinks[tmp].pl].ncl < 0 ||
	    plinks[vlinks[tmp].pl].nrl < 0)
	  printf("Circuits less than zero on link %d\n",vlinks[tmp].pl);
      }
    else
      {
	plinks[vlinks[tmp].pl].nrr--;
	plinks[vlinks[tmp].pl].ncr--;
	if (plinks[vlinks[tmp].pl].nrr < 0 ||
	    plinks[vlinks[tmp].pl].ncr < 0)
	  printf("Circuits less than zero on link %d\n",vlinks[tmp].pl);
      }
}

/*--------------------------------*/
/* inc_circuits(tmp)              */
/*--------------------------------*/
inc_circuits(tmp) {
  if (tmp>=0)
    if (sign_of_link (tmp) == L)
      {
	plinks[vlinks[tmp].pl].nsl--;
	plinks[vlinks[tmp].pl].ncl++;
	if (plinks[vlinks[tmp].pl].nsl < 0 ||
	    plinks[vlinks[tmp].pl].ncl > 32)
	  printf("Circuits greater than 32 on link %d\n",vlinks[tmp].pl);
      }
    else
      {
	plinks[vlinks[tmp].pl].nsr--;
	plinks[vlinks[tmp].pl].ncr++;
	if (plinks[vlinks[tmp].pl].nsr < 0 ||
	    plinks[vlinks[tmp].pl].ncr > 32)
	  printf("Circuits greater than 32 on link %d\n",vlinks[tmp].pl);
      }
}

/*-----------------------------------------------------------------*/
/* next_in_flit (ovl)                                              */
/*-----------------------------------------------------------------*/
next_in_flit(ovl) {
  int tkn;
  
  tkn = 0;
  if (compvlinks[ovl].in_buffer) 
    tkn = compvlinks[ovl].in_buffer;
  else if (corrvlinks[ovl].in_buffer)
    tkn = corrvlinks[ovl].in_buffer;
  else if (vlinks[ovl].in_buf > 0)
    tkn = vlinks[ovl].in_buffer[vlinks[ovl].in_head];
  return tkn;
}

/*-----------------------------------------------------------------*/
/* remove_in_flit (ovl)                                            */
/*-----------------------------------------------------------------*/
remove_in_flit(ovl) {
  int tkn,i;
  if (compvlinks[ovl].in_buffer) {
    tkn = compvlinks[ovl].in_buffer;
    compvlinks[ovl].in_buffer = 0;
  } else
    if (corrvlinks[ovl].in_buffer) {
      tkn = corrvlinks[ovl].in_buffer;
      corrvlinks[ovl].in_buffer=0;
    } else {
      tkn = vlinks[ovl].in_buffer[vlinks[ovl].in_head];
      vlinks[ovl].in_buffer[vlinks[ovl].in_head] = 0;
      vlinks[ovl].in_head = (vlinks[ovl].in_head + 1) % BUFFERS;
      vlinks[ovl].in_buf--;
    }
  if (XDISP) {
    i = next_in_flit(ovl);
    mark_tkn_buff(ovl, i, IN);
  }
  activity++;
  return tkn;
}

/*-----------------------------------------------------------------*/
/* deposit_in_flit (ovl, tkn)                                      */
/*-----------------------------------------------------------------*/
deposit_in_flit(ovl,tkn) {
  vlinks[ovl].in_buffer[vlinks[ovl].in_tail] = tkn;
  vlinks[ovl].in_tail = (vlinks[ovl].in_tail + 1) % BUFFERS;
  vlinks[ovl].in_buf++;
  vlinks[ovl].data = 1;
  mark_tkn_buff(ovl, vlinks[ovl].in_buffer[vlinks[ovl].in_head], IN);
}

/*-----------------------------------------------------------------*/
/* deposit_out_flit (ovl, tkn)                                     */
/*-----------------------------------------------------------------*/
deposit_out_flit(ovl,tkn) {
  if (TestAck(tkn) || TestBack(tkn) || TestTAck(tkn) || TestBKill(tkn)) {
    compvlinks[ovl].out_buffer = tkn;
  } else if (TestData(tkn) || TestTail(tkn)) {
    vlinks[ovl].out_buffer = tkn;
  } else
    corrvlinks[ovl].out_buffer = tkn;

  plinks[vlinks[ovl].pl].used++;
  
  mark_tkn_buff(ovl, tkn, OUT);
}

/*-----------------------------------------------------------------*/
/* connect_link (vl, next, mode, tkn) Utility for making circuits  */
/*-----------------------------------------------------------------*/
connect_link(vl,next,mode, tkn) {
  int comp;

  if (mode == LINK_TO_NODE) {
    vlinks[vl].next = (-next-2);
  }
  if (mode == NODE_TO_LINK) {
    frame_busy(next,IN, tkn);
    vlinks[next].forward_block = 0;
    vlinks[next].back_block = 0;
    vlinks[next].data = 0; /* I don't know if this should be here */
  }
  if (mode == LINK_TO_LINK) {
    vlinks[vl].next = next;
    comp = complement[next];
    compvlinks[comp].next = complement[vl];
    frame_busy(next,IN, tkn);
    vlinks[next].forward_block = 0;
    vlinks[comp].back_block = 0;
  }
}

/*-------------------------------------------------------*/
/* release_link(vl)                                      */
/*-------------------------------------------------------*/
release_link(vl) {
  int comp, owner, prev, next;

  if (vl != -1) {
    if (vlinks[vl].busy > 0) {
      owner = vlinks[vl].busy-1;
      circs[owner].links_res--;
      if (circs[owner].links_res == 0) {
	/* Dead circuit */
	circs[owner].src = (-1);
      }
    }
    vlinks[vl].busy = -10;
    vlinks[vl].data = 0;
    next = vlinks[vl].next;
    vlinks[vl].next = (-1);
    comp = complement[vl];
    prev = compvlinks[comp].next;
    compvlinks[comp].next = (-1);
    
    /*------------------------------------------------------------*/
    /* Conditionally remove the next mapping of the previous link */
    /*------------------------------------------------------------*/
    if (prev != -1) {
      if (vlinks[complement[prev]].next == vl)
	vlinks[complement[prev]].next = (-1);
    }
    
    /*------------------------------------------------------------*/
    /* Conditionally remove the bcp mapping of the next link      */
    /*------------------------------------------------------------*/
    if (next != -1) {
      comp = complement[next];
      if (compvlinks[comp].next == complement[vl])
	compvlinks[comp].next = (-1);
    }
    
    frame_busy(vl,IN, 0);
  }
}

/*------------------------*/
/* frame_busy(vl,in, tkn) */
/*------------------------*/
frame_busy(vl,in, tkn) {
  int busy;
  if (!XDISP) return;
  if (!tkn)
    busy = (-1);
  else
    busy = UnToken(tkn);
  if (busy < 0) 
    frame_tkn_buff(vl,0,in);
  else
    frame_tkn_buff(vl,Pr(busy),in);
}


/*---------------------------*/
/* frame_tkn_buff(vl,tkn,in) */
/*---------------------------*/
frame_tkn_buff(vl,tkn,in) {
  int nn, n, d, s, v, lev;
  int pl;

  if (!XDISP) return;

  n = node_of_link(vl);
  d = dim_of_link(vl);
  s = sign_of_link(vl);
  v = virt_of_link(vl);
  lev = level_of_link(vl);
  if (vl < numnvlinks) {
    if (s>0 && !in) return;
    if (s<0 && in) return;
    nn = n;
  } else
    nn = get_next_node(n,d,s);
  
  if (tkn)
    if (TestFKill(tkn) || TestBKill(tkn)) 
      tkn = (-2);
    else
      tkn = msgs[UnToken(tkn)].circ_index + 1;

#ifndef NO_X
  if (in == IN) 
    frame_buff(nn,d,-s,v,IN,tkn,lev);
  else
    frame_buff(n,d,s,v,OUT,tkn,lev);
#endif
}

/*----------------------------*/
/* mark_tkn_buff(vl,tkn,in)   */
/*----------------------------*/
mark_tkn_buff(vl,tkn,in) {
  int nn, n, d, s, v, lev;
  
  if (!XDISP) return;

  n = node_of_link(vl);
  d = dim_of_link(vl);
  s = sign_of_link(vl);
  v = virt_of_link(vl);
  lev = level_of_link(vl);
  if (vl < numnvlinks) {
    if (s>0 && !in) return;
    if (s<0 && in) return;
    nn = n; 
  } else 
    nn = get_next_node(n,d,s);

  if (tkn)
    if (TestFKill(tkn) || TestBKill(tkn)) 
      tkn = (-2);
    else
      tkn = msgs[UnToken(tkn)].circ_index + 1;
  
#ifndef NO_X
  if (in==IN) 
    mark_buff(nn, d, -s, v, IN, tkn, lev);
  else 
    mark_buff(n, d, s, v, OUT, tkn, lev);
#endif
}

/*--------------------------------------------------------------*/
/* draw_vlink(vl,n)                                             */
/*--------------------------------------------------------------*/
draw_vlink(vl,n) {
  if (!XDISP) return;
#ifndef NO_X
  draw_link(vl,n);
#endif
  vlinks[vl].spec_col = 0;
}

/*--------------------------------------------------------------*/
/* uniform_dist()  Get an uniformly distributed random value    */
/*--------------------------------------------------------------*/
int uniform_dist (upper_bound)
     int upper_bound;
{
  float y, inc;
  int x;
  
  y = drand48 ();
  inc = 1.0/ (float) (upper_bound);
  
  x =  y / inc;
  return (x);
}

/*--------------------------------------------------------------*/
/* set_sim_time()  Set the sim time based on the cycle count    */
/*--------------------------------------------------------------*/
set_sim_time (i)
     int i;
{
  sim_time = i;
}

/*-----------------------------------------------------------*/
/* Falling exponent (ie. x(x-1)(x-2)...)                     */
/*-----------------------------------------------------------*/
float fall_exp (v, e)
     float v;
     int e;
{
  float prod;
  int i;
  prod = 1;
  for (i = 0; i < e; i++)
    prod = prod * (v - i);
  return prod;
}

/*-----------------------------------------------------------*/
/* Factorial n!                                              */
/*-----------------------------------------------------------*/
float factorial (n)
     float n;
{
  if (n <= 1.0)
    return 1;
  return n * factorial (n - 1);
}

/*-----------------------------------------------------------*/
/* v choose n                                                */
/*-----------------------------------------------------------*/
float choose (v, n)
     float v;
     int n;
{
  return fall_exp (v, n) / factorial ((float) n);
}

/*-----------------------------------------------------------*/
/* binomial                                                  */
/*-----------------------------------------------------------*/
double binomial (n, i, p)
     int n, i;
     float p;
{
  if (n < 0)
    return 0.0;
  if (i < 0 || i > n)
    return 0.0;
  return choose ((float) n, i) * pow (p, (float) i) * pow (1.0 - p, (float) (n - i));
}

/*--------------------------------------------------------------*/
/* init_stats()  Initialize the statistic variables             */
/*--------------------------------------------------------------*/
init_stats ()
{
  int i, j;

  control_moves = 0;
  cum_control_collide = 0;
  retry_failures = 0;
  src_kill_informs = 0;
  src_kill_inform_time = 0;

  memset(lat_hist,0,10000*sizeof(int));
  memset(su_hist,0,500*sizeof(int));
  memset(path_hist,0,50*sizeof(int));
  memset(mis_hist,0,10*sizeof(int));
  nu_c = 0;
  nu_s = 0;
  nu_B = 0;
  P_U = 0;
  P_B = 0;
  paths_checked = 0;
  t_head = 0;
  heads = 0;
  t_service = 0;
  t_served = 0;
  flits_delivered = 0;
  INIT = 1;
  for (i = 0; i <= 32; i++)
    {
      rdist[i] = 0;
      sdist[i] = 0;
      cdist[i] = 0;
      rhdist[i] = 0;
      rfcdist[i] = 0;
      rhcdist[i] = 0;
    }
  t_init = sim_time;

  injects = 0;
  setups = 0;
  tot_tacks = 0;
  tot_done = 0;

  tacklat_ave=0;
  lat_ave = 0;
  su_ave = 0;
  q_ave = 0;

  tacklat_sqave=0;
  lat_sqave = 0;
  su_sqave = 0;
  q_sqave = 0;

  path_length_ave = 0;
  misroutes_ave = 0;

  con_tacks = 0;
  con_su = 0;
  con_circs = 0;

  ave_tacks = 0;
  ave_su = 0;
  ave_circs = 0;

  backs_to_src = 0;
  p_blk = 0;
  blk_ave = 0;
  tot_traversed = 0;

  if (HYBRID)
  {
    lng_tot_done = 0;
    lng_lat_ave = 0;
  }


}

/*-----------------------------------------------------*/
/* pwr2(n)  Self explanatory                           */
/*-----------------------------------------------------*/
pwr2 (n)
     int n;
{
  int i, p;
  p = 1;
  for (i = 0; i < n; i++)
    p = p * 2;
  return p;
}


/*------------------------------------------------------------*/
/* get_next_node(n, d, sign)   Give next node for given plink */
/*------------------------------------------------------------*/
get_next_node(n, d, sign)
int n; int d; int sign;
 {
  int vl, pl;
  if (d < 0) return n;
  if (sign < 0) {
    /* Quick virtual link address */
    vl = (n << (dimlog + vlog)) + d + d + numnvlinks;
    pl = vlinks[vl].pl;
    return plinks[pl].Lnode;
  } else {
    pl = ((n << (dimlog-1)) + d) + NODES*plinknode;
    return plinks[pl].Rnode;
  }
}

/*-----------------------------------------------------*/
/* set_link_hist(n)   Set a nodes history vector       */
/*-----------------------------------------------------*/
set_link_hist (n)
     int n;
{
  int next;
  next = vlinks[n].next;
  vlinks[n].next = -1;
  vlinks[n].hist |= Hist_Mask (dim_of_link (next), sign_of_link (next));
}

/*------------------------------------------------------*/
/* init_hist(vl)   Initialize history vector for a link */
/*------------------------------------------------------*/
init_hist (vl)
     int vl;
{
  int d, v, s;
  vlinks[vl].hist = 0;
  d = dim_of_link (vl);
  s = sign_of_link (vl);
  for (v = 0; v < virts; v++)
    {
      vlinks[vl].hist |= Hist_Mask (d, -s);
    }
}

/*-----------------------------------------------------*/
/* find_msg()  Find the next free msg in msgs array    */
/*-----------------------------------------------------*/
find_msg ()
{
  int i;
  
  for (i = 0; i < MAXMESSAGE; i++)
    if (msgs[i].circ_index == (-1))
      return i;
  return MAXMESSAGE;
}

/*-----------------------------------------------------*/
/* find_circ()  Find the next free circ in circs array */
/*-----------------------------------------------------*/
find_circ ()
{
  int i;
  
  for (i = 0; i < MAXCIRC; i++)
    if (circs[i].src == (-1))
      return i;
  return MAXCIRC;
}

/*-----------------------------------------------------*/
/* int_cmp(int *a, int *b)  Used with qsort            */
/*-----------------------------------------------------*/
int_cmp(a,b)
int *a, *b;{
  if (*a>*b) return 1;
  if (*a<*b) return -1;
  return 0;
}

/*-----------------------------------------------------*/
/* parse_command_line(argc, argv)  ditto               */
/*-----------------------------------------------------*/
parse_command_line (argc, argv, disp)
     int argc, disp;
     char **argv;
{
  long l;
  int i, j, k, h[16];
  char c1, c2, c3, c4, rest[20];
  char file[40];
  float f;
  
  strcpy(THREAD_MAP, "LINEAR");
  /* Initialize with default values in case they leave some out. */
  FLIT_SIZE=2;
  EXECUTE_CYCLES=1;
  traffic = SYNTHETIC;
  SRC_QUEUE = 8;
  PROBE_LEAD = 1;
  dyn_cnt = 0;
  MESH=0;
  HSPOTS = 0;		/* Any hot spots? */
  HSPERCENT = 0;	/* Percent of hot spot traffic */
  NO_CTS = 0;		/* No CTS lookahead? */
  INIT = 0;
  MAP = 0;			/* Congestion mapping on? */
  backtrack = 0;
  virts = 2;
  node_virts = 2;
  XDISP = 0;			/* Visualization on/off toggle */
  FAULTS = 0;
  SLOW = 0;
  HALF = 0;
  DEMAND = 1;
  TRANSIENT = 1;
  DIST = 0;
  PER = 1000;
  MISROUTES = 0;			/* Misroute count for MB-m (LMB), DISHA */
  ABORT = 0;
  select_fnc = 0;
  BUFFERS = 2;
  DEBUG = 0;
  RADIUS = 3;
  SECURITY_LEVELS = 1;
  route_delay = 1;
  data_delay = 1;
  inject_rate = 1;
  comm_mech = WR;
  protocol = Ecube;
  RUN_TIME = 10000;
  MSGL = 32;
  RETRY = 10;
  dimensions = 2;
  width = (int *) malloc (sizeof (int) * dimensions);
  part = (int *) malloc (sizeof (int) * dimensions);
  num_dyn_faults = 0;

  HYBRID = 0;
  LMSGL = 512;
  LMSGPCT = 5;

  for (i = 0; i < dimensions; i++)
    {
      width[i] = 16;
      part[i] = 4;
    }
  link_time = 1;
  link_arb_time = 0;
  ack_time = 1;

#ifdef SET_PORT
  useinjvcs=2;
  usedelvcs=2;
#endif

#ifdef PLINK
  plinknode=1;
  pl_injection=1;
  pl_delivery=1;
#endif
  
  /* Parse the command line options */
  for (i = 1; i < argc; i++)
    {
      if (sscanf (argv[i], "TRACE=%s", trace_file_name) == 1) 
	{
	  traffic = TRACEDRIVEN;
	} 
      else
      if (sscanf (argv[i], "D=%d", &j) == 1)
	{
	  if (j > 0)
	    {
	      dimensions = j;
	      free (width);
	      free (part);
	      width = (int *) malloc (sizeof (int) * dimensions);
	      part = (int *) malloc (sizeof (int) * dimensions);
	    }
	}
      else if (sscanf (argv[i], "HALF=%d", &j) == 1)
	{
	  HALF = j;
	}
      else if (sscanf (argv[i], "SCOUT=%d", &j) == 1)
	{
	  PROBE_LEAD = j;
	}
      else if (sscanf (argv[i], "FLIT_SIZE=%d", &j) == 1)
	{
	  FLIT_SIZE = j;
	}
      else if (sscanf (argv[i], "PROFILE=%d", &j) == 1)
	{
	  PROFILE = j;
	}
      else if (sscanf (argv[i], "THREAD_OVER=%d", &j) == 1)
	{
	  THREAD_OVER = j;
	}
      else if (sscanf (argv[i], "THREAD_MAP=%s", rest) == 1)
	{
	  strncpy(THREAD_MAP, rest, 39);
	}
      else if (sscanf (argv[i], "OVER=%d", &j) == 1)
	{
	  RCV_OVER = j;
	  SEND_OVER = j;
	}
      else if (sscanf (argv[i], "PRE_OVER=%d", &j) == 1)
	{
	  PRE_OVER = j;
	}
      else if (sscanf (argv[i], "REL_OVER=%d", &j) == 1)
	{
	  REL_OVER = j;
	}
      else if (sscanf (argv[i], "OVER_PER=%d", &j) == 1)
	{
	  RCV_OVER_PER = j;
	  SEND_OVER_PER = j;
	}
      else if (sscanf (argv[i], "RCV_OVER=%d", &j) == 1)
	{
	  RCV_OVER = j;
	}
      else if (sscanf (argv[i], "RCV_OVER_PER=%d", &j) == 1)
	{
	  RCV_OVER_PER = j;
	}
      else if (sscanf (argv[i], "SEND_OVER=%d", &j) == 1)
	{
	  SEND_OVER = j;
	}
      else if (sscanf (argv[i], "SEND_OVER_PER=%d", &j) == 1)
	{
	  SEND_OVER_PER = j;
	}
      else if (sscanf (argv[i], "SRCQ=%d", &j) == 1)
	{
	  SRC_QUEUE = j;
	}
      else if (sscanf (argv[i], "DEMAND=%d", &j) == 1)
	{
	  DEMAND = j;
	}
      else if (sscanf (argv[i], "DEBUG=%d", &j) == 1)
	{
	  DEBUG = j;
	}
      else if (sscanf (argv[i], "SECURITY_LEVELS=%d", &j) == 1)
	{
	  SECURITY_LEVELS = j;
	  if ( SECURITY_LEVELS < 1 ) SECURITY_LEVELS = 1;
	}
      else if (sscanf (argv[i], "RADIUS=%d", &RAD) == 1)
	{
	  if (RAD >= 1) RADIUS = RAD;
	}
      else if (sscanf (argv[i], "BUFFERS=%d", &j) == 1)
	{
	  BUFFERS = j;
	}
      else if (sscanf (argv[i], "RETRY=%d", &j) == 1)
	{
	  RETRY = j;
	}
      else if (sscanf (argv[i], "RDELAY=%d", &j) == 1)
	{
	  route_delay = j;
	}
      else if (sscanf (argv[i], "DDELAY=%d", &j) == 1)
	{
	  data_delay = j;
	}
      else if (sscanf (argv[i], "ADELAY=%d", &j) == 1)
	{
	  ack_time = j;
	}
      else if (sscanf (argv[i], "LDELAY=%d", &j) == 1)
	{
	  link_time = j;
	}
      else if (sscanf (argv[i], "LARB=%d", &j) == 1)
	{
	  link_arb_time = j;
	}
      else if (sscanf (argv[i], "SEED=%ld", &l) == 1)
	{
	  seed = l;
	  random_seed(seed);
	}
      else if (sscanf (argv[i], "FLIP=%d", &j) == 1)
	{
	  if (j != 0)
	    flip_dim = 1;
	  else
	    flip_dim = 0;
	}
      else if (sscanf (argv[i], "VARY=%d", &j) == 1)
	{
	  if (j != 0)
	    vary_virts = 1;
	  else
	    vary_virts = 0;
	}
      else if (sscanf (argv[i], "HSPERCENT=%d", &j) == 1)
	{
	  if (j >= 1 && j <= 99)
	    HSPERCENT = j;
	  else 
	    HSPERCENT = 20;
	}
      else if (sscanf (argv[i], "HSPOTS=%d", &j) == 1)
	{
	  HSPOTS = j;
	  hsnodes = (int *) malloc (HSPOTS * sizeof (int));
	  for (j = 0; j < HSPOTS; j++)
	    hsnodes[j] = (-1);
	}
      else if (sscanf (argv[i], "HSPLACE=%s", rest) == 1)
	{
	  j = 0;
	  for (k = 0; k < strlen (rest) && k < dimensions; k++)
	    {
	      c1 = rest[k];
	      if (c1 >= '0' && c1 <= '9')
		{
		  n[k] = c1 - '0';
		}
	      else if (c1>='A' && c1 <='Z')
		n[k] = c1 - 'A' + 10;
	      else if (c1>='a' && c1 <='z')
		n[k] = c1 - 'a' + 37;
	      j += n[k];
	      if (k < dimensions - 1)
		j = j * width[k];
	    }
	  for (k = 0; k < HSPOTS; k++)
	    if (hsnodes[k] == (-1))
	      {
		hsnodes[k] = j;
		break;
	      }
	}
      else if (sscanf (argv[i], "MISROUTES=%d", &j) == 1)
	{
	  MISROUTES = j;
	}
      else if (sscanf (argv[i], "NO_CTS=%d", &j) == 1)
	{
	  NO_CTS = j;
	}
      else if (sscanf (argv[i], "MAP=%d", &j) == 1)
	{
	  MAP = j;
	}
      else if (sscanf (argv[i], "X=%d", &j) == 1)
	{
	  if (j > 0)
	    XDISP = j;
	}
      else if (sscanf (argv[i], "DYN=%d", &j) == 1)
	{
	  physical_faults = 0;
	  num_dyn_faults = j;
	}
      else if (sscanf (argv[i], "PDYN=%d", &j) == 1)
	{
	  physical_faults = 1;
	  num_dyn_faults = j;
	}
      else if (sscanf (argv[i], "SLOW=%d", &j) == 1)
	{
	  if (j > 0)
	    SLOW = j;
	}
      else if (sscanf (argv[i], "FAULTS=%d", &j) == 1)
	{
	  if (j > 0)
	    FAULTS = j;
	}
      else if (sscanf (argv[i], "PFAULTS=%d", &j) == 1)
	{
	  physical_faults=1;
	  if (j > 0)
	    FAULTS = j;
	}
      /* yungho : number of reception queues 
      else if (sscanf (argv[i], "QNUM=%d",&j) == 1)
	{
	  if (j>= 0 && j <=8)
	    QNUM=j;
	  else
	    QNUM=2;
	}
      */
      else if (sscanf (argv[i], "ORDER=%d",&j) == 1)
	{
	  if (j>= 0 && j <= 3)
	    ORDER=j;
	  else
	    ORDER=0;
	}
      else if (sscanf (argv[i], "PER=%d", &j) == 1)
	{
	  if (j > 0)
	    PER = j;
	}
      else if (sscanf (argv[i], "TRANS=%d", &j) == 1)
	{
	  TRANSIENT = j;
	}
      else if (sscanf (argv[i], "COMM=%c%c", &c1,&c2) > 0)
	{
	  if (c1 == 'W')
	    comm_mech = WR;
	  else if (c1 == 'P')
	    comm_mech = PCS;
	  else if (c1 == 'R')
	    comm_mech = RECON;
	  else if (c1 == 'A') 
	    if (c2 == 'P')
	      comm_mech = APCS;
	    else
	      comm_mech = AWR;
	}
      else if (sscanf (argv[i], "PROTO=%c", &c1) == 1)
	{
	  switch (c1)
	    {
	    case 'E':
	      protocol = Ecube;
	      break;
	    case 'D':
	      protocol = DP;
	      break;
	    case 'M':
	      protocol = DPM;
	      break;
	    case 'O':
	      protocol = DP;
	      select_fnc = ORD;
	      break;
	    case 'C':
	      protocol = DP;
	      select_fnc = MIN_CON;
	      break;
	    case 'N':
	      protocol = NEG;
	      MESH=1;
	      break;
	    case 'R':
	      protocol = DR;
	      break;
	    case 'L':
	      protocol = MBM;
	      comm_mech = PCS;
	      break;
	    case 'P':
	      protocol = MBMP;
	      comm_mech = PCS;
	      break;
	    case 'B':
	      protocol = EPB;
	      comm_mech = PCS;
	      break;
	    case 'T':
	      protocol = TPB;
	      comm_mech = PCS;
	      break;
	    case 'A':
	      protocol = A1;
	      comm_mech = PCS;
	      break;
	    case 'F':
	      protocol = MBMSWF;
	      comm_mech = PCS;
	      break;
	    case 'S':
	      protocol = SBT;
	      comm_mech = PCS;
	      break;
	    case 'I':
	      protocol = DOR;
	      break;
	    case 'Z':
	      protocol = SBM;
	      comm_mech = RECON;
	      break;
	    case 'Y':
	      protocol = DISHA_TORUS;
	      virts = 8;
	      node_virts = 8;
	      break;
	    case 'w':
	      protocol = DISHA_TORUS_SYNCH_TOKEN;
	      virts = 8;
	      node_virts = 8;
              NEW_DISHA_FEATURES = 1;
	      break;
	    case 's':
	      protocol = DISHA_DOR_TORUS_SYNCH_TOKEN;
	      virts = 8;
	      node_virts = 8;
              NEW_DISHA_FEATURES = 1;
	      break;
	    case 'z':
	      protocol = DD_MIN_ADAPTIVE_BIDIR_TORUS;
	      break;
	    case 'y':
	      protocol = DD_MIN_ADAPTIVE_UNIDIR_TORUS;
	      break;
	    case 'u':
	      protocol = DD_NON_MIN_ADAPTIVE_BIDIR_TORUS;
	      break;
	    case 'v':
	      protocol = DD_NON_MIN_ADAPTIVE_UNIDIR_TORUS;
	      break;
	    case 'd':
	      protocol = DD_DETXY_BIDIR_TOR;
	      break;
	    case 'e':
	      protocol = DD_DETXY_UNIDIR_TOR;
	      break;
	    case 't':
	      protocol = DD_TEST_ADAPTIVE;
	      break;
	    case 'W':
	      protocol = DISHA_MESH;
	      break;
	    case 'x':
	      protocol = DISHA_MESH_ONEDB;
	      break;
	    case 'X':
	      protocol = DUATO;
	      break;
	    }
	}
      else if (sscanf (argv[i], "SELECT=%c", &c1) == 1)
	{
	  switch (c1)
	    {
	    case 'N':
	      select_fnc = NORM;
	      break;
	    case 'M':
	      select_fnc = MIN_CON;
	      break;
	    case 'O':
	      select_fnc = ORD;
	      break;
	    }
	}
      else if (sscanf (argv[i], "PL=%d", &j) == 1)
	{
	  if (j > 0)
	    PL = j;
	}
      else if (sscanf (argv[i], "MSGL=%d", &j) == 1)
	{
	  if (j > 0)
	    MSGL = j;
	}

      else if (sscanf (argv[i], "HYBRID=%d", &j) == 1)
	{
	  if (j > 0)
	    HYBRID = 1;
	}

      else if (sscanf (argv[i], "LMSGL=%d", &j) == 1)
	{
	  if (j > 0)
	    LMSGL = j;
	}

      else if (sscanf (argv[i], "LMSGPCT=%d", &j) == 1)
	{
	  if (j > 0)
	    LMSGPCT = j;
	}

      else if (sscanf (argv[i], "INJECT=%d", &j) == 1)
	{
	  inject_rate = j;
	  traffic = SYNTHETIC;
	  /* The inject rate is the number of msgs injected */
	  /* per node over PER flit cycles.  A negative     */
	  /* inject rate indicates a saturation load.       */
	}
      else if (sscanf (argv[i], "DUR=%d", &j) == 1)
	{
	  if (j > 0)
	    RUN_TIME = j;
	}
      else if (sscanf (argv[i], "VIRTS=%d", &j) == 1)
	{
	  virts = j;
	  node_virts = j;
	  use_virts = j;
	  useinjvcs=j;
	  usedelvcs=j;
	}
       else if (sscanf (argv[i], "USEVIRTS=%d", &j) == 1)
	{
	  use_virts = j;
	  useinjvcs=j;
	  usedelvcs=j;
	}
#ifdef SET_PORT
	else if (sscanf (argv[i], "USEDELVC=%d", &j) == 1)
        {
          usedelvcs = j;
        }
      else if (sscanf (argv[i], "USEINJVC=%d", &j) == 1)
        {
          useinjvcs = j;
        }
#endif 
#ifdef PLINK
        else if (sscanf (argv[i], "PLINK=%d", &j) == 1)
	{
	  plinknode = j;
	  pl_delivery=j;
	  pl_injection=j;
	}
        else if (sscanf (argv[i], "PLINKDEL=%d", &j) == 1)
        {
          pl_delivery = j;
        }
        else if (sscanf (argv[i], "PLINKINJ=%d", &j) == 1)
        {
          pl_injection = j;
        }
#endif
     else if (sscanf (argv[i], "DIST=%d", &j) == 1)
	{
	  DIST = j;
	}
      else if (sscanf (argv[i], "SIZE=%s", rest) > 0)
	{
	  /* Size is of the form ABC... where the dimentions of the network */
	  /*  are 2^A * 2^B * 2^C * ... */
	  
	  for (k = 0; k < strlen (rest) && k < dimensions; k++)
	    {
	      c1 = rest[k];
	      if (c1 >= '0' && c1 <= '9')
		{
		  part[k] = c1 - '0';
		  width[k] = pwr2 (part[k]);
		}
	    }
	}
	/* newly added to support scaling */ 
      else if (sscanf (argv[i], "SCALE=%d", &j) == 1)
	{
	  SCALE = j;
	}
	/* newly added to support testing */ 
      else if (sscanf (argv[i], "TEST=%d", &j) == 1)
	{
	  TEST = j;
	}
	/* newly added to support variable time outs - for DISHA */ 
      else if (sscanf (argv[i], "WAITTIME=%d", &j) == 1)
	{
	  WAITTIME = j;
	}
	/* newly added to support variable time outs - for DISHA */ 
      else if (sscanf (argv[i], "CUMWAIT=%d", &j) == 1)
	{
	  CUMWAIT = j;
	}
	/* newly added to support changing speed of synchronous token - for DISHA */ 
      else if (sscanf (argv[i], "TOKENSPEED=%d", &j) == 1)
	{
	  TOKENSPEED = j;
	  HALFTOKENSPEED = TOKENSPEED / 2;
	}

	/* newly added to support Deadlock Detection (turn on/off cycle detection) */ 
     else if (sscanf (argv[i], "CD=%d", &j) == 1)
	{
	  CD = j;
	}
	/* newly added to support Deadlock Detection (turn ON/OFF deadlock detection) */ 
	/* Note: turning ON Deadlock Detection also turns on Cycle Detection */ 
      else if (sscanf (argv[i], "DD=%d", &j) == 1)
	{
	  DD = j;
	  if (DD)
            CD=1;
	}

	/* newly added to support Deadlock Detection (sets frequency of Cycle Detection algorithm) */ 
     else if (sscanf (argv[i], "CDFREQ=%d", &j) == 1)
	{
	  CDFREQ = j;
	}
	/* newly added to support Deadlock Detection (sets frequency of Deadlock Detection algorithm)   */ 
	/* Note: Deadlock Detection algorithm is not allowed to be run more often then Cycle Detection  */ 
      else if (sscanf (argv[i], "DDFREQ=%d", &j) == 1)
	{
	  DDFREQ = j;
	  if (DDFREQ < CDFREQ)
		DDFREQ = CDFREQ;
	}
	/* newly added to support Deadlock Detection (turns ON/OFF the printing of data structures at every CDFREQ) */ 
	/* Note: dedault is OFF                                                                                     */ 
      else if (sscanf (argv[i], "DDTEST=%d", &j) == 1)
	{
	  DDTEST = j;
	}

      else
	printf("Did not understand command line argument %s\n",argv[i]);
    }
  
  if (disp)
    {
      print_options();
    }
  cum_part = (int *)malloc(sizeof(int)*dimensions);
  cum_part[0]=0;
  for (i=1; i<dimensions; i++) 
    cum_part[i] = cum_part[i-1]+part[i-1];
  
  if (HALF)
  {
    MSGL = 2 * MSGL;
    if (HYBRID)
      LMSGL = 2 * LMSGL;
  }

  if (num_dyn_faults) {
    dyn_fault_times = (int *)malloc(sizeof(int)*num_dyn_faults);
    for (i=0; i<num_dyn_faults; i++) {
      dyn_fault_times[i] = RUN_TIME*drand48();
    }
    qsort(dyn_fault_times, num_dyn_faults, sizeof(int), int_cmp);
  }
}

print_options() {
  int i;
  printf ("Routing Protocol: ");
  switch (protocol)
    {
    case Ecube:
      printf ("E-cube\n");
      sprintf (proto_name,"E-cube");
      break;
    case DP:
      printf ("Duato\n");
      sprintf (proto_name,"Duato");
      break;
    case DPM:
      printf ("Duato-m\n");
      sprintf (proto_name,"Duato-m");
      break;
    case NEG:
      printf ("Negative-first (mesh)\n");
      sprintf (proto_name,"Negative-first");
      break;
    case DR:
      printf ("Dally DR\n");
      sprintf (proto_name,"Dally DR");
      break;
    case EPB:
      printf ("EPB\n");
      sprintf (proto_name,"EPB");
      break;
    case MBM:
      printf ("MB-m\n");
      sprintf (proto_name,"MB-m");
      break;
    case MBMP:
      printf ("MB-m+\n");
      sprintf (proto_name,"MB-m+");
      break;
    case TPB:
      printf ("Two-phase backtracking\n");
      sprintf (proto_name,"Two-phase backtracking");
      break;
    case A1:
      printf ("A1\n");
      sprintf (proto_name,"A1");
      break;
    case MBMSWF:
      printf ("MB-m SW\n");
      sprintf (proto_name,"MB-m SW");
      break;
    case SBT:
      printf ("Selective Backtracking\n");
      sprintf (proto_name,"Selective Backtracking");
      break;
    case DOR:
      printf ("Directional Order Routing (oblivious)\n");
      sprintf (proto_name,"Directional Order Routing (oblivious)");
      break;
    case SBM:
      printf ("Scouting Backtracking - m\n");
      sprintf (proto_name,"Scouting Backtracking - m");
      break;
    case DISHA_TORUS:
      printf ("DISHA for a Torus (Timeout = %d)\n", WAITTIME);
      sprintf (proto_name,"DISHA for a Torus\n");
      break;
    case DISHA_TORUS_SYNCH_TOKEN:
      printf ("DISHA for a Torus (w/ Synhcronous Token) (Timeout = %d)\n", WAITTIME);
      sprintf (proto_name,"DISHA for a Torus (w/ Synhcronous Token)\n");
      break;
    case DD_MIN_ADAPTIVE_BIDIR_TORUS:
      printf ("Minimal Adaptive (w/ Deadlock Detection)\n");
      sprintf (proto_name,"Minimal Adaptive (w/ Deadlock Detection)\n");
      break;
    case DD_NON_MIN_ADAPTIVE_BIDIR_TORUS:
      printf ("Non-Minimal Adaptive (w/ Deadlock Detection)\n");
      sprintf (proto_name,"Non-Minimal Adaptive (w/ Deadlock Detection)\n");
      break;
    case DD_TEST_ADAPTIVE:
      printf ("Adaptive (for Testing !) (w/ Deadlock Detection)\n");
      sprintf (proto_name,"Adaptive (for Testing !) (w/ Deadlock Detection)\n");
      break;
    }
  printf ("Selection Function: ");
  switch (select_fnc)
    {
    case NORM:
      printf ("Virtual Link Cycling\n");
      break;
    case ORD:
      printf ("Dimension Order Link Cycling\n");
      break;
    case MIN_CON:
      printf ("Minimum Congestion\n");
      break;
    }
  printf ("Switching Technique: ");
  switch(comm_mech) {
  case WR:	printf ("Wormhole Routing\n"); break;
  case PCS:	printf ("Pipelined Circuit-Switching\n"); break;
  case APCS: printf("Acknowledged Pipelined Circuit-Switching\n"); break;
  case AWR: printf("Acknowledged Wormhole Routing\n"); break;
  case RECON: printf("Reconnaisance Routing\n"); break;
  }
  printf ("Buffer Depth: %d flits\n", BUFFERS);
  printf ("Debug is %d\n", DEBUG);
  printf ("Using %d Security Level(s)\n", SECURITY_LEVELS);
  printf ("Exhaustive Search Radius (MB-m SW): %d\n",RADIUS);
  printf ("Number of misroutes allowed : %d\n",MISROUTES);
  
  if (MAP)
    printf ("Link congestion mapping on.\n");
  if (XDISP)
    printf ("X-windows display!\n");
  if (SLOW)
    printf ("Slow stepping enabled!\n");
  if (!TRANSIENT)
    printf ("Transient stats disabled.\n");
  printf ("Inject %d Message(s) Per (PER=) %d Cycles Per Node\n",
	  inject_rate, PER);


/* Incorrect ... should not be hardcoded
  printf ("Load Rate = %f\n",1000/(PER*15.625));
*/

  if (protocol==DOR && ORDER==0)
    printf("DOR with order y+x+y-x- (toroid)\n");
  else if (protocol==DOR && ORDER==1)
    printf("DOR with order y+x+x-y- (toroid)\n");
  else if (protocol==DOR && ORDER==2) {
    MESH=1;
    printf("DOR with order y+x+y-x- (mesh)\n");
  } else if (protocol==DOR && ORDER==3) {
    printf("DOR with order y+x+x-y- (mesh)\n");
    MESH = 1;
  } else if (protocol==Ecube && ORDER==0)
    printf("Ecube (toroid)\n");
  else if (protocol==Ecube && ORDER==1) {
    printf("Ecube (mesh)\n");
    MESH=1; 
  }
  printf ("Message Length: %d flits\n", MSGL);
  if (HYBRID)
    printf ("Hybrid Traffic Enabled with %d Percent Long Messages (of Length %d flits)\n", LMSGPCT, LMSGL);
  if (DIST == (0))
    printf ("Random Communication Pattern\n");
  if (DIST == (-100))
    printf ("Bit-Reversal Communication Pattern\n");
  if (DIST == (-900))
    printf ("Matrix Transpose Communication Pattern\n");
  if (DIST==(-999))
    printf("Perfect Shuffle Communication Pattern\n");
  if (DIST==(-1000))
    printf("Flipped Bits Communication Pattern\n");
  if (DIST==(-1001))
    printf("Hot Spot Communication Pattern\n");

  printf ("Simulation Duration: %d cycles\n", RUN_TIME);

  printf ("Network Configured as: ");
  for (i = 0; i < dimensions - 1; i++)
    printf ("%d X ", pwr2 (part[i]));
  printf ("%d\n", pwr2 (part[dimensions - 1]));
  if (FAULTS) {
    printf ("%d static faults\n", FAULTS);
    if (physical_faults) 
      printf("\tPhysical failures\n");
  }
  if (num_dyn_faults) {
    printf("%d Dynamic faults\n",
	   num_dyn_faults);
    if (physical_faults) 
      printf("\tPhysical failures\n");
  }
  if (HALF)
    printf ("Half-duplex\n");
  else 
    {
      printf ("Full-duplex\n");
    }
}

/*----------------------------------------------------------------*/
/* get_coords() -- Breaks node down into its respective dimension */
/*                 coordinates.                                   */
/*----------------------------------------------------------------*/
get_coords (src, n)
     int src, n[16];
{
  register int j, nd;
  
  if (src < 0 || src >= NODES)
    {
      printf ("Node %d out of range!\n", src);
      exit (1);
      src %= NODES;
    }
  nd = src;
  for (j = 0; j < dimensions; j++)
    {
      n[j] = nd % width[j];
      nd = (nd >> part[j]);
    }
}

/*---------------------------------------------------------------*/
/* get_addr() -- Converts back from coords to addresses          */
/*---------------------------------------------------------------*/
get_addr (n)
     int n[16];
{
  int nd, j, w;
  
  nd = n[0];
  for (j = 1; j < dimensions; j++)
    {
      nd |= (n[j] << cum_part[j]);
    }
  if (nd < 0 || nd >= NODES)
    {
      printf ("Node out of range (get_addr)\n");
      abort ();
      nd %= NODES;
    }
  return nd;
}

int t[16];
/*-------------------------------------------------------------------*/
/* print_coords(n)  Print the coords of a node                       */
/*-------------------------------------------------------------------*/
print_coords (n)
     int n;
{
  int j;
  get_coords (n, t);
  printf ("\t");
  for (j = 0; j < dimensions; j++)
    printf ("%d ", t[j]);
  printf ("\n");
}

/*-------------------------------------------------------------------*/
/* print_link(vl)  Print the location  of a link                     */
/*-------------------------------------------------------------------*/
print_link (vl)
     int vl;
{
  int n,d,s,v,nn,j;
  
  n = node_of_link(vl);
  d = dim_of_link(vl);
  s = sign_of_link(vl);
  v = virt_of_link(vl);
  nn = calc_next(n,d,s);
  get_coords (n, t);
  for (j = 0; j < dimensions; j++)
    printf ("%d ", t[j]);
  printf (" --> ");
  get_coords (nn, t);
  for (j = 0; j < dimensions; j++)
    printf ("%d ", t[j]);
  printf(" (%d)\n",v);
}

/*-------------------------------------------------------------------*/
/* calc_next()  -- Given a node, dimension and delta, give the       */
/*                 node number on the other side of the link.        */
/*-------------------------------------------------------------------*/
calc_next (n, j, i)
     int n;
     int j, i;
{
  int k;
  
  get_coords (n, t1);
  t1[j] = (t1[j] + i + width[j]) % width[j];
  return get_addr (t1);
}


/*--------------------------------*/
/* virt_of_link()                 */
/*--------------------------------*/
virt_of_link (l)
{
  if (l < numnvlinks)
    return (l>>1)%node_virts;
  else
    return ((l-numnvlinks) >> dimlog) % virts;
}

/*--------------------------------*/
/* node_of_link()                 */
/*--------------------------------*/
node_of_link (l)
{
  if (l < numnvlinks) 
#ifndef PLINK
    return (l >> (nvlog+1));
#else
    return (l >> (nvlog+1))/plinknode;
#endif
  else
    return (((l-numnvlinks) % numvlinks) >> (dimlog + vlog));
}

/*--------------------------------*/
/* dim_of_link(l)                 */
/*--------------------------------*/
dim_of_link (l)
{
  int d;
  if (l < numnvlinks) return -1;
  d = ((l-numnvlinks) >> 1) % dimensions;
  return d;
}

/*--------------------------------*/
/* sign_of_link(l)                */
/*--------------------------------*/
sign_of_link (l)
{
  if (l < numnvlinks) 
    return ( l%2 << 1) - 1;
  else
    return (((l % 2) << 1) - 1);
}

/*--------------------------------*/
/* level_of_link(l)               */
/*--------------------------------*/
level_of_link (l)
{
  return (vlinks[l].security_level);
}


/*--------------------------------*/
/* num_of_plinknode(l)               */
/*--------------------------------*/

num_of_plinknode(l)
{
  if(l>= numnvlinks)
    printf("Error at num of plink\n");
  else
    return (l >> (nvlog+1))%plinknode;
}

/*--------------------------------*/
/* get_complement(l)              */
/*--------------------------------*/
get_complement (l)
{
  int n, d, s, v, pl, comp, lev;
#ifdef PLINK
  int npl;
#endif

  n = node_of_link (l);
  s = sign_of_link (l);
  v = virt_of_link (l);
  lev = level_of_link (l);
  if (l < numnvlinks) {
#ifdef PLINK
  npl = num_of_plinknode(l);
  comp = nlink_addr(n , v, (1-s)/2, lev, npl);
#else
    comp = nlink_addr(n, v, (1-s)/2, lev);
#endif
    return comp;
  }
  d = dim_of_link (l);
  n = get_next_node (n, d, s);
  comp = link_addr (n, d, -s, v, lev);
  return comp;
}

/**********************************************************/
/* Network handling routines                              */
/**********************************************************/

/*----------------------------------------------------------------------*/
/* create_network()  Create the data structures for the network         */
/*----------------------------------------------------------------------*/
create_network ()
{
  int i, j, k, bits, v, vl, w;
  int f, fcnt, match;

#ifdef PLINK
  int l;
#endif

  activity = 0;
  t_next = 0.0;
  i = 1;
  dimlog = 0;
  while (i < dimensions)
    {			/* Find dimlog */
      i *= 2;
      dimlog++;
    }
  dimlog++;
  
  vlog = 0;
  i = 1;
  while (i < virts)
    {			/* Find vlog */
      i *= 2;
      vlog++;
    }
  nvlog = 0;
  i = 1;
  while (i < node_virts)
    {			/* Find nvlog */
      i *= 2;
      nvlog++;
    }

  /* messages for testing - Sugath 
  printf("******DIMLOG: %d\n", dimlog);
  printf("******VLOG: %d\n", vlog);
  printf("******NVLOG: %d\n", nvlog);
  */

  bits = 0;			/* Calculate system size (NODES) */
  for (i = 0; i < dimensions; i++)
    bits += part[i];
  nlog = bits;
  /* Initialize messages */
  for (i = 0; i < MAXMESSAGE; i++)
    {
      msgs[i].circ_index = (-1);
    }
  /* Initialize circuits */
  for (i = 0; i < MAXCIRC; i++)
    {
      circs[i].src = (-1);
    }
  
  NODES = 1;			/* Allocate nodes */
  for (i = 0; i < bits; i++)
    NODES *= 2;
  printf ("%d Nodes\n", NODES);
  nodes = (node *) malloc (NODES * sizeof (node));
  if (!nodes)
    {
      printf ("Out of memory!\n");
      exit (0);
    }
  if (comm_mech == RECON)
    printf("PROBE_LEAD - %d\n",PROBE_LEAD);
  for (i = 0; i < NODES; i++)
    {
#ifndef PLINK
      nodes[i].send = (int *) malloc(sizeof(int)*node_virts);
      nodes[i].generate = (int *) malloc(sizeof(int)*node_virts);
      nodes[i].consume = (int *) malloc(sizeof(int)*node_virts);
      for (j=0; j<node_virts; j++) {
	nodes[i].send[j] = 0;
	nodes[i].generate[j] = 0;
	nodes[i].consume[j] = 0;
      }
#else
      nodes[i].send = (int *) malloc(sizeof(int)*node_virts*plinknode);
      nodes[i].generate = (int *) malloc(sizeof(int)*node_virts*plinknode);
      nodes[i].consume = (int *) malloc(sizeof(int)*node_virts*plinknode);
      for (j=0; j<(node_virts*plinknode); j++) {
	nodes[i].send[j] = 0;
	nodes[i].generate[j] = 0;
	nodes[i].consume[j] = 0;
      }
#endif
      nodes[i].security_level = 0;
      nodes[i].queue = (int *)malloc(sizeof(int)*SRC_QUEUE);
      for (j = 0; j < SRC_QUEUE; j++)/* Clear source queues */
	nodes[i].queue[j] = (-1);
      nodes[i].num_head = 0;
      for (j = 0; j < 4; j++)/* Clear header queues */
	nodes[i].headers[j] = (-1);
      nodes[i].idle = 0;
      nodes[i].over = 0;
      nodes[i].disha_token = 0;
      nodes[i].disha_token_siezed = 0;
      nodes[i].disha_deadlock_buffer_used = 0;

    }
  if (HSPOTS)
    {
      for (j = 0; j < HSPOTS; j++)
	if (hsnodes[j] == (-1))
	  {
	    do
	      {
		i = (int) (drand48 ()* NODES);
		match = 0;
		for (k = 0; k < j; k++)
		  if (hsnodes[k] == i)
		    match = 1;
	      }
	    while (match);
	    hsnodes[j] = i;
	  }
    }
  if (HSPOTS)
    {
      printf ("Hot spots enabled!\n");
      printf ("Hot spot locations:\n");
      for (i = 0; i < HSPOTS; i++)
	print_coords (hsnodes[i]);
    }
  /* Allocate virtual channels */
  numvlinks = NODES << (dimlog + vlog);
  numnvlinks = ( NODES << (nvlog+1))*plinknode;
  printf ("%d Virtual Channels\n", numvlinks);
  
  /* allocate same number of vlinks, but multiply by the number of */
  /* security levels so that we can run multiple virtual networks */
  vlinks = (vlane *) malloc (sizeof (vlane) * (numvlinks+ numnvlinks) 
			     * SECURITY_LEVELS);
  corrvlinks = (cvlane *) malloc (sizeof (cvlane) * (numvlinks+ numnvlinks) 
				  * SECURITY_LEVELS);
  compvlinks = (cvlane *) malloc (sizeof (cvlane) * (numvlinks+ numnvlinks) 
				  * SECURITY_LEVELS);
  complement = (int *) malloc(sizeof(int) * (numvlinks+ numnvlinks) 
			      * SECURITY_LEVELS+1);
  for (i = 0; i < (numvlinks + numnvlinks) * SECURITY_LEVELS; i++) {
    corrvlinks[i].out_buffer = 0; /* Control channels */
    compvlinks[i].out_buffer = 0;
    corrvlinks[i].in_buffer = 0;
    compvlinks[i].in_buffer = 0;
    corrvlinks[i].next = (-1);
    compvlinks[i].next = (-1);
    corrvlinks[i].next_in = 0;
    compvlinks[i].next_in = 0;

    vlinks[i].back_block = 0;
    vlinks[i].forward_block = 0;
    vlinks[i].busy = 0;
    vlinks[i].data = 0;

    vlinks[i].out_buffer = 0;
    vlinks[i].in_buffer = (int *)malloc(sizeof(int)*BUFFERS);
    if (!vlinks[i].in_buffer) {
      printf("Out of memory on vlink %d\n",i);
      abort();
    }
    for (j = 0; j < BUFFERS; j++)
      {
	vlinks[i].in_buffer[j] = 0;
      }
    vlinks[i].in_head = 0;
    vlinks[i].in_tail = 0;
    vlinks[i].in_buf = 0;
    
    vlinks[i].hist = 0;
    vlinks[i].next = (-1);
    vlinks[i].hist = 0;
    vlinks[i].data = 0;

    vlinks[i].spec_col = 0;

    /* for now, lets make the array "blocks" at each security */
    /* level */
    if (i >= numnvlinks) 
	vlinks[i].security_level = (i-numnvlinks) / numvlinks;
      else
	vlinks[i].security_level = (i / numnvlinks);
    }
  
  /* Allocate physical links */
  numplinks = NODES << (dimlog - 1);
  printf ("%d Physical Channels\n", numplinks);
  plinks = (plink *) malloc (sizeof (plink) * (numplinks+(NODES*plinknode)));

  if (!plinks) {
    printf("Out of memory at plinks\n");
    abort();
  }
  for (i = 0; i < (numplinks+NODES*plinknode); i++)
    {
      plinks[i].used = 0;
      if (i<(NODES*plinknode) ) {
	plinks[i].Lnode = i/plinknode;
	plinks[i].Rnode = i/plinknode;
      }
      plinks[i].security_level = 0;
      plinks[i].LR = L;
      plinks[i].startlane = 0;
      plinks[i].Llane = 0;
      plinks[i].Rlane = 0;
      plinks[i].nsl = 0;
      plinks[i].nsr = 0;
      plinks[i].csl = 0;
      plinks[i].csr = 0;
      plinks[i].ncl = 0;
      plinks[i].ncr = 0;
      plinks[i].ccl = 0;
      plinks[i].ccr = 0;
      plinks[i].nrl = 0;
      plinks[i].nrr = 0;
      plinks[i].crl = 0;
      plinks[i].crr = 0;
      plinks[i].arbcnt = 0;
      plinks[i].lcnt = 0;
      plinks[i].rcnt = 0;
      plinks[i].probe_register = PROBE_LEAD;
    }
  for (k = 0; k < NODES; k++)
    {
      /* Connect plinks to nodes & virtual channels */
#ifdef PLINK
      for( l=0;l < plinknode;l++) {
	for (v=0; v<node_virts; v++) { /* Do the node connections */
	  vl = nlink_addr(k, v, 1, 0, l);
	  vlinks[vl].pl = k*plinknode+l;
	  vl = nlink_addr(k, v, 0, 0, l);
	  vlinks[vl].pl = k*plinknode+l;
	}
      }
#else
      for (v=0; v<node_virts; v++) { /* Do the node connections */
	vl = nlink_addr(k, v, 1, 0);
	vlinks[vl].pl = k;
	vl = nlink_addr(k, v, 0, 0);
	vlinks[vl].pl = k;
      }
#endif
      for (j = 0; j < dimensions; j++) /* Do the network connections */
	{
	  i = plink_addr(k, j);
	  plinks[i].dim = j;
	  plinks[i].Lnode = k;
	  plinks[i].Rnode = calc_next (k, j, 1);
	 /* printf(" pl = %d, Lnode=%d,R =%d\n",i,k,plinks[i].Rnode);*/
	  for (v = 0; v < virts; v++)
	    {		/* Associate each vlink with a plink */
	      for ( w = 0; w < SECURITY_LEVELS ; w++) 
		{
		  vl = link_addr (k, j, 1, v, w);
		  vlinks[vl].pl = i;
		  vl = link_addr (plinks[i].Rnode, j, -1, v, w);
		  vlinks[vl].pl = i;
		}
	    }
	}
    }
  
  if (XDISP)
    {
      for (j = 0; j < dimensions; j++)
	if ( DEBUG >= 3) {
	  printf ("j=%d part[j]=%d width[j]=%d\n", j, part[j],
		  width[j]);
	}
#ifndef NO_X
      /*	 xinit (width[0], width[1]); 
		 draw_network (width[0], width[1]); 
       */
#endif
    }
  
  complement[0] = (-1);
  complement++;

  for (i = 0; i < (numvlinks+numnvlinks) * SECURITY_LEVELS; i++)
    complement[i] = get_complement(i);

#ifdef SET_PORT
  printf("%d VCs Per Internode Physical Link\n", use_virts);
  printf("%d VCs Per Injection Physical Link\n", useinjvcs);
  printf("%d VCs Per Delivery  Physical Link\n", usedelvcs);
#endif

#ifdef PLINK
  printf("%d Network (Internode) Physical Links\n", plinknode);
  printf("%d Injection Physical Links (Per Node)\n", pl_injection);
  printf("%d Delivery Physical Links (Per Node)\n", pl_delivery);
#endif

  printf ("Network created.\n");
}

/*----------------------------------------------------------------------*/
/* do_faults() makes all types of faults                                */
/*----------------------------------------------------------------------*/
do_faults ()
{
  int fcnt;
  int link;

  if (FAULTS > 0)
    if (physical_faults) {
      if (FAULTS > numplinks) {
	printf("Faults > number of physical links!\n");
	exit(1);
      }
    } else {
      if (FAULTS > numvlinks) {
	printf("Faults > number of virtual channels!\n");
	exit(1);
      }
    }
    for (fcnt = 0; fcnt < FAULTS; fcnt++)
      if (physical_faults) {
	do {
	  link = (int) (drand48 ()* numplinks)+NODES*plinknode;
	} while (make_fault (link) == 0);
      } else {
	do {
	  link = (int) (drand48 ()* numvlinks)+numnvlinks;
	} while (vlinks[link].busy == -1);
	make_vtrio_fault (link);
      }
}

/*----------------------------------------------------------------------*/
/* make_fault(vl) makes a fault at physical link i                      */
/*----------------------------------------------------------------------*/
make_fault (i)
     int i;
{
  int k, kk, j, v, vl;
  
  k = plinks[i].Lnode;
  j = plinks[i].dim;
  printf("Making fault at time %d\n",sim_time);
  printf(" at location %d %d (physical channel %d) address %d\n",k,j,i,link_addr(k,j,1,v,0));
  for (v = 0; v < virts; v++)
    {			/* Associate each vlink with a plink */
      vl = link_addr (k, j, 1, v, 0);
      if (vlinks[vl].busy == -1) return 0;
      make_vtrio_fault(vl);
      kk = get_next_node(k,j,1);
      vl = link_addr (kk, j, -1, v, 0);
      make_vtrio_fault(vl);
    }
  return 1;
}

/*----------------------------------------------------------------------*/
/* make_vtrio_fault(vl) fail the virtual trio of channels               */
/*----------------------------------------------------------------------*/
make_vtrio_fault(vl) 
int vl;
{
  int k, kk, j, v, d, ov;

  k = node_of_link(vl);
  j = sign_of_link(vl);
  d = dim_of_link(vl);
  v = virt_of_link(vl);
  kk = get_next_node(k, d, j);

  if (vlinks[vl].out_buffer)
    plinks[vlinks[vl].pl].used--;
  if (corrvlinks[vl].out_buffer)
    plinks[vlinks[vl].pl].used--;
  if (compvlinks[complement[vl]].out_buffer)
    plinks[vlinks[vl].pl].used--;

  compvlinks[complement[vl]].out_buffer = 0;
  compvlinks[complement[vl]].next_in=0;
  corrvlinks[vl].out_buffer = 0;
  corrvlinks[vl].next_in = 0;
  vlinks[vl].out_buffer=0;
  vlinks[vl].next_in=0;
  vlinks[vl].in_buf=0;
  vlinks[vl].in_head=0;
  vlinks[vl].in_tail=0;

  if (vlinks[vl].busy !=0) {
    generate_kill_flits(vl);
    release_link(vl);
  }
  vlinks[vl].busy = -1;
  vlinks[vl].forward_block = 1;
  vlinks[vl].back_block = 1;
  vlinks[vl].security_level = 0;

#ifndef NO_X
  if (XDISP)
    {
      frame_buff (k, d, j, v, 0, -2, 0);
      mark_buff (k, d, j, v, 0, -2, 0);
      frame_buff (kk, d, -j, v, 1, -2, 0);
      mark_buff (kk, d, -j, v, 1, -2, 0);
    }
#endif
}

/*----------------------------------------------------------------------*/
/* generate_kill_flits()  Send kill flits upstream and downstream       */
/*----------------------------------------------------------------------*/
generate_kill_flits(vl)
int vl;
{
  int ov, i , bv, tkn, tkn2;

  tkn =  vlinks[vl].busy - 1;
/*  msgs[tkn].t_intr = sim_time; */
  ov = vlinks[vl].next;
  vlinks[vl].forward_block = 1;
  vlinks[vl].back_block = 1;
  if (ov != -1) {		/* Forward kill flit */
    if (vlinks[ov].busy == tkn+1) {
      flush_control_flits(ov,tkn);
      
      if (corrvlinks[ov].out_buffer == 0)
	plinks[vlinks[ov].pl].used++;
      corrvlinks[ov].out_buffer = FKill(tkn);
      
      vlinks[ov].back_block = 1;
      mark_tkn_buff(ov, -2, OUT);
    }
  }
  ov = complement[vl];
  i = compvlinks[ov].next;
  if (i != -1) {
    if (vlinks[complement[i]].busy == tkn+1) {
      vlinks[complement[i]].forward_block = 1;
      bv = i;

      if (compvlinks[bv].out_buffer == 0)
	plinks[vlinks[bv].pl].used++;
      compvlinks[bv].out_buffer = BKill(tkn);
    
      mark_tkn_buff(bv, -2, OUT);
      frame_tkn_buff(i, 0, IN);
    }
  }
}

/*----------------------------------------------------------------------*/
/* inject_new()  Put new messages into the network                      */
/*----------------------------------------------------------------------*/
inject_new ()
{
  float t;
  int src;
  int i, j, k, num, rem;
  
  /*--------------------------------------*/
  /*  Calculate ave circs and ave setups  */
  /*--------------------------------------*/
  ave_circs = ave_circs + con_circs;
  ave_su = ave_su + con_su;
  ave_tacks = ave_tacks + con_tacks;
  
  if (traffic == SYNTHETIC) 
    inject_synth();
  else
    inject_trace();
}

/*--------------------------------------*/
/* create_new_circ()                    */
/*--------------------------------------*/
create_new_circ (i, src, dest, slot, time, lev, msgtype)
     int i, src, dest, slot, time, lev, msgtype;
{
  int j, k, cnt,q;
  float f, a;

  /* For DISHA Support ... added by Anjan */
  int min_dist;

  if (dest == src)
    dest = generate_dest(src);
  
  /* For DISHA Support ... added by Anjan */
  min_dist = distance (src, dest);
  if (min_dist < 3) circs[i].M = 0;
  if (min_dist > 2 && min_dist < 7) circs[i].M = 1;
  if (min_dist > 6 && min_dist < 11) circs[i].M = 2;
  if (min_dist > 10 && min_dist < 15) circs[i].M = 3;
  if (min_dist > 14) circs[i].M = 4;

  q = (sim_time - time);
  q_ave += q;
  q_sqave += (float)q*q;
  circs[i].acked = 0;
  circs[i].inject_virt = slot;
  circs[i].src = src;
  circs[i].dest = dest;
  circs[i].retries = 0;
  circs[i].cnt = 0;
  circs[i].misc = 0;
  circs[i].misc2 = 0;
  circs[i].msgtype = msgtype;
  circs[i].links_res = 0;
  circs[i].complete = 0;
  circs[i].blocked = 0;
  circs[i].security_level = lev;
  circs[i].path_length = 1;
  circs[i].times_bt = 0;
  circs[i].pl = PROBE_LEAD;

/* ----- DISHA ----- */

  circs[i].token = 0;
  circs[i].wait_time = 0;

/* ----- DISHA ----- */

  injects++;
  con_su++;
}

/*--------------------------------------*/
/* create_new_msg()                     */
/*--------------------------------------*/
create_new_msg (msgno, circno, msglen, time, pre) {
  int src, slot;

  msgs[msgno].circ_index = circno;
  msgs[msgno].preroute = pre;
  msgs[msgno].t_queue = time;
  msgs[msgno].t_inject = sim_time;
  msgs[msgno].msglen = msglen;
  src = circs[circno].src;
  slot = circs[circno].inject_virt;
  nodes[src].send[slot] = Pr (msgno);
  nodes[src].generate[slot] = msglen;
}

/*--------------------------------------*/
/* recreate_circ()                      */
/*--------------------------------------*/
recreate_circ (i,j,slot)
{
  int src;
  src = circs[j].src;
  if (i < MAXCIRC) {
    circs[i].src = src;
    circs[i].dest = circs[j].dest;
    circs[i].retries = circs[j].retries+1;
    circs[i].cnt = 0;
    circs[i].misc = 0;
    circs[i].misc2 = 0;
    circs[i].links_res = 0;
    circs[i].complete = 0;
    circs[i].blocked = 0;
    circs[i].security_level =  circs[j].security_level;
    circs[i].path_length = 1;
    circs[i].msgtype = circs[j].msgtype;
    circs[i].times_bt = 0;
    circs[i].pl = PROBE_LEAD;
    nodes[src].send[slot] = Pr (i);

/* ----- DISHA ----- */
 
  circs[i].token = 0;
  circs[i].wait_time = 0;
 
/* ----- DISHA ----- */
 

  } else {
    printf("Circuit overflow.\n");
    nodes[src].send[slot] = 0;
  }
}

/*--------------------------------------*/
/* recreate_msg()                       */
/*--------------------------------------*/
recreate_msg (i,j,slot)
{
  int src;
  int ic, jc;
  if (i < MAXCIRC) {
    ic = msgs[i].circ_index;
    jc = find_circ();
    recreate_circ(ic, jc, slot);

    msgs[j].circ_index = jc;
    msgs[j].preroute = msgs[i].preroute;
    msgs[j].t_queue = msgs[i].t_queue;
    msgs[j].t_inject = msgs[i].t_inject;
    msgs[j].msglen = msgs[i].msglen;
    src = circs[jc].src;
    nodes[src].generate[slot] = msgs[i].msglen;
    msgs[i].circ_index = -1;
  } else {
    printf("Message overflow.\n");
  }
}

/*----------------------------------------------------------------------*/
/* update_network()                                                     */
/*   This routine works in the following manner:  1) Scan through all   */
/*   plinks and perform a physical cycle allocation moving appropriate  */
/*   flit tokens.  2) Scan through all nodes and generate new flits for */
/*   appropriate circuits. 3) Scan through all virtual channels and     */
/*   move flits from input to output buffers.                           */
/*                                                                      */
/*----------------------------------------------------------------------*/
update_network ()
{
  int pl, vl, n, tkn, result;
  int i, j, k, v, l;
  int circ_no;
  int error;

  /* Newly added to suppoort variable delays */
  int	delay;	
  int  temp_nd, save_node;
  int disha_token_found;

  /*
  print_nodes2();
  print_circs();
  */

  /*--------------------------------*/
  /* DD (Deadlock Detection) Code   */
  /*--------------------------------*/

  /* Check for cycles at some reasonable interval */
  if (CD && ((sim_time + 1) % CDFREQ == 0))
  {

	/* Clear 'InACycle' flags - in preparation for Cycle detection */
	/* PATCH ClearInACycleFlags(); */

	/* Run Cycle Detection */
  	/* PATCH error = DetectCycles(); */

	/* if we have an error, print out data structures and exit */
	
	/*
	if (error != NOERROR)
	{
		printf("ERROR: from 'DetectCycles' in 'update_network'\n");
		PrintCircuitStats();
		PrintCycleStats();
		PrintDeadlockStats();

      		if (DDTEST >= PRINT_WHEN_ERROR)
     		{
			PrintAdjacencyList();
  			PrintCycleList();
  			PrintDeadlockList();
		}
		exit(1);
	}
	*/

 	/* Retire those cycles which are no longer active */
 	/* PATCH error = RetireEligibleCycles(); */

	/* if we have an error, print out data structures and exit */
	/*
	if (error != NOERROR)
	{
		printf("ERROR: from 'RetireEligibleCycles' in 'update_network'\n");
		PrintCircuitStats();
		PrintCycleStats();
		PrintDeadlockStats();

      		if (DDTEST >= PRINT_WHEN_ERROR)
     		{
			PrintAdjacencyList();
  			PrintCycleList();
  			PrintDeadlockList();
		}
		exit(1);
	}
	*/
  }

  /* Check for deadlocks at a (hopefully) even more reasonable interval */
  if (DD & ((sim_time + 1) % DDFREQ == 0))
  {
    /* PATCH error = MarkCircuitsWithAllReqVCsInACycle(); */

    /* if we have an error, print out data structures and exit */
    /*
    if (error != NOERROR)
    {
      printf("ERROR: from 'ClearInACycleFlags' in 'update_network'\n");
      PrintCircuitStats();
      PrintCycleStats();
      PrintDeadlockStats();

     if (DDTEST >= PRINT_WHEN_ERROR)
      {
        PrintAdjacencyList();
        PrintCycleList();
      	PrintDeadlockList();
      }
      exit(1);
    }
    */

    /* Run Deadlock Detection */
    /* PATCH error = DetectDeadlocks(); */

    /* if we have an error, print out data structures and exit */
    /*
    if (error != NOERROR)
    {
      printf("ERROR: from 'DetectDeadlocks' in 'update_network'\n");
      PrintCircuitStats();
      PrintCycleStats();
      PrintDeadlockStats();

      if (DDTEST >= PRINT_WHEN_ERROR)
      {
        PrintAdjacencyList();
        PrintCycleList();
        PrintDeadlockList();
      }
      exit(1);
    }
    */

	/* Retire those deadlocks which are no longer active (broken !) */
 	/* error = RetireEligibleDeadlocks(); */

	/* if we have an error, print out data structures and exit */
	/*
	if (error != NOERROR)
	{
		printf("ERROR: from 'RetireEligibleDeadlocks' in 'update_network'\n");
		PrintCircuitStats();
		PrintCycleStats();
		PrintDeadlockStats();

      		if (DDTEST  >= PRINT_WHEN_ERROR)
     		{
			PrintAdjacencyList();
  			PrintCycleList();
  			PrintDeadlockList();
		}
		exit(1);
	}
	*/

  }

  /* Report (Checkpoint) stats for Deadlock Detection */
  /* If we just checked for cycles or deadlocks, then report stats on these */
  if ((CD) && (((sim_time + 1) % CDFREQ == 0) || ((sim_time + 1) % DDFREQ == 0)))
  {
	PrintCircuitStats();
	PrintCycleStats();
	PrintDeadlockStats();

	if (DDTEST >= PRINT_AT_CHECKPOINT)
        {
		PrintAdjacencyList();
        	PrintCycleList();
      		PrintDeadlockList();
	}
  }

  /*--------------------------------*/

  /* newly added ... for testing */
  if (TEST)
  {
  	print_msgs();
 	print_circs();
  	print_vlinks();
 	print_corrvlinks();
 	print_compvlinks();
  	print_used_plinks();
  }


  if (NEW_DISHA_FEATURES)
  {
      for (temp_nd = 0; temp_nd < NODES; temp_nd++)
      {

        	if ((nodes[temp_nd].disha_deadlock_buffer_used == HIGHLIGHT_AS_USING_DBUFFER) ||
            		(nodes[temp_nd].disha_deadlock_buffer_used == HIGHLIGHT_AS_DESTINATION) ||
           		 (nodes[temp_nd].disha_token_siezed == 1))
              		DEADLOCK_BUFFER_UTILIZED++;

		/* HIGHLIGHT or DIM BASED ON THE IF TOKEN WAS JUST SIEZED */
		if (nodes[temp_nd].disha_token_siezed == 1)
		{
			nodes[temp_nd].disha_token = 0;
			nodes[temp_nd].disha_token_siezed++;

#ifndef NO_X
     			if (XDISP) highlight_node(temp_nd, 1);
#endif

		}
		else if (nodes[temp_nd].disha_token_siezed > 1)
		{
			nodes[temp_nd].disha_token_siezed = 0;

#ifndef NO_X
      			if (XDISP) highlight_node(temp_nd, 0); 
#endif

		}
 
		/* HIGHLIGHT or DIM BASED ON THE IF DB BUFFER WAS USED or ARRIVED AT DESTINATION */
		if (nodes[temp_nd].disha_deadlock_buffer_used == HIGHLIGHT_AS_USING_DBUFFER)
		{
			nodes[temp_nd].disha_deadlock_buffer_used = HIGHLIGHT_AS_UNUSED;

#ifndef NO_X
     			if (XDISP) highlight_node(temp_nd, 2);
#endif

		}
		else if (nodes[temp_nd].disha_deadlock_buffer_used == HIGHLIGHT_AS_DESTINATION)
		{
			nodes[temp_nd].disha_deadlock_buffer_used = HIGHLIGHT_AS_UNUSED;

#ifndef NO_X
      			if (XDISP) highlight_node(temp_nd, 3); 
#endif

		}
		else if (nodes[temp_nd].disha_deadlock_buffer_used == HIGHLIGHT_AS_UNUSED)
		{
			nodes[temp_nd].disha_deadlock_buffer_used = 0;

#ifndef NO_X
      			if (XDISP) highlight_node(temp_nd, 0); 
#endif
		}
      }

      /* clear out others ... just in case */
      for (temp_nd = 0; temp_nd < NODES; temp_nd++)
      {
#ifndef NO_X
        if (!nodes[temp_nd].disha_deadlock_buffer_used && !nodes[temp_nd].disha_token_siezed)
      	  if (XDISP) highlight_node(temp_nd, 0);
#endif

      }
      

    /* newly added ... to support new version of DISHA */
    /* Move the token from one node to another         */
    /* take the token from the previous node that had it */

    disha_token_found = 0;
    for (temp_nd = 0; temp_nd < NODES; temp_nd++)
    {
		if (nodes[temp_nd].disha_token == 1)
		{
			nodes[temp_nd].disha_token++ ;

#ifndef NO_X
      			if (XDISP) mark_disha_token(temp_nd, 1);
#endif
			save_node = temp_nd;
			disha_token_found = 1;
		}
		else if (nodes[temp_nd].disha_token > 1)
		{
			nodes[temp_nd].disha_token = 0;

#ifndef NO_X
      			if (XDISP) mark_disha_token(temp_nd, 0);
#endif
		}
    }
    if (disha_token_found)
	{
      		temp_nd = (save_node+1) % NODES;
      		nodes[temp_nd].disha_token = 1;
#ifndef NO_X
      		if (XDISP) mark_disha_token(temp_nd, 1);
#endif
	}

      /* clear out others ... just in case */
#ifndef NO_X
    for (temp_nd = 0; temp_nd < NODES; temp_nd++)
      if (nodes[temp_nd].disha_token != 1)
      	if (XDISP) mark_disha_token(temp_nd, 0);
#endif

  }


  /* only applies to trace traffic ??? */
  if (PROFILE) 
  {
    if (oldflits != msgflits) {
      fprintf(commprofile, "%d\t%d\n",sim_time-1, oldflits);
      fprintf(commprofile, "%d\t%d\n",sim_time, msgflits);
      oldflits = msgflits;
    }
    if (old_idle != idle_pes) {
      fprintf(peprofile, "%d\t%d\n",sim_time-1, NODES-old_idle);
      fprintf(peprofile, "%d\t%d\n",sim_time, NODES-idle_pes);
      old_idle = idle_pes;
    }
    if (old_over != over_pes) {
      fprintf(overprofile, "%d\t%d\n",sim_time-1, old_over);
      fprintf(overprofile, "%d\t%d\n",sim_time, over_pes);
      old_over = over_pes;
    }
    if (blocked != old_blocked) {
      fprintf(blkprofile, "%d\t%d\n",sim_time-1, old_blocked);
      fprintf(blkprofile, "%d\t%d\n",sim_time, blocked);
      old_blocked = blocked;
    }
  }
  blocked = 0;

  if (activity==0 && msgflits == 0 && sim_time < min_event_time) {
    sim_time = min_event_time;
  }

  if (activity == 0 && msgflits > 0) {
    printf("Network is deadlocked.\n");
    exit(1);
  }

  activity = 0;
  if (dyn_cnt < num_dyn_faults) 
    while (dyn_fault_times[dyn_cnt] <= sim_time &&
	   dyn_cnt<num_dyn_faults) {
      if (physical_faults) 
	make_fault((int)(drand48()*numplinks));
      else 
	make_vtrio_fault((int)(drand48()*numvlinks));
      dyn_cnt++;
    }
  
  /* newly added to support vriable size delays */
  if (SLOW) 
  {
    for (delay = 0; delay < pwr2(SLOW) ; delay++);
    /* sleep (1); */
  }

  /*-------------------------------------------------*/
  /* Process physical links first (inter_node_moves) */
  /*-------------------------------------------------*/
#ifndef PLINK
  for (pl = 0; pl < numplinks+NODES; pl++)
#else
  for (pl = 0; pl < numplinks+NODES*plinknode; pl++)
#endif  
    {
      if (INIT && (TRANSIENT || MAP))
	{
	/*  rdist[plinks[pl].nrl]++;
	  rdist[plinks[pl].nrr]++;
	  rhdist[plinks[pl].nrl + plinks[pl].nrr]++;
	  if (plinks[pl].nrl > 0)
	    {
	      rfcdist[plinks[pl].nrl]++;
	      rhcdist[plinks[pl].nrl + plinks[pl].nrr]++;
	    }
	  sdist[plinks[pl].nsl]++;
	  sdist[plinks[pl].nsr]++;
	  cdist[plinks[pl].ncl]++;
	  cdist[plinks[pl].ncr]++; 
	  
	  plinks[pl].crl += plinks[pl].nrl;
	  plinks[pl].crr += plinks[pl].nrr;
	  plinks[pl].ccl += plinks[pl].ncl;
	  plinks[pl].ccr += plinks[pl].ncr;
	  plinks[pl].csl += plinks[pl].nsl;
	  plinks[pl].csr += plinks[pl].nsr; */
	}
      if (plinks[pl].used == 0)
	{
	  plinks[pl].lcnt = 0;
	  plinks[pl].rcnt = 0;
	}
      else
	{			/* If the link is used, process it */
	  do_physical_link(pl);
	}
    }
  
  /*------------------------------------------------------------*/
  /* Process nodes  (flit injections)                           */
  /*------------------------------------------------------------*/
  for (n = 0; n < NODES; n++) {
#ifdef PLINK
    for( l= 0;l < pl_injection; l++) {
#endif
#ifndef SET_PORT
    for (i=0; i<node_virts; i++) {
#else
    for(i=0; i <useinjvcs;i++) {
#endif
      if (nodes[n].send[i+l*node_virts] != 0)
	{			/* Process node if sending anything */
	  if (nodes[n].send[i+l*node_virts] != HOLDER)
	    {
	      int skip;
	      skip = 0;
	      tkn = nodes[n].send[i+l*node_virts];
	      circ_no = msgs[UnToken(tkn)].circ_index;
#ifdef PLINK
	      vl = nlink_addr(n, i, IN, circs[circ_no].security_level, l);
#else
	      vl = nlink_addr(n, i, IN, circs[circ_no].security_level);
#endif
	      if (vlinks[vl].busy == 0 || TestPr(tkn) == 0) {
		if (TestData(tkn) || TestTail(tkn)) {
		  if (vlinks[vl].out_buffer) 
		    skip = 1;
		  else
		    vlinks[vl].out_buffer = tkn;
		} else {
		  if (corrvlinks[vl].out_buffer) 
		    skip = 1;
		  else
		    corrvlinks[vl].out_buffer = tkn;
		}
		if (!skip) {
		  plinks[vlinks[vl].pl].used++;
		  /*------------------------------------*/
		  /* Routing probe                      */
		  /*------------------------------------*/
		  if (TestPr (tkn) ) 
		    {		/* Its a routing probe */
		      vlinks[vl].busy = circ_no+1;
/*		      vlinks[vl].acks = PROBE_LEAD; */
		      vlinks[vl].acks = plinks[vlinks[vl].pl].probe_register;
		      init_hist(vl);
		      inc_setup(vl);
		      connect_link(n, vl, NODE_TO_LINK, tkn);
		      circs[circ_no].links_res=1;
		      if (comm_mech == PCS || comm_mech == APCS)
			{
			  nodes[n].send[i+l*node_virts] = HOLDER;
			} else 
			  if (msgs[UnPr(tkn)].preroute)
			    nodes[n].send[i+l*node_virts] = HOLDER;
			  else
			    nodes[n].send[i+l*node_virts] = Data (UnPr (tkn));
		      activity++;

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/
			
			/* Do this only if Cycle/Deadlock detection is ON */
			if (CD)
			{
				/* ... Add the VC to the Circuit */
				error = AddVCToCircuit(circ_no, vl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'AddVCToCircuit' - circ_no: %d, vl: %d in 'update_network'\n", circ_no, vl);
					PrintAdjacencyList();

				}
			}
			/*--------------------------------*/
		    }
		  else
		    /*--------------------------------*/
		    /* Data flit                      */
		    /*--------------------------------*/
		    if (TestData(tkn)) {
		      nodes[n].generate[i+l*node_virts]--;
		      vlinks[vl].data = 1;
		      if (nodes[n].generate[i+l*node_virts] <= 1)
			nodes[n].send[i+l*node_virts] = Tail(UnData(tkn));
		      activity++;
		    } 
		    else
		      /*--------------------------------*/
		      /* Tail flit                      */
		      /*--------------------------------*/
		      if (TestTail (tkn))
			{
			  if (comm_mech == PCS || 
			      comm_mech == WR ||
			      comm_mech == RECON)
			    if (msgs[UnTail(tkn)].preroute) 
			      nodes[n].send[i+l*node_virts] = HOLDER;
			    else
			      nodes[n].send[i+l*node_virts] = 0;
			  else
			    nodes[n].send[i+l*node_virts] = HOLDER;
			  activity++;
			}
		}
	      }
	    }
	}
    }
#ifdef PLINK
    }
#endif
  }
  /*-------------------------------------------------*/
  /* Intra-node transfers                            */
  /*-------------------------------------------------*/

  /* newly added to support vriable size delays */
  if (SLOW) 
  {
    for (delay = 0; delay < pwr2(SLOW) ; delay++)
	;
    /* sleep (1); */
  }

  for (n = 0; n < NODES; n++) {
    for (i=0; i<nodes[n].num_head; i++) {
      j = nodes[n].headers[i];
      if (corrvlinks[j].in_buffer != 0) {
	intra_node_move(node_of_link(j), dim_of_link(j), sign_of_link(j), j);
      }
    }
  }
  
  for (n=0; n<NODES; n++) {

#ifdef PLINK
    for(l =0; l<pl_delivery; l++) 
 #endif
      for (v=0; v<usedelvcs;v++) {
	 pe_intra_node_move(n, v, l);
      }
#ifdef PLINK
    for(l =0; l<pl_injection; l++) 
#endif
      for (v=0; v<useinjvcs;v++) {
        j = nlink_addr(n, v, IN, 0,l);
	intra_node_move(n, -1, 1, j);
      }

    control_collide = 0;
    for (i=0; i<dimensions; i++) {
      for (k= (-1); k<2; k+=2) {
	for (v=0; v<virts; v++) {
	  j = link_addr(n, i, k, v, 0);
	  if (!find_header(get_next_node(n, i, k), j)) {
	    intra_node_move (n, i, k, j);
	  }
	}
      }
    }
    if (control_collide > 1)
      cum_control_collide += (control_collide-1);
  }
  for (n = 0; n < NODES; n++) {
    for (i=0; i<nodes[n].num_head; i++) {
      j = nodes[n].headers[i];
      if (TestPr(corrvlinks[j].in_buffer ) == 0)
	delete_header(n, i);
    }
  }
  /*--------------------------------------------------------------*/
  /* Place incoming flits in in-buffers and release links         */
  /*--------------------------------------------------------------*/
  for (j = 0; j < numvlinks+numnvlinks; j++)
    {
      if (vlinks[j].busy == (-10))
	vlinks[j].busy = 0;
      if (vlinks[j].next_in)
	{
	  vlinks[j].in_buffer[vlinks[j].in_tail] = vlinks[j].next_in;
	  vlinks[j].in_tail = (vlinks[j].in_tail + 1) % BUFFERS;
	  vlinks[j].in_buf++;
	  vlinks[j].next_in = 0;
	  vlinks[j].data = 1;
	}
      if (corrvlinks[j].next_in) {
	corrvlinks[j].in_buffer = corrvlinks[j].next_in;
	corrvlinks[j].next_in = 0;
      }
      if (compvlinks[j].next_in) {
	compvlinks[j].in_buffer = compvlinks[j].next_in;
	compvlinks[j].next_in = 0;
      }
    }
#ifndef NO_X
  Sync_display();
#endif
}

/*--------------------------------------------------------------*/
/* ok_to_out()    Is it ok to put a flit into the out_buffer of */
/*                vlink vl?                                     */
/*--------------------------------------------------------------*/
ok_to_out (vl)
     int vl;
{
  if (vlinks[vl].out_buffer != 0)
    return 0;
  return 1;
}

int wrapped;
/*--------------------------------------------------------------*/
/* do_phyiscal_link(pl)                                         */  
/*--------------------------------------------------------------*/
do_physical_link(pl)
{
  int rvirt, lvirt, result, rwrap, lwrap;
  /*---------------------*/
  /* Half duplex link    */
  /*---------------------*/
  if (HALF)
    {
      if (plinks[pl].arbcnt > 0) {	/* Account for arbitration idle time */
	plinks[pl].arbcnt--;
      } else {
	rvirt = select_flit_to_transfer(pl,R);
	rwrap = wrapped;
	lvirt = select_flit_to_transfer(pl,L);
	lwrap = wrapped;
	if (rvirt != (-1) || lvirt != (-1)) { /* Someone has data to send */
	  if (plinks[pl].LR == R)
	    result = rvirt;
	  else
	    result = lvirt;

	  /* Need to switch directions? */
	  if (need_to_switch(pl, rvirt, lvirt, rwrap, lwrap)) {

	    plinks[pl].LR = (-plinks[pl].LR);
	    if (link_arb_time > 0) { /* Wait a link_arb_time to switch */
	      plinks[pl].startlane = 0;
	      if (plinks[pl].LR == L) 
		plinks[pl].Llane = plinks[pl].startlane;
	      else
		plinks[pl].Rlane = plinks[pl].startlane;
	      plinks[pl].arbcnt = link_arb_time-1;
	    } else 
	      if (plinks[pl].LR == R) /* Can switch immediately */
		result = rvirt;
	      else
		result = lvirt;
	  }
	  
	  plinks[pl].rcnt++;
	  if (plinks[pl].rcnt >= link_time) /* Account for link time */
	    {
	      inter_node_move(result);
	      if (plinks[pl].LR == L) 
		plinks[pl].Llane = (virt_of_link(result)+1)%virts;
	      else
		plinks[pl].Rlane = (virt_of_link(result)+1)%virts;
	    }
	}
      }
    } else {
      /*---------------------*/
      /* Full duplex link    */
      /*---------------------*/
      plinks[pl].lcnt++;
      if (plinks[pl].lcnt >= link_time)
	{
	  plinks[pl].lcnt = 0;
	  lvirt = select_flit_to_transfer(pl,L);
	  if (lvirt > -1) {
	    inter_node_move (lvirt);
	    plinks[pl].Llane = (virt_of_link(lvirt)+1)%virts;
	  }
	}
      plinks[pl].rcnt++;
      if (plinks[pl].rcnt >= link_time)
	{
	  plinks[pl].rcnt = 0;
	  rvirt = select_flit_to_transfer(pl,R);
	  if (rvirt > -1) {
	    inter_node_move (rvirt);
	    plinks[pl].Rlane = (virt_of_link(rvirt)+1)%virts;
	  }
	}
    }
}

/*--------------------------------------------------*/
/* need_to_switch(pl, rvirt, lvirt, rwrap, lwrap)   */
/*--------------------------------------------------*/
need_to_switch(pl, rvirt, lvirt, rwrap, lwrap) {
  if (plinks[pl].LR == R) {
    /* Right side expressions */
    if (lvirt == (-1)) return 0;
    if (rvirt == (-1)) return 1;
    if (rwrap) return 1;
  } else {
    if (rvirt == (-1)) return 0;
    if (lvirt == (-1)) return 1;
    if (lwrap) return 1;
  }
  return 0;
}

/*-------------------------------------------------------------*/
/* select_flit_to_transfer(pl,LR)                              */
/*-------------------------------------------------------------*/
select_flit_to_transfer(pl,LR)                             
{
  int n, lane, dim, sign;
  int i, v, vl, ovl, done, nn, demand, tempvl;

  wrapped = 0;
  done = 0;

  /* if it's the RIGHT one */
  if (LR == R) {
    n = plinks[pl].Lnode;
    dim = plinks[pl].dim;
    sign = R;
    lane = plinks[pl].Rlane;
  } 
  /* if it's the LEFT one */
  else 
  {
    n = plinks[pl].Rnode;
    dim = plinks[pl].dim;
    sign = L;
    lane = plinks[pl].Llane;
  }
  demand = (-1);

  /* if ( pl >= NODES ) tempvl=link_addr (n, dim, sign, 0, 0); yungho
: calculate the address of db virtual channel */

  for (i = 0; i < virts && !done; i++)
    {
      v = (i + lane) % virts;
      if (v == virts-1) wrapped = 1;
#ifdef PLINK
      if (pl <NODES*plinknode)
	vl = nlink_addr(n, v, (sign+1)/2, 0, pl%plinknode);
#else
      if (pl < NODES) 
	vl = nlink_addr(n, v, (sign+1)/2, 0);
#endif
      else
	vl = link_addr (n, dim, sign, v, 0);
      if (corrvlinks[vl].out_buffer != 0 && vlinks[vl].forward_block==0)
	{
	  if (demand == (-1))
	    demand = v;
	  if (corrvlinks[vl].in_buffer == 0)
	    done = 1;
	} else
	  if (compvlinks[vl].out_buffer != 0)
	    {
	      if (demand == (-1))
		demand = v;
	      if (compvlinks[vl].in_buffer == 0)
		done = 1;
	    } else
	      if (vlinks[vl].out_buffer != 0)
		{
		  if (demand == (-1))
		    demand = v;
		  if (vlinks[vl].in_buf < BUFFERS)
		    done = 1;
		}
    }

  /* yungho : if db virtual channel has flit to send, DB use physical link bandwidth 

  if (pl >= NODES*plinknode)
   if ( vlinks[tempvl].out_buffer != 0)
    {
     if (demand == (-1))
      	    demand = v;
     if (vlinks[tempvl].in_buf < BUFFERS)
       {   done = 1;vl=tempvl; }  
    }
  */

  if (NO_CTS && (!done || demand != v))
    return demand;
  if (!done)
    return -1;		/* No lanes can send */
  return vl;
}

/*--------------------------------------------*/
/* ok_to_in(vl)                               */      
/*--------------------------------------------*/
ok_to_in(int vl) {
  if (corrvlinks[vl].out_buffer)
    if (corrvlinks[vl].in_buffer) 
      return 0;
    else
      return 1;
  if (compvlinks[vl].out_buffer)
    if (compvlinks[vl].in_buffer)
      return 0;
    else
      return 1;
  if (vlinks[vl].out_buffer)
    if (vlinks[vl].in_buf < BUFFERS)
      return 1;
    else
      return 0;
}

/*--------------------------------------------*/
/* inter_node_move(vl)                        */      
/*--------------------------------------------*/
inter_node_move(vl) {
  if (ok_to_in(vl)) {
    activity++;
    out_to_in (vl);
  }
}

/*--------------------------------------------------------------*/
/* out_to_in()      Move a flit from the output of one node to  */
/*                  the input of another.                       */
/*--------------------------------------------------------------*/
out_to_in (vl)
     int vl;
{
  int i, n, nn, d, s, v, tkn, tkn2, tail, btail;
  int control;

  control=0;
  tail = 0;
  btail = 0;
  if (corrvlinks[vl].out_buffer) {
    tkn = corrvlinks[vl].out_buffer;
    if (vlinks[vl].forward_block == 0) {
      corrvlinks[vl].next_in = tkn;
    }
    if (TestFKill(tkn)) tail = 1;
    corrvlinks[vl].out_buffer = 0;
    draw_vlink(vl,1);
  } else
    if (compvlinks[vl].out_buffer != 0)
      {
	tkn = compvlinks[vl].out_buffer;
	
	if (vlinks[complement[vl]].back_block == 0) {
	  compvlinks[vl].next_in = tkn;
	} else {
	  if (vlinks[complement[vl]].busy == 0) 
	    report_evap("Control flit","back block with free channel");
	}
	compvlinks[vl].out_buffer = 0;
	if (TestBKill(tkn) || TestTAck(tkn) || TestBack(tkn)) btail = 1;
	draw_vlink(complement[vl],1);
      } else {
	tkn = vlinks[vl].out_buffer;
	if (vlinks[vl].forward_block == 0) {
	  vlinks[vl].next_in = tkn;
	}
	if ( TestTail(tkn) && (comm_mech == PCS || comm_mech == WR || 
			     comm_mech == RECON))
	  if (msgs[UnTail(tkn)].preroute == 0)
	    tail = 1;
	vlinks[vl].out_buffer = 0;
	draw_vlink(vl,1);
      }
  
  
  /* Mark output buffer */
  mark_tkn_buff(vl,0,OUT);
      
  if (tail) 
    {
      frame_tkn_buff (vl,0,OUT);
     draw_vlink(vl,0);
    }
      
  if (btail) 
    {
      frame_tkn_buff (complement[vl],0,IN);
      draw_vlink(vl,0);
    }
      
  /* Mark input buffer */
  mark_tkn_buff(vl,tkn,IN);
  
  if (TestPr(tkn)) 
    frame_busy(vl,IN, tkn);

  i = vlinks[vl].pl;
  plinks[i].used--;
}

/*--------------------------------------------------------------*/
/* in_to_out()      Move a flit from the input of a node to its */
/*                  output buffer.                              */
/*--------------------------------------------------------------*/
in_to_out (ovl, vl, tkn)
     int ovl, vl, tkn;
{
  int i, t;
  
  i = vlinks[vl].pl;
  activity++;
  if (TestAck (tkn) || TestTAck (tkn) || TestBack (tkn) || TestBKill (tkn) ) {
    if (vlinks[complement[vl]].back_block == 0) {
      compvlinks[vl].out_buffer = tkn;
      plinks[i].used++;
    } else {
      if (vlinks[complement[vl]].busy == 0) {
	report_evap("Control flit", "intra move to free link");
      }
    }
  } else 
    if (TestPr(tkn) || TestFKill(tkn)) {
      if (vlinks[vl].forward_block == 0) {
	corrvlinks[vl].out_buffer = tkn;
	plinks[i].used++;
      }
    } else
	if (vlinks[vl].forward_block == 0) {
	  vlinks[vl].out_buffer = tkn;
	  plinks[i].used++;
	}

  remove_in_flit(ovl);

    t = 0;
  if (vlinks[ovl].in_buf)
    t = vlinks[ovl].in_buffer[vlinks[ovl].in_head];
  if (corrvlinks[ovl].in_buffer)
    t = corrvlinks[ovl].in_buffer;
  if (compvlinks[ovl].in_buffer)
    t = compvlinks[ovl].in_buffer;
  
  mark_tkn_buff(ovl,t,IN);
  
  if (vlinks[vl].out_buffer)
    t = vlinks[vl].out_buffer;
  if (corrvlinks[vl].out_buffer)
    t = corrvlinks[vl].out_buffer;
  if (compvlinks[vl].out_buffer)
    t = compvlinks[vl].out_buffer;
  
  mark_tkn_buff (vl,t,OUT);
  
  if (TestPr (tkn))
    {			/* Its a probe */
      frame_tkn_buff (vl, tkn, OUT);
    }
  if ((TestTail (tkn) && comm_mech!=APCS) 
      || TestFKill(tkn)) { /* It's a tail */
    frame_tkn_buff (ovl, 0, IN);
  }
  
  if (TestTAck(tkn) || TestBKill(tkn)) { /* It's a reverse moving tail */
    i = complement[ovl];
    frame_tkn_buff (i, 0, OUT);
  }
}

/*--------------------------------------------------------------*/
/* pe_intra_node_move  Handle movement of a flit into a pe   */
/*                        node.                                 */
/*--------------------------------------------------------------*/
pe_intra_node_move(n, v, l)
     int n, v, l;
{
  int ovl, tkn, cvl, j, i, vl;
  int Ack_evap, TAck_evap, Back_evap, BKill_evap;
  int circ_no;

  Ack_evap = 0;
  TAck_evap = 0;
  Back_evap = 0;
  BKill_evap = 0;
  ovl = nlink_addr(n, v, OUT, 0, l);
  cvl = complement[ovl]; /* Complementary link */
  /*-------------------------------------------------------*/
  /* virtual channel ovl sending a flit to the next vlink  */
  /*-------------------------------------------------------*/
  if (compvlinks[ovl].in_buffer) 
    tkn = compvlinks[ovl].in_buffer;
  else if (corrvlinks[ovl].in_buffer)
    tkn = corrvlinks[ovl].in_buffer;
  else if (vlinks[ovl].in_buf>0)
    tkn = vlinks[ovl].in_buffer[vlinks[ovl].in_head];
  else 
    return;
  circ_no = msgs[UnToken(tkn)].circ_index;
  /*------------------------------------*/
  /* Routing probe                      */
  /*------------------------------------*/
  if (TestPr (tkn))
    {			/* Its a routing probe */
      delay(nodes[circs[circ_no].src].cnt, ack_time);
      if (comm_mech == WR || comm_mech == AWR)
	{
	  done_su_stats(UnPr(tkn), ovl);
	  remove_in_flit(ovl);
	}
      else
	{   /* Start an ACK flit */
	    /* Move an Ack flit to out buffer */
	  if (comm_mech == RECON) 
/*	    if (nodes[n].consume[v+l*node_virts] < PROBE_LEAD) { */
	    if (nodes[n].consume[v+l*node_virts] < plinks[vlinks[ovl].pl].probe_register) {
	      if (compvlinks[cvl].out_buffer == 0) {
		deposit_out_flit(cvl, Ack(UnPr(tkn)));
		nodes[n].consume[v+l*node_virts]++;
	      }
	    } else {
	      nodes[n].consume[v+l*node_virts] = 0;
	      done_su_stats(UnPr(tkn), ovl);
	      remove_in_flit(ovl);
	    }
	  else
	    in_to_out (ovl, cvl, Ack (UnPr (tkn)));
	}
    }
  else
    /*--------------------------------*/
    /* Data flit                      */
    /*--------------------------------*/
    if (TestData (tkn))
      {
	if (circs[circ_no].complete > 0)	
	  {
	    t_head += sim_time - circs[circ_no].complete;
	    heads++;
	    circs[circ_no].complete = (-1);
	  }
	flits_delivered++;
	remove_in_flit(ovl);
      }
    else
      /*--------------------------------*/
      /* Tail flit                      */
      /*--------------------------------*/
      if (TestTail (tkn)) {
	  if (comm_mech == PCS || comm_mech == WR || comm_mech == RECON) {
	    flits_delivered++;
	    remove_in_flit(ovl);
	    /* printf("Tail: "); */
	    if (msgs[UnTail(tkn)].preroute == 0) { /* Only tear down circuit */
	      release_link(ovl);              /* If message is not prerouted */

				      
	      if (circs[circ_no].complete)
		dec_circuits(ovl);
	      else
		dec_setup(ovl);
	      circs[circ_no].src = (-1);
	    }
	    done_msg_stats (UnTail (tkn));           /* Message is */
	  } else {		                     /* done       */
	    done_msg_stats(UnTail(tkn));
	    if (msgs[UnTail(tkn)].preroute == 0) 
	      in_to_out(ovl, cvl, TAck(UnTail(tkn)));
	  }
	}
      else
	/*--------------------------------*/
	/* Ack flit                       */
	/*--------------------------------*/
	if (TestAck (tkn))
	  {
	    j = circs[circ_no].src;
	    if (j != n)
	      {
		Ack_evap=1;
		report_evap("Ack","node");
	      }
	    remove_in_flit(ovl);
	    if (comm_mech == PCS || comm_mech == APCS) {
	      if (!Ack_evap) {
		if (msgs[UnAck(tkn)].preroute) {
		  nodes[n].send[v+l*node_virts] = HOLDER;
		} else {
		  nodes[n].send[v+l*node_virts] = Data (UnAck (tkn));
		}
		circs[circ_no].acked = 1;
		done_su_stats (UnAck (tkn), complement[ovl]);
	      }
	    }
	  }
	else
	  /*--------------------------------*/
	  /* Tail Ack (TAck)flit            */
	  /*--------------------------------*/
	  if (TestTAck (tkn))
	    {
	      j = circs[circ_no].src;
	      if (j != n)
		{
		  TAck_evap=1;
		  report_evap("TAck","node");
		}
		      
	      remove_in_flit(ovl);
	      frame_tkn_buff(complement[ovl],0,OUT);
	      if (!TAck_evap) {
		/* printf("TAck: "); */
		release_link(complement[ovl]);
		dec_circuits(complement[ovl]);
		      
		done_msgack_stats (UnTAck (tkn));
		circs[circ_no].src = (-1);
		handle_msg_done_at_node(n, v);
	      }
	    }
	  else
	    /*--------------------------------*/
	    /* Back flit                      */
	    /*--------------------------------*/
	    if (TestBack (tkn))
	      {
		j = circs[circ_no].src;
		if (j != n)
		  {
		    Back_evap = 1;
		    report_evap("Back","node");
		  }
		remove_in_flit(ovl);
		circs[circ_no].path_length--;
		dec_setup(cvl);

		
		if (!Back_evap) {
		  /* printf("Back: "); */
		  if (circs[circ_no].retries < RETRY) {
		    nodes[n].send[v+l*node_virts] = Pr (UnBack (tkn));
		    vlinks[cvl].busy = 0;
		    circs[circ_no].retries++;
		  } else {
		    if (traffic == TRACEDRIVEN) {
		      printf("Message undeliverable.  Algorithm fails.\n");
		      exit(1);
		    }
		    release_link(cvl);
		    retry_fail_stats(UnBKill(tkn));
		    circs[circ_no].src = (-1);
		    nodes[n].send[v+l*node_virts] = 0;
		  }
			    
		}
	      }
	    else
		/*--------------------------------*/
		/* Forward Kill flit              */
		/*--------------------------------*/
		if (TestFKill (tkn))
		  {
		    flush_control_flits(ovl);
		    flush_in_data_flits(ovl, UnFKill(tkn));
		    remove_in_flit(ovl);
		    /* printf("FKill: "); */
		    release_link(ovl);
		    if (circs[circ_no].complete)
		      dec_circuits(ovl);
		    else
		      dec_setup(ovl);
		    
		    dest_kill_stats (UnFKill (tkn));
		  }
		else
		  /*--------------------------------*/
		  /* Backward Kill flit             */
		  /*--------------------------------*/
		  if (TestBKill (tkn))
		    {
		      int ov;
		      j = circs[circ_no].src;
		      if (j != n)
			{
			  BKill_evap = 1;
			  report_evap("BKill","node");
			}
		      remove_in_flit(ovl);
		      flush_out_data_flits(cvl, UnBKill(tkn));
		      if (vlinks[cvl].busy - 1 == UnBKill(tkn)) {
			frame_tkn_buff(cvl, 0, OUT);
			if (!BKill_evap) {
			  /* printf("BKill: "); */
			  if (circs[circ_no].retries < RETRY) {
			    i = find_msg();
			    recreate_msg(i,UnBKill(tkn), v);
			  } else {
			    if (traffic == TRACEDRIVEN) {
			  printf("Message undeliverable.  Algorithm fails.\n");
			      exit(1);
			    }
			    retry_fail_stats(UnBKill(tkn));
			    nodes[n].send[v+l*node_virts] = 0;
			  }
			  release_link(cvl);
			  circs[circ_no].src = (-1);
			  src_kill_stats (UnBKill (tkn));
			}
		      }
		    }
}

/*--------------------------------------------------------------*/
/* inc_pl_cntr()  go back three nodes and increment their probe */
/*                lead counter by 2                             */
/*--------------------------------------------------------------*/
inc_pl_cntr(vl)
     int vl;
{
  if (vl > 0)
    if (vlinks[vl].pl >= NODES) {
      printf("vl = %d is on plink %d\n",vl,vlinks[vl].pl);
      plinks[vlinks[vl].pl].probe_register = plinks[vlinks[vl].pl].probe_register + 2;
    }
}

/*--------------------------------------------------------------*/
/* intra_node_move  Handle movement of a flit across a node     */
/*                  Implemented as in -> out between vlinks     */
/*--------------------------------------------------------------*/
intra_node_move (n, dim, sign, vl)
     int n, dim, sign, vl;
{
  int ovl, nn, tkn, i, j, k, pl, pass;
  int circ_no;
  int error;

  ovl = vl;
  /*-------------------------------------------------------*/
  /* virtual channel ovl sending a flit to the next vlink  */
  /*-------------------------------------------------------*/
  if (compvlinks[ovl].in_buffer) {
    control_moves++;
    tkn = compvlinks[ovl].in_buffer;
  } else if (corrvlinks[ovl].in_buffer) {
    control_moves++;
    tkn = corrvlinks[ovl].in_buffer;
  } else if (vlinks[ovl].in_buf > 0)
    tkn = vlinks[ovl].in_buffer[vlinks[ovl].in_head];
  else 
    return;
  circ_no = msgs[UnToken(tkn)].circ_index;
  /*------------------------------------*/
  /* Routing probe                      */
  /*------------------------------------*/
  if (vl < numnvlinks) 
    nn = n;
  else
    nn = get_next_node (n, dim, sign);
  if (TestPr (tkn))
    {			/* Its a routing probe */
      control_collide++;
      delay(circs[circ_no].routecnt, route_delay);
      vl = route (nn, tkn, vlinks[ovl].hist);
      if (vl != -1)
	{			/* Route successful. */
	  if (circs[circ_no].blocked == 0)
	    tot_traversed++;
	  circs[circ_no].blocked = 0;
	  connect_link(ovl, vl, LINK_TO_LINK, tkn);
	  init_hist (vl);
	  if (comm_mech == RECON) {
	    if (compvlinks[complement[ovl]].out_buffer == 0) { 
	      vlinks[vl].acks = 0;
/*	      if (vlinks[ovl].acks < PROBE_LEAD)
	      if (vlinks[ovl].acks < nodes[nn].probe_lead[virt_of_link(ovl)]) */
	      if (vlinks[ovl].acks < plinks[vlinks[ovl].pl].probe_register)
		deposit_out_flit(complement[ovl], Ack(UnPr(tkn)));
	      in_to_out (ovl, vl, tkn);
	    }
	  } else 
	    in_to_out (ovl, vl, tkn);
	}
      else 
	{	
	  int done;
	  done=0;
	  if (backtrack == 1) {
	    /* Route failed. */
	    if (comm_mech == PCS || 
		comm_mech == APCS ||
		(comm_mech == RECON && vlinks[ovl].acks < plinks[vlinks[ovl].pl].probe_register)) {
		  p_blk++;
		  blocked++;
		  blk_ave++;
		  tot_traversed++;
		  
		  /* If backtracking, start a Back flit */
		  vl = complement[ovl];
		  frame_tkn_buff(vl,0,IN);
		  
		  /* Move Back flit to out buffer */
		  in_to_out (ovl, vl, Back (UnPr (tkn)));
		  circs[circ_no].misc = 2;
		  done = 1;
/*		  if (vlinks[vl].data == 1 && comm_mech == RECON) { */
		  if (vlinks[ovl].out_buffer != (-1)) {
		    circs[circ_no].times_bt++;
		    if (circs[circ_no].times_bt > 10) {
		      printf("This message in circ %d has backtracked to the data more than 10 times\n",circ_no);
		      printf("source = %d, dest = %d\n",circs[circ_no].src, circs[circ_no].dest);
		      vlinks[ovl].forward_block = 1;
		      vl = complement[ovl];
		      inc_pl_cntr(vl);
		      inc_pl_cntr(compvlinks[vl].next);
		      inc_pl_cntr(compvlinks[compvlinks[vl].next].next);
		      frame_tkn_buff(vl,0,IN);
		      msgs[UnPr(tkn)].t_intr = sim_time;
		      /* Move BKill flit to out buffer */
		      in_to_out (ovl, vl, BKill (UnPr (tkn)));
/*		      in_to_out (vl, ovl, FKill (UnPr (tkn))); */
		      done = 1;
		    }
		  }
		} else if (comm_mech == AWR) {
		  /*------------------------------------------------------*/
		  /* Routing failure & backtrack for AWR indicates a need */
		  /* to generate a BKill flit.                            */
		  /*------------------------------------------------------*/
		  vlinks[ovl].forward_block = 1;
		  vl = complement[ovl];
		  frame_tkn_buff(vl,0,IN);
		  msgs[UnPr(tkn)].t_intr = sim_time;
		  /* Move BKill flit to out buffer */
		  in_to_out (ovl, vl, BKill (UnPr (tkn)));
		  done = 1;
		}
	  }
	  if (!done) 
	    {			/* Block */
	      if (circs[circ_no].blocked == 0)
		{
		  init_hist(ovl);
		  if (!find_header(nn,ovl))
		    add_header(nn, ovl);
		  circs[circ_no].blocked = 1;
		  tot_traversed++;
		  p_blk++;
		}
	      blk_ave++;
	      blocked++;
	    }
	}
    }
  else
    {
      /*--------------------------------*/
      /* Data flit                      */
      /*--------------------------------*/
      if (TestData (tkn))
	{
	  delay(vlinks[ovl].cnt, data_delay);
	  vl = vlinks[ovl].next;
	  if (vl == (-1)) /* This path has been "killed" */
	    {
	      remove_in_flit(ovl);
	    } else {
/*	      if (comm_mech == RECON && vlinks[vl].acks < PROBE_LEAD) */
/*	      printf("pl = %d msgs.pl = %d\n",PROBE_LEAD,msgs[UnPr(tkn)].probe_lead); */
	      if (comm_mech == RECON && vlinks[vl].acks < plinks[vlinks[ovl].pl].probe_register)
		return;
	      if (ok_to_out (vl))
		{
		  in_to_out (ovl, vl, tkn);
		  vlinks[vl].data = 1;
		  vlinks[vlinks[vl].next].data = 1;
		}
	    }
	}
      else
	/*--------------------------------*/
	/* Tail flit                      */
	/*--------------------------------*/
	if (TestTail (tkn))
	  {
	    delay(vlinks[ovl].cnt, data_delay);
	    vl = vlinks[ovl].next;
	    if (vl == (-1)) /* This path has been "killed" */
	      {
		remove_in_flit(ovl);
	      } else if (ok_to_out (vl))
		{
		  in_to_out (ovl, vl, tkn);
		  if (comm_mech == PCS || comm_mech == WR 
		      || comm_mech == RECON) {
		    if (msgs[UnTail(tkn)].preroute == 0) {
		      /*------------------*/
		      /* Release the link */
		      /*------------------*/
		      release_link(ovl); 
		      if (circs[circ_no].complete)
			dec_circuits(ovl);
		      else
			dec_setup(ovl);

			/*--------------------------------*/
			/* DD (Deadlock Detection) Code   */
			/*--------------------------------*/

			/* Do this only if Cycle/Deadlock detection is ON */
			if (CD)
			{
				/* ... Remove the VC from the Circuit */
				error = RemoveVCFromCircuit(circ_no, ovl);

				/* if there's an error, report it */
				if (error)
				{
					printf("ERROR: from 'RemoveVCFromCircuit' - circ_no: %d, ovl: %d in 'intra_node_move'\n", circ_no, ovl);
					PrintAdjacencyList();
				}
			}
			/*--------------------------------*/
		    }
		  }
		}
	  }
	else
	  /*--------------------------------*/
	  /* Ack flit                       */
	  /*--------------------------------*/
	  if (TestAck (tkn))
	    {
	      int Ack_evap, cvl;
	      cvl = complement[ovl];
	      
	      control_collide++;
	      delay(vlinks[ovl].cnt, data_delay);

	      i = compvlinks[ovl].next;
	      Ack_evap=0;
	      if (i == (-1)) /* This path has been "killed" */
		{
		  remove_in_flit(ovl);
		} else {
		  pass = 1;
		  if (comm_mech == RECON) {
/*		    if (vlinks[complement[i]].acks >= PROBE_LEAD) */
		    if (vlinks[complement[i]].acks >= plinks[vlinks[ovl].pl].probe_register) 
		      pass=0;    /* Increment acks */
		  }
		  if (pass) {
		    if (compvlinks[i].out_buffer == 0) {
/*		      if (vlinks[cvl].acks < PROBE_LEAD) */
		      if (vlinks[cvl].acks < plinks[vlinks[ovl].pl].probe_register)
			vlinks[cvl].acks++;
		      if (vlinks[complement[i]].busy-1 == circ_no) 
			in_to_out (ovl, i, tkn);
		      else {
			report_evap("Ack", "link");
			remove_in_flit(ovl);
		      }
		    }
		  } else {
/*		    if (vlinks[cvl].acks < PROBE_LEAD) */
		    if (vlinks[cvl].acks < plinks[vlinks[ovl].pl].probe_register)
		      vlinks[cvl].acks++;
		    remove_in_flit(ovl);
		  }
		}
	    }
	  else
	    /*--------------------------------*/
	    /* Tail Ack (TAck)flit            */
	    /*--------------------------------*/
	    if (TestTAck (tkn))
	      {
		int TAck_evap;
		
		control_collide++;
		delay(vlinks[ovl].cnt, data_delay);

		i = compvlinks[ovl].next;
		TAck_evap=0;
	      
		if (i == (-1)) /* This path has been "killed" */
		  {
		    remove_in_flit(ovl);
		  } else {
		    vl = complement[i];
		    if (vlinks[vl].busy-1 == circ_no) {
		      /* printf("TAck: "); */
		      release_link(complement[ovl]);
		      dec_circuits(complement[ovl]);
		      in_to_out (ovl, i, tkn);
		    } else {
		      report_evap("TAck","link");
		      remove_in_flit(ovl);
		    }
		  }
	      }
	    else
	      /*--------------------------------*/
	      /* Back flit                      */
	      /*--------------------------------*/
	      if (TestBack (tkn))
		{
		  int nnn, s, d, v, Back_evap;
		  
		  control_collide++;
		  Back_evap = 0;
		  
		  /* Complementary link address */
		  vl = complement[ovl];
		
		  i = compvlinks[ovl].next;
		  if (i == (-1)) /* This path has been "killed" */
		    {
		      remove_in_flit(ovl);
		    } else {		/* At parent link */
			int ov;
		    
			vl = complement[i];
			ov = complement[ovl];
		    
			if (circ_no != vlinks[vl].busy-1) {
			  report_evap("Back","link");
			  remove_in_flit(ovl);
			} else 
			  if (corrvlinks[vl].in_buffer==0)
			    {
			      if (distance (n, circs[circ_no].dest) >
				  distance (nn, circs[circ_no].dest))
				circs[circ_no].cnt--;/* unprofitable link */
			      circs[circ_no].path_length--;
			      set_link_hist (vl);
			      release_link(ov);
			      frame_tkn_buff(ov, 0, OUT);
			      dec_setup(ov);
			      
			      remove_in_flit(ovl);
			      corrvlinks[vl].in_buffer = Pr (UnBack (tkn));
			      mark_tkn_buff(vl, next_in_flit(vl), IN);
			    }
		      }
		} else
		  /*--------------------------------*/
		  /* Forward Kill flit              */
		  /*--------------------------------*/
		  if (TestFKill (tkn))
		    {
		      control_collide++;
		      vl = vlinks[ovl].next;
		      if (vlinks[ovl].busy-1 != circ_no) {
			/* Collided with BKill or Back or TAck */
			printf("Link owner mismatch on FKill.\n");
			remove_in_flit(ovl);
			return;
		      } 
		      if (vl == (-1)) /* This path has been "killed" */
			{
			  remove_in_flit(ovl);
			} else {
			  int ov;
			  /* printf("FKill: "); */
			  release_link(ovl);
			  
			  vlinks[vl].back_block = 1;
			  flush_control_flits(vl);
			  flush_in_data_flits(ovl, UnFKill(tkn));
			  flush_out_data_flits(vl, UnFKill(tkn));
			
			  if (vlinks[vl].out_buffer)
			    plinks[vlinks[vl].pl].used--;
			  
			  vlinks[vl].out_buffer=0;
			  
			  in_to_out (ovl, vl, tkn);
			  if (circs[circ_no].complete)
			    dec_circuits(ovl);
			  else
			    dec_setup(ovl);
			}
		    }
		  else
		    /*--------------------------------*/
		    /* Backward Kill flit             */
		    /*--------------------------------*/
		    if (TestBKill (tkn))
		      {
			int BKill_evap;
			control_collide++;
			if (vlinks[complement[ovl]].busy-1 != circ_no) {
			  printf("Link owner mismatch on BKill.\n");
			  remove_in_flit(ovl);
			  return;
			}
			BKill_evap = 0;

			i = compvlinks[ovl].next;
			if (i == (-1)) /* This path has been "killed" */
			  {
			    remove_in_flit(ovl);
			  } else {
			    int ov;
			    
			    ov = complement[ovl];
			    flush_out_data_flits(ov, UnBKill(tkn));

			    release_link(ov);
			    frame_tkn_buff(ov, 0, OUT);
			  
			    vl = complement [i]; 
			    flush_in_data_flits(vl, UnBKill(tkn));
			    vlinks[vl].next = (-1);
			    vlinks[vl].forward_block = 1;
			    in_to_out (ovl, i, tkn);
			  }
		      }
    }
}

/*------------------------------------------------------------------*/
/* src_kill_stats()  Record statistics when a kill flit hits src    */
/*------------------------------------------------------------------*/
src_kill_stats(tkn)
     int tkn;
{
  src_kill_inform_time += (sim_time - msgs[tkn].t_intr);
  src_kill_informs++;
}

dest_kill_stats(){}

/*------------------------------------------------------------------*/
/* retry_fail_stats()  Record statistics when a message fails RETRY */
/*------------------------------------------------------------------*/
retry_fail_stats (tkn)
     int tkn;
{
  con_su--;
  injects--;
  retry_failures++;
}

/*------------------------------------------------------------------*/
/* done_msg_stats()  Record statistics when a message finishes      */
/*------------------------------------------------------------------*/
done_msg_stats (tkn)
     int tkn;
{
  int l, lng_l;
  int circ_no;

  if (msgs[tkn].t_inject <= t_init)
      return;

  if (HYBRID && (msgs[tkn].msglen == LMSGL))
  {

    /* printf("HYBRID %d  %d  %d  %d\n***", msgs[tkn].msglen, HYBRID, LMSGL, MSGL); */

    lng_tot_done++;
    lng_l = (sim_time - msgs[tkn].t_inject);
    lng_lat_ave += lng_l;
    if (lng_l >= 10000) 
      lng_l = 9999;
    if (lng_l <= 0) {
      printf("Nonpositive network latency (long message)!\n");
      lng_l=0;
    }
    lat_hist[lng_l]++;		/* update histogram bin */
    con_circs--;
    con_tacks++;
  
    msgs[tkn].msglen = 0;
    msgs[tkn].circ_index = (-1);

  }
  else
  {

    /* printf("NOT HYBRID %d  %d  %d  %d\n***", msgs[tkn].msglen, HYBRID, LMSGL, MSGL); */

    tot_done++;
    l = (sim_time - msgs[tkn].t_inject);
    lat_ave += l;
    lat_sqave += (float)l*l;
    if (l >= 10000) 
      l = 9999;
    if (l <= 0) {
      printf("Nonpositive network latency!\n");
      l=0;
    }
    lat_hist[l]++;		/* update histogram bin */
    con_circs--;
    con_tacks++;
    circ_no = msgs[tkn].circ_index;
  
    if (traffic == TRACEDRIVEN)
      rcv_call_back(circs[circ_no].dest, circs[circ_no].msgtype, 
  		  msgs[tkn].msglen);
    msgs[tkn].msglen = 0;
    msgs[tkn].circ_index = (-1);
  }
}

/*---------------------------------------------------------------------*/
/* done_msgack_stats()  Record statistics for a final acknowledge      */
/*---------------------------------------------------------------------*/
done_msgack_stats (tkn)
     int tkn;
{
  int l;
  if (msgs[tkn].t_inject <= t_init)
    return;
  tot_tacks++;
  l = (sim_time - msgs[tkn].t_inject);
  tacklat_ave += l;
  tacklat_sqave += (float)l*l;
  con_tacks--;
}

/*------------------------------------------------------------------*/
/* done_su_stats()  Record statistics when a setup finishes         */
/*------------------------------------------------------------------*/
done_su_stats (tkn,vl)
     int tkn,vl;
{
  int pvl, s, i;
  int circ_no;
  circ_no = msgs[tkn].circ_index;
  circs[circ_no].complete = sim_time;
  if (msgs[tkn].t_inject > t_init)
    {			/* Do only after transient */
      s = (sim_time + 1 - msgs[tkn].t_inject);
      su_ave += s;
      su_sqave += (float)s*s;
      if (s >= 500) 
	s = 499;
      if (s <= 0) {
	printf("Nonpositive setup time!\n");
	s=0;
      }
      su_hist[s]++;		/* update histogram bin */
      setups++;
      con_su--;
      con_circs++;
      path_length_ave += circs[circ_no].path_length;
      misroutes_ave += circs[circ_no].cnt;

      if (circs[circ_no].cnt >= 10) 
	mis_hist[9]++;
      else
	if (circs[circ_no].cnt < 0)
	  mis_hist[0]++;
	else
	  mis_hist[circs[circ_no].cnt]++;

      if (circs[circ_no].path_length >= 50) 
	path_hist[49]++;
      else
	if (circs[circ_no].path_length < 0) 
	  path_hist[0]++;
	else
	  path_hist[circs[circ_no].path_length]++;
    }
  
  /* Update circuit/setup counters */
  if (comm_mech == WR || comm_mech == AWR || comm_mech == RECON) {
    pvl = vl;
    while (pvl != -1) {
      inc_circuits(pvl);
      i = complement[pvl];
      pvl = complement[compvlinks[i].next];
    }
  } else {
    pvl = vl;
    while(pvl != -1) {
      inc_circuits(pvl);
      pvl = vlinks[pvl].next;
    }
  }
  if (msgs[tkn].preroute) {	 /* This message is simply a header. */
    msgs[tkn].circ_index = (-1); /* We can kill it now...            */
  }
}

/*------------------------------------------------------------------*/
/* check_stats()  Periodic check on statistics to give sim progress */
/*------------------------------------------------------------------*/
check_stats (i)
{
  static last_tot = 0, last_inj = 0, lng_last_tot = 0;

  if (!TRANSIENT)
    return;
  if (sim_time - t_init < 300)
    {
      last_tot = tot_done;
      last_inj = injects;
   
      if (HYBRID)
      {
        lng_last_tot = lng_tot_done;
      }
    }
  
  printf ("     %6s  %11s  %11s  %11s  %6s  %6s  %6s  %6s  %9s  %7s  %11s\n", 
	  "Time",
	  "Ave Msg Lat", 
	  "Ave Msg SU", 
	  "Ave Setups", 
	  "TotDone", 
	  "NewDone",
	  "Inject", 
	  "NewIn",
	  "Disha-Rec",
          "DB Util",
          "Tok. Wait"
	  );
  
  /* ave_su = average number of concurrent setups 
     su_ave = average setup time 
     setups = completed setups */
  printf ("=====%6d  %11.2f  %11.2f  %11.2f  %7d  %7d  %6d  %6d  %9d %8d\n", 
	  i + 1,
	  (float) lat_ave / tot_done, 
	  (float) su_ave / setups,
	  (float) ave_su / (sim_time - t_init), 
	  tot_done, 
	  tot_done - last_tot,
	  injects, 
	  injects - last_inj,
	  seize_count,
	  DEADLOCK_BUFFER_UTILIZED
/*	  (float) seize_wait_time / seize_count */
	);
  last_tot = tot_done;
  temp_tot_done = tot_done;
  last_inj = injects;


  if (HYBRID)
  {
    printf ("-----%6d  %11.2f  %33d  %7d\n", 
    	  i + 1,
	  (float) lng_lat_ave / lng_tot_done, 
	  lng_tot_done, 
	  lng_tot_done - lng_last_tot
	);
    lng_last_tot = lng_tot_done;
    lng_temp_tot_done = lng_tot_done;
  }

}

/*------------------------------------------------------------------*/
/* report_stats()  Report all statistics for simulation             */
/*------------------------------------------------------------------*/
report_stats ()
{
  int i, j;
  float p, q, sum, rd[33], sd[33], cd[33], m1, m2, q_var;
  float rhd[33], rfcd[33], rhcd[33];
  float m_cmax[33];
  float rtot, stot, ctot, mr, ms, mc, md, ma;
  float rhtot, rfctot, rhctot, mbmtot;
  float LM, l, M, S, T, N, n, cum;
  FILE *fm;
  float t_su, t_m, t_sv, t_p, t_l, t_rr, nu_c, nu_B, t_rpr;
  float m_c, Beta_tr, p_max;
  
  LM = MSGL;
  l = abs (DIST);
  M = (float) ave_circs / (sim_time - t_init);
  S = (float) ave_su / (sim_time - t_init);
  T = (float) ave_tacks / (sim_time - t_init);
  N = NODES;
  n = dimensions;

  m2 = q_sqave/injects;
  m1 = (float)q_ave/injects;
  q_var = m2 - m1*m1;

  m2 = lat_sqave/tot_done;
  m1 = (float)lat_ave/tot_done;
  
  printf ("Simulation Time:           %d\n", sim_time);

  if (!HYBRID)
  {
    printf ("Total Done:              %d\n",temp_tot_done);
  }
  else
  {
    printf ("Total Done (short):        %d\n",temp_tot_done);
    printf ("Total Done (long):         %d\n",lng_temp_tot_done);
    printf ("Total Done (all):          %d\n",temp_tot_done + lng_temp_tot_done);
  }

  if (!HYBRID)
  {
  	throughput = (float)(temp_tot_done*MSGL)/(NODES*(sim_time - 10000));
  }
  else
  {
  	throughput = (float)((temp_tot_done*MSGL) + (lng_temp_tot_done*LMSGL))/(NODES*(sim_time - 10000));
  }

  printf ("Throughput:                %f\n", throughput);


  if (!HYBRID)
  {
    printf ("Average message latency:   %f\n", (float) lat_ave / tot_done
	  + (float) q_ave / injects);
  }
  else
  {
    printf ("Average message latency (short):  %f\n", (float) lat_ave / tot_done
	  + (float) q_ave / injects);

    printf ("Average message latency (long):   %f\n", (float) lng_lat_ave / lng_tot_done
	  + (float) q_ave / injects);

    printf ("Average message latency (all):    %f\n", (float) (lat_ave + lng_lat_ave) / (tot_done + lng_tot_done)
	  + (float) q_ave / injects);

  }

  printf("Latency std:                %f\n", (float) sqrt(m2 - m1*m1 + q_var));
  printf ("Average message setup:     %f\n", (float) su_ave / setups);
  t_su = (float) su_ave / setups;
  t_m = (float) lat_ave / tot_done;
  m2 = su_sqave/setups;
  m1 = (float)su_ave/setups;
  printf("Setup std:                  %f\n",(float)sqrt(m2 - m1*m1));
  printf ("Average message queue time: %f\n", (float) q_ave / injects);
  printf("Queue time std:             %f\n", (float)sqrt(q_var));
  if (comm_mech == APCS) {
    printf("Average latency to TAck:    %f\n",(float) tacklat_ave/tot_tacks
	   + (float) q_ave/injects);
    m2 = tacklat_sqave/tot_tacks;
    m1 = (float)tacklat_ave/tot_tacks;
    printf("TAck latency std:           %f\n",(float)sqrt(m2 - m1*m1 + q_var));
    printf("Average concurrent TAcks:   %f\n", T);
  }
  printf ("Average concurrent circs:  %f\n", M);
  printf ("Average concurrent setups: %f\n", S);
  printf ("Average path length:       %f\n",(float)path_length_ave/setups);
  printf ("Average misroutes taken:   %f\n",(float)misroutes_ave/setups);
  printf ("Total flits delivered:     %d\n", flits_delivered);
  printf ("Total setups:              %d\n", setups);
  printf ("Total messages done:       %d\n", tot_done);
  printf ("\nPercentage of retry failures %f\n",(float)retry_failures/injects);

  printf("Mean time to inform src of dynamic fault: %f\n", 
	 (src_kill_informs > 0 ? 
	  (float)src_kill_inform_time/src_kill_informs : 
	  0.0));

  printf("Control flit collision percentage: %f\n",((float)cum_control_collide)
	 /control_moves);

  printf("Disha Recovery Procedures: %d\n", seize_count);
/*  printf("Disha Average Wait Time to Capture Token: %f\n", (float) seize_wait_time/seize_count); */

  if (traffic == TRACEDRIVEN) {
    printf("\nPE idle time:             %f\n",
	   ((float)cum_idle_pes)/NODES/sim_time);
    printf("\nPE overhead time:         %f\n",
	   ((float)cum_over_pes)/NODES/sim_time);
    printf("\nTotal idle cycles:        %d\n", cum_idle_pes);
    printf("\nTotal overhead cycles:    %d\n", cum_over_pes);
  }

/*  
  printf ("\nP_B = %f\n", P_B / paths_checked);
  printf ("P_U = %f\n", P_U / paths_checked); 
*/
  printf ("\n\n");
  write_hist(su_hist,500,"su.hist");
  write_hist(lat_hist,10000,"lat.hist");
  write_hist(path_hist,50,"path.hist");
  write_hist(mis_hist,10,"misroutes.hist");
  if (TRANSIENT)
    {
/*      printf ("Vlink distributions:\n");
      rtot = 0;
      rhtot = 0;
      rfctot = 0;
      rhctot = 0;
      stot = 0;
      ctot = 0;
      for (i = 0; i < 2 * virts + 1; i++)
	{
	  rtot += rdist[i];
	  rhtot += rhdist[i];
	  rfctot += rfcdist[i];
	  rhctot += rhcdist[i];
	  stot += sdist[i];
	  ctot += cdist[i];
	}
      mr = 0.0;
      mc = 0.0;
      ms = 0.0;
      md = 0.0;
      for (i = 0; i < 2 * virts + 1; i++)
	{
	  rd[i] = (float) rdist[i] / rtot;
	  rhd[i] = (float) rhdist[i] / rhtot;
	  rfcd[i] = (float) rfcdist[i] / rfctot;
	  rhcd[i] = (float) rhcdist[i] / rhctot;
	  sd[i] = (float) sdist[i] / stot;
	  cd[i] = (float) cdist[i] / ctot;
	  mr += rd[i] * i;
	  mc += cd[i] * i;
	  ms += sd[i] * i;
	} 
*/
     
 /*	}  else {
		printf("Distribution of busy vlinks:\n");
		mtot=0;
		for (i=0; i<2*virts+1; i++)
		for (j=0; j<2*virts+1; j++) {
		mtot += mdist2[i][j];
		}
		m = 0;
		for (i=0; i<2*virts+1; i++)
		md[i]=0;
		sum = 0.0;
		for (i=0; i<2*virts+1; i++) {
		for (j=0; j<2*virts+1; j++) {
		printf("%6.4f ", (float)mdist2[i][j]/mtot);
		m += (float)(i+j)*mdist2[i][j]/mtot;
		sum += (float)mdist2[i][j]/mtot;
		if (i+j<2*virts+1)
		md[i+j] += (float)mdist2[i][j]/mtot;
		}
		printf("\n");
		}
		} 
*/

/*
      ma = (M * (l * (LM - l) + l * (l + 1.0) / 2.0) / LM 
      + S * l / 2.0) / (2 * N * n);
      printf ("Reserved half link load %f analytical, %f actual\n", ma, mr);
      for (i = 0; i < virts + 1; i++)
	printf ("%6.4f ", rd[i]);
      printf ("\n");
      for (i = 0; i < virts + 1; i++)
	printf ("%6.4f ", rfcd[i]);
      printf ("\n");
      if (HALF)
	{
	  for (i = 0; i < 2 * virts + 1; i++)
	    printf ("%6.4f ", rhd[i]);
	  printf ("\n");
	  for (i = 0; i < 2 * virts + 1; i++)
	    printf ("%6.4f ", rhcd[i]);
	  printf ("\n");
	}
      printf ("\n");
      ma = (S * l / 2.0) / (2 * N * n);
      printf ("Setup half link load %f analytical, %f actual\n", ma, ms);
      for (i = 0; i < virts + 1; i++)
	printf ("%6.4f ", sd[i]);
      printf ("\n");
      printf ("\n");
      ma = (M * (l * (LM - l) + l * (l + 1.0) / 2.0) / LM) / (2 * N * n);
      printf ("Circuit half link load %f analytical, %f actual\n", ma, mc);
      for (i = 0; i < virts + 1; i++)
	printf ("%6.4f ", cd[i]);
      printf ("\n");
      if (HALF)
	{
	  m_cmax[0] = 0;
	  printf ("%f ", m_cmax[0]);
	  for (i = 1; i <= 2 * virts; i++)
	    {
	      m_cmax[i] = 0;
	      for (j = i; j <= virts; j++)
		{
		  if (i - j <= virts)
		    m_cmax[i] += cd[i - j] * cd[j] / (1.0 - cd[0]);
		}
	      printf ("%f ", m_cmax[i]);
	    }
	  printf ("\n");
	}
      else
	{
	  m_cmax[0] = 0;
	  printf ("%f ", m_cmax[0]);
	  for (i = 1; i <= virts; i++)
	    {
	      m_cmax[i] = cd[i] / (1.0 - cd[0]);
	      printf ("%f ", m_cmax[i]);
	    }
	  printf ("\n");
	}
      m_c = 0.0;
      mc = 0.0;
      cum = 0.0;
      for (i = 0; i <= (HALF ? 2 : 1) * virts; i++)
	{
	  m_c += i * (pow (cum + m_cmax[i], (float) l) - pow (cum, (float) l));
	  mc += i * m_cmax[i];
	  cum += m_cmax[i];
	}
      if (!HALF)
	{
	  m_c = m_c * 2;
	  mc = mc * 2;
	}
      
      printf ("\n");
      printf ("\n"); 
*/
      
/*      printf ("\nlambda_n = %f\n", (float) inject_rate / PER);
      printf ("lambda_i = %f\n", (float) injects / (N * (sim_time - t_init)));
      printf ("lambda_N = %f\n", (float) flits_delivered /
	      ((sim_time - t_init) * 0.3296 * N));
      if (!HALF)
	blk_ave *= 2;
      printf ("Blocked route attempts:   %d\n", p_blk);
      printf ("Total links crossed:      %d\n", tot_traversed);
      printf ("Average hops (backtrack): %f\n", 
	      (float) tot_traversed / injects);
      printf ("Probability of blocking:  %f\n", (float) p_blk /
	      (tot_traversed - p_blk));
      printf ("Average blocking time:    %f\n", (float) blk_ave / injects);
      printf ("Average wait/block:       %f\n", (float) blk_ave / p_blk);
      printf ("M = %f,  S = %f\n", M, S);
      printf ("\n"); 
*/
    }
  
  if (MAP)
    {
      /* Reserve map */
      fm = fopen ("mapr", "w");
      fprintf (fm, "%d", dimensions);
      for (i = 0; i < dimensions; i++)
	fprintf (fm, "\t%d", width[i]);
      fprintf (fm, "\n");
      if (HALF)
	fprintf (fm, "1\n");
      else
	fprintf (fm, "0\n");
      for (i = 0; i < NODES * dimensions; i++)
	{
	  if (HALF)
	    fprintf (fm, "%f\n", (float)
		     (plinks[i].crl + plinks[i].crr) / (sim_time - t_init));
	  else
	    fprintf (fm, "%f\t%f\n",
		     (float) (plinks[i].crl) / (sim_time - t_init),
		     (float) (plinks[i].crr) / (sim_time - t_init));
	}
      fclose (fm);
      
      /* Circuit map */
      fm = fopen ("mapc", "w");
      fprintf (fm, "%d", dimensions);
      for (i = 0; i < dimensions; i++)
	fprintf (fm, "\t%d", width[i]);
      fprintf (fm, "\n");
      if (HALF)
	fprintf (fm, "1\n");
      else
	fprintf (fm, "0\n");
      for (i = 0; i < NODES * dimensions; i++)
	{
	  if (HALF)
	    fprintf (fm, "%f\n", (float)
		     (plinks[i].ccl + plinks[i].ccr) / (sim_time - t_init));
	  else
	    fprintf (fm, "%f\t%f\n",
		     (float) (plinks[i].ccl) / (sim_time - t_init),
		     (float) (plinks[i].ccr) / (sim_time - t_init));
	}
      fclose (fm);
      
      /* Setup map */
      fm = fopen ("maps", "w");
      fprintf (fm, "%d", dimensions);
      for (i = 0; i < dimensions; i++)
	fprintf (fm, "\t%d", width[i]);
      fprintf (fm, "\n");
      if (HALF)
	fprintf (fm, "1\n");
      else
	fprintf (fm, "0\n");
      for (i = 0; i < NODES * dimensions; i++)
	{
	  if (HALF)
	    fprintf (fm, "%f\n", (float)
		     (plinks[i].csl + plinks[i].csr) / (sim_time - t_init));
	  else
	    fprintf (fm, "%f\t%f\n",
		     (float) (plinks[i].csl) / (sim_time - t_init),
		     (float) (plinks[i].csr) / (sim_time - t_init));
	}
      fclose (fm);
    }
}


/*------------------------------------------------------------------*/
/* Xreport_stats()  Report all statistics for simulation             */
/*------------------------------------------------------------------*/
Xreport_stats ()
{
  
}


write_hist(hist,num,file)
     char *file;
     int *hist,num;
{
  FILE *fp;
  int i;

  fp = fopen(file,"w");
  for (i=0; i<num; i++) {
    if (hist[i] != 0)
      fprintf(fp,"%d\t%d\n",i,hist[i]);
  }
  fclose(fp);
}
