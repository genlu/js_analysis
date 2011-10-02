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
	WriteSet *memLive;
	SlicingPropSet *propsLive;
}SlicingState;


Property *createProperty(ADDRESS obj,long id);
void propPrint(void *item);
int propCompare(void *o1, void *o2);
SlicingPropSet *createPropSet(void);
void destroyPropSet(SlicingPropSet *set);
SlicingState *initSlicingState(InstrList *iList, int order);
void destroySlicingState(SlicingState *state);
void printSlicingState(SlicingState *state);
void markUDchain(InstrList *iList, SlicingState *state, uint32_t flag);
void checkSlice(InstrList *iList);

void deobfSlicing(InstrList *iList);
ArrayList *findFuncStartInstrsInSlice(InstrList *iList);

void testUD(InstrList *iList, SlicingState *state);

#endif
