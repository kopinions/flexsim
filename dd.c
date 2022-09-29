/* 
*	Module: Deadlock Detection Main Module: 'dd.c'
*
*	Contains the following:
*
*	- Routines for Building and Maintaining a Resource Dependency Graph		
*	- Routines for Detecting Cycles in the Resource Dependency Graph
*	- Routines for Detecting Deadlocks Involving Groups of Cycles 
*/


#include	<malloc.h>
#include	"dat2.h"	
#include	"dd.h"


/*
*	
*	G L O B A L   V A R I A B L E S 
*
*/

/*
*
*	Global variable declarations for the adjacency list 
*	(which represents the Resource Dependency Graph)
*
*/

struct	CIRCNODE	CircList[MAXCIRC];	/* List of Circuits */

struct	VCNODE		VCList[MAXVC];		/* List of VCs */

/*
*
*	Global variable declarations for maintaining Cycle Information
*
*/

struct	CYCLENODE	CycleList[MAXCYCLES];	/* List of Cycles */

int	NextAvailCycle = 0;			/* Maintains index into end of "used region" of the Cycle List */
int	FirstAvailCycleSlot = 0;		/* Maintains index into first available slot in Cycle List */


/*
*
*	Global variable declarations for maintaining Deadlock Information
*
*/

struct	DEADLOCKNODE	DeadlockList[MAXDEADLOCKS];	/* List of Deadlocks */

int	NextAvailDeadlock = 0;				/* Maintains index into Deadlock List */


/*
*
*	Global variable declarations for maintaining Hash Table for Cycles
*
*/

#ifdef HASH
struct	HASHBUCKET	HashTable[HASHTABLESIZE];	/* List of HashBuckets which compose the Hash Table */
#endif



/*
*
*	Global variable declarations for maintaining Statistics
*
*/

long	TotalRetiredCycles = 0;			/* Maintains the total number of retired cycles*/
long	TotalRetiredCycleMessages = 0;		/* Maintains the total number of messages in retired cycles  */
long	TotalRetiredCycleVCs = 0;		/* Maintains the total number of VCs in retired cycles */
long	TotalRetiredCycleDuration = 0;		/* Maintains the accumulated duartion for all retired cycles */

long	TotalPreviousCircuits = 0;		/* Maintains the total number of active circuits */
long	TotalPreviousBlockedCircuits = 0;	/* Maintains the total number of active circuits which are blocked */
int	TotalCircuitReportingPeriods = 0;	/* Maintains the number of reporting periods for circuit stats */

int	MaxMessNumberInACycle=0;		/* Maintains the maximum number of messages found in any cycle */
int	MinMessNumberInACycle=MAXCIRC;		/* Maintains the minimum number of messages found in any cycle */


/* 
*	External variables (from Flitsim) 
* 
*/

extern int sim_time;				/* Keeps track of Simulation Time */
extern int DDTEST;				/* Indicates whether to print out status */




/* 
*
*	  I N I T I A L I Z A T I O N   R O U T I N E S 
*
*/

/*
*	InitCircList
*
*	Initializes all values and pointers in the Circuit List.
*	This is intended to be used to initialize this structure
*	before it is used.
*/

void	InitCircList()
{
	int	i;

	for (i = 0; i < MAXCIRC; i++)
	{
		CircList[i].CircID = i;			/* Not needed ? */
		CircList[i].AllReqVCsInACycle = FALSE;	
		CircList[i].FirstReqVCNode = NULL_;
		CircList[i].FirstVCNode = NULL_;
		CircList[i].LastVCNode = NULL_;
	}
}


/*
*	InitVCList
*
*	Initializes all values and pointers in the VC List.
*	This is intended to be used to initialize this structure
*	before it is used.
*/

void	InitVCList()
{
	int	i;

	for (i = 0; i < MAXVC; i++)
	{
		VCList[i].VCID = i;			/* Not needed ? */
		VCList[i].Visited = UNVISITED;
		VCList[i].Used = UNUSED;
		VCList[i].CircID = UNDEFINED_CIRCID;
		VCList[i].InACycle = FALSE;
		VCList[i].ReqVCNode = NULL_;
		VCList[i].NextVCNode = NULL_;
	}
}


/*
*	InitCycleList
*
*	Initializes all values and pointers in the Cycle List.
*	This is intended to be used to initialize this structure
*	before it is used.
*/

void	InitCycleList()
{
	int	i;

	for (i = 0; i < MAXCYCLES; i++)
	{
		CycleList[i].Active = FALSE;	
		CycleList[i].FirstDetected = 0;
		CycleList[i].LastDetected = 0;
		CycleList[i].NumofMessages = 0;
		CycleList[i].NumofVCs = 0;
		CycleList[i].CycleKey = 0;
#ifdef HASH
		CycleList[i].HashTableListIndex = UNDEFINED_LIST_INDEX;
#endif
		CycleList[i].MsgListSize = 0;
		CycleList[i].Messages = NULL_;
		CycleList[i].FirstCycleVCNode = NULL_;
		CycleList[i].LastCycleVCNode = NULL_;
	}
}


/*
*	InitDeadlockList
*
*	Initializes all values and pointers in the Deadlock List.
*	This is intended to be used to initialize this structure
*	before it is used.
*/

void	InitDeadlockList()
{
	int	i;

	for (i = 0; i < MAXDEADLOCKS; i++)
	{
		DeadlockList[i].Active = FALSE;
		DeadlockList[i].FirstDetected = 0;
		DeadlockList[i].LastDetected = 0;
		DeadlockList[i].NumofMessages = 0;
		DeadlockList[i].NumofVCs = 0;
		DeadlockList[i].NumofCycles = 0;
		DeadlockList[i].NumofDepMessages = 0;
		DeadlockList[i].NumofDepVCs = 0;
		DeadlockList[i].MsgListSize = 0;
		DeadlockList[i].DepMsgListSize = 0;
		DeadlockList[i].InvCyclesListSize = 0;
		DeadlockList[i].Messages = NULL_;
		DeadlockList[i].Cycles = NULL_;
		DeadlockList[i].DepMessages = NULL_;
	}
}




/* 
*
*	  P R I N T   ( D E B U G G I N G )   R O U T I N E S 
*
*/

/*
*	PrintAdjacencyList
*
*	Prints out the contents of the Adjacency List.
*	Intended to be used for testing/debugging.
*
*	Prints out only those Circuits which have either
*	VC nodes or ReqVC nodes allocated to them.
*/

void	PrintAdjacencyList()
{
	int		i, j;
	VCNODEPTR	CurrentVCNode;
	REQVCSPTR	CurrentReqVCNode;
	int		PrintThisCircuit;
	int		PrintAnyCircuit;

	PrintAnyCircuit = FALSE;

	printf("*** ADJACENCY LIST AT SIM_TIME: %d\n", sim_time+1);

	for (i = 0; i < MAXCIRC; i++)
	{
		PrintThisCircuit = FALSE;

		CurrentVCNode = CircList[i].FirstVCNode;
		CurrentReqVCNode = CircList[i].FirstReqVCNode;

		/* print out the info. regarding circuits */
		if ((CurrentVCNode != NULL_) || (CurrentReqVCNode != NULL_))
		{
			printf("[ CircID:%d | AIAC:%d ] -->", CircList[i].CircID, CircList[i].AllReqVCsInACycle);
			PrintThisCircuit = TRUE;
			PrintAnyCircuit = TRUE;
		}

		/* print out the list of VC nodes */
		while (CurrentVCNode != NULL_)
		{
			printf("[ ID:%d | Vis:%d | Use:%d |O:%d | IAC:%d ] -->", 
				CurrentVCNode->VCID, 
				CurrentVCNode->Visited, 
				CurrentVCNode->Used, 
				CurrentVCNode->CircID, 
				CurrentVCNode->InACycle);
			CurrentReqVCNode = CurrentVCNode->ReqVCNode;
			CurrentVCNode = CurrentVCNode->NextVCNode;
		}

		/* print out the list of ReqVC nodes */
		if (CurrentReqVCNode != NULL_)
		{
			for (j = 0; j < MAXFANOUT; j++)
			{
				if (CurrentReqVCNode->ReqVCNodeList[j] != NULL_)
				{
					printf("[* ID:%d | Vis:%d | Use:%d | O:%d | IAC:%d | BrVis: %d *] ==>", 
						CurrentReqVCNode->ReqVCNodeList[j]->VCID, 
						CurrentReqVCNode->ReqVCNodeList[j]->Visited, 
						CurrentReqVCNode->ReqVCNodeList[j]->Used, 
						CurrentReqVCNode->ReqVCNodeList[j]->CircID, 
						CurrentReqVCNode->ReqVCNodeList[j]->InACycle,
						CurrentReqVCNode->VisitedList[j]);
				}
			}
		}

		if (PrintThisCircuit)
			printf(" NULL_\n");

	}
	if (PrintAnyCircuit)
		printf("\n\n");

}


/*
*	PrintCycleList
*
*	Prints out the active cycles in the Cycle List.
*	Intended to be used for testing/debugging.
*/

void	PrintCycleList()
{
	int		i, j;
	CYCLEVCNODEPTR	CurrentCycleVCNode;

	printf("*** CYCLE LIST AT SIM_TIME: %d\n", sim_time+1);

	for (i = 0; i < NextAvailCycle; i++)
	{
		if (CycleList[i].Active)
		{
#ifdef HASH
			/* print out info. regarding the cycle */
			printf("[ I:%d | F:%d | L:%d | M#:%d | VC#:%d | K:%d | HI:%d | MS:%d ] ", 
				i,
				CycleList[i].FirstDetected,
				CycleList[i].LastDetected,
				CycleList[i].NumofMessages,
				CycleList[i].NumofVCs,
				CycleList[i].CycleKey,
				CycleList[i].HashTableListIndex,
				CycleList[i].MsgListSize
				);

#else
			/* print out info. regarding the cycle */
			printf("[ I:%d | F:%d | L:%d | M#:%d | VC#:%d | K:%d | MS:%d ] ", 
				i,
				CycleList[i].FirstDetected,
				CycleList[i].LastDetected,
				CycleList[i].NumofMessages,
				CycleList[i].NumofVCs,
				CycleList[i].CycleKey,
				CycleList[i].MsgListSize
				);

#endif

			printf("contains message set ");
			
			if (CycleList[i].Messages != NULL_)
				PrintMessageOrInvCyclesList(CycleList[i].Messages, CycleList[i].MsgListSize, UNDEFINED_CIRCID);
			else
				printf("{ }\n");


			/* print out the list of VCs in the cycle */
			CurrentCycleVCNode = CycleList[i].FirstCycleVCNode;

			while (CurrentCycleVCNode != NULL_)
			{
				printf("-->[ CircID:%d | BrIdx:%d ]",
					CurrentCycleVCNode->CircuitID,
					CurrentCycleVCNode->BranchIndex);

				CurrentCycleVCNode = CurrentCycleVCNode->NextCycleVCNode;
 
			}
			printf("--> NULL_\n\n");
		}
	}
}


/*
*	PrintDetectedCycle
*
*	Used for printing out a cycle (in the resource dependency graph) which
*	has just been detected.
*/

void	PrintDetectedCycle(StartingVC, VCNode)
int		StartingVC;	
VCNODEPTR	VCNode;
{
	printf("START: [ Circ: %d | VC: %d ] --> ", VCNode->CircID, VCNode->VCID);

	if (VCNode->NextVCNode != NULL_)
		PrintVisitVC(StartingVC, VCNode->NextVCNode);

	else if (VCNode->ReqVCNode != NULL_)
		PrintVisitReqVCs(StartingVC, VCNode->ReqVCNode);
}


/*
*	PrintVisitVC
*
*	Used for printing out a cycle (in the resource dependency graph) which
*	has just been detected.
*/

void	PrintVisitVC(StartingVC, VCNode)
int		StartingVC;	
VCNODEPTR	VCNode;
{

	/* This means we are NOT YET back where we started from (the whole cycle has NOT YET been traversed !) */
	if (VCNode->VCID != StartingVC)
	{
		printf("[ Circ: %d | VC: %d ] --> ", VCNode->CircID, VCNode->VCID);

		if (VCNode->NextVCNode != NULL_)
			PrintVisitVC(StartingVC, VCNode->NextVCNode);

		else if (VCNode->ReqVCNode != NULL_)
			PrintVisitReqVCs(StartingVC, VCNode->ReqVCNode);
	}

	/* Otherwise, we are back where we started - so start backtracking ! (the whole cycle has been traversed !) */
	else
	{
		printf("END ([ Circ: %d | VC: %d ])\n\n", VCNode->CircID, VCNode->VCID);
	}
}


/*
*	PrintVisitReqVCs
*
*	Used for printing out a cycle (in the resource dependency graph) which
*	has just been detected.
*/

void	PrintVisitReqVCs(StartingVC, ReqVCNode)
int		StartingVC;	
REQVCSPTR	ReqVCNode;
{
	int		i;

	for (i = 0; i < MAXFANOUT; i++)
	{
		/* We print and get out of here after finding the first ReqVC */
		/* since in a cycle, only one of the ReqVC can be in a single cycle */
		if ((ReqVCNode->ReqVCNodeList[i] != NULL_) && (ReqVCNode->VisitedList[i] == VISITED))
		{
			PrintVisitVC(StartingVC, ReqVCNode->ReqVCNodeList[i]);
			break;
		}
	}
}


/*
*	PrintMessageOrInvCyclesList
*
*	Prints out the contents of the specified Message or Involved Cycles List.
*	Intended to be used for testing/debugging.
*/

void	PrintMessageOrInvCyclesList(MessageList, MessageListSize, EndOfListSymbol)
MSGSNODEPTR 	MessageList;
int		MessageListSize;
int		EndOfListSymbol;
{
	int		i;

	printf("{");

	for (i = 0; i < MessageListSize; i++)
	{
		if (MessageList[i] != EndOfListSymbol)
			printf("%d, ", MessageList[i]);
		else
			break;
	}
	printf("}\n");
}


/*
*	PrintDeadlockList
*
*	Prints out the contents of the Deadlock List.
*	Intended to be used for testing/debugging.
*/

void	PrintDeadlockList()
{
	int		i, j;

	printf("*** DEADLOCK LIST AT SIM_TIME: %d\n", sim_time+1);

	for (i = 0; i < NextAvailDeadlock; i++)
	{
		/* print out info. regarding the deadlock */
		printf("[ I:%d | A:%d | F:%d | L:%d | M#:%d | VC#:%d | C#:%d | DM#:%d | DVC#:%d | MS#:%d | DMS#:%d | CS#:%d ] ", 
			i,
			DeadlockList[i].Active,
			DeadlockList[i].FirstDetected,
			DeadlockList[i].LastDetected,
			DeadlockList[i].NumofMessages,
			DeadlockList[i].NumofVCs,
			DeadlockList[i].NumofCycles,
			DeadlockList[i].NumofDepMessages,
			DeadlockList[i].NumofDepVCs,
			DeadlockList[i].MsgListSize,
			DeadlockList[i].DepMsgListSize,
			DeadlockList[i].InvCyclesListSize
			);

		/* print out the messages list */
		if (DeadlockList[i].Messages != NULL_)
			PrintMessageOrInvCyclesList(DeadlockList[i].Messages, DeadlockList[i].MsgListSize, UNDEFINED_CIRCID);

		/* print out the involved cycles list */
		if (DeadlockList[i].Cycles != NULL_)
			PrintMessageOrInvCyclesList(DeadlockList[i].Cycles, DeadlockList[i].InvCyclesListSize, UNDEFINED_CYCLEID);

		/* print out the dependent messages list */
		if (DeadlockList[i].DepMessages != NULL_)
			PrintMessageOrInvCyclesList(DeadlockList[i].DepMessages, DeadlockList[i].DepMsgListSize, UNDEFINED_CIRCID);

	}
}


/*
*	PrintHashTable
*
*	Prints out the contents of the Hash Table.
*	Intended to be used for testing/debugging.
*/
#ifdef HASH
void	PrintHashTable()
{
	int		i, j;

	printf("*** HASH TABLE AT SIM_TIME: %d\n", sim_time+1);

	for (i = 0; i < HASHTABLESIZE; i++)
	{
		/* print out info. regarding the Hash Bucket */
		printf("[ Index:%d | Allocated:%d | LastIdx:%d ] ", 
			i,
			HashTable[i].AllocatedSize,
			HashTable[i].LastItemIndex
			);

		/* print out the involved cycles list */
		if (HashTable[i].ItemList != NULL_)
			PrintMessageOrInvCyclesList(HashTable[i].ItemList, HashTable[i].AllocatedSize, UNDEFINED_CYCLEID);

		printf("\n");

	}
}
#endif


