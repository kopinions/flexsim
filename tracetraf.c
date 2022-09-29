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
#include <stdio.h>

extern node *nodes;		/* These variables hold the network state */

extern int dimensions, *width, numvlinks, numnvlinks, numplinks, *part,
  *cum_part;
extern int PL, MSGL, inject_rate, DIST, RUN_TIME, HALF, DEMAND, PER, M, MAP;
extern int NODES, virts, node_virts, comm_mech, FAULTS, ABORT, NO_CTS;
extern int BUFFERS, SECURITY_LEVELS;
extern int nvlog, vlog, dimlog, nlog, protocol, backtrack;
extern int HSPOTS, *hsnodes, HSPERCENT;
extern int route_delay, select_fnc, data_delay;
extern int link_time, ack_time, link_arb_time;
extern char proto_name[40];
extern int RADIUS, RAD, ORDER, RETRY;
extern int MESH, SRC_QUEUE, PROBE_LEAD;
extern int *dyn_fault_times, num_dyn_faults, dyn_cnt, physical_faults;
extern int traffic, sim_time;
extern int FLIT_SIZE;
extern int EXECUTE_CYCLES;
extern int RCV_OVER, RCV_OVER_PER;
extern int SEND_OVER, SEND_OVER_PER;
extern int THREAD_OVER, REL_OVER, PRE_OVER;
extern int PROFILE, msgflits;
extern int oldflits, old_idle, old_over;
extern int con_circs;

extern int idle_pes, cum_idle_pes, over_pes, cum_over_pes;
extern int activity;
int last_idle_pe_update;

extern int t1[16], t2[16], n[16];

extern msg msgs[MAXMESSAGE];	/* Misc information necessary for each msg */
extern circuit circs[MAXCIRC];

#define EXE     'E'		/* Execute event  */
#define SND     'S'		/* Send event     */
#define RCV     'R'		/* Receive event  */
#define OVR     'O'		/* Overhead event */
#define CMT     'C'		/* Comment event  */
#define DLN     'D'		/* Deadline event */
#define WAT     'W'		/* Wait event     */

#define PRE     'P'		/* Preroute setup    */
#define REL     'T'             /* Prerouted release */

typedef struct circ_des {
  int msgtype, dest, v, tkn;
  struct circ_des *next;
} circuit_descriptor;

typedef struct ev{
  short type, thread;
  union {
    int ex_cycles;
    int msgtype; 
    struct send { int msgtype, to, len;} send;
    char *cmt;
    int deadline;
  } data;
  struct ev *next, *prev;
} event;

typedef struct th {
  int id, node, ready;
  event *head, *tail;
  struct th *next;
  circuit_descriptor *cd;
} thread;

typedef struct rrec {
  event *recv;
  struct rrec *next;
} recv_record;

extern int min_event_time;

int last_deadline=0;
int num_empty;
thread **ready_head, **ready_tail;
int    *last_ready;
thread *all_threads;
int num_threads;

recv_record **recvs, **recv_tail;
int *event_start;
int *send_list, *execute_list;
int send_length, execute_length;
FILE *tracefile, *schedule_file;
extern FILE *commprofile, *peprofile, *overprofile, *blkprofile;
char *malloc();

