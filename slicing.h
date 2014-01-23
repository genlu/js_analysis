/***********************************************************************************
Copyright 2014 Arizona Board of Regents; all rights reserved.

This software is being provided by the copyright holders under the
following license.  By obtaining, using and/or copying this software, you
agree that you have read, understood, and will comply with the following
terms and conditions:

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee or royalty is hereby granted,
provided that the full text of this notice appears on all copies of the
software and documentation or portions thereof, including modifications,
that you make.

This software is provided "as is," and copyright holders make no
representations or warranties, expressed or implied.  By way of example, but
not limitation, copyright holders make no representations or warranties of
merchantability or fitness for any particular purpose or that the use of the
software or documentation will not infringe any third party patents,
copyrights, trademarks or other rights.  Copyright holders will bear no
liability for any use of this software or documentation.

The name and trademarks of copyright holders may not be used in advertising
or publicity pertaining to the software without specific, written prior
permission.  Title to copyright in this software and any associated
documentation will at all times remain with copyright holders.
***********************************************************************************/

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