/* 
*
*	  S T A T I S T I C S   P R I N T I N G   R O U T I N E S 
*
*/


/*
*	PrintCircuitStats
*
*	Prints out the statistics for circuits.
*
*/

void	PrintCircuitStats()
{
	int	i;

	int	TotalCircuits = 0;
	int	TotalBlockedCircuits = 0;

	for (i = 0; i < MAXCIRC; i++)
	{
		if ((CircList[i].FirstVCNode != NULL_) || (CircList[i].FirstReqVCNode != NULL_))
			TotalCircuits++;

		if (((CircList[i].LastVCNode != NULL_) && (CircList[i].LastVCNode->ReqVCNode != NULL_)) ||
			(CircList[i].FirstReqVCNode != NULL_))
			TotalBlockedCircuits++;
	}

	TotalPreviousCircuits += TotalCircuits;
	TotalPreviousBlockedCircuits += TotalBlockedCircuits;
	TotalCircuitReportingPeriods++;

	printf("\n");
	printf("m####  %9d  %9d  %9d  %11.2f  %11.2f  %11.2f  %11.2f\n", 
			sim_time+1, 
			TotalCircuits,
			TotalBlockedCircuits,
			(float) TotalBlockedCircuits / TotalCircuits,
			(float) TotalPreviousCircuits / TotalCircuitReportingPeriods,
			(float) TotalPreviousBlockedCircuits / TotalCircuitReportingPeriods,
			(float) TotalPreviousBlockedCircuits / TotalPreviousCircuits
		);
}

/*
*	PrintCycleStats
*
*	Prints out the statistics for cycles detected.
*
*/

void	PrintCycleStats()
{
	int	i;

	int	TotalActiveCycles = 0;
	int	TotalActiveCycleMessages = 0;
	int	TotalActiveCycleVCs = 0;
	int	TotalActiveCycleDuration = 0;

	int	TotalCycles = 0;

	for (i = 0; i < NextAvailCycle; i++)
	{
		if (CycleList[i].Active)
		{
			TotalActiveCycles++;
	
			TotalActiveCycleMessages += CycleList[i].NumofMessages;
			TotalActiveCycleVCs += CycleList[i].NumofVCs;
			TotalActiveCycleDuration += (CycleList[i].LastDetected - CycleList[i].FirstDetected);

			if (CycleList[i].NumofMessages > MaxMessNumberInACycle)
				MaxMessNumberInACycle = CycleList[i].NumofMessages;

			if (CycleList[i].NumofMessages < MinMessNumberInACycle)
				MinMessNumberInACycle = CycleList[i].NumofMessages;
		}
	}

	TotalCycles = TotalActiveCycles + TotalRetiredCycles;

	printf("\n");
	printf("c####  %9d  %9d  %9d  %11.2f  %11.2f  %11.2f  %9d:%d\n", 
			sim_time+1, 
			TotalCycles, 
			TotalActiveCycles,
			(float) (TotalRetiredCycleMessages + TotalActiveCycleMessages) / TotalCycles,
			(float) (TotalRetiredCycleVCs + TotalActiveCycleVCs) / TotalCycles,
			(float) (TotalRetiredCycleDuration + TotalActiveCycleDuration) / TotalCycles,
			MaxMessNumberInACycle,
			MinMessNumberInACycle
		);
}


/*
*	PrintDeadlockStats
*
*	Prints out the statistics for deadlocks detected.
*
*/

void	PrintDeadlockStats()
{
	int	i;
	int	ActiveDeadlockCount=0;

	long	TotalDeadlockMessagesCount=0;
	long	TotalDeadlockVCCount=0;
	long	TotalDeadlockCycleCount=0;
	long	TotalDeadlockDepMsgsCount=0;
	long	TotalDeadlockDepVCsCount=0;
	int	MaxMessNumberInADeadlock=0;
	int	MinMessNumberInADeadlock=MAXCIRC;
	int	MaxCycleNumberInADeadlock=0;
	int	MinCycleNumberInADeadlock=MAXCYCLES;

	for (i = 0; i < NextAvailDeadlock; i++)
	{
		if (DeadlockList[i].Active)
		{
			ActiveDeadlockCount++;
		}

		TotalDeadlockMessagesCount += DeadlockList[i].NumofMessages;
		TotalDeadlockVCCount += DeadlockList[i].NumofVCs;
		TotalDeadlockCycleCount += DeadlockList[i].NumofCycles;
		TotalDeadlockDepMsgsCount += DeadlockList[i].NumofDepMessages;
		TotalDeadlockDepVCsCount += DeadlockList[i].NumofDepVCs;

		if (DeadlockList[i].NumofMessages > MaxMessNumberInADeadlock)
			MaxMessNumberInADeadlock = DeadlockList[i].NumofMessages;

		if (DeadlockList[i].NumofMessages < MinMessNumberInADeadlock)
			MinMessNumberInADeadlock = DeadlockList[i].NumofMessages;

		if (DeadlockList[i].NumofCycles > MaxCycleNumberInADeadlock)
			MaxCycleNumberInADeadlock = DeadlockList[i].NumofCycles;

		if (DeadlockList[i].NumofCycles < MinCycleNumberInADeadlock)
			MinCycleNumberInADeadlock = DeadlockList[i].NumofCycles;

	}

	printf("\n");
	printf("d####  %9d  %9d  %9d  %11.2f  %11.2f  %11.2f  %11.2f  %11.2f  %9d:%d  %9d:%d\n", 
			sim_time+1, 
			NextAvailDeadlock, 
			ActiveDeadlockCount,
			(float) TotalDeadlockMessagesCount / NextAvailDeadlock,
			(float) TotalDeadlockVCCount / NextAvailDeadlock,
			(float) TotalDeadlockCycleCount / NextAvailDeadlock,
			(float) TotalDeadlockDepMsgsCount / NextAvailDeadlock,
			(float) TotalDeadlockDepVCsCount / NextAvailDeadlock,
			MaxMessNumberInADeadlock,
			MinMessNumberInADeadlock,
			MaxCycleNumberInADeadlock,
			MinCycleNumberInADeadlock
		);
}



/* 
*
*	  R E S O U R C E   D E P E N D E N C Y   G R A P H   M A N A G E M E N T   R O U T I N E S 
*
*/

/*
*	AddVCToCircuit
*
*	Given a VC, adds it as being allocated to a specified circuit.
*
*	Returns ERROR if:
*		- an invalid VC or Circuit ID are specified.
*
*	Returns NOERROR otherwise.
*
*/

int	AddVCToCircuit(Circuit, VC)
int	Circuit;
int	VC;
{
	/* range check array bounds */	
	if ((Circuit >= MAXCIRC) || (VC >= MAXVC))
	{
		printf("ERROR: 'Circuit' and/or 'VC' out of range in 'AddVCToCircuit'\n");
		return(ERROR);
	}

	/* if there are no VCs for this Circuit, set the new one as the first */
	if (CircList[Circuit].FirstVCNode == NULL_)
	{
		CircList[Circuit].FirstVCNode = &(VCList[VC]);
		CircList[Circuit].LastVCNode = &(VCList[VC]);

	}
	/* if there are existing VCs, find the last one, and append to the end */
	else
	{
		CircList[Circuit].LastVCNode->NextVCNode = &(VCList[VC]);
		CircList[Circuit].LastVCNode = &(VCList[VC]);
	}

	/* initialize values of newly added VC */
	CircList[Circuit].LastVCNode->Used = USED;
	CircList[Circuit].LastVCNode->CircID = Circuit;
	CircList[Circuit].LastVCNode->InACycle = FALSE;
	CircList[Circuit].LastVCNode->ReqVCNode = NULL_;
	CircList[Circuit].LastVCNode->NextVCNode = NULL_;

	return(NOERROR);	
}


/*
*	AddReqVCToCircuit
*
*	Given a desired VC, adds it as being allocated to a specified circuit.
*
*	Returns ERROR if:
*		- an invalid VC or Circuit ID are specified.
*		- memory allocation for fanout node fails.
		- existing fanout node is full (no more slots !)
*		
*	Returns NOERROR otherwise.
*
*/

int	AddReqVCToCircuit(Circuit, ReqVC)
int	Circuit;
int	ReqVC;
{
	REQVCSPTR	FanoutNode;
	int		i;

	/* range check array bounds */	
	if ((Circuit >= MAXCIRC) || (ReqVC >= MAXVC))
	{
		printf("ERROR: 'Circuit' and/or 'ReqVC' out of range in 'AddReqVCToCircuit'\n");
		return(ERROR);
	}

	/* if there are no VCs for this Circuit, append to the begining*/
	if (CircList[Circuit].FirstVCNode == NULL_)
	{
		/* and if there are no other desired VCs, then add the node for fanout */
		if (CircList[Circuit].FirstReqVCNode == NULL_)
		{
			CircList[Circuit].FirstReqVCNode = (REQVCSPTR) malloc(sizeof(REQVCS));

			/* check if memory allocation failed or not */
			if (CircList[Circuit].FirstReqVCNode == NULL_)
			{
				printf("ERROR: malloc returned NULL_ in 'AddReqVCToCircuit'\n");
				return(ERROR);
			}

			/* Initialize the Fanout Node */
			for (i = 0; i < MAXFANOUT; i++)
			{
				CircList[Circuit].FirstReqVCNode->ReqVCNodeList[i] = NULL_;
				CircList[Circuit].FirstReqVCNode->VisitedList[i] = UNVISITED;
			}
		}
		
		/* Set 'FanoutNode' to point to the list of ReqVCs */
		FanoutNode = CircList[Circuit].FirstReqVCNode;

	}
	/* if there are VCs for this circuit, then append at the end */
	else 
	{
		if (CircList[Circuit].LastVCNode->ReqVCNode == NULL_)
		{
			CircList[Circuit].LastVCNode->ReqVCNode = (REQVCSPTR) malloc(sizeof(REQVCS));

			/* check if memory allocation failed or not */
			if (CircList[Circuit].LastVCNode->ReqVCNode == NULL_)
			{
				printf("ERROR: malloc returned NULL_ in 'AddReqVCToCircuit'\n");
				return(ERROR);
			}

			/* Initialize the Fanout Node */
			for (i = 0; i < MAXFANOUT; i++)
			{
				CircList[Circuit].LastVCNode->ReqVCNode->ReqVCNodeList[i] = NULL_;
				CircList[Circuit].LastVCNode->ReqVCNode->VisitedList[i] = UNVISITED;
			}
		}

		/* Set 'FanoutNode' to point to the list of ReqVCs */
		FanoutNode = CircList[Circuit].LastVCNode->ReqVCNode;
	}
	
	/* find where to add new ReqVC */
	/* Note: We may want to do better checking here ... right now we are */
	/* not checking to see if we are repeatedly asked to add same ReqVCnode - */
	/* which may indicate Flitsim error */
	for (i = 0; i < MAXFANOUT; i++)
	{
		/* if we get to an empty slot, that means the ReqVC is not in the list --- so add it ! */
		if (FanoutNode->ReqVCNodeList[i] == NULL_)
		{
			/* Point to the VC from Fanout Node */
			FanoutNode->ReqVCNodeList[i] = &(VCList[ReqVC]);
			return(NOERROR);	
		}

		/* if the ReqVC is already in the list, then do nothing */
		if (FanoutNode->ReqVCNodeList[i]->VCID == ReqVC)
			return(NOERROR);	
	}

	/* If we're here, then we've exceeded the capacity of fanout node, so report error */
	printf("ERROR: capacity of fanout node exceeded in 'AddReqVCToCircuit'\n");
	return(ERROR);
}


/*
*	RemoveVCFromCircuit
*
*	Given a VC, removes it from the specified circuit.
*
*	Returns ERROR if:
*		- an invalid (out of range) VC or Circuit ID are specified.
*		- a VC which is not owned by the Circuit is specified .
*
*	Returns NOERROR otherwise.
*
*/

int	RemoveVCFromCircuit(Circuit, VC)
int	Circuit;
int	VC;
{
	VCNODEPTR	CurrentVCNode, PreviousVCNode, TempVCNode;
	int		error;

	/* range check array bounds */	
	if ((Circuit >= MAXCIRC) || (VC >= MAXVC))
	{
		printf("ERROR: 'Circuit' and/or 'VC' out of range in 'RemoveVCFromCircuit'\n");
		return(ERROR);
	}

	/* if its the first VC on the list */

	if ((CircList[Circuit].FirstVCNode != NULL_) &&
	    (CircList[Circuit].FirstVCNode->VCID == VC))
	{
		/* and if it's the only VC (since first = last) */
		if (CircList[Circuit].FirstVCNode == CircList[Circuit].LastVCNode)
		{
			CircList[Circuit].LastVCNode = NULL_;
		}
	
		/* reset some of the fields */
		CircList[Circuit].FirstVCNode->Visited = UNVISITED;
		CircList[Circuit].FirstVCNode->Used = UNUSED;
		CircList[Circuit].FirstVCNode->CircID = UNDEFINED_CIRCID;
		CircList[Circuit].FirstVCNode->InACycle = NOTINACYCLE;

		/* deallocate the fanout node - if one exists */
		error = RemoveAllReqVCsFromVCNode(CircList[Circuit].FirstVCNode);

		/* save a pointer to the just removed VC */ 
		TempVCNode = CircList[Circuit].FirstVCNode;

		CircList[Circuit].FirstVCNode = CircList[Circuit].FirstVCNode->NextVCNode;

		/* reset the pointer of the just removed VC */ 
		TempVCNode->NextVCNode = NULL_;
	}

	/* if it is not the first VC on the list */
	else
	{	
		CurrentVCNode = CircList[Circuit].FirstVCNode;
		PreviousVCNode = CircList[Circuit].FirstVCNode;

		while ((CurrentVCNode != NULL_) &&
			(CurrentVCNode->VCID != VC))
		{
			PreviousVCNode = CurrentVCNode;
			CurrentVCNode = CurrentVCNode->NextVCNode;
		}

		if (CurrentVCNode == NULL_)
		{
			printf("ERROR: Specified VC does not exist in 'RemoveVCFromCircuit'\n");
			return(ERROR);
		}

		else
		{
			/* if we are removing the last VC, then we update the pointer to the last VC */
			if (CurrentVCNode == CircList[Circuit].LastVCNode)
			{
				CircList[Circuit].LastVCNode = PreviousVCNode;
			}

			/* reset some of the fields */
			CurrentVCNode->Visited = UNVISITED;
			CurrentVCNode->Used = UNUSED;
			CurrentVCNode->CircID = UNDEFINED_CIRCID;
			CurrentVCNode->InACycle = NOTINACYCLE;

			/* deallocate the fanout node - if one exists */
			error = RemoveAllReqVCsFromVCNode(CurrentVCNode);

			PreviousVCNode->NextVCNode = CurrentVCNode->NextVCNode;

			/* reset the pointer of the just removed VC */ 
			CurrentVCNode->NextVCNode = NULL_;
		}
	}

	return(NOERROR);
}


/*
*	RemoveAllReqVCsFromCircuit
*
*	Given a circuit, removes all ReqVCs from the specified circuit.
*	Note that the ReqVCNode (fanout node) is either linked to the
*	very begining of the Circuit or at the end of the list of
*	VCNodes.
*
*	Need to modify error detection.
*/

int	RemoveAllReqVCsFromCircuit(Circuit)
int	Circuit;
{

	/* if the fanout node is attached to the begining  */
	if (CircList[Circuit].FirstReqVCNode != NULL_)
	{
		free(CircList[Circuit].FirstReqVCNode);
		CircList[Circuit].FirstReqVCNode = NULL_;
	}

	/* if the fanout node is attached to the end  */
	if ((CircList[Circuit].LastVCNode != NULL_) &&
		(CircList[Circuit].LastVCNode->ReqVCNode != NULL_))
	{
		free(CircList[Circuit].LastVCNode->ReqVCNode);
		CircList[Circuit].LastVCNode->ReqVCNode = NULL_;
	}

	return(NOERROR);
}


/*
*	RemoveAllReqVCsFromVCNode
*
*	Given a VCNode, removes the ReqVCNode (fanout nodes) attached to it (if any).
*
*	Need to modify error detection.
*/

