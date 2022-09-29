/* 
*	Module: Deadlock Detection Header: 'dd.h'
*/

/*
*	Constants (General Purpose)
*/
 

#define		NULL_		0		


#define		NOERROR		0
#define		ERROR		1

#define		UNVISITED	0
#define		VISITED		1

#define		UNUSED		0
#define		USED		1

#define		FALSE		0
#define		TRUE		1

#define		NOTINACYCLE	0
#define		INACYCLE	1

#define		UNDEFINED_CIRCID  -999
#define		UNDEFINED_CYCLEID -999
#define		UNDEFINED_LIST_INDEX -1

#define		PRINT_AT_END			5
#define		PRINT_WHEN_ERROR		4
#define		PRINT_AT_CHECKPOINT		3
#define		PRINT_WHEN_DEADLOCK_FOUND	2
#define		PRINT_WHEN_CYCLE_FOUND		1

#define		INITIALMSGLISTSIZE	16
#define		MSGLISTINCREMENTSIZE	16

#define		INITIALINVCYCLISTSIZE	32
#define		INVCYCLISTINCREMENTSIZE	32

#define		INITIALHASHTABLELISTSIZE	32
#define		HASHTABLELISTINCREMENTSIZE	32

/*
*	Constants 
*
*	Note: 'MAXCIRC' is defined in 'dat2.h' (as 8192)
*/

#define		MAXVC		8192 		/*  use 8192 for now - potentially a variable in the future ... */

#define		MAXFANOUT	16 		/*  potentially a variable in the future ... */

/*
*----------------------------------------------------------------------------------
*/

#define		MAXCYCLES	65536 		/*  Maximum number of active cycles allowed (expected) for a simulation run */


/*
*----------------------------------------------------------------------------------
*/

#define		MAXDEADLOCKS	1024 		/*  Maximum number of deadlocks allowed (expected) for a simulation run */

/*
*----------------------------------------------------------------------------------
*/

#define		HASHTABLESIZE	1023 		/*  Size of hash table used for quick lookup of cycles (using Cycle key) */


/*
*
*	Type definitions for Deadlock Detection
*
*/

/*
*	These structures are used for maintaining adjancency list representation of resource dependency graph,
*	and are used for detecting resource dependency cycles. 
*/

typedef struct REQVCS 
{
	struct  VCNODE  *ReqVCNodeList[MAXFANOUT];	/*  Points to the desired VCs (if any) */
	int		 VisitedList[MAXFANOUT];	/*  Indicates if this branch is being visited (in depth first search) */
} REQVCS, *REQVCSPTR;

typedef struct VCNODE 
{
	int	VCID;			/*  Identifies the Virtual Channel */
	int	Visited;		/*  Identifies whether the VC has been visited (during graph traversal) */
	int	Used;			/*  Identifies whether VC is currently being used */
	int	CircID;			/*  Identifies the Circuit which is currently using (owns) the VC */
	int	InACycle;		/*  Used when detecting groups of cycles */
	struct  REQVCS  *ReqVCNode;	/*  Pointers to zero or more desired VCs */
	struct  VCNODE  *NextVCNode;	/*  Points to the next VC being used (if any) */

} VCNODE, *VCNODEPTR;


typedef struct CIRCNODE 
{
	int	CircID;			/*  Identifies the Circuit ID */
	int	AllReqVCsInACycle;	/*  Identifies if all desired VCs are in a cycle - for deadlock detection */
	struct  REQVCS  *FirstReqVCNode;/*  Points to the first desired VCs (if any) */
	struct  VCNODE  *FirstVCNode;	/*  Points to the first allocated VC (if any) */
	struct  VCNODE  *LastVCNode;	/*  Points to the last allocated VC (if any) */
} CIRCNODE, *CIRCNODEPTR;


/*
*----------------------------------------------------------------------------------
*/

/*
*	These structures are used for maintaining information about cycles detected, and
*	are used for detecting groups of cycles involved in a deadlock.
*/

typedef int	MSGSNODE, *MSGSNODEPTR, **MSGSNODEPTRPTR;

