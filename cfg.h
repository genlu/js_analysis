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