int	RemoveAllReqVCsFromVCNode(VCNode)
VCNODEPTR	VCNode;
{

	/* if there is a ReqVCNode attached  */
	if (VCNode->ReqVCNode != NULL_)
	{
		/* deallocate its memory and set pointer to NULL_ */
		free(VCNode->ReqVCNode);
		VCNode->ReqVCNode = NULL_;
	}

	return(NOERROR);
}


/*
*	CountNumberofCircuitVCs
*
*	Given a Circuit ID, returns the number of VCs owned by the specified Circuit.
*
*/

int	CountNumberofCircuitVCs(Circuit)
{	
	int		Count=0;
	VCNODEPTR	CurrentVCNode;

	CurrentVCNode = CircList[Circuit].FirstVCNode;

	while (CurrentVCNode != NULL_)
	{
		Count++;
		CurrentVCNode = CurrentVCNode->NextVCNode;
	}

	return(Count);
}



/* 
*
*	  C Y C L E   D E T E C T I O N   R O U T I N E S 
*
*/

/*
*	DetectCycles
*
*	Used for detecting cycles in the resource allocation graph.
*/

int	DetectCycles()
{
	int		i, j;
	int		error;
	VCNODEPTR	CurrentVCNode;
	REQVCSPTR	CurrentReqVCNode;

	for (i = 0; i < MAXCIRC; i++)
	{
		/* if there are any VCs assigned to this circuit */
		if (CircList[i].FirstVCNode != NULL_)
		{
			error = VisitVC(CircList[i].FirstVCNode, i);

			if (error != NOERROR)
				return(error);
		}

		/* otherwise, if there are any ReqVC Nodes attached to this circuit */
		else 
			if (CircList[i].FirstReqVCNode != NULL_)
			{
				error = VisitReqVCs(CircList[i].FirstReqVCNode, i);

				if (error != NOERROR)
					return(error);
			}
	}

	return(NOERROR);
}


/*
*	VisitVC
*
*	Used for detecting cycles in the resource allocation graph. 
*/

int	VisitVC(VCNode, StartingMessage)
VCNODEPTR		VCNode;
int			StartingMessage;
{
	
	int		i;
	MSGSNODEPTR 	MessageList;
	int		MessageListSize = 0;
	int		error;
	int		MatchingCycleIndex;
	int		FoundMatchingList;
	int		FoundMatchingKey;
	int		StartIndex;
	int		NewIndex;
	int		FoundMatchingCycle;
	int		CycleKey;
	int		NumberOfMessages;
	int		NumberOfVCs;
#ifdef HASH
	MSGSNODEPTR 	PotentiallyMatchingCycleList;
	int		PotentiallyMatchingCycleListSize;
	int		HashTableListIndex;
#endif

	/* if we encounter an already visited VC, then we've detected a cycle */
	if (VCNode->Visited == VISITED)
	{

		/* Allocate memory for message list */
		MessageListSize = INITIALMSGLISTSIZE;

		MessageList = (MSGSNODEPTR) malloc(sizeof(MSGSNODE) * MessageListSize);
		/* If memory allocation fails, then return error */
		if (MessageList == NULL_)
		{
			printf("ERROR: returned by 'malloc' in 'VisitVC'\n");
			return(ERROR);
		}

		/* Initialize the Message List */
		InitMessageOrInvCycleList(MessageList, MessageListSize, UNDEFINED_CIRCID);

		/* Build Message List ... and Mark fanout Nodes with 'InACycle' */
		error = CollectMsgsAndTagInACycle(&MessageList, &MessageListSize, VCNode->VCID, VCNode, &NumberOfVCs);
		if (error != NOERROR)
		{
			printf("ERROR: returned by 'CollectMsgsAndTagInACycle' in 'VisitVC'\n");
			return(ERROR);
		}

		/* Sort the Message List */
		SortMessageOrInvCyclesList(MessageList, MessageListSize, UNDEFINED_CIRCID);

		/* Generate the key for the cycle */
		GenerateCycleKeyAndCountMessages(MessageList, MessageListSize, &CycleKey, &NumberOfMessages);

#ifdef HASH
		/* Get Hash Table List and Size of List for the specified Cycle Key */
		GetHashTableList(CycleKey, &PotentiallyMatchingCycleList, &PotentiallyMatchingCycleListSize);

		/* Initialize */
		FoundMatchingCycle = FALSE;

/*
printf("here 1, %d\n", PotentiallyMatchingCycleListSize);
*/
		for (i = 0; i < PotentiallyMatchingCycleListSize; i++)
		{
/*
PrintHashTable();
printf("here 2\n");
*/
			if (CycleList[PotentiallyMatchingCycleList[i]].CycleKey == CycleKey)
			{
				MatchingCycleIndex = PotentiallyMatchingCycleList[i];
/*
printf("here 3\n");
*/

				/* then see if the message lists match exactly */
				FoundMatchingList = CompareMessageOrInvCyclesLists(CycleList[MatchingCycleIndex].Messages, 
									CycleList[MatchingCycleIndex].MsgListSize,
									MessageList, MessageListSize, UNDEFINED_CIRCID);

				/* if the message lists match exactly also */
				if  (FoundMatchingList)
				{
/*
printf("here 4\n");
*/
					/* then compare the two cycles, VC by VC */
					FoundMatchingCycle = CompareCycles(MatchingCycleIndex,  VCNode->CircID);
			
					/* if the cycles match exactly (every VC) */
					/* then we found a matching cycle (already in the Cycle List), */
					/* So, simply update it's detected time */
					if (FoundMatchingCycle)
					{
						/* update the "TimeLastDetected" field */
						UpdateCycleTimeLastDetected(MatchingCycleIndex, sim_time);

						/* free memory for message list - since it is no longer needed */
						free(MessageList);
						MessageListSize = 0;

						break;
					}
				}

			}
		}
#else

		/* Repeatedly find potentially matching Cycles and */
		/* compare the complete cycles to see if it is an exact match  */
		/* until we find exact match or no match exists */
		StartIndex = 0;
		FoundMatchingCycle = FALSE;
		do 
		{
			/* try to find a cycle with a matching key */
			FoundMatchingKey = FindCycleWithMatchingCycleKey(StartIndex, CycleKey, &MatchingCycleIndex);

			/* if we found a cycle with a matching key */
			if (FoundMatchingKey)
			{
				/* then see if the message lists match exactly */
				FoundMatchingList = CompareMessageOrInvCyclesLists(CycleList[MatchingCycleIndex].Messages, 
									CycleList[MatchingCycleIndex].MsgListSize,
									MessageList, MessageListSize, UNDEFINED_CIRCID);

				/* if the message lists match exactly also */
				if  (FoundMatchingList)
				{
					/* then compare the two cycles, VC by VC */
					FoundMatchingCycle = CompareCycles(MatchingCycleIndex,  VCNode->CircID);
			
					/* if the cycles match exactly (every VC) */
					/* then we found a matching cycle (already in the Cycle List), */
					/* So, simply update it's detected time */
					if (FoundMatchingCycle)
					{
						/* update the "TimeLastDetected" field */
						UpdateCycleTimeLastDetected(MatchingCycleIndex, sim_time);

						/* free memory for message list - since it is no longer needed */
						free(MessageList);
						MessageListSize = 0;

						break;
					}
				}

				StartIndex = MatchingCycleIndex+1;
			}
		} while (FoundMatchingKey);
#endif

		/* If we did NOT find a matching cycle already in the Cycle List,   */
		/* Then we add the newly detected cycle to the Cycle List           */
		/* and create a linked list of VCNodes to replicate the found cycle */
		/* and calculate and set the values of various fields               */
		if (! FoundMatchingCycle)
		{
			error = AddCycleToList(sim_time, &NewIndex, MessageList, MessageListSize, CycleKey);

			if (error != NOERROR)
			{
				printf("ERROR: returned by 'AddCycleToList' in 'VisitVC'\n");
				return(ERROR);
			}

#ifdef HASH
			error = AddToHashTable(CycleKey, NewIndex, &HashTableListIndex);

			if (error != NOERROR)
			{
				printf("ERROR: returned by 'AddToHashTable' in 'VisitVC'\n");
				return(ERROR);
			}

			error = UpdateCycleHashTableListIndex(NewIndex, HashTableListIndex);

			if (error != NOERROR)
			{
				printf("ERROR: returned by 'UpdateCycleHashTableListIndex' in 'VisitVC'\n");
				return(ERROR);
			}
#endif

			error = CreateDetectedCycle(NewIndex, VCNode->CircID);

			if (error != NOERROR)
			{
				printf("ERROR: returned by 'CreateDetectedCycle' in 'VisitVC'\n");
				return(ERROR);
			}

			/* Set the number of messages (calculated above) for this new cycle */
			CycleList[NewIndex].NumofMessages = NumberOfMessages;

			/* Set the number of VCs (calculated above) for this new cycle */
			CycleList[NewIndex].NumofVCs = NumberOfVCs;

			if (DDTEST == PRINT_WHEN_CYCLE_FOUND)
			{
				printf("\n\n*** NEW CYCLE DETECTED ***\n\n");
				PrintMessageOrInvCyclesList(MessageList, MessageListSize, UNDEFINED_CIRCID);
				PrintAdjacencyList();
				PrintCycleList();
				PrintDeadlockList();
			}

		}	
/*
PrintHashTable();
PrintCycleList();
*/
	}
	else
	{
		/* mark node as visited */
		VCNode->Visited = VISITED;
		
		if (VCNode->NextVCNode != NULL_)
		{
			error = VisitVC(VCNode->NextVCNode, StartingMessage);
			if (error != NOERROR)
			{
				printf("ERROR: returned by 'VisitVC' in 'VisitVC'\n");
				return(ERROR);
			}
		}

		else if (VCNode->ReqVCNode != NULL_)
		{
			error = VisitReqVCs(VCNode->ReqVCNode, StartingMessage);
			if (error != NOERROR)
			{
				printf("ERROR: returned by 'VisitReqVCs' in 'VisitVC'\n");
				return(ERROR);
			}
		}

		/* If we are backtracking (during search for cycle), unmark as we backtrack */
		VCNode->Visited = UNVISITED;

	}

	return(NOERROR);
	
}


/*
*	VisitReqVCs
*
*	Used for detecting cycles in the resource allocation graph. 
*/

int	VisitReqVCs(ReqVCNode, StartingMessage)
REQVCSPTR	ReqVCNode;
int		StartingMessage;
{
	int	i;
	int	error;

	for (i = 0; i < MAXFANOUT; i++)
	{
		/* Chcek against 'StartingMessage' to prune search */
		/* this should avoid visting (detecting) the same cycle multiple times */
		if ((ReqVCNode->ReqVCNodeList[i] != NULL_) && 
			(ReqVCNode->ReqVCNodeList[i]->CircID >= StartingMessage))
		{
			ReqVCNode->VisitedList[i] = VISITED;
			error = VisitVC(ReqVCNode->ReqVCNodeList[i], StartingMessage);
			ReqVCNode->VisitedList[i] = UNVISITED;
			if (error != NOERROR)
			{
				printf("ERROR: returned by 'VisitVC' in 'VisitReqVCs'\n");
				return(ERROR);
			}
		}
	}
	return(NOERROR);
}



/* 
*
*	  C Y C L E   L I S T   M A N A G E M E N T   R O U T I N E S 
*
*/

/*
*	AddCycleToList
*
*	Adds a new cycle to the Cycle List and Returns an index to the
*	Cycle List where the Cycle was added.  Also, records the specified
*	time as the time at which the cycle was first detected.
*
*	Returns ERROR if:
*		- Cycle list is already filled to capacity
*
*	Returns NOERROR otherwise.
*/

int	AddCycleToList(TimeFirstDetected, CycleIndex, MessageList, MessageListSize, CycleKey)
int		TimeFirstDetected;
int		*CycleIndex;
MSGSNODEPTR	MessageList;
int		MessageListSize;
int		CycleKey;
{
	int	i;
	int	FoundInactiveCycle = FALSE;
	int	AddIndex;
	int	error;

	/* Try to find a previously used Cycle slot that is no longer active */
	for (i = FirstAvailCycleSlot; i < NextAvailCycle; i++)
	{
		/* if an inactive cycle is found, clear it and reuse that slot */
		if (! CycleList[i].Active)
		{
			FoundInactiveCycle = TRUE;
			AddIndex = i;
			break;
		}
	}

	/* if we dont find a used slot that is no longer active, then use new slot */
	if (!FoundInactiveCycle)
	{
		/* check to see if there are any new slots left - if not, report error */
		if (NextAvailCycle >= MAXCYCLES)
		{
			printf("ERROR: Cycle list is full in 'AddCycleToList'\n");
			return(ERROR);
		}

		/* else, get slot from the end */
		AddIndex = NextAvailCycle;
		NextAvailCycle++;
		FirstAvailCycleSlot = NextAvailCycle;
	}
	/* if we do find a used slot that is no longer active, then increment index pointer */
	/* so that next time around, we start looking for an inactive slot from the next slot */
	/* Note: we are guranteed that incrementing 'FirstAvailCycleSlot' here will not set it  */
	/* greater than 'NextAvailCycle' (at most, equal to 'NextAvailCycle')               */
	else
		FirstAvailCycleSlot++;

	/* 'Activate' this cycle, and set its first/last detected times */
	CycleList[AddIndex].Active = TRUE;	
	CycleList[AddIndex].FirstDetected = TimeFirstDetected;
	CycleList[AddIndex].LastDetected = TimeFirstDetected;

	CycleList[AddIndex].CycleKey = CycleKey;

	CycleList[AddIndex].MsgListSize = MessageListSize;
	CycleList[AddIndex].Messages = MessageList;

	/* return the index where cycle was added */
	*CycleIndex = AddIndex;

	return(NOERROR);
}


/*
*	UpdateCycleTimeLastDetected
*
*	Updates the 'LastDetected' field of the cycle specified via
*	the 'CycleIndex'.
*
*	Returns ERROR if:
*		- an illegal Cycle List index is given
*		- an inactive Cycle is specified via index
*
*	Returns NOERROR otherwise.
*/

int	UpdateCycleTimeLastDetected(CycleIndex, TimeLastDetected)
int	CycleIndex;
int	TimeLastDetected;
{
	/* if given an illegal index, return error */
	if (CycleIndex >= MAXCYCLES)
	{
		printf("ERROR: illegal Cycle List index specified in 'UpdateCycleTimeLastDetected'\n");
		return(ERROR);
	}

	if (! CycleList[CycleIndex].Active)
	{
		printf("ERROR: Inactive Cycle specified in 'UpdateCycleTimeLastDetected'\n");
		return(ERROR);
	}

	/* otherwise, update time last detected and return no error */
	CycleList[CycleIndex].LastDetected = TimeLastDetected;

	return(NOERROR);
}


/*
*	UpdateCycleHashTableListIndex
*
*	Updates the 'HashTableListIndex' field of the cycle specified via
*	the 'CycleIndex'.
*
*	Returns ERROR if:
*		- an illegal Cycle List index is given
*		- an inactive Cycle is specified via index
*
*	Returns NOERROR otherwise.
*/

#ifdef HASH
int	UpdateCycleHashTableListIndex(CycleIndex, HashTableListIndex)
int	CycleIndex;
int	HashTableListIndex;
{
	/* if given an illegal index, return error */
	if (CycleIndex >= MAXCYCLES)
	{
		printf("ERROR: illegal Cycle List index specified in 'UpdateCycleHashTableListIndex'\n");
		return(ERROR);
	}

	if (! CycleList[CycleIndex].Active)
	{
		printf("ERROR: Inactive Cycle specified in 'UpdateCycleHashTableListIndex'\n");
		return(ERROR);
	}

	/* otherwise, update time last detected and return no error */
	CycleList[CycleIndex].HashTableListIndex = HashTableListIndex;

	return(NOERROR);
}
#endif


/*
*	AddCycleVCNodeToCycle
*
*	Adds a new CycleVC to the specified Cycle (by allocating memory for the
*	CycleVCNode, adding the node to the end of the linked list, and 
*	setting the values for the CircuitID and BranchIndex)
*
*	Returns ERROR if:
*		- memory allocation for a new CYCLEVCNODE fails
*
*	Returns NOERROR otherwise.
*/