/*--------------------------------------------------------------*/
/* Trace-driven traffic generator for flitsim                   */
/*--------------------------------------------------------------*/
/* init_trace()  Open trace file                                */
/*--------------------------------------------------------------*/
init_trace(file)
     char *file;
{
  int i;

  oldflits=0;
  old_idle=0;
  old_over=NODES;
  idle_pes = 0;
  cum_idle_pes = 0;
  over_pes = 0;
  cum_over_pes = 0;
  last_idle_pe_update = 0;

  send_length = 0;
  execute_length = 0;

  tracefile = fopen(file,"r");
  if (!tracefile) {
    printf("Cannot open trace file.\n");
    exit(1);
  }
  fscanf(tracefile,"%d",&num_threads);

  if (PROFILE) {
    schedule_file = fopen("schedule","w");
  }
  all_threads = (thread *)malloc(sizeof(thread)*num_threads);
  make_thread_map(num_threads);
  for (i=0; i<num_threads; i++) {
    all_threads[i].id = i;
    all_threads[i].node = map_thread(i);
    all_threads[i].ready = 0;
    all_threads[i].head = NULL;
    all_threads[i].tail = NULL;
    all_threads[i].next = NULL;
    all_threads[i].cd = NULL;
  }
  num_empty = num_threads;
  
  last_ready = (int *)malloc(sizeof(int)*NODES);
  ready_head = (thread **)malloc(sizeof(thread *)*NODES);
  ready_tail = (thread **)malloc(sizeof(thread *)*NODES);
  recvs = (recv_record **) malloc(sizeof(recv_record *)*NODES);
  recv_tail = (recv_record **) malloc(sizeof(recv_record *)*NODES);
  send_list = (int *)malloc(sizeof(int)*NODES);
  execute_list = (int *)malloc(sizeof(int)*NODES);
  event_start = (int *)malloc(sizeof(int)*NODES);

  for (i=0; i<NODES; i++) {
    last_ready[i] = -1;
    recvs[i] = NULL;
    recv_tail[i] = NULL;
    ready_head[i] = NULL;
    ready_tail[i] = NULL;
    event_start[i] = 0;
  }
  read_events();
  if (PROFILE) {
    commprofile = fopen("commprofile","w");
    peprofile = fopen("peprofile","w");
    overprofile = fopen("overprofile","w");
    blkprofile = fopen("blockprofile","w");
    fprintf(commprofile,"0\t0\n");
    fprintf(peprofile,"0\t%d\n", NODES);
    fprintf(overprofile,"0\t%d\n", NODES);
    fprintf(blkprofile,"0\t0\n");
  }
  for (i=0; i<NODES; i++)
    schedule(i);
}

/*--------------------------------------------------------------*/
/* find_cd(th, msgtype)  Find a prerouted circuit for msgtype */
/*--------------------------------------------------------------*/
circuit_descriptor *find_cd(id, msgtype, dest)
     int id, msgtype, dest;
{
  circuit_descriptor *c;
  thread *th;
  th = &all_threads[id];
  c = th->cd;
  while (c) {
    if (c->msgtype == msgtype && c->dest == dest) return c;
    c = c->next;
  }
  return 0;
}

/*--------------------------------------------------------------*/
/* delete_circ()  Delete a prerouted circuit                    */
/*--------------------------------------------------------------*/
delete_circ(id, cd)
     int id;
     circuit_descriptor *cd;
{
  circuit_descriptor *c;
  thread *th;

  th = &all_threads[id];
  c = th->cd;
  if (!c) return;
  if (c == cd) {		/* First element in list */
    th->cd = c->next;
    free(cd);
    return;
  }
  while (c->next) {		/* Subsequent elements */
    if (c->next == cd) {
      c->next = cd->next;
      free(cd);
      return;
    } else
      c = c->next;
  }
}

/*--------------------------------------------------------------*/
/* add_circ()  Add a prerouted circuit to a thread              */
/*--------------------------------------------------------------*/
add_circ(id, c)
     int id;
     circuit_descriptor *c;
{
  thread *th;
  th = &all_threads[id];
  c->next = th->cd;
  th->cd = c;
}

/*--------------------------------------------------------------*/
/* ready(id)    Add a thread to its node's ready queue          */
/*--------------------------------------------------------------*/
ready(id) {
  int node;

  if (all_threads[id].ready) return;
  all_threads[id].ready = 1;

  node = all_threads[id].node;
  if (ready_head[node] != NULL)
    ready_tail[node]->next = &all_threads[id];
  else {
    ready_head[node] = &all_threads[id];
    ready_head[node]->next = NULL;
  }
  ready_tail[node] = &all_threads[id];
}

