#ifndef _SLICING_H
#define _SLICING_H

#include "prv_types.h"
#include "array_list.h"

#define	MAX_FUNCNAME_LENGTH	128

typedef struct SlicingPropSet{
    ArrayList	*propSet;
}SlicingPropSet;

typedef struct SlicingState{
	InstrList *iList;
	int currect_instr;
	OpStack *stack;
	InstrList *lastFrame;
	//ArrayList *lastFrameForward;
	WriteSet *memLive;
	SlicingPropSet *propsLive;
}SlicingState;


Property *createProperty(ADDRESS obj,long id, char *name);
void propPrint(void *item);
int propCompare(void *o1, void *o2);
SlicingPropSet *createPropSet(void);
void destroyPropSet(SlicingPropSet *set);
SlicingState *initSlicingState(InstrList *iList, int order, int direction);
void destroySlicingState(SlicingState *state, int direction);
void printSlicingState(SlicingState *state);
void markUDchain(InstrList *iList, SlicingState *state, uint32_t flag);
void checkSlice(InstrList *iList);

void deobfSlicing(InstrList *iList);
ArrayList *findFuncStartInstrsInSlice(InstrList *iList);

void testUD(InstrList *iList, SlicingState *state);
void forwardUDchain(InstrList *iList, int order);

void backwardSlicing(InstrList *iList, int order, ArrayList *blocksList, uint32_t flag);
void forwardSlicing(InstrList *iList, int order, ArrayList *blocksList, uint32_t flag);
void envBranchSlicing(InstrList *iList, ArrayList *blocksList, uint32_t flag);

#endif