int	AddCycleVCNodeToCycle(CycleIndex, CircuitID, BranchIndex)
int	CycleIndex;
int	CircuitID;
int	BranchIndex;
{
	CYCLEVCNODEPTR	NewCycleVCNode;

	/* try to allocate memory for a 'NewCycleVCNode' */
	NewCycleVCNode = (CYCLEVCNODEPTR) malloc(sizeof(CYCLEVCNODE));

	/* if memory allocation fails, then return error */
	if (NewCycleVCNode == NULL_)
	{
		printf("ERROR: Memory allocation failed in 'AddCycleVCNodeToCycle'\n");
		return(ERROR);
	}

	/* otherwise */

	/* if there are no VCs for this Cycle, set the new one as the first */
	if (CycleList[CycleIndex].FirstCycleVCNode == NULL_)
	{
		CycleList[CycleIndex].FirstCycleVCNode = NewCycleVCNode;
		CycleList[CycleIndex].LastCycleVCNode = NewCycleVCNode;

	}
	/* if there are existing VCs, find the last one, and append to the end */
	else
	{
		CycleList[CycleIndex].LastCycleVCNode->NextCycleVCNode = NewCycleVCNode;
		CycleList[CycleIndex].LastCycleVCNode = NewCycleVCNode;
	}

	/* Initialize the fileds of the newly created CYCLEVCNODE structure */
	CycleList[CycleIndex].LastCycleVCNode->CircuitID = CircuitID;
	CycleList[CycleIndex].LastCycleVCNode->BranchIndex = BranchIndex;
	CycleList[CycleIndex].LastCycleVCNode->NextCycleVCNode = NULL_;

	return(NOERROR);	
}


/*
*	RetireCycle
*
*	Removes all the CycleVCNodes from the specified cycle and updates the 
*	"Total Retired" counts for various items (i.e. number of messages, VCs, etc.).
*	Also reinitializes the fields of the deactivated cycle for subsequent use.
*
*	This is intended to be used when "deactivating" a cycle
*
*	Returnes ERROR:
*	- (Currently does not check for any error conditions)
*
*	Returns NOERROR otherwise.
*/

int	RetireCycle(CycleIndex)
int	CycleIndex;
{
	CYCLEVCNODEPTR	CurrentCycleVCNode;
	CYCLEVCNODEPTR	NxtCycleVCNode;
	int		error;

	/* set the pointer to the start of the linked list of Cycle VCs */
	CurrentCycleVCNode = CycleList[CycleIndex].FirstCycleVCNode;
	
	/* traverse the linked list, removing one by one, */
	while(CurrentCycleVCNode != NULL_)
	{
		NxtCycleVCNode = CurrentCycleVCNode->NextCycleVCNode;
		
		free(CurrentCycleVCNode);

		CurrentCycleVCNode = NxtCycleVCNode;
	}

	/* Free the memory allocated to the message list */
	free(CycleList[CycleIndex].Messages);

	/* Tabulate information regarding this cycle */
	TotalRetiredCycles++;	
	TotalRetiredCycleDuration += (CycleList[CycleIndex].LastDetected - CycleList[CycleIndex].FirstDetected);
	TotalRetiredCycleMessages += CycleList[CycleIndex].NumofMessages;		
	TotalRetiredCycleVCs += CycleList[CycleIndex].NumofVCs;	

#ifdef HASH
	/* Clear the entry in the Hash Table */
	error = RemoveFromHashTable(CycleList[CycleIndex].CycleKey, CycleIndex, CycleList[CycleIndex].HashTableListIndex);
	if (error != NOERROR)
	{
		printf("ERROR: 'RemoveFromHashTable' returned error in 'RetireCycle'\n");
		return(error);
	}
#endif

	/* Clear the fields and Reset everything to show "deactivation" */	
	CycleList[CycleIndex].Active = FALSE;
	CycleList[CycleIndex].FirstDetected = 0;
	CycleList[CycleIndex].LastDetected = 0;
	CycleList[CycleIndex].NumofMessages = 0;
	CycleList[CycleIndex].NumofVCs = 0;
	CycleList[CycleIndex].CycleKey = 0;
#ifdef HASH
	CycleList[CycleIndex].HashTableListIndex = UNDEFINED_LIST_INDEX;
#endif
	CycleList[CycleIndex].MsgListSize = 0;
	CycleList[CycleIndex].Messages = NULL_;
	CycleList[CycleIndex].FirstCycleVCNode = NULL_;
	CycleList[CycleIndex].LastCycleVCNode = NULL_;

	/* Set the 'FirstAvailCycleSlot' to the newly freed cycle slot if it is lower in index */
	if (CycleIndex < FirstAvailCycleSlot)
		FirstAvailCycleSlot = CycleIndex;

	return(NOERROR);
}


/*
*	RetireEligibleCycles
*
*	Checks every "active" cycle to see if it has been observed (and 
*	therefore updated) during the current simulation cycle.  If NOT,
*	then it calls 'RetireCycle' to deactivate the cycle.
*
*	Returnes ERROR:
*	- If a call to 'RetireCycle' generates an error
*
*	Returns NOERROR otherwise.
*/

int	RetireEligibleCycles()
{
	int	i;
	int	error;

	for (i = 0; i < NextAvailCycle; i++)
	{
		if ((CycleList[i].Active) && (CycleList[i].LastDetected < sim_time))
		{
			error = RetireCycle(i);

			if (error != NOERROR)
			{
				printf("ERROR: 'RetireCycle' returned an error in 'RetireEligibleCycles'\n");
				return(ERROR);
			}
		}
	}
	return(NOERROR);
}



/* 
*
*	  C Y C L E   M A T C H I N G   R O U T I N E S 
*
*/


/*
*	FindCycleWithMatchingCycleKey
*
*	Searches the Cycle List (starting at the specified 'StartIndex') for
*	an active cycle which has the same associated message key (key generated by
*	summing up the values of all IDs (Circuit IDs) of messages in the cycle.
*
*	If a cycle with a matching key is found, then it returns TRUE along with 
*	the index of the matching cycle in the 'MatchingCycleIndex' field.
*
*	Returns FALSE if no matching cycle is found.
*
*/

int	FindCycleWithMatchingCycleKey(StartIndex, CycleKey, MatchingCycleIndex)
int		StartIndex;
int		CycleKey;
int		*MatchingCycleIndex;
{
	int	i;

	/* try to find a matching cycle - starting from the 'StartIndex' */
	for (i = StartIndex; i < NextAvailCycle; i++)
	{
		/* check only the active cycles */
		if ((CycleList[i].Active) && (CycleList[i].CycleKey == CycleKey))
		{
			*MatchingCycleIndex = i;
			return(TRUE);
		}

	}
	/* if we reach here, then no match was found (so return FALSE) */
	return(FALSE);
}


/*
*	CompareCycles
*
*	Used to compare a cycle detected in the Resource Dependency Graph (Adjacency List)
*	with a specified cycle (specified via 'CycleIndex')  stored in the Cycle List.
*
*	Returns TRUE if the two cycles macth completely.  Returns FALSE otherwise.
*/

int	CompareCycles(CycleIndex, StartingCircuit)
int		CycleIndex;
int		StartingCircuit;
{
	CYCLEVCNODEPTR	CurrentCycleVCNode;
	REQVCSPTR	CurrentReqVCNode;
	int		FoundStartingBranchIndex;
	int		StartingBranchIndex;
	int		i;
	int		NextCircuit;

	/* Find the ReqVCNode for the StartingCircuit */
	if (CircList[StartingCircuit].FirstReqVCNode != NULL_)
		CurrentReqVCNode = CircList[StartingCircuit].FirstReqVCNode;
	else
		CurrentReqVCNode = CircList[StartingCircuit].LastVCNode->ReqVCNode;

	/* Find the Branch Index for the StartingCircuit (StartingBranchIndex) */
	FoundStartingBranchIndex = FALSE;
	for (i = 0; i < MAXFANOUT; i++)
	{
		if ((CurrentReqVCNode->ReqVCNodeList[i] != NULL_) && (CurrentReqVCNode->VisitedList[i] == VISITED))
		{
			FoundStartingBranchIndex = TRUE;
			StartingBranchIndex = i;
			break;
		}
	}

	/* if we didn't find a branch index, then there's a problem - return FALSE (no match) */
	if (! FoundStartingBranchIndex)
	{
		printf("ERROR: Unexpected condition in 'CompareCycles'\n");
		return(FALSE);
	}

	/* First, find the Node in the Cycle which matches the Starting CircuitID and BranchIndex */

	/* set the pointer to the start of the linked list of Cycle VCs */
	CurrentCycleVCNode = CycleList[CycleIndex].FirstCycleVCNode;
	
	/* and traverse the link list until we find a CycleVCNode that maches Starting VCNode */
	while ((CurrentCycleVCNode != NULL_) && 
		((CurrentCycleVCNode->CircuitID != StartingCircuit) || (CurrentCycleVCNode->BranchIndex != StartingBranchIndex)))
	{
		CurrentCycleVCNode = CurrentCycleVCNode->NextCycleVCNode;
	}

	/* if we reach the end of the linked list without a match, then they are not the same */
	/* so, return FALSE */
	if (CurrentCycleVCNode == NULL_)
		return(FALSE);

	/* Get the ID of the Next Message (circuit) in the Cycle */
	NextCircuit = CurrentReqVCNode->ReqVCNodeList[StartingBranchIndex]->CircID;

	/* If we are here, then the first Circuit IDs and Branch Indeces matched, so we keep going (to see if others match) !!! */
	return(CompareCyclesVisitVC(CycleIndex, CurrentCycleVCNode->NextCycleVCNode, StartingCircuit, StartingBranchIndex, NextCircuit));

}


/*
*	CompareCyclesVisitVC
*
*	Used to compare a cycle detected in the Resource Dependency Graph (Adjacency List)
*	with a cycle in the Cycle List.
*/

int	CompareCyclesVisitVC(CycleIndex, CurrCycleVCNode, StartingCircuit, StartingBranchIndex, CurrentCircuit)
int		CycleIndex;
CYCLEVCNODEPTR	CurrCycleVCNode;	
int		StartingCircuit;	
int		StartingBranchIndex;	
int		CurrentCircuit;	
{
	CYCLEVCNODEPTR	CurrentCycleVCNode;	
	REQVCSPTR	CurrentReqVCNode;
	int		FoundCurrentBranchIndex;
	int		CurrentBranchIndex;
	int		i;
	int		NextCircuit;

	/* Find the ReqVCNode for the CurrentCircuit */
	if (CircList[CurrentCircuit].FirstReqVCNode != NULL_)
		CurrentReqVCNode = CircList[CurrentCircuit].FirstReqVCNode;
	else
		CurrentReqVCNode = CircList[CurrentCircuit].LastVCNode->ReqVCNode;

	/* Find the Branch Index for the CurrentCircuit (CurrentBranchIndex) */
	FoundCurrentBranchIndex = FALSE;
	for (i = 0; i < MAXFANOUT; i++)
	{
		if ((CurrentReqVCNode->ReqVCNodeList[i] != NULL_) && (CurrentReqVCNode->VisitedList[i] == VISITED))
		{
			FoundCurrentBranchIndex = TRUE;
			CurrentBranchIndex = i;
			break;
		}
	}

	/* if we didn't find a branch index, then there's a problem - return FALSE (no match) */
	if (! FoundCurrentBranchIndex)
	{
		printf("ERROR: Unexpected condition in 'CompareCyclesVisitVC'\n");
		return(FALSE);
	}

	/* if we are at the last CycleVCNode in the linked list, we need to reset to the first ("wrap-around")*/
	if (CurrCycleVCNode == NULL_)
		CurrentCycleVCNode = CycleList[CycleIndex].FirstCycleVCNode;
	else
		CurrentCycleVCNode = CurrCycleVCNode;

	/* if the two sets of corresponding values (CircuitID & Branch Index) are not the same, we dont have a match */
	/* so, return FALSE */
	if ((CurrentCycleVCNode->CircuitID != CurrentCircuit) || (CurrentCycleVCNode->BranchIndex != CurrentBranchIndex))
		return(FALSE);

	/* If we are here, then the values matched, so we keep going (to see if others match) !!! */

	/* If we are NOT YET back where we started from (the whole cycle has NOT YET been traversed !) */
	/* So keep on going ! */
	if ((CurrentCircuit != StartingCircuit) || (CurrentBranchIndex != StartingBranchIndex))
	{
		/* Get the ID of the Next Message (circuit) in the Cycle */
		NextCircuit = CurrentReqVCNode->ReqVCNodeList[CurrentBranchIndex]->CircID;

		/* If we are here, then the first Circuit IDs and Branch Indeces matched, so we keep going (to see if others match) !!! */
		return(CompareCyclesVisitVC(CycleIndex, CurrentCycleVCNode->NextCycleVCNode, StartingCircuit, StartingBranchIndex, NextCircuit));

	}

	/* Otherwise, we are back where we started - and we've matched completely, so return TRUE */
	else
	{
		return(TRUE);
	}
}



/*
*	
*	M E S S A G E / I N V O L V E D   C Y C L E    L I S T   M A N A G E M E N T 
*
*/

/*
*	InitMessageOrInvCycleList
*
*	Given a MessageList (array), simply initializes its fields.
*
*/

void	InitMessageOrInvCycleList(MessageList, MessageListSize, InitValue)
MSGSNODEPTR	MessageList;
int		MessageListSize;
int		InitValue;
{	
	int	i;

	for (i = 0; i < MessageListSize; i++)
		MessageList[i] = InitValue;
}


/*
*	AddToMessageOrInvCyclesList
*
*	Given a MessageList (array) and a MsgID, adds the MSgID to the first empty slot
*	(if any) if the message is not already in the list.  If there is no more space,
*	the size of the array is increased, and the element is added.
*	Nothing happens if the message is already in the list.
*
*	Returns ERROR:
*		- If memory reallocation fails
*
*	Returns NOERROR otherwise.
*/

int	AddToMessageOrInvCyclesList(MessageListPtr, MessageListSizePtr, MsgID, EndOfListSymbol, IncrementSize)
MSGSNODEPTRPTR	MessageListPtr;
int		*MessageListSizePtr;
int		MsgID;
int		EndOfListSymbol;
int		IncrementSize;
{	
	int		i=0;
	int		NewMessageListSize;
	MSGSNODEPTR	MessageList;

	/* Set value of pointer to the message list */
	MessageList = *MessageListPtr;

	/* Loop until we are at the end of the array or have found empty slot */
	while ((i < *MessageListSizePtr) && (MessageList[i] != EndOfListSymbol))
	{
		/* if this message is already in the list, then just return NOERROR */
		if (MessageList[i] == MsgID)
			return(NOERROR);

		i++;	
	}

	/* if we've reached the end of the array without finding a match */
	/* then the array is full - so we need to (try to) allocate more memory */
	if (i >= *MessageListSizePtr)
	{
		NewMessageListSize = *MessageListSizePtr + IncrementSize;

		*MessageListPtr = (MSGSNODEPTR) realloc(MessageList, NewMessageListSize * sizeof(MSGSNODE));

		if (*MessageListPtr == NULL_)
		{
			printf("ERROR: Memory re-allocation failed in 'AddToMessageOrInvCyclesList'\n");
			return(ERROR);
		}
	
		/* If we succesfully rellocated, then reset the size */
		*MessageListSizePtr = NewMessageListSize; 

		/* Reset value of pointer to the message list */
		MessageList = *MessageListPtr;

		/* Initialize the newly added portion */
		InitMessageOrInvCycleList(&MessageList[i], IncrementSize, EndOfListSymbol);
	}

	/* If we're here we add the element in the first empty slot- and return NOERROR */
	MessageList[i] = MsgID;
	return(NOERROR);
}


/*
*	RemoveFromMessageList
*
*	Given a MessageList (array) and a MsgIndex, removes the element contained
*	in the specified index (regardless of whether the index refers to a nonempty
*	slot).
*
*	Returns ERROR:
*		- If an invalid index (out of range) is specified.
*
*	Returns NOERROR otherwise.
*/