/*--------------------------------------------------------------*/
/* suspend(id)    Remove a thread to its node's ready queue     */
/*--------------------------------------------------------------*/
suspend(id) {
  int node;
  thread *th;

  if (all_threads[id].ready == 0) return;
  all_threads[id].ready = 0;

  node = all_threads[id].node;
  if (ready_head[node] != NULL) {
    th = ready_head[node]->next;
    if (ready_tail[node] == ready_head[node])
      ready_tail[node] = th;
    ready_head[node]->next = NULL;
    ready_head[node] = th;
  }
}

/*--------------------------------------------------------------*/
/* read_events()  Read events until end of file or each thread  */
/*                has at least one event                        */
/*--------------------------------------------------------------*/
read_events()
{
  event *e;
  while (!feof(tracefile) && num_empty>0) {
    get_event(&e);
    if (e->type)
      queue_event(e);
  }
}

/*--------------------------------------------------------------*/
/* queue_event()  Put an event in its proper thread queue       */
/*--------------------------------------------------------------*/
queue_event(e)
     event *e;
{
  if (e->thread > num_threads) {
    printf("Trace file is inconsistent!\n");
    printf("\tThread id out of range\n");
    exit(1);
  }
  if (all_threads[e->thread].head == (event *)NULL) {
    all_threads[e->thread].tail = NULL;
    insert_event(e);
    proc_new_head(e);
    num_empty--;	/* Number of empty thread lists decreased */
  } else
    insert_event(e);

  if (e->type == RCV) {		/* Post to receive list for node */
    recv_record *rr;
    int node;
    node = all_threads[e->thread].node;
    rr = (recv_record *)malloc(sizeof(recv_record));
    rr->recv = e;
    rr->next = NULL;
    if (recvs[node] == NULL) {
      recvs[node] = rr;
      recv_tail[node] = rr;
    } else {
      recv_tail[node]->next = rr;
      recv_tail[node] = rr;
    }
  }
}

/*--------------------------------------------------------------*/
/* read_for_thread_event()  Read events until end of file or an */
/*                event with matching thread id                 */
/*--------------------------------------------------------------*/
read_for_thread_event(id)
{
  event *e;
  while (!feof(tracefile)) {
    get_event(&e);
    if (e->type) {
      queue_event(e);
      if (e->thread == id)
	return 1;
    }
  }
  return 0;
}

/*--------------------------------------------------------------------*/
/* read_for_rcv_event() Read events until end of file or a rcv event  */
/*                      with matching thread id and msgtype is read   */
/*--------------------------------------------------------------------*/
read_for_rcv_event(n, msgtype)
{
  event *e;
  int dest;
  while (!feof(tracefile)) {
    get_event(&e);
    if (e->type) {
      queue_event(e);
      dest = map_thread(e->thread);
      if (dest == n && e->data.msgtype == msgtype)
	return 1;
    }
  }
  return 0;
}

