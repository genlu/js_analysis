/*
 * df.c
 *
 *  computation of dominance frontier and related funtions
 *
 *  Created on: Jan 31, 2013
 *      Author: genlu
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "prv_types.h"
#include "cfg.h"
#include "df.h"

//id for augmented blocks/edges starts from -1 and decreases
static int augBlockIdCtr;
static int augEdgeIdCtr;

/*
 *  augExitPrint(item) -- print memory addresses.
 */
void augExitPrint(void *item) {
    BasicBlock *augExit =  (BasicBlock *) item;
    assert(augExit->type == BT_AUG_EXIT);
    printf("augExit block for function: %lx\tblock id: %3d\naugmented edges:\n", augExit->inFunction, augExit->id);
    printEdges(augExit);
}

//this function should be called AFTER function blocks are cut out from main CFG
//because we are rely on the inFunction field of block for linking return blocks to augmented block for each function
//   after the call, blockList will include all the BT_AUG_EXIT nodes (1 for each function)
//   and corresponding edges are inserted to all BT_RET nodes as well.
ArrayList *addAugmentedExitBlocks(ArrayList *blockList){
	assert(blockList);
	if(!functionCFGProcessed){
		printf("ERROR: please call buildFunctionCFGs() before call addAugmentedExitBlocks()!\n");
		abort();
	}

	ArrayList *augmentedBlocks = al_newGeneric(AL_LIST_SET, blockInFunctionCompare, augExitPrint, NULL);
	assert(augmentedBlocks);

	int i,j;
	BasicBlock *block1, *block2;
	BlockEdge *augEdge = NULL;

	//reset id
	augBlockIdCtr = -1;
	augEdgeIdCtr = -1;

	//search for BT_RET blocks in the block list.
	for(i=0;i<al_size(blockList);i++){
		block1 = (BasicBlock *)al_get(blockList, i);
		//if block is BT_RET blocks, then we need to
		//	a. connect it with existing augExit block (located from blockListaugmentedBlocks) if it exists
		//  b. create a new augExit block for this function and link them
		//	   (also add newly created augExit into blockListaugmentedBlocks and blockList)
		if(block1->type==BT_RET){
			//create a new BasicBlock for augmened exit block
			block2 = createBasicBlock();
			assert(block2);
			block2->type = BT_AUG_EXIT;
			block2->inFunction = block1->inFunction;
			if(block1->inFunction){
				assert(BBL_HAS_FLAG(block1, BBL_IS_FUNC_RET));
			}else{
				assert(BBL_HAS_FLAG(block1, BBL_IS_HALT_NODE));
			}

			//check if the augExit node is already created by another BT_RET node in the same function before
			int augIndex = al_indexOf(augmentedBlocks, (void *)block2);
			//found, free block2 and reset it to point to the existing node
			if(augIndex>=0){
				destroyBasicBlock((void *)block2);
				block2 = (BasicBlock *)al_get(augmentedBlocks, augIndex);
				assert(al_contains(blockList, (void *)block2));
				assert(block2->inFunction==block1->inFunction);
			}
			//not found, have to add block2 to augmentedBlocks and blockList
			else{
				block2->id = augBlockIdCtr--;
				//note that blockList and augmentedBlocks use different function to find given block
				//blockList use block ID, so we cant ue al_contain to check whether to find block2 in it.
				//since it uses a newly generated unique ID.
				for(j=0;j<al_size(blockList);j++){
					BasicBlock *tempBlock = (BasicBlock *)al_get(blockList,j);
					if(tempBlock->type==BT_AUG_EXIT && tempBlock->inFunction==block2->inFunction)
						assert(0);
				}
				assert(!al_contains(augmentedBlocks, (void *)block2));
				al_add(augmentedBlocks, (void *)block2);
				al_add(blockList, (void *)block2);
			}
			//connect BT_RET and BT_AUG_EXIT
			augEdge = createBlockEdge();
			augEdge->tail = block1;
			augEdge->head = block2;
			augEdge->id = augEdgeIdCtr--;
			EDGE_SET_FLAG(augEdge, EDGE_IS_TO_AUG_EXIT);
			if(!al_contains(block2->preds, augEdge)){
				al_add(block2->preds, augEdge);
			}else{
				//block2 and block1 are already connected, cant happen
				assert(0);
			}
			al_add(block1->succs, augEdge);
			assert(al_size(block1->succs)==1);	//each BT_RET can only have 1 successor (this augmented exit block)
		}//end if(block->type==BT_RET)
	}//end for

	return augmentedBlocks;
}

void removeAugmentedExitBlocks(ArrayList *blockList, ArrayList *augmentedBlocks){

}