int	RemoveFromMessageList(MessageList, MessageListSize, InACycleList, MsgIndex)
MSGSNODEPTR	MessageList;
int		MessageListSize;
MSGSNODEPTR	InACycleList;	
int		MsgIndex;
{	
	int	i;

	/* if the specified index is illegal, then report error */
	if (MsgIndex >= MessageListSize)
	{
		printf("ERROR: Invalid index (out of range) specified in 'RemoveFromMessageList'\n");
		return(ERROR);
	}

	/* initialize */
	i=MsgIndex;

	/* From the point of removal, shift all the remaining elements on slot to the left */
	while ((i < (MessageListSize-1)) && (MessageList[i] != UNDEFINED_CIRCID))
	{
		/* shift the element to the left (in the Message List) */
		MessageList[i] = MessageList[i+1];

		/* shift the element to the left (in the InACycle List) - to mirror the Message List */
		InACycleList[i] = InACycleList[i+1];

		i++;
	}

	/* fill the gaps in the two lists - if one is left at the end */
	MessageList[i] = UNDEFINED_CIRCID;
	InACycleList[i] = NOTINACYCLE;

	return(NOERROR);
}



/*
*	SortMessageOrInvCyclesList
*
*	Given a MessageList (array), sorts it by ascending order (MessageID).
*	Uses the 'Selection Sort' algorithm.
*
*/

void	SortMessageOrInvCyclesList(MessageList, MessageListSize, EndOfListSymbol)
MSGSNODEPTR	MessageList;
int		MessageListSize;
int		EndOfListSymbol;
{	
	int	i, j, Minimum;
	int	TempMsgID;

	for (i = 0; i < MessageListSize-1; i++)
	{
		/* if the i index gets to a empty slot, break out of the outer loop - because we are done */
		if (MessageList[i] == EndOfListSymbol)
			break;

		Minimum = i;

		for (j = i+1; j < MessageListSize; j++)
		{
			/* if the j index gets to a empty slot, break out of the inner loop */
			if (MessageList[j] == EndOfListSymbol)
				break;

			/* Save the index of the next smallest element */
			if (MessageList[j] < MessageList[Minimum])
				Minimum = j;
		}

		/* if we have a new minimum value, then swap elements */
		if (Minimum != i)
		{
			TempMsgID = MessageList[i];
			MessageList[i] = MessageList[Minimum];
			MessageList[Minimum] = TempMsgID;
		}

	}
}


/*
*	CompareMessageOrInvCyclesLists
*
*	Given two message lists:
*	- Returns TRUE if they match exactly (including if they are both empty)
*	- Returns FALSE otherwise
*
*	Note: Assumes both lists are in same order.
*/

int	CompareMessageOrInvCyclesLists(MessageList1, MessageList1Size, MessageList2, MessageList2Size, EndOfListSymbol)
MSGSNODEPTR	MessageList1;
int		MessageList1Size;
MSGSNODEPTR	MessageList2;
int		MessageList2Size;
int		EndOfListSymbol;
{	
	int		i;
	int		SmallerListSize;
	int		LargerListSize;
	MSGSNODEPTR	LargerList;
	
	if (MessageList1Size > MessageList2Size)
	{	
		LargerListSize = MessageList1Size;
		SmallerListSize = MessageList2Size;
		LargerList = MessageList1;
	}
	else
	{
		LargerListSize = MessageList2Size;
		SmallerListSize = MessageList1Size;
		LargerList = MessageList2;
	}

	for (i = 0; i < SmallerListSize; i++)
	{
		/* if we've reached empty cells in both, break out of loop - because we are done */
		if ((MessageList1[i] == EndOfListSymbol) && 
			(MessageList2[i] == EndOfListSymbol))
			break;

		if (MessageList1[i] != MessageList2[i])
			return(FALSE);

	}

	/* this loop will be entered (for one iteration) if the two lists are not of the size */
	for (i = SmallerListSize; i < LargerListSize; i++)
	{
		if (LargerList[i] == EndOfListSymbol)
			break;

		else
			return(FALSE);

	}

	return(TRUE);
}


/*
*	CombineMessageLists
*
*	Given two message lists (MessageList1 & MessageList2), adds to the first message list, 
*	all of the elements int the second message list.
*
*	Returns ERROR if:
*	- The first message list becomes full (when elements are added to it)
*
*	Returns NOERROR otherwise
*
*/

int	CombineMessageLists(MessageList1Ptr, MessageList1SizePtr, MessageList2, MessageList2Size)
MSGSNODEPTRPTR	MessageList1Ptr;
int		*MessageList1SizePtr;
MSGSNODEPTR	MessageList2;
int		MessageList2Size;
{	
	int	i;
	int	error;

	for (i = 0; i < MessageList2Size; i++)
	{
		/* if we've reached empty cells in the second list, break out of loop - because we are done */
		if (MessageList2[i] == UNDEFINED_CIRCID)
			break;

		/* Add to the second message list an item from the first message list */
		error = AddToMessageOrInvCyclesList(MessageList1Ptr, MessageList1SizePtr, MessageList2[i], UNDEFINED_CIRCID, MSGLISTINCREMENTSIZE);

		/* if there's an error, then return error */
		if (error != NOERROR)
		{
			printf("ERROR: 'AddToMessageOrInvCyclesList' returned an error in 'CombineMessageLists'\n");
			return(ERROR);
		}
	}
	return(NOERROR);
}


/*
*	FindAndMarkIfInMessageList
*
*	Given a MessageList (array) and a MessageID., attempts to find the specified
*	message in the list using binary serach, and:
*	- returns TRUE if the Message (specified by 'MessageID') is in the list
*	- returns FALSE otherwise
*
*	NOTE: if found, marks the item in the list with Also, marks the '' (a temporary fix)
*	NOTE: only 'marks' if 'Mark' is TRUE.
*	NOTE: Assumes that the MessageList is sorted (in ascending order).
*/

int	FindAndMarkIfInMessageList(MessageList, MessageListSize, InACycleList, MessageID, Mark)
MSGSNODEPTR	MessageList;
int		MessageListSize;
MSGSNODEPTR	InACycleList;
int		MessageID;
int		Mark;
{	
	int	high, low, mid;

	low = 0;
	high = MessageListSize-1;

	while (low <= high)
	{
		mid = (low + high) / 2;

		/* if the middle element is an 'UNDEFINED_CIRCID', look in first half */
		if (MessageList[mid] == UNDEFINED_CIRCID)
			high = mid - 1;

		/* if the middle element is larger than 'MessageID', look in first half */
		else if (MessageID < MessageList[mid])
			high = mid - 1;

		/* if the middle element is smaller than 'MessageID', look in second half */
		else if (MessageID > MessageList[mid])
			low = mid + 1;

		/* if we're here, then we've found a match */
		else
		{
			/* Mark as being in a cycle only if the 'Mark' flag is set */
			if (Mark)
				InACycleList[mid] = INACYCLE;

			return(TRUE);
		}
	}

	return(FALSE);
}


/*
*	CountMessagesInList
*
*
*/

int	CountMessagesInList(MessageList, MessageListSize)
MSGSNODEPTR	MessageList;
int		MessageListSize;
{
	int	i;
	int	Count=0;

	for (i = 0; i < MessageListSize; i++)
	{
		if (MessageList[i] == UNDEFINED_CIRCID)
			break;
	
		Count++;
	}
	return(Count);
}


/*
*	CollectMsgsAndTagInACycle
*
*	Used to traverse a Cycle which has been detected, in order
*	to :
*		- gather information regarding the messages involved in the 
*		deadlock and
*
*		- to tag each VC Node in the Cycle as being 'InACycle'
*/

int	CollectMsgsAndTagInACycle(MessageListPtr, MessageListSizePtr, StartingVC, VCNode, NumOfVCs)
MSGSNODEPTRPTR	MessageListPtr;
int		*MessageListSizePtr;
int		StartingVC;	
VCNODEPTR	VCNode;
int		*NumOfVCs;	
{	
	int	i;
	int	error;
	int	LastCircIDAdded;

	/* Add the Circuit/Message ID of the first VC to the Message List */
	error = AddToMessageOrInvCyclesList(MessageListPtr, MessageListSizePtr, VCNode->CircID, UNDEFINED_CIRCID, MSGLISTINCREMENTSIZE);

	/* Initialize the Number of VCs found so far */
	*NumOfVCs = 1;

	/* Keep track of the last CircID added, to avoid repeatedly adding the same CircID - as an optimization */
	LastCircIDAdded = VCNode->CircID;

	/* Mark the node as being 'InACycle' */
	VCNode->InACycle = TRUE;

	/* if there's an error, then return error */
	if (error != NOERROR)
	{
		printf("ERROR: 'AddToMessageOrInvCyclesList' returned an error in 'CollectMsgsAndTagInACycle'\n");
		return(ERROR);
	}

	/* Otherwise, we will visit the cycle */

	if (VCNode->NextVCNode != NULL_)
	{
		return(CollectMsgsAndTagInACycleVisitVC(MessageListPtr, MessageListSizePtr, 
							StartingVC, VCNode->NextVCNode, LastCircIDAdded, NumOfVCs));
	}
	else if (VCNode->ReqVCNode != NULL_)
	{
		for (i = 0; i < MAXFANOUT; i++)
		{
			if ((VCNode->ReqVCNode->ReqVCNodeList[i] != NULL_) && (VCNode->ReqVCNode->VisitedList[i] == VISITED))
			{
				return(CollectMsgsAndTagInACycleVisitVC(MessageListPtr, MessageListSizePtr, StartingVC, 
									VCNode->ReqVCNode->ReqVCNodeList[i], LastCircIDAdded, NumOfVCs));
			}
		}

		printf("ERROR: Unexpected condition (1) in 'CollectMsgsAndTagInACycle'\n");
		return(ERROR);
	}
	else
	{
		printf("ERROR: Unexpected condition (2) in 'CollectMsgsAndTagInACycle'\n");
		return(ERROR);
	}
}


/*
*	CollectMsgsAndTagInACycleVisitVC
*
*	Used to traverse a Cycle which has been detected, in order
*	to :
*		- gather information regarding the messages involved in the 
*		deadlock and
*
*		- to tag each VC Node in the Cycle as being 'InACycle'
*/

int	CollectMsgsAndTagInACycleVisitVC(MessageListPtr, MessageListSizePtr, StartingVC, VCNode, LastCircIDAdded, NumOfVCs)
MSGSNODEPTRPTR	MessageListPtr;
int		*MessageListSizePtr;
int		StartingVC;	
VCNODEPTR	VCNode;
int		LastCircIDAdded;
int		*NumOfVCs;	
{
	int	error;
	int	i;
	int	FoundBranch;

	/* Do this until we are back where we started from (until whole cycle has been traversed !) */
	while (VCNode->VCID != StartingVC)
	{
		/* Increment the Number of VCs found so far */
		(*NumOfVCs)++;

		/* Mark the node as being 'InACycle' */
		VCNode->InACycle = TRUE;

		/* Only add the CircID to the Message List if it was not added previously - as an optimization */
		if (VCNode->CircID != LastCircIDAdded)
		{
			/* Add the Circuit/Message ID of the first VC to the Message List */
			error = AddToMessageOrInvCyclesList(MessageListPtr, MessageListSizePtr, VCNode->CircID, UNDEFINED_CIRCID, MSGLISTINCREMENTSIZE);

			/* Keep track of the last CircID added, to avoid repeatedly adding the same CircID */
			LastCircIDAdded = VCNode->CircID;

			/* if there's an error, then return error */
			if (error != NOERROR)
			{
				printf("ERROR: 'AddToMessageOrInvCyclesList' returned an error in 'CollectMsgsAndTagInACycleVisitVC'\n");
				return(ERROR);
			}
		}

		if (VCNode->NextVCNode != NULL_)
		{
			VCNode = VCNode->NextVCNode;
		}

		else if (VCNode->ReqVCNode != NULL_)
		{
			FoundBranch = FALSE;
			for (i = 0; i < MAXFANOUT; i++)
			{
				if ((VCNode->ReqVCNode->ReqVCNodeList[i] != NULL_) && (VCNode->ReqVCNode->VisitedList[i] == VISITED))
				{
					FoundBranch = TRUE;
					VCNode = VCNode->ReqVCNode->ReqVCNodeList[i];
					break;
				}
			}

			if (! FoundBranch)
			{
				printf("ERROR: Unexpected condition (1) in 'CollectMsgsAndTagInACycleVisitVC'\n");
				return(ERROR);
			}
		}
		else
		{
			printf("ERROR: Unexpected condition (2) in 'CollectMsgsAndTagInACycleVisitVC'\n");
			return(ERROR);
		}
	}

	/* If we are here, that means we are back where we started - so we're done ! */

	/* Mark the first node again (just in case!) as being 'InACycle' */
	VCNode->InACycle = TRUE;

	return(NOERROR);

}


/*
*	GenerateCycleKeyAndCountMessages
*
*	Used for generating a Cycle Key (by simply summing up the
*	MessageIDs of the specified Message List). 
*/

int	GenerateCycleKeyAndCountMessages(MessageList, MessageListSize, CycleKey, NumberOfMessages)
MSGSNODEPTR	MessageList;
int		MessageListSize;
int		*CycleKey;
int		*NumberOfMessages;
{
	int	i;

	/* Initialize */
	*CycleKey = 0;
	*NumberOfMessages = 0;

	/* Iterate and Generate Key and Count Messages Simultaneously */
	for (i = 0; i < MessageListSize; i++)
	{
		if (MessageList[i] == UNDEFINED_CIRCID)
			break;
	
		*CycleKey += MessageList[i];
		(*NumberOfMessages)++;
	}
}


/*
*	
*	C Y C L E   H A S H   T A B L E   M A N A G E M E N T  
*
*/

#ifdef HASH

/*
*	InitHashTable
*
*	Initializes all values and pointers in the Hash Table.
*	This is intended to be used to initialize this structure
*	before it is used.
*/

void	InitHashTable()
{
	int	i;

	for (i = 0; i < HASHTABLESIZE; i++)
	{
		HashTable[i].AllocatedSize = 0;	
		HashTable[i].LastItemIndex = UNDEFINED_LIST_INDEX;
		HashTable[i].ItemList = NULL_;
	}
}


/*
*	Hash
*
*	Simply computes the hash function.
*	Since the hash function is simple, it is currently "inlined"
*	to save the function call (so not used currently).
*
*/

int	Hash(CycleKey)
int	CycleKey;
{
	return((CycleKey << 8) % HASHTABLESIZE);
}


/*
*	AddToHashTable
*
*	Given a CycleKey and a CycleIndex, performs the has function on the CycleKey
*	to obtain an index into the Hash Table.  Then adds the CycleIndex to the
*	List contained in the hash bucket specified by the index into the Hash Table.
*	In the process, allocates memory for buckets which do not contain lists, 
*	and reallocates (additional) memory for buckets which have lists that are full.
*
*	Returns ERROR:
*		- If memory allocation for a new list fails
*		- If memory re-allocation for increasing the size of an existing list fails
*
*	Returns NOERROR otherwise.
*/

int	AddToHashTable(CycleKey, CycleIndex, HashTableListIndex)
int		CycleKey;
int		CycleIndex;
int		*HashTableListIndex;
{	
	int		HashIndex;
	int		NewHashTableListSize;
	MSGSNODEPTR	NewHashTableList;

/*
printf("Entering 'AddToHashTable'\n");
*/
	/* Calculate the Hash Function */
	HashIndex = (CycleKey << 8) % HASHTABLESIZE;

	/* If we have yet to allocate memory for a list */
	if (!HashTable[HashIndex].AllocatedSize)
	{
		/* Then allocate memory */
		HashTable[HashIndex].ItemList = (MSGSNODEPTR) malloc(INITIALHASHTABLELISTSIZE * sizeof(MSGSNODE));

		/* Check if error occured */
		if (HashTable[HashIndex].ItemList == NULL_)
		{
			printf("ERROR: Memory allocation failed in 'AddToHashTable'\n");
			return(ERROR);
		}

		/* if no error, initialize values */
		InitMessageOrInvCycleList(HashTable[HashIndex].ItemList, INITIALHASHTABLELISTSIZE, UNDEFINED_CYCLEID);

		HashTable[HashIndex].AllocatedSize = INITIALHASHTABLELISTSIZE;

		/* set this to '-1' so that when we increment before adding,	*/
		/* we are at the first slot ('0')				*/
		HashTable[HashIndex].LastItemIndex = -1;
	}

	/* if memory has been allocated, but the list is full */
	else if ((HashTable[HashIndex].LastItemIndex + 1) == HashTable[HashIndex].AllocatedSize)
	{

		/* Then re-allocate memory */
		NewHashTableListSize = HashTable[HashIndex].AllocatedSize + HASHTABLELISTINCREMENTSIZE;

		NewHashTableList = (MSGSNODEPTR) realloc(HashTable[HashIndex].ItemList, NewHashTableListSize * sizeof(MSGSNODE));

		if (NewHashTableList == NULL_)
		{
			printf("ERROR: Memory re-allocation failed in 'AddToHashTable'\n");
			return(ERROR);
		}

		/* if no error, then initialize and reset the values */

		/* Reset value of pointer to the newly allocated list */
		HashTable[HashIndex].ItemList = NewHashTableList;

		/* Initialize the newly allocated portion of the list */
		InitMessageOrInvCycleList(&HashTable[HashIndex].ItemList[HashTable[HashIndex].AllocatedSize], HASHTABLELISTINCREMENTSIZE, UNDEFINED_CYCLEID);

		HashTable[HashIndex].AllocatedSize = NewHashTableListSize; 


	}

	/* Increment List Index - to add new entry */
	(HashTable[HashIndex].LastItemIndex)++;

	HashTable[HashIndex].ItemList[HashTable[HashIndex].LastItemIndex] = CycleIndex;

	*HashTableListIndex = HashTable[HashIndex].LastItemIndex;

	/* return no error */
	return(NOERROR);
/*
printf("Leaving 'AddToHashTable'\n");
*/

}