/*--------------------------------------------------------------*/
/* get_event()  Get an event from the trace file                */
/*--------------------------------------------------------------*/
get_event(ev)
     event **ev;
{
  char s[40];
  int n;
  event *e, *o;
  e = (event *)malloc(sizeof(event));
  *ev = e;
  e->next = NULL;
  e->prev = NULL;

  s[0]=0;
  fscanf(tracefile, "%s",&s);
  e->type = s[0];
  fscanf(tracefile, "%d",&n);
  e->thread = n;

  switch(s[0]) {
  case CMT:
    fscanf(tracefile,"%s",s);
    e->data.cmt = (char *)malloc((strlen(s)+1)*sizeof(char));
    strcpy(e->data.cmt,s);
    break;
  case DLN:
    fscanf(tracefile,"%d",&e->data.deadline);
    e->data.deadline *= EXECUTE_CYCLES;
    break;
  case EXE:
    fscanf(tracefile,"%d",&e->data.ex_cycles);
    e->data.ex_cycles *= EXECUTE_CYCLES;
    break;
  case WAT:
    fscanf(tracefile,"%d",&e->data.ex_cycles);
    e->data.ex_cycles *= EXECUTE_CYCLES;
    break;
  case PRE:
    fscanf(tracefile,"%d",&e->data.send.msgtype);
    fscanf(tracefile,"%d",&e->data.send.to);
    e->data.send.msgtype = e->data.send.msgtype + e->data.send.to*1000;
    e->data.send.len = 0;
    if (PRE_OVER > 0) {
      o = (event *)malloc(sizeof(event));
      o->thread = e->thread;
      *ev = o;
      o->next = e;
      e->prev = o;
      o->type = OVR;
      o->data.ex_cycles = PRE_OVER*EXECUTE_CYCLES;
    }
    break;
  case REL:
    fscanf(tracefile,"%d",&e->data.send.msgtype);
    fscanf(tracefile,"%d",&e->data.send.to);
    e->data.send.msgtype = e->data.send.msgtype + e->data.send.to*1000;
    e->data.send.len = 0;
    if (REL_OVER > 0) {
      o = (event *)malloc(sizeof(event));
      o->thread = e->thread;
      *ev = o;
      o->next = e;
      e->prev = o;
      o->type = OVR;
      o->data.ex_cycles = REL_OVER*EXECUTE_CYCLES;
    }
    break;
  case SND:
    fscanf(tracefile,"%d",&e->data.send.msgtype);
    fscanf(tracefile,"%d",&e->data.send.to);
    fscanf(tracefile,"%d",&e->data.send.len);
    e->data.send.msgtype = e->data.send.msgtype + e->data.send.to*1000;
    if (SEND_OVER + SEND_OVER_PER*e->data.send.len > 0) {
      o = (event *)malloc(sizeof(event));
      o->thread = e->thread;
      *ev = o;
      o->next = e;
      e->prev = o;
      o->type = OVR;
      o->data.ex_cycles = (SEND_OVER + SEND_OVER_PER*e->data.send.len)
	*EXECUTE_CYCLES;
    }
    if (e->data.send.len > FLIT_SIZE) 
      e->data.send.len /= FLIT_SIZE;
    else
      e->data.send.len = 1;
    break;
  case RCV:
    fscanf(tracefile,"%d",&e->data.msgtype);
    fscanf(tracefile,"%d",&e->data.send.len);
    e->data.msgtype = e->data.msgtype + e->thread*1000;
    if ((RCV_OVER + RCV_OVER_PER*e->data.send.len) > 0) {
      o = (event *)malloc(sizeof(event));
      o->thread = e->thread;
      e->next = o;
      o->prev = e;
      o->type = OVR;
      o->data.ex_cycles = (RCV_OVER + RCV_OVER_PER*e->data.send.len)
	*EXECUTE_CYCLES;
      o->next = NULL;
    }
    if (e->data.send.len > FLIT_SIZE) 
      e->data.send.len /= FLIT_SIZE;
    else
      e->data.send.len = 1;
    break;
  }
}

/*--------------------------------------------------------------*/
/* insert_event()  Put an event at the end of a list            */
/*--------------------------------------------------------------*/
insert_event(e)
     event *e;
{
  thread *th;
  th = &all_threads[e->thread];

  if (th->tail != NULL) {
    th->tail->next = e;
    e->prev = th->tail;
  } else
    th->head = e;
  while (e) {			/* inserted sequence may be a list */
    th->tail = e;
    e = e->next;
  }
}

/*--------------------------------------------------------------*/
/* delete_event()  Remove an event from the head of a list      */
/*--------------------------------------------------------------*/
delete_event(id)
{
  event *e;
  thread *th;

  th = &all_threads[id];
  if (th->head != NULL) {
    e = th->head->next;
    if (th->tail == th->head)
      th->tail = e;
    if (th->head->type == CMT) 
      free(th->head->data.cmt);
    free(th->head);
    th->head = e;
    if (e) {
      e->prev = NULL;
      proc_new_head(e);
    } else
      num_empty++;
  } 
  if (num_empty>0) read_events();
}

