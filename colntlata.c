/* Collate the data into a plottable throughput/latency data file */

#include <stdio.h>
#include <math.h>

float x[50],y[50];
int t[50], cnt[50];
char s[80],l[80],l2[80];

main()
{
  int i, j, k, n, mn;

  for (i=0; i<50; i++) {
    cnt[i]=0;
    t[i]=0;
  }
  l[0]=0;
  l2[0]=0;
  s[0]=0;
  i=0;
  while (!feof(stdin)) {
/*-------------------------------------------------------*/
/* Look for header of file containing things like        */
/*   t_l load time                                       */
/*-------------------------------------------------------*/
    scanf("%s",s);
    while (strcmp(s,"Load:")!=0 &&!feof(stdin)) {
      scanf("%s",s);
    }
    if (feof(stdin)) break;
    scanf("%s",l);
    t[i] = atoi(l);		/* Got the load */

    scanf("%s",s);
    while (strcmp(s,"tack_latency:")!=0 &&!feof(stdin)) {
      scanf("%s",s);
    }
    if (feof(stdin)) break;
    scanf("%s",l);
    y[i] = atof(l);		/* Got the latency */
    
    scanf("%s",s);
    while (strcmp(s,"time:")!=0 &&!feof(stdin)) {
      scanf("%s",s);
    }
/*    if (feof(stdin)) break;
    scanf("%s",l);
    y[i] = y[i] - atof(l);		/* Got the queue time */
    
    scanf("%s",s);
    while (strcmp(s,"throughput:")!=0 &&!feof(stdin)) {
      scanf("%s",s);
    }
    if (feof(stdin)) break;
    scanf("%s",l);
    x[i] = atof(l);		/* Got the throughput */
    cnt[i]=1;
    i++;
  }
    
  for (k=0; k<i; k++) {
    mn = 0.0;
    n=0;
    for (j=0; j<50; j++) {
      if (cnt[j]>0 && t[j] > mn) {
	mn = t[j];
	n = j;
      }
    }  if (cnt[n]>0)
      printf("%f\t%f\n",x[n],y[n]);
    else
      break;
    cnt[n]=0;
  }
}