/*
*	RemoveFromHashTable
*
*	Given a CycleKey, performs the hash function on the CycleKey to obtain
*	an index into the hash table.  Then, removes an item from the list belonging
*	to the hash bucket specified by the calculated index.  Uses 'HashTableListIndex'
*	as the index (within the list) of the item to be deleted.  Also, moves the
*	last item on the list to the slot vacated by the deleted item (to eliminate
*	gaps in the list).
*
*	Returns ERROR:
*		- If an invalid index (out of range) is specified.
*		- If the value intended to be deleted is not in the list at the specified index
*
*	Returns NOERROR otherwise.
*/

int	RemoveFromHashTable(CycleKey, CycleIndex, HashTableListIndex)
int		CycleKey;
int		CycleIndex;
int		HashTableListIndex;
{	
	int	HashIndex;
/*
printf("Entering 'RemoveFromHashTable'\n");
*/
	/* Calculate the Hash Function */
	HashIndex = (CycleKey << 8) % HASHTABLESIZE;

	/* Check the Range of the HashTableListIndex */
	if ((HashTableListIndex >= HashTable[HashIndex].AllocatedSize) || (HashTableListIndex > HashTable[HashIndex].LastItemIndex))
	{
		printf("ERROR: Invalid HashTableListIndex (out of range) specified in 'RemoveFromHashTable'\n");
		return(ERROR);
	}

	/* Check for Validity of contents in location specified by the HashTableListIndex (is this the value we intend to delete ?) */
	if (HashTable[HashIndex].ItemList[HashTableListIndex] == CycleIndex)
	{
		/* If the one we are deleting is not the last item in the list */
		if (HashTable[HashIndex].LastItemIndex != HashTableListIndex)
		{
			/* Move the last item in the list to fill the hole left from deleting this */	
			HashTable[HashIndex].ItemList[HashTableListIndex] = HashTable[HashIndex].ItemList[HashTable[HashIndex].LastItemIndex];

			/* Then update the 'HashTableListIndex' field of the Cycle represented by the last item in the list */
			UpdateCycleHashTableListIndex(HashTable[HashIndex].ItemList[HashTable[HashIndex].LastItemIndex], HashTableListIndex);	
		}

		/* Clear the last item */
		HashTable[HashIndex].ItemList[HashTable[HashIndex].LastItemIndex] = UNDEFINED_CYCLEID;

		/* Decrement the index to the Last Item */
		 (HashTable[HashIndex].LastItemIndex)--;

		/* return no error */
/*
printf("Leaving 'RemoveFromHashTable'\n");
*/
		return(NOERROR);
	}

	/* Report error if there is a mismatch in the value to be deleted */
	printf("ERROR: Incorrect CycleIndex value specified in 'RemoveFromHashTable'\n");
	return(ERROR);
}


/*
*	GetHashTableList
*
*	Given a CycleKey, performs the hash function on the CycleKey to obtain
*	an index into the hash table.  Then, returns the List and the Size of the
*	list stored in the hash bucket specified by the calculated index.
*
*	Returns NOERROR otherwise.
*/

void	GetHashTableList(CycleKey, HashTableList, HashTableListSize)
int		CycleKey;
MSGSNODEPTR	*HashTableList;
int		*HashTableListSize;
{	
	int	HashIndex;
/*
printf("Entering 'GetHashTableList'\n");
*/
	/* Calculate the Hash Function */
	HashIndex = (CycleKey << 8) % HASHTABLESIZE;

	/* Return pointer to the list */
	*HashTableList = HashTable[HashIndex].ItemList;

	/* Return size of the list */
	*HashTableListSize = HashTable[HashIndex].LastItemIndex + 1;
/*
printf("Leaving 'GetHashTableList'\n");
*/
}
#endif


/* 
*
*	  C Y C L E   B U I L D I N G   R O U T I N E S 
*
*/

/*
*	CreateDetectedCycle
*
*	Used for creating a copy of a detected cycle in the Cycle List. 
*/

int	CreateDetectedCycle(CycleIndex, StartingCircuit)
int	CycleIndex;
int	StartingCircuit;	
{
	int		error;
	REQVCSPTR	CurrentReqVCNode;
	int		FoundStartingBranchIndex;
	int		StartingBranchIndex;
	int		i;
	int		NextCircuit;

	/* Find the ReqVCNode for the StartingCircuit */
	if (CircList[StartingCircuit].FirstReqVCNode != NULL_)
		CurrentReqVCNode = CircList[StartingCircuit].FirstReqVCNode;
	else
		CurrentReqVCNode = CircList[StartingCircuit].LastVCNode->ReqVCNode;

	/* Find the Branch Index for the StartingCircuit (StartingBranchIndex) */
	FoundStartingBranchIndex = FALSE;
	for (i = 0; i < MAXFANOUT; i++)
	{
		if ((CurrentReqVCNode->ReqVCNodeList[i] != NULL_) && (CurrentReqVCNode->VisitedList[i] == VISITED))
		{
			FoundStartingBranchIndex = TRUE;
			StartingBranchIndex = i;
			break;
		}
	}

	/* if we didn't find a branch index, then there's a problem - return ERROR */
	if (! FoundStartingBranchIndex)
	{
		printf("ERROR: Unexpected condition (1) in 'CreateDetectedCycle'\n");
		return(ERROR);
	}

	/* Add node to cycle */
	error = AddCycleVCNodeToCycle(CycleIndex, StartingCircuit, StartingBranchIndex);

	/* if there's an error, then return error */
	if (error != NOERROR)
	{
		printf("ERROR: 'AddCycleVCNodeToCycle' returned an error in 'CreateDetectedCycle'\n");
		return(ERROR);
	}

	/* Get the ID of the Next Message (circuit) in the Cycle */
	NextCircuit = CurrentReqVCNode->ReqVCNodeList[StartingBranchIndex]->CircID;

	return(CreateDetectedCycleVisitVC(CycleIndex, StartingCircuit, StartingBranchIndex, NextCircuit));
}


/*
*	CreateDetectedCycleVisitVC
*
*	Used for creating a copy of a detected cycle in the Cycle List. 
*/

int	CreateDetectedCycleVisitVC(CycleIndex, StartingCircuit, StartingBranchIndex, CurrentCircuit)
int		CycleIndex;
int		StartingCircuit;	
int		StartingBranchIndex;	
int		CurrentCircuit;	
{

	int		error;
	REQVCSPTR	CurrentReqVCNode;
	int		FoundCurrentBranchIndex;
	int		CurrentBranchIndex;
	int		i;

	/* Find the ReqVCNode for the CurrentCircuit */
	if (CircList[CurrentCircuit].FirstReqVCNode != NULL_)
		CurrentReqVCNode = CircList[CurrentCircuit].FirstReqVCNode;
	else
		CurrentReqVCNode = CircList[CurrentCircuit].LastVCNode->ReqVCNode;

	/* Find the Branch Index for the CurrentCircuit (CurrentBranchIndex) */
	FoundCurrentBranchIndex = FALSE;
	for (i = 0; i < MAXFANOUT; i++)
	{
		if ((CurrentReqVCNode->ReqVCNodeList[i] != NULL_) && (CurrentReqVCNode->VisitedList[i] == VISITED))
		{
			FoundCurrentBranchIndex = TRUE;
			CurrentBranchIndex = i;
			break;
		}
	}

	/* if we didn't find a branch index, then there's a problem - return ERROR */
	if (! FoundCurrentBranchIndex)
	{
		printf("ERROR: Unexpected condition (1) in 'CreateDetectedCycleVisitVC'\n");
		return(ERROR);
	}

	/* Do this until we are back where we started from (until whole cycle has been traversed !) */
	while ((CurrentCircuit != StartingCircuit) || (CurrentBranchIndex != StartingBranchIndex))
	{
		error = AddCycleVCNodeToCycle(CycleIndex, CurrentCircuit, CurrentBranchIndex);

		/* if there's an error, then return error */
		if (error != NOERROR)
		{
			printf("ERROR: 'AddCycleVCNodeToCycle' returned an error in 'CreateDetectedCycleVisitVC'\n");
			return(ERROR);
		}

		/* Get the ID of the Next Message (circuit) in the Cycle */
		CurrentCircuit = CurrentReqVCNode->ReqVCNodeList[CurrentBranchIndex]->CircID;

		/* Find the ReqVCNode for the NextCircuit */
		if (CircList[CurrentCircuit].FirstReqVCNode != NULL_)
			CurrentReqVCNode = CircList[CurrentCircuit].FirstReqVCNode;
		else
			CurrentReqVCNode = CircList[CurrentCircuit].LastVCNode->ReqVCNode;

		/* Find the Branch Index for the NextCircuit (CurrentBranchIndex) */
		FoundCurrentBranchIndex = FALSE;
		for (i = 0; i < MAXFANOUT; i++)
		{
			if ((CurrentReqVCNode->ReqVCNodeList[i] != NULL_) && (CurrentReqVCNode->VisitedList[i] == VISITED))
			{
				FoundCurrentBranchIndex = TRUE;
				CurrentBranchIndex = i;
				break;
			}
		}

		/* if we didn't find a branch index, then there's a problem - return ERROR */
		if (! FoundCurrentBranchIndex)
		{
			printf("ERROR: Unexpected condition (2) in 'CreateDetectedCycleVisitVC'\n");
			return(ERROR);
		}

	}

	/* If we are here, that means we are back where we started - so we're done ! */
	return(NOERROR);	
}


/* 
*
*	  D E A D L O C K   L I S T   M A N A G E M E N T   R O U T I N E S 
*
*/

/*
*	AddDeadlockToList
*
*	Adds a new deadlock to the Deadlock List and Returns an index to the
*	Deadlock List where the Deadlock was added.  Also, records the specified
*	time as the time at which the deadlock was first detected.
*
*	Returns ERROR if:
*		- Deadlock list is already filled to capacity
*
*	Returns NOERROR otherwise.
*/

int	AddDeadlockToList(TimeFirstDetected, DeadlockIndex, InvolvedMessagesList, InvolvedCyclesList, DependentMessagesList, 
			MsgListSize, InvolvedCyclesListSize, DepMsgListSize)
int		TimeFirstDetected;
int		*DeadlockIndex;
MSGSNODEPTR	InvolvedMessagesList;
MSGSNODEPTR	InvolvedCyclesList;
MSGSNODEPTR	DependentMessagesList;
int		MsgListSize;
int		InvolvedCyclesListSize;
int		DepMsgListSize;
{
	int	i;

	/* if we have already filled the list of Cycles, we cant add anymore ! */
	if (NextAvailDeadlock >= MAXDEADLOCKS)
	{
		printf("ERROR: Deadlock list is full in 'AddDeadlockToList'\n");
		return(ERROR);
	}

	/* Activate it */
	DeadlockList[NextAvailDeadlock].Active = TRUE;

	/* Set its first/last detected times */
	DeadlockList[NextAvailDeadlock].FirstDetected = TimeFirstDetected;
	DeadlockList[NextAvailDeadlock].LastDetected = TimeFirstDetected;

	/* What do we do with the other fields ??? */
	/* We should set these fields properly	   */
	DeadlockList[NextAvailDeadlock].NumofMessages = 0;		
	DeadlockList[NextAvailDeadlock].NumofVCs = 0;		
	DeadlockList[NextAvailDeadlock].NumofCycles = 0;
	DeadlockList[NextAvailDeadlock].NumofDepMessages = 0;
	DeadlockList[NextAvailDeadlock].NumofDepVCs = 0;

	DeadlockList[NextAvailDeadlock].MsgListSize = MsgListSize;
	DeadlockList[NextAvailDeadlock].InvCyclesListSize = InvolvedCyclesListSize;
	DeadlockList[NextAvailDeadlock].DepMsgListSize = DepMsgListSize;

	DeadlockList[NextAvailDeadlock].Messages = InvolvedMessagesList;
	DeadlockList[NextAvailDeadlock].Cycles = InvolvedCyclesList;
	DeadlockList[NextAvailDeadlock].DepMessages = DependentMessagesList;

	/* return the index where deadlock was added, and increment indxe */
	*DeadlockIndex = NextAvailDeadlock++;

	return(NOERROR);
}


/*
*	UpdateDeadlockTimeLastDetected
*
*	Updates the 'LastDetected' field of the deadlock specified via
*	the 'DeadlockIndex'.  Also, adds to the existing dependent message lists
*	new dependent messages found (via the NewDependentMessagesList)
*
*	Returns ERROR if:
*		- an illegal Deadlock List index is given
*		- an error is generated when combining the existing and new dependent messages lists
*
*	Returns NOERROR otherwise.
*/

int	UpdateDeadlockTimeLastDetected(DeadlockIndex, TimeLastDetected, NewDependentMessagesList, NewDependentMessagesListSize)
int		DeadlockIndex;
int		TimeLastDetected;
MSGSNODEPTR	NewDependentMessagesList;
int		NewDependentMessagesListSize;
{
	int	error;

	/* if given an illegal index, return error */
	if (DeadlockIndex >= MAXDEADLOCKS)
	{
		printf("ERROR: illegal Deadlock List index specified in 'UpdateDeadlockTimeLastDetected'\n");
		return(ERROR);
	}

	/* otherwise, update time last detected and return no error */
	DeadlockList[DeadlockIndex].LastDetected = TimeLastDetected;

	error = CombineMessageLists(&(DeadlockList[DeadlockIndex].DepMessages), 
					&(DeadlockList[DeadlockIndex].DepMsgListSize), 
					NewDependentMessagesList, NewDependentMessagesListSize);

	if (error != NOERROR)
	{
		printf("ERROR: 'CombineMessageLists' returned error in 'UpdateDeadlockTimeLastDetected'\n");
		return(ERROR);
	}

	SortMessageOrInvCyclesList(DeadlockList[DeadlockIndex].DepMessages, DeadlockList[DeadlockIndex].DepMsgListSize, UNDEFINED_CIRCID);

	return(NOERROR);
}


/*
*	CountAndSetNumberofDeadlockMessagesAndVCs
*
*	Counts the number of messages in the specified deadlock and sets
*	the 'Count' value accordingly.  Also, counts and sets the 
*	the total number of VCs in the deadlock.
*	
*
*	Returnes ERROR:
*	- If a the messages list is invalid (NULL_)
*
*	Returns NOERROR otherwise.
*
*/

int	CountAndSetNumberofDeadlockMessagesAndVCs(DeadlockIndex)
int	DeadlockIndex;
{
	int	i;
	
	if (DeadlockList[DeadlockIndex].Messages == NULL_)
	{
		printf("ERROR: NULL_ Message List pointer found in 'CountAndSetNumberofDeadlockMessagesAndVCs'\n");
		return(ERROR);
	}

	for (i = 0; (i < DeadlockList[DeadlockIndex].MsgListSize) && (DeadlockList[DeadlockIndex].Messages[i] != UNDEFINED_CIRCID); i++)
	{
		DeadlockList[DeadlockIndex].NumofMessages++;

		/* Also tabulate the number of VCs for each message */
		DeadlockList[DeadlockIndex].NumofVCs += CountNumberofCircuitVCs(DeadlockList[DeadlockIndex].Messages[i]);
	}

	return(NOERROR);
}