/*--------------------------------------------------------------*/
/* schedule()  Update task scheduling on node                   */
/*--------------------------------------------------------------*/
schedule(n) {
  int type; 
  event *e;
  if (ready_head[n] == NULL) {	/* Is PE idle? */
    if (nodes[n].idle == 0) {
      idle_pes++;
      nodes[n].idle = 1;
    }
    if (nodes[n].over == 1) {
      over_pes--;
      nodes[n].over = 0;
    }
#ifndef NO_X
    thread_node(n, -1);
    mark_node(n, RCV);
#endif
    return;
  }
  if (last_ready[n] != ready_head[n]->id
      && THREAD_OVER > 0) { /* Thread swap overhead */
    e = (event *)malloc(sizeof(event));
    e->thread = ready_head[n]->id;
    e->type = OVR;
    e->data.ex_cycles = THREAD_OVER;
    e->next = ready_head[n]->head;
    ready_head[n]->head = e;
  }
  type = ready_head[n]->head->type;
  if (type == EXE || type == OVR || type==WAT) {
    if (nodes[n].idle) {	/* PE idle time */
      idle_pes--;
      nodes[n].idle = 0;
    }
    if (nodes[n].over==0 && (type == OVR || type==WAT)) { 
      /* PE overhead time */
      over_pes++;
      nodes[n].over = 1;
    }
    if (nodes[n].over==1 && type == EXE) { /* PE computation time */
      over_pes--;
      nodes[n].over = 0;
    }
#ifndef NO_X
    thread_node(n, ready_head[n]->id);
    mark_node(n, type);
#endif
    execute_list[execute_length] = n;
    event_start[n] = sim_time;
    if (PROFILE) {
      fprintf(schedule_file, "%d %d %d\n", n, sim_time, 
	      ready_head[n]->head->data.ex_cycles);
    }
    execute_length++;
  } else {
    if (type == SND || type == PRE || type == REL) {
      if (nodes[n].idle == 0) {
	idle_pes++;
	nodes[n].idle = 1;
      }
      if (nodes[n].over == 1) {
	over_pes--;
	nodes[n].over = 0;
      }
#ifndef NO_X
      thread_node(n, ready_head[n]->id);
      mark_node(n, SND);
#endif
      send_list[send_length] = n;
      event_start[n] = sim_time;
      send_length++;
    }
  }
  last_ready[n] = ready_head[n]->id;
}

/*--------------------------------------------------------------*/
/* proc_new_head()  A new head event needs to be processed      */
/*--------------------------------------------------------------*/
proc_new_head(e)
     event *e;
{
  int sched;
  thread *th;
  
/*  th = &all_threads[e->thread];
  if (!ready_head[th->node])
    sched = 1; */
  if (e->type == CMT) {
    printf("%s\n", e->data.cmt);
    delete_event(e->thread);
  } else if (e->type == DLN) {
    printf("Deadline %d\n", e->data.deadline+last_deadline);
    if (sim_time <= e->data.deadline+last_deadline)
      printf("Deadline met at %d.\n", sim_time);
    else
      printf("Deadline exceeded at %d.\n", sim_time);
/*    last_deadline = sim_time; */
    delete_event(e->thread);
  } else if (e->type != RCV) {
    ready(e->thread);
  }
}

/*--------------------------------------------------------------*/
/* delete_from_list()                                           */
/*--------------------------------------------------------------*/
delete_from_list(list, i, len)
     int *list, i, *len;
{
  int j;
  for (j=i; j< (*len)-1; j++) 
    list[j] = list[j+1];
  *len = *len-1;
}

/*--------------------------------------------------------------*/
/* release_circuit()  Release circuit                           */
/*--------------------------------------------------------------*/
release_circuit(id, cd)
     int id;
     circuit_descriptor *cd;
{
  int src, mr;
  src = map_thread(id);
  if ( nodes[src].send[cd->v] == HOLDER) {
    mr = find_msg();
    msgs[mr].circ_index = cd->tkn;
    msgs[mr].preroute = 0;
    nodes[src].send[cd->v] = Tail(mr);
  } else {			/* Mark current message as releaser */
    mr = UnToken(nodes[src].send[cd->v]);
    msgs[mr].preroute = 0;
  }
  delete_circ(id, cd);
}