typedef struct CYCLEVCNODE 
{
	int 	CircuitID;			/* ID of the Circuit in the cycle */
	int	BranchIndex;			/* The "branch" of the ReqVCNode which (w/ circuit id) helps form a unique cycle */
	struct  CYCLEVCNODE  *NextCycleVCNode;	/* Points to the next VC in the cycle */
} CYCLEVCNODE, *CYCLEVCNODEPTR;

typedef struct CYCLENODE 
{
	int	Active;				/*  Identifies whether the cycle is still active (messages are still in transit) */
	int	FirstDetected;			/*  Identifies the simulation time at which this cycle was first detected */
	int	LastDetected;			/*  Identifies the simulation time at which this cycle was last detected */
	int	NumofMessages;			/*  Identifies the number of messages involved in this cycle */
	int	NumofVCs;			/*  Identifies the total number of VCs involved in this cycle */
	int	CycleKey;			/*  Identified the Key for this Cycle (for optimized matching & hash function) */
#ifdef HASH
	int	HashTableListIndex;		/*  Identifies the index in the list (in a hash table slot) where this cycle is stored */
#endif
	int	MsgListSize;			/*  Maintains the current size of the Message List */
	MSGSNODEPTR	Messages;		/*  Points to a list of messages (circuits) involved in this cycle  */
	struct  CYCLEVCNODE  *FirstCycleVCNode;	/*  Points to ths first VC of the linked list of VCs which make up the cycle  */
	struct  CYCLEVCNODE  *LastCycleVCNode;	/*  Points to ths last VC of the linked list of VCs which make up the cycle  */
} CYCLENODE, *CYCLENODEPTR;


/*
*----------------------------------------------------------------------------------
*/

/*
*	These structures used for maintaining information about deadlocks detected 
*
*/

typedef struct DEADLOCKNODE 
{
	int	Active;				/*  Identifies hether the deadlock is still active */
	int	FirstDetected;			/*  Identifies the simulation time at which this cycle was first detected */
	int	LastDetected;			/*  Identifies the simulation time at which this cycle was last detected */
	int	NumofMessages;			/*  Identifies the number of messages involved in this deadlock */
	int	NumofVCs;			/*  Identifies the total number of VCs involved in this deadlock */
	int	NumofCycles;			/*  Identifies the number of cycles involved in this deadlock */
	int	NumofDepMessages;		/*  Identifies the number of messages dependent on this deadlock */
	int	NumofDepVCs;			/*  Identifies the total number of VCs involved in dependent messages */
	int	MsgListSize;			/*  Maintains the current size of the Message List */
	int	DepMsgListSize;			/*  Maintains the current size of the Dependent Messages List */
	int	InvCyclesListSize;		/*  Maintains the current size of the Involved Cycles List */
	MSGSNODEPTR	Messages;		/*  Points to a list of messages (circuits) involved in this deadlock  */
	MSGSNODEPTR 	Cycles;			/*  Points to a list of cycles (circuits) involved in this deadlock  */
	MSGSNODEPTR	DepMessages;		/*  Points to a list of dependent messages (circuits) involved in this deadlock  */
} DEADLOCKNODE, *DEADLOCKNODEPTR;


/*
*----------------------------------------------------------------------------------
*/

/*
*	These structures are used for maintaining the hash table for quick 
*	lookup of cycles (using cycle key).
*
*/

#ifdef HASH
typedef struct	HASHBUCKET 
{
	int	AllocatedSize;			/*  Amount of memory allocated to the list associated with this bucket */
	int	LastItemIndex;			/*  The last index at which an item exists in the list associated with this bucket */
	MSGSNODEPTR	ItemList;		/*  Points to a list of items hashed to this bucket */
} HASHBUCKET, *HASHBUCKETPTR;
#endif


/*
*----------------------------------------------------------------------------------
*/


/* 
*
*	  I N I T I A L I Z A T I O N   R O U T I N E S 
*
*/

void	InitCircList();

void	InitVCList();

void	InitCycleList();

void	InitDeadlockList();


/* 
*
*	  P R I N T   ( D E B U G G I N G )   R O U T I N E S 
*
*/