/*
*	CountAndSetNumberofDependentMessagesAndVCs
*
*	Counts the number of dependent messages in the specified deadlock and sets
*	the 'Count' value accordingly.
*
*	Returnes ERROR:
*	- If a the messages list is invalid (NULL_)
*
*	Returns NOERROR otherwise.
*
*/

int	CountAndSetNumberofDependentMessagesAndVCs(DeadlockIndex)
int		DeadlockIndex;
{
	int	i;
	
	/* Need to reset each time - since this may be called for the same list multiple times */
	DeadlockList[DeadlockIndex].NumofDepMessages = 0;
	DeadlockList[DeadlockIndex].NumofDepVCs = 0;

	if (DeadlockList[DeadlockIndex].DepMessages == NULL_)
	{
		printf("ERROR: NULL_ Message List pointer found in 'CountAndSetNumberofDependentMessagesAndVCs'\n");
		return(ERROR);
	}

	for (i = 0; (i < DeadlockList[DeadlockIndex].DepMsgListSize) && (DeadlockList[DeadlockIndex].DepMessages[i] != UNDEFINED_CIRCID); i++)
	{
		DeadlockList[DeadlockIndex].NumofDepMessages++;

		/* Also tabulate the number of VCs for each dependent message */
		DeadlockList[DeadlockIndex].NumofDepVCs += CountNumberofCircuitVCs(DeadlockList[DeadlockIndex].DepMessages[i]);
	}

	return(NOERROR);
}


/*
*	CountAndSetNumberofDeadlockCycles
*
*	Counts the number of cycles in the specified deadlock and sets
*	the 'NumofCycles' field accordingly.
*
*	Returnes ERROR:
*	- If a the cycle list is invalid (NULL_)
*
*	Returns NOERROR otherwise.
*
*/

int	CountAndSetNumberofDeadlockCycles(DeadlockIndex)
int	DeadlockIndex;
{
	int	i;
	
	if (DeadlockList[DeadlockIndex].Cycles == NULL_)
	{
		printf("ERROR: NULL_ Cycle List pointer found in 'CountAndSetNumberofDeadlockCycles'\n");
		return(ERROR);
	}

	DeadlockList[DeadlockIndex].NumofCycles = 0;

	for (i = 0; (i < DeadlockList[DeadlockIndex].InvCyclesListSize) && (DeadlockList[DeadlockIndex].Cycles[i] != UNDEFINED_CYCLEID); i++)
	{
		DeadlockList[DeadlockIndex].NumofCycles++;
	}

	return(NOERROR);
}


/*
*	RetireDeadlock
*
*	This is intended to be used when "deactivating" a deadlock.
*	It sets the 'Active' flag to FALSE (Inactive) and frees memory
*	for the messages, involved cycles, and dependent messages lists.
*	Also resets some of the fields.
*
*	Returnes ERROR:
*
*	Returns NOERROR otherwise.
*/

int	RetireDeadlock(DeadlockIndex)
int	DeadlockIndex;
{
	int error;

	/* Reset everything to show "deactivation" */	
	DeadlockList[DeadlockIndex].Active = FALSE;

	/* free the memory for the message list */
	free(DeadlockList[DeadlockIndex].Messages);

	/* free the memory for the involved cycle list */
	free(DeadlockList[DeadlockIndex].Cycles);

	/* free the memory for the dependent message list */
	free(DeadlockList[DeadlockIndex].DepMessages);

	DeadlockList[DeadlockIndex].Messages = NULL_;
	DeadlockList[DeadlockIndex].Cycles = NULL_;
	DeadlockList[DeadlockIndex].DepMessages = NULL_;

	return(NOERROR);
}


/*
*	RetireEligibleDeadlocks
*
*	Checks every "active" deadlock to see if it has been observed (and 
*	therefore updated) during the current simulation cycle.  If NOT,
*	then it calls 'RetireDeadlock' to deactivate the deadlock.
*
*	Returnes ERROR:
*	- If a call to 'RetireDeadlock' generates an error
*
*	Returns NOERROR otherwise.
*/

int	RetireEligibleDeadlocks()
{
	int	i;
	int	error;

	for (i = 0; i < NextAvailDeadlock; i++)
	{
		if ((DeadlockList[i].Active) && (DeadlockList[i].LastDetected < sim_time))
		{
			error = RetireDeadlock(i);

			if (error != NOERROR)
			{
				printf("ERROR: 'RetireDeadlock' returned an error in 'RetireEligibleDeadlocks'\n");
				return(ERROR);
			}
		}
	}
	return(NOERROR);
}


/*
*	IsMessageInADeadlock
*
*	Given a Message ID, checks every deadlock to see if the specified
*	message belongs to a currently active deadlock.  
*
*	If found in a deadlock, returns TRUE.  Returns FALSE otherwise.
*
*/

int	IsMessageInADeadlock(MessageID, MatchingDeadlockIndex)
int	MessageID;
int	*MatchingDeadlockIndex;
{
	int	i;

	for (i = 0; i < NextAvailDeadlock; i++)
	{
		if ((DeadlockList[i].Active))
		{
			if (FindAndMarkIfInMessageList(DeadlockList[i].Messages, DeadlockList[i].MsgListSize, NULL_, MessageID, FALSE))
			{
				*MatchingDeadlockIndex = i;
				return(TRUE);
			}	
		}
	}
	return(FALSE);
}



/* 
*
*	  D E P E N D E N T   M E S S A G E S   L I S T  M A N A G E M E N T  R O U T I N E S 
*
*/

/*
*	SeparateDependentMessages
*
*	Given a MessageList (containing both messages in a deadlock and dependent messages),
*	removes the dependent messages and places them in a separate list.
*
*	Note: Dependent messages are distinguished from messages in the deadlock via 
*	the entries in the  InACycleList list.  If a message is in a cycle, then it is in the 
*	deadlock.  If not, it is simply a dependent message.
*
*/

int	SeparateDependentMessages(MessageList, MessageListSize, InACycleList, DepMessageListPtr, DepMessageListSizePtr)
MSGSNODEPTR	MessageList;
int		MessageListSize;
MSGSNODEPTR	InACycleList;
MSGSNODEPTRPTR	DepMessageListPtr;
int		*DepMessageListSizePtr;
{
	int	i;
	int	error;

	for (i = 0; i < MessageListSize; i++)
	{
		if (MessageList[i] == UNDEFINED_CIRCID)
			break;

		if (InACycleList[i] == NOTINACYCLE)
		{
			/* Add this message to the Dependent Message List */
			error = AddToMessageOrInvCyclesList(DepMessageListPtr, DepMessageListSizePtr, MessageList[i], UNDEFINED_CIRCID, MSGLISTINCREMENTSIZE);

			/* if there's an error, then return error */
			if (error != NOERROR)
			{
				printf("ERROR: 'AddToMessageOrInvCyclesList' returned an error in 'SeparateDependentMessages'\n");
				return(ERROR);
			}

			/* Remove this message from the Message List */
			error = RemoveFromMessageList(MessageList, MessageListSize, InACycleList, i);

			/* if there's an error, then return error */
			if (error != NOERROR)
			{
				printf("ERROR: 'RemoveFromMessageList' returned an error in 'SeparateDependentMessages'\n");
				return(ERROR);
			}

		}
	}
}



/* 
*
*	  D E A D L O C K   D E T E C T I O N   R O U T I N E S 
*
*/

/*
*	ClearInACycleFlags
*
*	Clears all of the "InACycle" flags of ReqVC nodes.  Also clears the 
*	"AllReqVCsInACycle" flag for each circuit.  
*
*	This is intended to initialize these flags prior to running the 
*	Cycle Detection algorithm.  Note that the Cycle Detection algorithm 
*	sets these flags, which are then used to detect groups of cycles in a Deadlock.
*
*/

void	ClearInACycleFlags()
{
	int		i;

	/* For each VC, clear its 'InACycle' flag */
	for (i = 0; i < MAXVC; i++)
	{
		VCList[i].InACycle = FALSE;
	}

	/* For each circuit, clear its 'AllReqVCsInACycle' flag */
	for (i = 0; i < MAXCIRC; i++)
	{
		CircList[i].AllReqVCsInACycle = FALSE;
	}
}



/*
*	MarkCircuitsWithAllReqVCsInACycle
*
*	Checks each circuit to see if all of its ReqVCNodes have been marked as being in a Cycle
*	(by the Cycle Detection algorithm).  Marks those which have all their ReqVCNodes in a Cycle.
*	This information is used (by deadlock detection) to find groups of cycles in Deadlock.
*
*	Returns ERROR if:
*		- In the process, we find a Circuit w/ mismatching First and Last VC Node pointers
*
*	Returns NOERROR otherwise.
*/


int	MarkCircuitsWithAllReqVCsInACycle()
{
	int		i, j;
	REQVCSPTR	FanoutNode=NULL_;
	int		AllInACycleFlagsAreSet;
	int		AnyReqVCsFound;

	for (i = 0; i < MAXCIRC; i++)
	{
		/* only do stuff if there are VC nodes OR ReqVC nodes at the begining */
		/* otherwise, iterate to the next element as fast as possible         */
		if ((CircList[i].FirstVCNode != NULL_) || (CircList[i].FirstReqVCNode != NULL_))
		{
			/* if we are here, then				*/
			/* either there are VC nodes (at the begining) 	*/
			if  (CircList[i].FirstVCNode != NULL_)
			{
				/* Note that if the 'FirstVCNode' is set, then also the	*/
				/* 'LastVCNode' must be set.				*/
	 			if (CircList[i].LastVCNode != NULL_)
					FanoutNode = CircList[i].LastVCNode->ReqVCNode;

				/* report an error otherwise */
				else
				{
					printf("ERROR: Unexpected condition in 'MarkCircuitsWithAllReqVCsInACycle'\n");
					return(ERROR);
				}
			}

			/* or there is a ReqVCNode at the begining */ 
			else
			{
				FanoutNode = CircList[i].FirstReqVCNode;
			}

			/* if there is an actual fanout node */
			if (FanoutNode != NULL_)
			{
				/* Then check if all the 'InACycle' flags are set */
	
				AllInACycleFlagsAreSet = TRUE;
				AnyReqVCsFound = FALSE;

				for (j = 0; j < MAXFANOUT; j++)
				{	
					if (FanoutNode->ReqVCNodeList[j] != NULL_)
					{
						/* If we're here, then there is atleast one ReqVC Node - so set this */
						AnyReqVCsFound = TRUE;

						/* Maintain 'AllInACycleFlagsAreSet' - TRUE only if all flags are TRUE */
						AllInACycleFlagsAreSet = (AllInACycleFlagsAreSet && FanoutNode->ReqVCNodeList[j]->InACycle);

						/* if it's FALSE already, it'll never be TRUE again - so break out of the loop */
						if (! AllInACycleFlagsAreSet)
							break;
					}
				}

				/* Set this value only if there was atleast one ReqVCNode found */
				CircList[i].AllReqVCsInACycle = (AllInACycleFlagsAreSet && AnyReqVCsFound);
			}
		}	
	}

	return(NOERROR);
}



/*
*	FindMatchingDeadlock
*
*	Given an 'InvolvedMessages' List and an 'InvolvedCycles' List, 
*	checks in the deadlock list to see if there already exists a 
*	deadlock with the same 'InvolvedMessages' and 'InvolvedCycles'.
*	If there is a match, returns the index (into the deadlock list)
*	of the matching deadlock.
*
*	Returns TRUE if there is an exact match in the deadlock list.
*
*	Returns FALSE otherwise.
*
*	Note: that the index of the first matching deadlock is returned.
*	So it assumes that the Deadlock list does not contain duplicates.
*
*	Also: SHOULD ERROR CHECK TO SEE IF THE LISTS ARE EMPTY ...
*/


int	FindMatchingDeadlock(InvolvedMessages, InvolvedMessagesListSize, InvolvedCycles, InvolvedCyclesListSize, index)
MSGSNODEPTR		InvolvedMessages;
int			InvolvedMessagesListSize;
MSGSNODEPTR		InvolvedCycles;
int			InvolvedCyclesListSize;
int			*index;
{
	int	i;

	for (i = 0; i < NextAvailDeadlock; i++)
	{
		if (DeadlockList[i].Active)
		{
			/* if both lists match, then we found a matching deadlock */
			if ((CompareMessageOrInvCyclesLists(DeadlockList[i].Messages, DeadlockList[i].MsgListSize, InvolvedMessages, 
								InvolvedMessagesListSize, UNDEFINED_CIRCID)) &&
				(CompareMessageOrInvCyclesLists(DeadlockList[i].Cycles, DeadlockList[i].InvCyclesListSize, InvolvedCycles, 
								InvolvedCyclesListSize, UNDEFINED_CYCLEID)))
			{
				*index = i;
				return(TRUE);
			}
		}
	}

	/* if we are here, then no match was found */
	return(FALSE);
}


/*
*	FindProperCyclesforDeadlock
*
*	Given an Involved Messages List, traverses the cycle list to see if
*	there are cycles which are composed of only those messages in the 
*	'Involved Messages List'.  These (proper) cycles are added to 
*	the specified 'InvolvedCycles' list.
*
*	Returns ERROR if:
*	- Adding Cycle to InvolvedCyclesList fails.
*	- An illegal cycle (w/ no messages) is found
*	- An illegal cycle (w/ some messages in deadlock and other not in deadlock) is found
*
*	Returns NOERROR otherwise.
*
*	'Found' is set to TRUE if one or more proper cycles are found.  'Found' is set
*	to FALSE otherwise.
*/

int	FindProperCyclesforDeadlock(InvolvedMessages, InvolvedMessagesListSize, InACycleList, InvolvedCyclesPtr, InvolvedCyclesListSizePtr, Found)
MSGSNODEPTR		InvolvedMessages;
int			InvolvedMessagesListSize;
MSGSNODEPTR		InACycleList;
MSGSNODEPTRPTR		InvolvedCyclesPtr;
int			*InvolvedCyclesListSizePtr;
int			*Found;
{
	int 	i, j;
	int	InInvMessageList;
	int	AtleastOneInInvMessageList;
	int	AtleastOneNotInInvMessageList;
	int	error;

	*Found = FALSE;

	for (i = 0; i < NextAvailCycle; i++)
	{
		/* Only compare active cycles */
		if (CycleList[i].Active)
		{
			/* Initialize */
			InInvMessageList = FALSE;
			AtleastOneInInvMessageList = FALSE;
			AtleastOneNotInInvMessageList = FALSE;

			/* For each message in this Cycle */
			for (j = 0; j <  CycleList[i].MsgListSize ; j++)
			{
				/* Break out of loop when we reach the end of contents */
				if (CycleList[i].Messages[j] == UNDEFINED_CIRCID)
					break;

				/* Check to see if the message is in the  'InvolvedMessages' list */
				/* Also, if the message is in the cycle, it will be marked as such in the InACycleList */
				InInvMessageList = FindAndMarkIfInMessageList(InvolvedMessages, InvolvedMessagesListSize, 
										InACycleList, CycleList[i].Messages[j], TRUE) ;

				/* if it's in, then note that atleast one is in ! */
				if (InInvMessageList)
				{
					AtleastOneInInvMessageList = TRUE;
				}

				/* if it's NOT in, then note that atleast one is NOT in ! */
				else /* if (!InInvMessageList) */
				{
					AtleastOneNotInInvMessageList = TRUE;
				}
			}

			/* if they are all in, then this cycle belongs to the deadlock */
			if ((AtleastOneInInvMessageList) && (!AtleastOneNotInInvMessageList))
			{
				error = AddToMessageOrInvCyclesList(InvolvedCyclesPtr, InvolvedCyclesListSizePtr, i, 
								UNDEFINED_CYCLEID, INVCYCLISTINCREMENTSIZE);
				if (error != NOERROR)
				{
					printf("ERROR: 'AddToMessageOrInvCyclesList' returned error in 'FindProperCyclesforDeadlock'\n");
					return(ERROR);
				}
				*Found = TRUE;
			}

			/* if none of them are in, then it's ok */
			else if ((!AtleastOneInInvMessageList) && (AtleastOneNotInInvMessageList))
			{
			}

			/* if some are in and others are not in, then we have a problem */
			else if ((AtleastOneInInvMessageList) && (AtleastOneNotInInvMessageList))
			{
				printf("ERROR: Unexpected condition (unexpected Cycle) in 'FindProperCyclesforDeadlock'\n");
				return(ERROR);
			}

			/* if there are no messages in this cycle, then we have a problem also */
			else /* ((!AtleastOneInInvMessageList) && (!AtleastOneNotInInvMessageList)) */
			{
				printf("ERROR: Unexpected condition (Empty Message List) in 'FindProperCyclesforDeadlock'\n");
				return(ERROR);
			}
		}
	}
	
	return(NOERROR);	
}