/*--------------------------------------------------------------*/
/* prerouted_send()                                             */
/*--------------------------------------------------------------*/
prerouted_send(cd, src, dest, len)
     int dest, src, len;
     circuit_descriptor *cd;
{
  int m;
  m = find_msg();
  msgs[m].circ_index = cd->tkn;
  msgs[m].t_inject = sim_time;
  msgs[m].t_queue = sim_time;
  msgs[m].msglen = len;
  msgs[m].preroute = 1;
  nodes[src].generate[cd->v] = len;
  nodes[src].send[cd->v] = Data(m);
}

/*--------------------------------------------------------------*/
/* inject_trace()  Handle events from the trace                 */
/*--------------------------------------------------------------*/
inject_trace() 
{
  int i, j, k, n, dest;
  int msgtype, len, type, id, cycles;
  circuit_descriptor *cd;
  int m, c;

  cum_idle_pes += (sim_time - last_idle_pe_update)*idle_pes;
  cum_over_pes += (sim_time - last_idle_pe_update)*over_pes;
  last_idle_pe_update = sim_time;
  min_event_time = 2000000000;
  for (i=execute_length-1; i>=0; i--) {
    n = execute_list[i];
    if (ready_head[n] == NULL) {
      printf("Null ready list on execute list\n");
    } else {
      id = ready_head[n]->id;
      type = ready_head[n]->head->type;
      cycles = ready_head[n]->head->data.ex_cycles;
      if (type != EXE && type != OVR && type != WAT) 
	printf("Non-execute event in the executelist\n");
      else {
	k = event_start[n] + cycles - 1;
	if (k <= sim_time || (cycles <= sim_time && type == WAT)) {
	  min_event_time = sim_time;
	
	  delete_from_list(execute_list, i, &execute_length);
	  if (type == EXE)
	    suspend(id);
	  delete_event(id);
	  schedule(n);
	} else
	  if (min_event_time > k)
	    min_event_time = k;
      }
    }
  }
  for (i=send_length-1; i>=0; i--) {
    min_event_time = sim_time;
    n = send_list[i];
    if (ready_head[n] == NULL) 
      printf("Null ready list on send list\n");
    else {
      id = ready_head[n]->id;
      type = ready_head[n]->head->type;
      if (type != SND && type != REL && type != PRE) 
	printf("Non-comm event in the sendlist\n");
      else {
	dest = map_thread(ready_head[n]->head->data.send.to);
	msgtype = ready_head[n]->head->data.send.msgtype;
	len = ready_head[n]->head->data.send.len;
	  
	cd = find_cd(id, msgtype, dest);
	if (cd) {
	  if (type == REL) {
	    delete_from_list(send_list, i, &send_length);
	    suspend(id);
	    delete_event(id);
	    schedule(n);
	    activity++;

	    release_circuit(id, cd);

	  } else if (type == PRE) {
	    delete_from_list(send_list, i, &send_length);
	    suspend(id);
	    delete_event(id);
	    schedule(n);
	    activity++;
	    
	  } else if (type == SND) {
	    if (circuit_ready(n,cd)) {
	      con_circs++;
	      msgflits += ready_head[n]->head->data.send.len;
	      delete_from_list(send_list, i, &send_length);
	      suspend(id);
	      delete_event(id);
	      schedule(n);
	      activity++;
	    
	      prerouted_send(cd, n, dest, len);
	    }

	  }
	} else {
	  if ((k = can_send(n))>= 0 || n==dest || type == REL) {
	    msgflits += ready_head[n]->head->data.send.len;
	    delete_from_list(send_list, i, &send_length);
	    suspend(id);
	    delete_event(id);
	    schedule(n);
	    activity++;
	    
	    if (dest == n) {
	      if (type == SND)
		rcv_call_back(n, msgtype, len);
	    } else {
	      c = find_circ();
	      m = find_msg();
	      inject_spec(n, dest, len, 0, msgtype);
	      if (type == PRE) {
		msgs[m].preroute = 1;
		cd = (circuit_descriptor *)malloc(sizeof(circuit_descriptor));
		cd->v = circs[c].inject_virt;
		cd->msgtype = msgtype;
		cd->dest = dest;
		cd->next = NULL;
		cd->tkn = c;
		add_circ(id, cd);
	      }
	    }
	  }
	}
      }
    }
  }
  if (min_event_time == 2000000000) {
    min_event_time = sim_time;
  }
  
}