void	PrintAdjacencyList();

void	PrintCycleList();

void	PrintVisitedNodes();

void	PrintDetectedCycle(int StartingVC, VCNODEPTR VCNode);

void	PrintVisitVC(int StartingVC, VCNODEPTR VCNode);

void	PrintVisitReqVCs(int StartingVC, REQVCSPTR ReqVCNode);

void	PrintMessageOrInvCyclesList(MSGSNODEPTR MessageList, int MessageListSize, int EndOfListSymbol);

void	PrintDeadlockList();

#ifdef HASH
void	PrintHashTable();
#endif

/* 
*
*	  S T A T I S T I C S   P R I N T I N G   R O U T I N E S 
*
*/

void	PrintCircuitStats();

void	PrintCycleStats();

void	PrintDeadlockStats();


/* 
*
*	  R E S O U R C E   D E P E N D E N C Y   G R A P H   M A N A G E M E N T   R O U T I N E S 
*
*/

int	AddVCToCircuit(int Circuit, int VC);

int	AddReqVCToCircuit(int Circuit, int ReqVC);

int	RemoveVCFromCircuit(int Circuit, int VC);

int	RemoveAllReqVCsFromCircuit(int Circuit);

int	RemoveAllReqVCsFromVCNode(VCNODEPTR VCNode);

int	CountNumberofCircuitVCs(int Circuit);


/* 
*
*	  C Y C L E   D E T E C T I O N   R O U T I N E S 
*
*/

int	DetectCycles();

int	VisitVC(VCNODEPTR VCNode, int StartingMessage);

int	VisitReqVCs(REQVCSPTR ReqVCNode, int StartingMessage);


/* 
*
*	  C Y C L E   L I S T   M A N A G E M E N T   R O U T I N E S 
*
*/

int	AddCycleToList(int TimeFirstDetected, int *CycleIndex, MSGSNODEPTR MessagesList, int MessageListSize, int CycleKey);

int	UpdateCycleTimeLastDetected(int CycleIndex, int TimeLastDetected);

#ifdef HASH
int	UpdateCycleHashTableListIndex(int CycleIndex, int HashTableListIndex);
#endif

int	AddCycleVCNodeToCycle(int CycleIndex, int CycleVCID, int CycleVCOwnerID);

int	IncrementOwnedVCs(int CycleIndex, int MsgID);

int	CountAndSetNumberofMessages(int CycleIndex);

int	CountAndSetNumberofVCs(int CycleIndex);

int	RetireCycle(int CycleIndex);

int	RetireEligibleCycles();


/* 
*
*	  C Y C L E   M A T C H I N G   R O U T I N E S 
*
*/

int	FindCycleWithMatchingCycleKey(int StartIndex, int CycleKey, int *MatchingCycleIndex);

int	CompareCycles(int CycleIndex, int StartingCircuit);

int	CompareCyclesVisitVC(int CycleIndex, CYCLEVCNODEPTR CurrCycleVCNode, int StartingCircuit, int StartingBranchIndex, int CurrentCircuit);


/*
*	
*	M E S S A G E   L I S T   M A N A G E M E N T 
*
*/

void	InitMessageOrInvCycleList(MSGSNODEPTR MessageList, int MessageListSize, int InitValue);

int	AddToMessageOrInvCyclesList(MSGSNODEPTRPTR MessageListPtr, int *MessageListSizePtr, int MsgID, int EndOfListSymbo, int IncrementSize);

int	RemoveFromMessageList(MSGSNODEPTR MessageList, int MessageListSize, MSGSNODEPTR InACycleList, int MsgIndex);

void	SortMessageOrInvCyclesList(MSGSNODEPTR MessageList, int MessageListSize, int EndOfListSymbol);

int	CompareMessageOrInvCyclesLists(MSGSNODEPTR MessageList1, int MessageList1Size, MSGSNODEPTR MessageList2, int MessageList2Size, int EndOfListSymbol);

int	CombineMessageLists(MSGSNODEPTRPTR MessageList1Ptr, int *MessageList1SizePtr, MSGSNODEPTR MessageList2, int MessageList2Size);