/*
*	DetectDeadlocks
*
*	Searches through the Circuit List to see if there are any circuits which are marked
*	as having all of its ReqVCs in a Cycle ('AllReqVCsInACycle' flag set).  For each one of
*	these circuits, it does the following:
*
*		- Creates a list of Messages which own it's ReqVCs (recursively)
*		- ...
*
*	Returns ERROR if:
*		- ... 
*
*	Returns NOERROR otherwise.
*/


int	DetectDeadlocks()
{
	int		i, j;
	REQVCSPTR	FanoutNode;
	MSGSNODEPTR	MessageList;
	int		MessageListSize=0;
	MSGSNODEPTR	DepMessageList;
	int		DepMessageListSize=0;
	int		PotentialDeadlock;
	int		SetIndex;
	int		IsDeadlock;
	int		error;
	int		NewIndex;
	MSGSNODEPTR	InvolvedCycles;
	int		InvolvedCyclesListSize;
	int		FoundProperCycles;
	int		FoundMatchingDeadlock;
	int		MatchingDeadlockIndex;
	MSGSNODEPTR	InACycleList;

	for (i = 0; i < MAXCIRC; i++)
	{
		/* Only consider those message whose 'AllReqVCsInACycle' flag is set */
		if (CircList[i].AllReqVCsInACycle)
		{
			/* It's either at the begining (if No VCs) or at the end */
			/* No need to further check data structure integrity at this point */
			if  (CircList[i].FirstVCNode != NULL_)
				FanoutNode = CircList[i].LastVCNode->ReqVCNode;
			else
				FanoutNode = CircList[i].FirstReqVCNode;

			/* if there is No ReqVCNode, and this message is marked 'AllReqVCsInACycle' */
			/* then we have a problem - */ 
			if (FanoutNode == NULL_)
			{
				printf("ERROR: Unexpected condition in 'ClearInACycleFlags'\n");
				return(ERROR);
			}


			/* Otherwise, ...*/

			/* Create a MessageList (allocate memory for message list) */
			MessageListSize = INITIALMSGLISTSIZE;

			MessageList = (MSGSNODEPTR) malloc(sizeof(MSGSNODE) * MessageListSize);

			/* If memory allocation fails, then return error */
			if (MessageList == NULL_)
			{
				printf("ERROR: returned by 'malloc'(MessageList) in 'DetectDeadlocks'\n");
				return(ERROR);
			}

			/* Initialize the Message List */
			InitMessageOrInvCycleList(MessageList, MessageListSize, UNDEFINED_CIRCID);

			/* Add Itself to the MessageList */
			error = AddToMessageOrInvCyclesList(&MessageList, &MessageListSize, i, UNDEFINED_CIRCID, MSGLISTINCREMENTSIZE);

			/* if there's an error, then return error */
			if (error != NOERROR)
			{
				printf("ERROR: 'AddToMessageOrInvCyclesList' returned an error in 'DetectDeadlocks'\n");
				return(ERROR);
			}

			/* Initialize  */
			PotentialDeadlock = FALSE;
			SetIndex = 1;
			j = 0;

			/* Check to see if all of the ReqVCNodes are in a cycle also ...*/
			/* if so, there is potentially a deadlock 			*/
			/* if not, go to the next message 				*/
			while ((j < MAXFANOUT) && (FanoutNode->ReqVCNodeList[j] != NULL_))
			{
				/* If the owner of (each) ReqVC Node also has its 'AllReqVCsInACycle' flag set */
				if (CircList[FanoutNode->ReqVCNodeList[j]->CircID].AllReqVCsInACycle)
				{
					/* If we are here, there is still potential for deadlock for this message */
					PotentialDeadlock = TRUE;

					/* Add it to the Message List */
					error = AddToMessageOrInvCyclesList(&MessageList, &MessageListSize, FanoutNode->ReqVCNodeList[j]->CircID, 
										UNDEFINED_CIRCID, MSGLISTINCREMENTSIZE);
					/* if there's an error, then return error */
					if (error != NOERROR)
					{
						printf("ERROR: 'AddToMessageOrInvCyclesList' returned an error in 'DetectDeadlocks'\n");
						return(ERROR);
					}
				}

				/* Otherwise, we know that this message is not in a Deadlock */
				else
				{
					/* So, set a flag --- so that we can deallocate memory, and clean up */
					PotentialDeadlock = FALSE;

					/* get out of while loop to go to the next message */
					break;
				}

				j++;
			}; /* while loop */

			if (!PotentialDeadlock)
			{
				/* If this message is not in a potential deadlock, free memoryfor message list */
				free(MessageList);
				MessageListSize = 0;
			}
			else /* (PotentialDeadlock) */
			{
				/* Otherwise (if this message is in a potential deadlock)	*/
				/* Check the other messages in the message list to see if they	*/
				/* meet the same criteria as well				*/
				error = ValidateDeadlockSetMembers(SetIndex, &MessageList, &MessageListSize, &IsDeadlock);

				if (error != NOERROR)
				{
					printf("ERROR: 'ValidateDeadlockSetMembers' returned error in 'DetectDeadlocks'\n");
					return(ERROR);
				}

				/* If 'IsDeadlock' is set, then we have a deadlock set */
				if (IsDeadlock)
				{
					/* Sort the Message List */
					SortMessageOrInvCyclesList(MessageList, MessageListSize, UNDEFINED_CIRCID);

					/* allocate memory for an 'InACycle List' */
					InACycleList = (MSGSNODEPTR) malloc(sizeof(MSGSNODE) * MessageListSize);

					/* check if memory allocation failed or not */
					if (InACycleList == NULL_)
					{
						printf("ERROR: returned by 'malloc'(InACycleList) in 'DetectDeadlocks'\n");
						return(ERROR);
					}

					/* Initialize the 'InACycle List' */
					InitMessageOrInvCycleList(InACycleList, MessageListSize, NOTINACYCLE);

					/* allocate memory for an 'Involved Cycles List' */
					InvolvedCyclesListSize = INITIALINVCYCLISTSIZE;

					InvolvedCycles = (MSGSNODEPTR) malloc(sizeof(MSGSNODE) * InvolvedCyclesListSize);

					/* check if memory allocation failed or not */
					if (InvolvedCycles == NULL_)
					{
						printf("ERROR: returned by 'malloc'(InvolvedCycles) in 'DetectDeadlocks'\n");
						return(ERROR);
					}

					/* Initialize the Involved Cycles List */
					InitMessageOrInvCycleList(InvolvedCycles, InvolvedCyclesListSize, UNDEFINED_CYCLEID);

					/* Find cycles involved in this deadlock */
					error = FindProperCyclesforDeadlock(MessageList, MessageListSize, InACycleList, &InvolvedCycles, 
										&InvolvedCyclesListSize, &FoundProperCycles);

					/* check error */
					if (error != NOERROR)
					{
						printf("ERROR: 'FindProperCyclesforDeadlock' returned error in 'DetectDeadlocks'\n");
						return(ERROR);
					}

					/* check if we found proper cycles - if not report error also */
					if (!FoundProperCycles)
					{
						printf("ERROR: Cannot find a proper set of Cycles for Deadlock in 'DetectDeadlocks'\n");
						return(ERROR);
					}

					/* Sort the involved cycles list */
					SortMessageOrInvCyclesList(InvolvedCycles, InvolvedCyclesListSize, UNDEFINED_CYCLEID);

					/* Create a Dependent Message List (allocate memory for dependent message list) */
					DepMessageListSize = INITIALMSGLISTSIZE;
					DepMessageList = (MSGSNODEPTR) malloc(sizeof(MSGSNODE) * DepMessageListSize);

					/* If memory allocation fails, then return error */
					if (DepMessageList == NULL_)
					{
						printf("ERROR: returned by 'malloc'(DepMessageList) in 'DetectDeadlocks'\n");
						return(ERROR);
					}

					/* Initialize the Dependent Message List */
					InitMessageOrInvCycleList(DepMessageList, DepMessageListSize, UNDEFINED_CIRCID);

					/* Remove Dependent Messages from Involved Messages List and Place in DepMessagesList */
					SeparateDependentMessages(MessageList, MessageListSize, InACycleList, &DepMessageList, &DepMessageListSize);

					if (error != NOERROR)
					{
						printf("ERROR: returned by 'SeparateDependentMessages' in 'DetectDeadlocks'\n");
						return(ERROR);
					}

					/* deallocate the memory for the InACycleList - since we no longer need it (after separation of dep. messages */
					free(InACycleList);

					/* Find a matching Deadlock --- if one exists */
					FoundMatchingDeadlock = FindMatchingDeadlock(MessageList, MessageListSize, InvolvedCycles, InvolvedCyclesListSize,
											&MatchingDeadlockIndex);

					if (FoundMatchingDeadlock)
					{
						/* update the "TimeLastDetected" field */
						error = UpdateDeadlockTimeLastDetected(MatchingDeadlockIndex, sim_time, 
											DepMessageList, DepMessageListSize);
						
						if (error != NOERROR)
						{
							printf("ERROR: returned by 'UpdateDeadlockTimeLastDetected' in 'DetectDeadlocks'\n");
							return(ERROR);
						}


						/* Count and reset the number of Dependent Messages in this deadlock */	
						error = CountAndSetNumberofDependentMessagesAndVCs(MatchingDeadlockIndex);

						if (error != NOERROR)
						{
							printf("ERROR: 'CountAndSetNumberofDeadlockMessages' (reset) returned an error in 'DetectDeadlocks'\n");
							return(ERROR);
						}

						/* Free the lists - which are no longer needed */
						free(MessageList);
						MessageListSize = 0;
						free(InvolvedCycles);
						InvolvedCyclesListSize = 0;
						free(DepMessageList);
						DepMessageListSize = 0;

					}
					else
					{
						/* Note: we should only add if there isn't a duplicate copy already */
						/* Add to Deadlock List (note - Pointer to cyclesLIst should replace NULL_) */
						error = AddDeadlockToList(sim_time, &NewIndex, MessageList, InvolvedCycles, DepMessageList, 
										MessageListSize, InvolvedCyclesListSize, DepMessageListSize);

						if (error != NOERROR)
						{
							printf("ERROR: returned by 'AddDeadlockToList' in 'DetectDeadlocks'\n");
							return(ERROR);
						}

						/* Count and set the number of Messages in this deadlock */	
						error = CountAndSetNumberofDeadlockMessagesAndVCs(NewIndex);

						if (error != NOERROR)
						{
							printf("ERROR: 'CountAndSetNumberofDeadlockMessagesAndVCs' returned an error in 'DetectDeadlocks'\n");
							return(ERROR);
						}

						/* Count and set the number of Cycles in this deadlock */	
						error = CountAndSetNumberofDeadlockCycles(NewIndex);

						if (error != NOERROR)
						{
							printf("ERROR: 'CountAndSetNumberofDeadlockCycles' returned an error in 'DetectDeadlocks'\n");
							return(ERROR);
						}

						/* Count and set the number of Dependent Messages in this deadlock */	
						error = CountAndSetNumberofDependentMessagesAndVCs(NewIndex);

						if (error != NOERROR)
						{
							printf("ERROR: 'CountAndSetNumberofDependentMessagesAndVCs' returned an error in 'DetectDeadlocks'\n");
							return(ERROR);
						}

						/* Print out the messages involved and the status of the data structures --- */
						/* ONLY ONCE for each (NEW) deadlock detected*/
						if (DDTEST == PRINT_WHEN_DEADLOCK_FOUND)
						{
							printf("\n\n*** NEW DEADLOCK DETECTED ***\n\n");
							PrintMessageOrInvCyclesList(MessageList, MessageListSize, UNDEFINED_CIRCID);
							PrintAdjacencyList();
							PrintCycleList();
							PrintDeadlockList();
						}

					}
	
				} /* (IsDeadlock) */

				else /* (!IsDeadlock) */
				{
					/* If this message is not in a deadlock, free memory for message list */
					free(MessageList);
					MessageListSize = 0;
				}
			} /* (PotentialDeadlock) */
	
		} /* if 'AllReqVCsInACycle' for this circuit */

	} /* for loop */

	return(NOERROR);
}



/*
*	ValidateDeadlockSetMembers
*
*	
*
*	Returns ERROR if:
*		- In the process, we find a Circuit w/ mismatching First and Last VC Node pointers
*
*	Returns NOERROR otherwise.
*/


int	ValidateDeadlockSetMembers(SetIndex, MessageListPtr, MessageListSizePtr, IsDeadlock)
int		SetIndex;
MSGSNODEPTRPTR	MessageListPtr;
int		*MessageListSizePtr;
int		*IsDeadlock;
{
	int		j;
	REQVCSPTR	FanoutNode;
	int		PotentialDeadlock;
	int		error;
	MSGSNODEPTR	MessageList;

	/* Set the value of Message List */
	MessageList = *MessageListPtr;

	/* Check has gone beyond the size of the list, */
	/* then we have a problem  */
	if (SetIndex > *MessageListSizePtr)
	{
		printf("ERROR: Unexpected condition (1) in 'ValidateDeadlockSetMembers'\n");
		return(ERROR);
	}

	/* If we've reached the end of the messagelist .. */
	if ((SetIndex == *MessageListSizePtr) || (MessageList[SetIndex] == UNDEFINED_CIRCID))
	{
		/* Then we've properly validated the message list, so return TRUE/NOERROR */
		*IsDeadlock = TRUE;
		return(NOERROR);
	}

	/* It's either at the begining (if No VCs) or at the end */
	/* No need to further check data structure integrity at this point */
	if  (CircList[MessageList[SetIndex]].FirstVCNode != NULL_)
		FanoutNode = CircList[MessageList[SetIndex]].LastVCNode->ReqVCNode;
	else
		FanoutNode = CircList[MessageList[SetIndex]].FirstReqVCNode;

	/* if there is No ReqVCNode, and this message is marked 'AllReqVCsInACycle' */
	/* then we have a problem - */ 
	if (FanoutNode == NULL_)
	{
		printf("ERROR: Unexpected condition (2) in 'ValidateDeadlockSetMembers'\n");
		return(ERROR);
	}

	/* Initialize  */
	PotentialDeadlock = FALSE;
	j = 0;

	/* Check to see if all of the ReqVCNodes are in a cycle also ...*/
	/* if so, there is potentially a deadlock 			*/
	/* if there is atleast one exception, then not in deadlock 	*/
	while ((j < MAXFANOUT) && (FanoutNode->ReqVCNodeList[j] != NULL_))
	{
		/* If the owner of (each) ReqVC Node also has its 'AllReqVCsInACycle' flag set */
		if (CircList[FanoutNode->ReqVCNodeList[j]->CircID].AllReqVCsInACycle)
		{
			/* If we are here, there is still potential for deadlock for this message */
			PotentialDeadlock = TRUE;

			/* Add it to the Message List */
			error = AddToMessageOrInvCyclesList(MessageListPtr, MessageListSizePtr, FanoutNode->ReqVCNodeList[j]->CircID, UNDEFINED_CIRCID, MSGLISTINCREMENTSIZE);
			/* if there's an error, then return error */
			if (error != NOERROR)
			{
				printf("ERROR: 'AddToMessageOrInvCyclesList' returned an error in 'ValidateDeadlockSetMembers'\n");
				return(ERROR);
			}
			
			/* Reset Message List - just in case  it has changed */
			MessageList = *MessageListPtr;

		}

		/* Otherwise, know that this message is not in a Deadlock */
		else
		{
			/* So, set a flag --- so that we can deallocate memory, and clean up */
			PotentialDeadlock = FALSE;

			/* get out of while loop to go to the next message */
			break;
		}

		j++;
	}

	if (!PotentialDeadlock)
	{
		*IsDeadlock = FALSE;
		return(NOERROR);
	}
	else /* (PotentialDeadlock) */
	{
		/* If there is still potential for deadlock, keep checking recursively */ 
		error = ValidateDeadlockSetMembers(SetIndex+1, MessageListPtr, MessageListSizePtr, IsDeadlock);

		if (error != NOERROR)
		{
			printf("ERROR: 'ValidateDeadlockSetMembers' returned error in 'ValidateDeadlockSetMembers'\n");
		}

		return(error);
	}
}