/*--------------------------------------------------------------*/
/* trace_done()  trace done?                                    */
/*--------------------------------------------------------------*/
trace_done() {
  if (!feof(tracefile)) 
    return 0;
  if (num_empty == num_threads && send_length == 0 && execute_length==0) 
    /* All processors with no events */
    return 1;
  if (send_length == 0 && execute_length==0 && activity == 0) {
    /* No send or execute events pending, no activity in the network. */
    /* This implies that the algorithm is deadlocked for some reason. */
    printf("Network deadlocked.\n");
    exit(1);
  }
  return 0;
}


/*--------------------------------------------------------------*/
/* can_send()  can a node send a message?                       */
/*--------------------------------------------------------------*/
can_send(src) {
  int i;
  
  for (i=0; i<node_virts; i++) {
    if (nodes[src].send[i]==0) return i;
  }
  return -1;
}

/*--------------------------------------------------------------*/
/* circuit_ready()  is a prerouted circuit ready for data?      */
/*--------------------------------------------------------------*/
circuit_ready(n, cd)
circuit_descriptor *cd;
{
  if (nodes[n].send[cd->v] != HOLDER) return 0;
  if (comm_mech == WR || comm_mech == AWR || comm_mech == RECON)
    return 1;
  if (circs[cd->tkn].acked) return 1;
  return 0;
}


/*--------------------------------------------------------------*/
/* rcv_call_back()  Handle message recv                         */
/*--------------------------------------------------------------*/
rcv_call_back(n, msgtype, len) 
{
  int dest;
  if (!match_rcv_event(n, msgtype, len)) {
    if (read_for_rcv_event(n, msgtype)) {
      if (!match_rcv_event(n, msgtype, len)) {
	printf("This should NEVER happen.\n");
      }
    } else {
      printf("Trace file is not complete.  Could not find matching rcv\n");
      printf("event for message.\n");
      exit(1);
    }
  }
}

/*--------------------------------------------------------------*/
/* match_rcv_event()                                            */
/*--------------------------------------------------------------*/
match_rcv_event(n, msgtype, len) {
  int i, tmp;
  event *l, *e;
  thread *th;
  recv_record *rr, *nr;

  if (len == 0) return 1;	/* Message is simply a circuit teardown */

  l = (event *)NULL;
  rr = recvs[n];
  nr = NULL;
  if (rr == NULL) return 0;
  e = rr->recv;
  th = &all_threads[e->thread];
  while (e) {
    if (e->type == RCV && e->data.msgtype == msgtype) {
      msgflits -= len;

      l = e->prev;
      if (th->tail == e)
	th->tail = l;
      if (l) {
	l->next = e->next;
	if (l->next)
	  l->next->prev = l;
      } else
	if (e->next)
	  e->next->prev = e->prev;

      if (nr == NULL) 
	recvs[n] = rr->next;
      else {
	nr->next = rr->next;
	if (rr->next == NULL)
	  recv_tail[n] = nr;
      }
      free(rr);
      if (e == th->head) {
	delete_event(e->thread);
	if (ready_head[th->node] == th)
	  schedule(th->node);
      } else {
	free(e);
      }
      return 1;
    }
    nr = rr;
    rr = rr->next;
    if (rr) {
      e = rr->recv;
      th = &all_threads[e->thread];
    } else
      e = NULL;
  }
  return 0;
}
