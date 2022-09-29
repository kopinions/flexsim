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

static char util_rcsid[] = "$Id: ut2.c,v 1.1 1994/01/21 18:55:19 pgaughan Exp dao $";

#include <stdio.h>
#include "dat2.h"
#ifndef NO_X
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif

int origred[19] = {65,40,55,65,30,65,65,65,30,40,65,20,65,40,20,50, 20,40,65};
int origgreen[19]={45,65,55,65,65,30,65,30,65,40,50,60,20,20,65,65, 20,40, 0};
int origblue[19] ={45,40,65,30,65,65,65,30,30,65,20,65,50,65,50,20, 20,40,65};

extern int DEBUG, nvlog, NODES, virts, node_virts, numnvlinks, numvlinks;
extern int dimlog, vlog, dimensions;
extern int SECURITY_LEVELS;

extern node *nodes;

#ifdef PLINK

extern int plinknode;

#endif


int col[19];

extern struct plinkEnd *plinkTable;

   /*-----------------------------------------------------*/
   /* link_addr(src,dim,delta, v, level)                  */
   /*-----------------------------------------------------*/
   link_addr (src, dim, delta, v, level)
     int src, dim, delta, v, level;
   {
     int j, k, m,n,i;

     /* dimlog = log of number of dimensions in the network */
     /* vlog = the number of dimensions for each physical link's */
     /*        virtual channels */
     /* dimensions = the actual number of dimensions */
     /* NODES = number of nodes in the network */

     /* we want to calculate the array index for a virtual link at */
     /* source node (src) in the dimension (dim) */

#ifndef IRREGULAR
     
     j = (delta + 1) >> 1;
     k = src << (dimlog + vlog);
     m = v << dimlog;
     if (src >= NODES || src < 0 || dim < 0 || dim >= dimensions || v > virts)
       {
	 printf ("Gack!  Link address error!\n");
	 printf("Src -%d, v -> %d d= %d\n",src,v,dim);
	 abort ();
	 exit (0);
       }

     /* adding + level * numvlinks moves the index range into the */
     /* proper security level range in the virts array */

     return (k + m + dim + dim + j + level * (numvlinks+numnvlinks)
	     + numnvlinks);
#else
     /*j = (delta +1) >> 1;*/
      
     
      n = nodes[src].conTable[dim]-NODES;
      
      if( plinkTable[n].Rnode==src)
	delta=(-1);
      /*      printf("L = %d R =%d del=%d ",plinkTable[n].Rnode,plinkTable[n].Lnode,delta);*/
      j = (delta+1) >> 1;
      
      k = ( n << vlog)*2 ;
      m =  v*2;

       if (src >= NODES || src < 0 || dim < 0 || dim >= nodes[src].connection
	   || v > virts)
       {
	 printf ("Gack!  Link address error!\n");
	 printf("Src -%d, v -> %d d= %d\n",src,v,dim);
	 abort ();
	 exit (0);
       }

       n= (k+m+j+level * (numvlinks+numnvlinks)+numnvlinks);
       return n;
#endif

   }

   /*-----------------------------------------------------*/
   /* nlink_addr(src, v, in, level)                       */
   /*-----------------------------------------------------*/
#ifdef PLINK
   nlink_addr (src, v, in, level,num_plink)
     int src, v, in, level,num_plink;
   {
     int j, k, m,l;     
     j = (src << (nvlog+1))*plinknode;
     k = (v << 1);
     m = level*(numnvlinks+numvlinks);
     l = (1 << (nvlog+1)) * num_plink;
     if (src >= NODES || src < 0 || v > node_virts)
       {
	 printf ("Gack!  Link address error! in node\n");
	 abort ();
	 exit (0);
       }
     
     /* adding + level * numvlinks moves the index range into the */
     /* proper security level range in the virts array */
     
     return (j + k + m + in + l );
   }
#else
 nlink_addr (src, v, in, level)
     int src, v, in, level;
   {
     int j, k, m,l;     
     j = (src << (nvlog+1));
     k = (v << 1);
     m = level*(numnvlinks+numvlinks);
     if (src >= NODES || src < 0 || v > node_virts)
       {
	 printf ("Gack!  Link address error!\n");
	 abort ();
	 exit (0);
       }
     
     /* adding + level * numvlinks moves the index range into the */
     /* proper security level range in the virts array */
     
     return (j + k + m + in );
   }
#endif

rgb *build_colors(levels)
{
  int i,j;
  double scale, inv_scale;
  rgb *map;

  map = (rgb *) calloc(levels, sizeof(rgb));

  if ( map )
    {
      if ( levels == 1 ) 
	{
	  for ( j = 0 ; j < 16 ; j++ ) 
	    {
	      /* populate the rest of the colors */
	      map[0].red[j] = origred[j];
	      map[0].green[j] = origgreen[j];
	      map[0].blue[j] = origblue[j];
	    }
	  map[0].red[16] = 20;
	  map[0].green[16] = 20;
	  map[0].blue[16] = 20;
	      
	  map[0].red[17] = 50;
	  map[0].green[17] = 50;
	  map[0].blue[17] = 50;
	}
      else
	{
	  
	  for ( i = 0 ; i < levels ; i++ )
	    {
	      
	      scale = (double) i / (double) (levels-1);
	      inv_scale = 1.0 - scale;
	      if ( DEBUG >= 2) {
		printf("levels = %d, i = %d, scale = %g\n", levels,
		       i, scale);
	      }
	      
	      for ( j = 0 ; j < 16 ; j++ ) 
		{
		  /* populate the rest of the colors */
		  map[i].red[j] = (63-4*j)*scale;
		  map[i].green[j] = (63-4*j)*inv_scale;
		  map[i].blue[j] = 0;
		}
	      
	      /* allocate the gray colors identically for each */
	      map[i].red[16] = 20;
	      map[i].green[16] = 20;
	      map[i].blue[16] = 20;
	      
	      map[i].red[17] = 50;
	      map[i].green[17] = 50;
	      map[i].blue[17] = 50;
	    }
	}
      return(map);
    }
  else
    {
      printf("Unable to allocate storage for colormaps!\n");
      exit(0);
    }

}