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
/*
 * cfg.h
 *
 *  Created on: May 3, 2011
 *      Author: genlu
 */

#ifndef _CFG_H
#define _CFG_H


#include <stdint.h>
#include "prv_types.h"
#include "instr.h"
#include "array_list.h"

void printNaturalLoopList(ArrayList *loopList);
void printBasicBlockList(ArrayList *blockList);

int blockIdCompare(void *i1, void *i2);
int blockInFunctionCompare(void *i1, void *i2);
int edgeBlockIdCompare(void *i1, void *i2);

ArrayList*	findLeaders(InstrList *iList);
void 		markBasicBlockBoundary(InstrList *iList, ArrayList *leaders);
BasicBlock*	createBasicBlock(void);
void destroyBasicBlock(void *block);
void printBasicBlock(BasicBlock *block);
void printEdges(BasicBlock *block);

BlockEdge *createBlockEdge(void);
void destroyBlockEdge(void *edge);

BasicBlock *findBasicBlockFromListByID(ArrayList *blockList, int id);
BasicBlock *findBasicBlockFromListByAddr(ArrayList *blockList, ADDRESS addr);

ArrayList*	buildDynamicCFG(InstrList *iList);
void 		destroyBasicBlockList(ArrayList *blockList);

void 		findDominators(ArrayList *blockList);

void 		buildDFSTree(ArrayList *blockList);

int 		findBackEdges(ArrayList *blockList);

ArrayList* 	buildNaturalLoopList(ArrayList *blockList);
void 		destroyNaturalLoopList(ArrayList *loopList);

void printNaturalLoop(NaturalLoop *loop);

BasicBlock *isBlockInTheLoop(NaturalLoop *loop,int block);
BasicBlock *isBlockTheLoopHeader(NaturalLoop *loop, int block);

NaturalLoop *findSmallestUnprocessedLoop(ArrayList *loopList, uint32_t flag);


void processANDandORExps(InstrList *iList, ArrayList *blockList, ArrayList *funcCFGs);

#endif /* _CFG_H */
