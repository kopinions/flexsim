/* 
*	Module: Driver_dd.c
*/

#include	<stdio.h>
#include	<string.h>

#include	"dd.h"

int sim_time;
int DDTEST;


main(argc, argv)
int	argc;
char	*argv[];
{

	int		i, error;

	extern int NextAvailCycle;

/* Initialize Stuff*/

    	InitCircList();
   	InitVCList();
   	InitCycleList();
	InitDeadlockList();

/*
-------------------------------------------
	EXAMPLE 2
-------------------------------------------

	error = AddVCToCircuit(1, 100);
	error = AddVCToCircuit(1, 101);
	error = AddReqVCToCircuit(1, 102);
	error = AddReqVCToCircuit(1, 202);

	error = AddVCToCircuit(2, 102);
	error = AddVCToCircuit(2, 103);
	error = AddReqVCToCircuit(2, 100);
	error = AddReqVCToCircuit(2, 200);

	error = AddVCToCircuit(3, 200);
	error = AddVCToCircuit(3, 201);
	error = AddReqVCToCircuit(3, 102);
	error = AddReqVCToCircuit(3, 202);

	error = AddVCToCircuit(4, 202);
	error = AddVCToCircuit(4, 203);
	error = AddReqVCToCircuit(4, 100);
	error = AddReqVCToCircuit(4, 200);
*/
/*
-------------------------------------------
	EXAMPLE 3
-------------------------------------------

	error = AddVCToCircuit(1, 100);
	error = AddVCToCircuit(1, 101);
	error = AddReqVCToCircuit(1, 103);

	error = AddVCToCircuit(2, 102);
	error = AddVCToCircuit(2, 103);
	error = AddReqVCToCircuit(2, 105);

	error = AddVCToCircuit(3, 104);
	error = AddVCToCircuit(3, 105);
	error = AddReqVCToCircuit(3, 107);

	error = AddVCToCircuit(4, 106);
	error = AddVCToCircuit(4, 107);
	error = AddReqVCToCircuit(4, 101);
*/
/*
-------------------------------------------
	EXAMPLE 4
-------------------------------------------
	error = AddVCToCircuit(1, 100);
	error = AddVCToCircuit(1, 101);
	error = AddReqVCToCircuit(1, 102);
	error = AddReqVCToCircuit(1, 202);

	error = AddVCToCircuit(2, 102);
	error = AddVCToCircuit(2, 103);
	error = AddReqVCToCircuit(2, 100);
	error = AddReqVCToCircuit(2, 200);

	error = AddVCToCircuit(3, 201);
	error = AddVCToCircuit(3, 202);
	error = AddReqVCToCircuit(3, 103);
	error = AddReqVCToCircuit(3, 203);

	error = AddVCToCircuit(4, 203);
	error = AddVCToCircuit(4, 200);
	error = AddReqVCToCircuit(4, 101);
	error = AddReqVCToCircuit(4, 201);
*/
/*
-------------------------------------------
	EXAMPLE 5
-------------------------------------------
	error = AddVCToCircuit(1, 101);
	error = AddVCToCircuit(1, 102);
	error = AddVCToCircuit(1, 103);
	error = AddReqVCToCircuit(1, 104);
	error = AddReqVCToCircuit(1, 204);

	error = AddVCToCircuit(2, 104);
	error = AddVCToCircuit(2, 205);
	error = AddVCToCircuit(2, 206);
	error = AddReqVCToCircuit(2, 107);
	error = AddReqVCToCircuit(2, 207);

	error = AddVCToCircuit(3, 107);
	error = AddVCToCircuit(3, 200);
	error = AddVCToCircuit(3, 201);
	error = AddReqVCToCircuit(3, 102);
	error = AddReqVCToCircuit(3, 202);

	error = AddVCToCircuit(4, 202);
	error = AddVCToCircuit(4, 203);
	error = AddVCToCircuit(4, 204);
	error = AddReqVCToCircuit(4, 105);
	error = AddReqVCToCircuit(4, 205);

	error = AddVCToCircuit(5, 105);
	error = AddVCToCircuit(5, 106);
	error = AddVCToCircuit(5, 207);
	error = AddReqVCToCircuit(5, 100);
	error = AddReqVCToCircuit(5, 200);

	error = AddVCToCircuit(6, 100);
	error = AddReqVCToCircuit(6, 101);
	error = AddReqVCToCircuit(6, 201);
*/
/*
-------------------------------------------
	EXAMPLE 6
-------------------------------------------
	error = AddVCToCircuit(1, 101);
	error = AddVCToCircuit(1, 104);
	error = AddReqVCToCircuit(1, 403);
	error = AddReqVCToCircuit(1, 404);

	error = AddVCToCircuit(2, 402);
	error = AddVCToCircuit(2, 403);
	error = AddReqVCToCircuit(2, 203);
	error = AddReqVCToCircuit(2, 204);

	error = AddVCToCircuit(3, 202);
	error = AddVCToCircuit(3, 203);
	error = AddReqVCToCircuit(3, 303);
	error = AddReqVCToCircuit(3, 304);

	error = AddVCToCircuit(4, 301);
	error = AddVCToCircuit(4, 304);
	error = AddReqVCToCircuit(4, 103);
	error = AddReqVCToCircuit(4, 104);

	error = AddVCToCircuit(5, 401);
	error = AddVCToCircuit(5, 103);
	error = AddReqVCToCircuit(5, 403);
	error = AddReqVCToCircuit(5, 404);

	error = AddVCToCircuit(6, 201);
	error = AddVCToCircuit(6, 404);
	error = AddReqVCToCircuit(6, 203);
	error = AddReqVCToCircuit(6, 204);

	error = AddVCToCircuit(7, 302);
	error = AddVCToCircuit(7, 204);
	error = AddReqVCToCircuit(7, 303);
	error = AddReqVCToCircuit(7, 304);

	error = AddVCToCircuit(8, 102);
	error = AddVCToCircuit(8, 303);
	error = AddReqVCToCircuit(8, 103);
	error = AddReqVCToCircuit(8, 104);
*/
/*
-------------------------------------------
	EXAMPLE 6 (No deadlock)
-------------------------------------------
*/
	error = AddVCToCircuit(1, 101);
	error = AddVCToCircuit(1, 104);
	error = AddReqVCToCircuit(1, 403);
	error = AddReqVCToCircuit(1, 404);

	error = AddVCToCircuit(2, 402);
	error = AddVCToCircuit(2, 403);
	error = AddReqVCToCircuit(2, 203);
	error = AddReqVCToCircuit(2, 204);

	error = AddVCToCircuit(3, 202);
	error = AddVCToCircuit(3, 203);
	error = AddReqVCToCircuit(3, 303);
	error = AddReqVCToCircuit(3, 304);

	error = AddVCToCircuit(4, 301);
	error = AddVCToCircuit(4, 304);

	error = AddVCToCircuit(5, 401);
	error = AddVCToCircuit(5, 103);
	error = AddReqVCToCircuit(5, 403);
	error = AddReqVCToCircuit(5, 404);

	error = AddVCToCircuit(6, 201);
	error = AddVCToCircuit(6, 404);
	error = AddReqVCToCircuit(6, 203);
	error = AddReqVCToCircuit(6, 204);

	error = AddVCToCircuit(7, 302);
	error = AddVCToCircuit(7, 204);
	error = AddReqVCToCircuit(7, 303);
	error = AddReqVCToCircuit(7, 304);

	error = AddVCToCircuit(8, 102);
	error = AddVCToCircuit(8, 303);
	error = AddReqVCToCircuit(8, 103);
	error = AddReqVCToCircuit(8, 104);
/*
-------------------------------------------
	EXAMPLE 7
-------------------------------------------
	error = AddVCToCircuit(1, 100);
	error = AddVCToCircuit(1, 101);
	error = AddVCToCircuit(1, 102);
	error = AddVCToCircuit(1, 103);
	error = AddVCToCircuit(1, 104);
	error = AddVCToCircuit(1, 105);
	error = AddVCToCircuit(1, 106);
	error = AddReqVCToCircuit(1, 107);
	error = AddReqVCToCircuit(1, 207);

	error = AddVCToCircuit(2, 107);
	error = AddVCToCircuit(2, 108);
	error = AddVCToCircuit(2, 109);
	error = AddVCToCircuit(2, 110);
	error = AddVCToCircuit(2, 111);
	error = AddVCToCircuit(2, 112);
	error = AddVCToCircuit(2, 113);
	error = AddReqVCToCircuit(2, 114);
	error = AddReqVCToCircuit(2, 214);

	error = AddVCToCircuit(3, 114);
	error = AddVCToCircuit(3, 115);
	error = AddVCToCircuit(3, 200);
	error = AddVCToCircuit(3, 201);
	error = AddVCToCircuit(3, 202);
	error = AddVCToCircuit(3, 203);
	error = AddVCToCircuit(3, 204);
	error = AddReqVCToCircuit(3, 105);
	error = AddReqVCToCircuit(3, 205);

	error = AddVCToCircuit(4, 205);
	error = AddVCToCircuit(4, 206);
	error = AddVCToCircuit(4, 207);
	error = AddVCToCircuit(4, 208);
	error = AddVCToCircuit(4, 209);
	error = AddVCToCircuit(4, 210);
	error = AddVCToCircuit(4, 211);
	error = AddReqVCToCircuit(4, 112);
	error = AddReqVCToCircuit(4, 212);

	error = AddVCToCircuit(5, 212);
	error = AddVCToCircuit(5, 213);
	error = AddVCToCircuit(5, 214);
	error = AddVCToCircuit(5, 215);
	error = AddReqVCToCircuit(5, 100);
	error = AddReqVCToCircuit(5, 200);
*/
/*
-------------------------------------------
	SCRATCH
-------------------------------------------

	error = RemoveAllReqVCsFromCircuit(0);
	error = RemoveVCFromCircuit(1, 1);

*/


/* Print the lists */

	PrintAdjacencyList();

/* Detect Cycles */

/*
	printf("  REQVCS: %d\n", sizeof (REQVCS));
	printf("  VCNODE: %d\n", sizeof (VCNODE));
	printf("  CIRCNODE: %d\n", sizeof (CIRCNODE));

	printf("  MSGSNODE: %d\n", sizeof (MSGSNODE));
	printf("  CYCLEVCNODE: %d\n", sizeof (CYCLEVCNODE));
	printf("  CYCLENODE: %d\n", sizeof (CYCLENODE));

*/

	error = DetectCycles();
	if (error != NOERROR)
		printf("ERROR: error retunred by 'DetectCycles'\n");

    	error = MarkCircuitsWithAllReqVCsInACycle();
	if (error != NOERROR)
		printf("ERROR: error retunred by 'MarkCircuitsWithAllReqVCsInACycle'\n");

    	error = DetectDeadlocks();
	if (error != NOERROR)
		printf("ERROR: error retunred by 'DetectDeadlocks'\n");

/*
	for (i = 0; i < NextAvailCycle; i++)
		RetireCycle(i);
*/

	PrintCycleList();
	PrintDeadlockList();

}