int	FindAndMarkIfInMessageList(MSGSNODEPTR MessageList, int MessageListSize, MSGSNODEPTR InACycleList, int MessageID, int Mark);

int	CountMessagesInList(MSGSNODEPTR MessageList, int MessageListSize);

int	CollectMsgsAndTagInACycle(MSGSNODEPTRPTR MessageListPtr, int *MessageListSizePtr, int StartingVC, VCNODEPTR VCNode, int *NumOfVCs);

int	CollectMsgsAndTagInACycleVisitVC(MSGSNODEPTRPTR MessageListPtr, int *MessageListSizePtr, int StartingVC, VCNODEPTR VCNode, int LastCircIDAdded, int *NumOfVCs);

int	GenerateCycleKeyAndCountMessages(MSGSNODEPTR MessageList, int MessageListSize, int *CycleKey, int *NumberOfMessages);


/*
*	
*	C Y C L E   H A S H   T A B L E   M A N A G E M E N T  
*
*/

#ifdef HASH
void	InitHashTable();

int	Hash(int CycleKey);

int	AddToHashTable(int CycleKey, int CycleIndex, int *HashTableListIndex);

int	RemoveFromHashTable(int CycleKey, int CycleIndex, int HashTableListIndex);

void	GetHashTableList(int CycleKey, MSGSNODEPTR *HashTableList, int *HashTableListSize);
#endif


/* 
*
*	  C Y C L E   B U I L D I N G   R O U T I N E S 
*
*/

int	CreateDetectedCycle(int CycleIndex, int StartingCircuit);

int	CreateDetectedCycleVisitVC(int CycleIndex, int StartingCircuit, int StartingBranchIndex, int CurrentCircuit);


/* 
*
*	  D E A D L O C K   L I S T   M A N A G E M E N T   R O U T I N E S 
*
*/

int	AddDeadlockToList(int TimeFirstDetected, int *DeadlockIndex, MSGSNODEPTR InvolvedMessagesList, MSGSNODEPTR InvolvedCyclesList, MSGSNODEPTR DependentMessagesList, int MsgListSize, int InvolvedCyclesListSize, int DepMsgListSize);

int	UpdateDeadlockTimeLastDetected(int DeadlockIndex, int TimeLastDetected, MSGSNODEPTR NewDependentMessagesList, int NewDependentMessagesListSize);

int	CountAndSetNumberofDeadlockMessagesAndVCs(int DeadlockIndex);

int	CountAndSetNumberofDependentMessagesAndVCs(int DeadlockIndex);

int	CountAndSetNumberofDeadlockCycles(int DeadlockIndex);

int	RetireDeadlock(int DeadlockIndex);

int	RetireEligibleDeadlocks();

int	IsMessageInADeadlock(int MessageID, int *MatchingDeadlockIndex);

/* 
*
*	  D E P E N D E N T   M E S S A G E S   L I S T  M A N A G E M E N T  R O U T I N E S 
*
*/

int	SeparateDependentMessages(MSGSNODEPTR MessageList, int MessageListSize, MSGSNODEPTR InACycleList, MSGSNODEPTRPTR DepMessageListPtr, int *DepMessageListSizePtr);



/* 
*
*	  D E A D L O C K   D E T E C T I O N   R O U T I N E S 
*
*/

void	ClearInACycleFlags();

int	MarkCircuitsWithAllReqVCsInACycle();

int	FindMatchingDeadlock(MSGSNODEPTR InvolvedMessages, int InvolvedMessagesListSize, MSGSNODEPTR InvolvedCycles, int InvolvedCyclesListSize, int *index);

int	FindProperCyclesforDeadlock(MSGSNODEPTR InvolvedMessages, int InvolvedMessagesListSize, MSGSNODEPTR InACycleList, MSGSNODEPTRPTR InvolvedCycles, int *InvolvedCyclesListSizePtr, int *Found);

int	DetectDeadlocks();

int	ValidateDeadlockSetMembers(int SetIndex, MSGSNODEPTRPTR MessageListPtr, int *MessageListSizePtr, int *IsDeadlock);
