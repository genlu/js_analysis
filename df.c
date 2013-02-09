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
#include "stack_sim.h"

//a flag used to ensure augmented elemented get cleaned up before proceed to
//slicing, etc.
static int augmented = 0;

//id for augmented blocks/edges starts from -1 and decreases
//static int augBlockIdCtr;	//not used cuz blocklist is sorted array list...
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


/*
 * Free augmented exit block
 */
void destroyAugmentExit(void *block) {
	BasicBlock *bbl;
	if(!block)
		return;
	bbl = (BasicBlock *)block;
	if(bbl->instrs)
		al_free(bbl->instrs);
	if(bbl->preds)
		al_free(bbl->preds);
	if(bbl->succs)
		al_free(bbl->succs);
	if(bbl->dominators)
		al_free(bbl->dominators);
	if(bbl->dominate)
		al_free(bbl->dominate);
	if(bbl->immDomPreds)
		al_free(bbl->immDomPreds);
	if(bbl->immDomSuccs)
		al_free(bbl->immDomSuccs);
	if(bbl->reverseDominators)
		al_free(bbl->reverseDominators);
	if(bbl->reverseDominate)
		al_free(bbl->reverseDominate);
	if(bbl->reverseImmDomPreds)
		al_free(bbl->reverseImmDomPreds);
	if(bbl->reverseImmDomSuccs)
		al_free(bbl->reverseImmDomSuccs);

	bbl->instrs = bbl->preds = bbl->succs =
			bbl->dominators = bbl->dominate = bbl->immDomPreds = bbl->immDomSuccs =
			bbl->reverseDominators = bbl->reverseDominate = bbl->reverseImmDomPreds = bbl->reverseImmDomSuccs = NULL;
	free(bbl);
	bbl=NULL;
	return;
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

	ArrayList *augmentedBlocks = al_newGeneric(AL_LIST_SET, blockInFunctionCompare, augExitPrint, destroyAugmentExit);
	assert(augmentedBlocks);

	int i,j;
	BasicBlock *block1, *block2;
	BlockEdge *augEdge = NULL;

	//reset id, set flag
	//augBlockIdCtr = -1;
	augEdgeIdCtr = -1;
	augmented = 1;

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
				//block2->id = augBlockIdCtr--;
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

			printf("\nadding %d->%d\n", augEdge->tail->id, augEdge->head->id);

			if(!al_contains(block2->preds, augEdge)){
				al_add(block2->preds, augEdge);
			}else{
				//block2 and block1 are already connected, cant happen
				printBasicBlock(block1);
				printBasicBlock(block2);
				assert(0);
			}
			al_add(block1->succs, augEdge);
			assert(al_size(block1->succs)==1);	//each BT_RET can only have 1 successor (this augmented exit block)
		}//end if(block->type==BT_RET)
	}//end for

	return augmentedBlocks;
}


/*
 * this function is used to clean up all the augmented blocks/edges from CFGs, after reverse domination analysis is done.
 * it doesn't nothing if no augmented elements found in the CFGs.
 * This function has to be called before loop-analysis/slicing/decompilation.
 */
void removeAugmentedExitBlocks(ArrayList *blockList, ArrayList *augmentedBlocks){
	assert(blockList && augmentedBlocks);
	if(augmented==0 || al_size(augmentedBlocks)==0)
		return;

	int i,j;
	BasicBlock *block1, *block2;
	BlockEdge *edge1;

	printf("\naugmented blocks:\n");
	printBasicBlockList(augmentedBlocks);

	//first have to remove augmented exit block from all dominator/dominate lists in each normal basicBlock
	//since the augmented exit block is either a leaf (in forward CFG) or root (in reverse CFG), remove it from dom tree
	//	won't break the domination relationship of other blocks (might break tree into forest in reverse dom tree though)
	for(i=0;i<al_size(blockList);i++){
		block1 = (BasicBlock *)al_get(blockList, i);
		if(block1->type==BT_AUG_EXIT)
			continue;

		printf("processing block %d\n", block1->id);
		//check each of following lists for BT_AUG_EXIT block,
		//	if found,remove that block (and destroy adjacent edge in dom tree)

		//also, only one aug exit block in each CFG, so a BT_AUG_EXIT can only appear at most once in each of following lists
		if(block1->dominators){
			for(j=0;j<al_size(block1->dominators);j++){
				block2 = (BasicBlock *)al_get(block1->dominators, j);
				if(block2->type==BT_AUG_EXIT){
					al_remove(block1->dominators, (void *)block2);
					break;
				}
			}
		}
		if(block1->dominate){
			for(j=0;j<al_size(block1->dominate);j++){
				edge1 = (BlockEdge *)al_get(block1->dominate, j);
				block2 = edge1->head;
				if(block2->type==BT_AUG_EXIT){	//cant use al_contains since the compare function only compare inFunction field
					al_remove(block1->dominate, (void *)edge1);
					destroyBlockEdge(edge1);
					break;
				}
			}
		}
		if(block1->immDomPreds){
			for(j=0;j<al_size(block1->immDomPreds);j++){
				edge1 = (BlockEdge *)al_get(block1->immDomPreds, j);
				block2 = edge1->tail;
				if(block2->type==BT_AUG_EXIT)
					assert(0);
			}
		}
		if(block1->immDomSuccs){
			for(j=0;j<al_size(block1->immDomSuccs);j++){
				edge1 = (BlockEdge *)al_get(block1->immDomSuccs, j);
				block2 = edge1->head;
				if(block2->type==BT_AUG_EXIT){
					al_remove(block1->immDomSuccs, (void *)edge1);
					al_remove(block2->immDomPreds, (void *)edge1);
					destroyBlockEdge(edge1);
					break;
				}
			}
		}
		if(block1->reverseDominators){
			for(j=0;j<al_size(block1->reverseDominators);j++){
				block2 = (BasicBlock *)al_get(block1->reverseDominators, j);
				if(block2->type==BT_AUG_EXIT){
					al_remove(block1->reverseDominators, (void *)block2);
					break;
				}
			}
		}
		if(block1->reverseDominate){
			for(j=0;j<al_size(block1->reverseDominate);j++){
				edge1 = (BlockEdge *)al_get(block1->reverseDominate, j);
				block2 = edge1->head;
				if(block2->type==BT_AUG_EXIT){	//cant use al_contains since the compare function only compare inFunction field
					al_remove(block1->reverseDominate, (void *)edge1);
					destroyBlockEdge(edge1);
					break;
				}
			}
		}
		if(block1->reverseImmDomPreds){
			for(j=0;j<al_size(block1->reverseImmDomPreds);j++){
				edge1 = (BlockEdge *)al_get(block1->reverseImmDomPreds, j);
				block2 = edge1->tail;
				if(block2->type==BT_AUG_EXIT){
					al_remove(block1->reverseImmDomPreds, (void *)edge1);
					al_remove(block2->reverseImmDomSuccs, (void *)edge1);
					destroyBlockEdge(edge1);
					break;
				}
			}
		}
		if(block1->reverseImmDomSuccs){
			for(j=0;j<al_size(block1->reverseImmDomSuccs);j++){
				edge1 = (BlockEdge *)al_get(block1->reverseImmDomSuccs, j);
				block2 = edge1->head;
				if(block2->type==BT_AUG_EXIT)
					assert(0);
			}
		}
	}


	for(i=0;i<al_size(augmentedBlocks);i++){
		block1 = (BasicBlock *)al_get(augmentedBlocks, i);
		assert(block1->type==BT_AUG_EXIT);
		assert(al_size(block1->succs)==0);

		//get each inward edge and remove itself from each predecessing BT_RET block
		for(j=0;j<al_size(block1->preds);j++){
			edge1 = (BlockEdge *)al_get(block1->preds, j);
			assert(EDGE_HAS_FLAG(edge1, EDGE_IS_TO_AUG_EXIT)&&edge1->head==block1);
			block2 = edge1->tail;
			assert(block2->type==BT_RET);
			assert(al_size(block2->succs)==1);	//each BT_RET block can only have one successor
			al_remove(block2->succs, (void *)edge1);
			assert(al_size(block2->succs)==0);
		}
		//free all augmented edges point to block1, but not the list itself (which will be done when block1 is destroyed)
		al_clearAndFreeElements(block1->preds);
		//remove block1 from block list, but keep it in augmentedBlocks for now
		al_remove(blockList, (void *)block1);
		assert(!al_contains(blockList, (void *)block1));
	}
	al_freeWithElements(augmentedBlocks);
	augmented=0;
	return;
}



/*
 * return true if block1 dominate block2 in reversed CFG
 */
bool reverseDominate(BasicBlock *block1, BasicBlock *block2){
	assert(block1&&block2);
	assert(block1->reverseDominate);
	int i;
	BlockEdge *edge;
	for(i=0;i<al_size(block1->reverseDominate);i++){
		edge = al_get(block1->reverseDominate, i);
		if(edge->head->id == block2->id)
			return true;
	}
	return false;
}

void printRevDominators(ArrayList *blockList){
	int i,j;
	BasicBlock *b1, *b2;
	printf("\nReverse Dominators for each block\n");
	for(i=0;i<al_size(blockList);i++){
		b1 = (BasicBlock *)al_get(blockList, i);
		printf(" Reverse Dominators of block %3d:\t", b1->id);
		for(j=0;j<al_size(b1->reverseDominators);j++){
			b2 = (BasicBlock *)al_get(b1->reverseDominators, j);
			printf("%d\t", b2->id);
		}
		printf("\n");
	}
}

void printRevDominanceFrontiers(ArrayList *blockList){
	int i,j;
	BasicBlock *b1, *b2;
	printf("\nReverse Dominance Frontier for each block\n");
	for(i=0;i<al_size(blockList);i++){
		b1 = (BasicBlock *)al_get(blockList, i);
		printf(" Reverse DF of block %3d:\t", b1->id);
		for(j=0;j<al_size(b1->reverseDomFrontier);j++){
			b2 = (BasicBlock *)al_get(b1->reverseDomFrontier, j);
			printf("%d\t", b2->id);
		}
		printf("\n");
	}
}

void printRevDomList(ArrayList *blockList){
	int i,j;
	BasicBlock *b1;
	BlockEdge *e;
	printf("\nRevDomListfor each block\n");
	for(i=0;i<al_size(blockList);i++){
		b1 = (BasicBlock *)al_get(blockList, i);
		printf(" blocks reverse dominated by block %3d:\t", b1->id);
		for(j=0;j<al_size(b1->reverseDominate);j++){
			e = (BlockEdge *)al_get(b1->reverseDominate, j);
			assert(EDGE_HAS_FLAG(e, EDGE_IS_REV_DOMINATE));
			printf("\t%d\t", e->head->id);
		}
		printf("\n");
	}
}

void printRevImmDomList(ArrayList *blockList){
	int i,j;
	BasicBlock *b1;
	BlockEdge *e;
	printf("\nReverse Imm Dom List for each block\n");
	for(i=0;i<al_size(blockList);i++){
		b1 = (BasicBlock *)al_get(blockList, i);
		printf(" blocks reserve immediately dominated by block %3d:\t", b1->id);
		for(j=0;j<al_size(b1->reverseImmDomSuccs);j++){
			e = (BlockEdge *)al_get(b1->reverseImmDomSuccs, j);
			printf("\t%d\t", e->head->id);
		}
		printf("\n");
	}
}

/*
 * auxiliary function used by findReverseDominators()
 */
ArrayList *findReverseDominatorsHelper(BasicBlock *block){
	//printf("findReverseDominatorsHelper: id %d\n", block->id);

	assert(block->type != BT_AUG_EXIT);

	int i;
	ArrayList *revDominators, *revDominatorsTemp;
	BasicBlock *predNode;	//in reversed CFG, predeccsor node is the successor of current node in CFG

	/*
	 * D(n) = {n} 'union' ('intersection' D(p) | for each p 'in' preds(n))
	 */
	predNode = (BasicBlock *)((BlockEdge *)al_get(block->succs, 0))->head;
	revDominators = revDominatorsTemp = al_clone(predNode->reverseDominators);
	assert(revDominators && revDominatorsTemp);

	for(i=1;i<al_size(block->succs);i++){
		predNode = (BasicBlock *)((BlockEdge *)al_get(block->succs, i))->head;
		revDominators = al_intersection(revDominatorsTemp, predNode->reverseDominators);
		al_free(revDominatorsTemp);
		revDominatorsTemp = NULL;
		revDominatorsTemp = revDominators;
	}

	assert(revDominators);
	al_add(revDominators, (void *)block);

	return revDominators;
}



//todo: modify following functions for reversed analysis

/*
 * find all the dominators for each basicBlock in the reversed CFG
 * this function would create following lists for each BasicBlock:
 *  ArrayList *reverseDominators;
    ArrayList *reverseDominate;
    ArrayList *reverseImmDomPreds;
    ArrayList *reverseImmDomSuccs;
 */
void findReverseDominators(ArrayList *blockList){
	assert(blockList);
	BasicBlock *block, *block2, *block3;
	ArrayList *domTemp;
	BlockEdge *domEdge;
	int change;
	int i,j,k;

	/* Initialize D(n) for each node n.
	 * D(n) = {n0}, if n==n0
	 * D(n) = N (all nodes), otherwise
	 * Here entry nodes represented by all augmented exit nodes
	 */
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(block->reverseDominators){
			al_free(block->reverseDominators);
		}
		printf("block#%d\t", block->id);
		//block 0 is garanteed to be the entry node
		if(block->type == BT_AUG_EXIT){
			printf("entry\n");
			block->reverseDominators = al_newGeneric(AL_LIST_SET, blockIdCompare, NULL, NULL);
			al_add(block->reverseDominators, (void *)block);
		}
		else{
			printf("reg\n");
			block->reverseDominators = al_clone(blockList);
			block->reverseDominators->al_type = AL_LIST_SET;
			block->reverseDominators->al_print = NULL;
			block->reverseDominators->al_free = NULL;
		}
	}

	//printRevDominators(blockList);

	do{
		change = 0;
		//for each node != n0
		for(i=0;i<al_size(blockList);i++){
			block = (BasicBlock *)al_get(blockList,i);
			if(block->type == BT_AUG_EXIT)
				continue;
			domTemp = findReverseDominatorsHelper(block);
			assert(domTemp);

			if(al_setEquals(block->reverseDominators, domTemp)){
				al_free(domTemp);
				domTemp = NULL;
			}else{
				al_free(block->reverseDominators);
				block->reverseDominators = NULL;
				block->reverseDominators = domTemp;
				change++;
			}

		}//end for-loop
	}while(change);

	printRevDominators(blockList);


	/* CAUTION - IMPORTANT:
	 * 	following 2 loops MUST starts from the end of arraylist and decrement i to 1st element
	 *  otherwise reverseDominate and reverseImmDomSuccs fields used would be NULL
	 */

	//calculate block->reverseDominate
	printf("\ncomputing reverse dominate lists\n");
	for(i=al_size(blockList)-1;i>=0;i--){
		block = (BasicBlock *)al_get(blockList,i);
		printf("processing block %d\n", block->id);
		/*
		 if(block->reverseDominate){
			al_freeWithElements(block->reverseDominate);
		}
		block->reverseDominate = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
		 */
		if(!block->reverseDominate){
			block->reverseDominate = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
		}

		assert(block->reverseDominate);
		for(j=0;j<al_size(block->reverseDominators);j++){
			//block2 reverse dominates block
			block2 =  (BasicBlock *)al_get(block->reverseDominators,j);
			printf("%d\n", block2->id);

			if(!block2->reverseDominate){
				block2->reverseDominate = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
			}

			domEdge = createBlockEdge();
			domEdge->tail = block2;
			domEdge->head = block;
			EDGE_SET_FLAG(domEdge, EDGE_IS_REV_DOMINATE);

			if(!al_contains(block2->reverseDominate, domEdge)){
				al_add(block2->reverseDominate, domEdge);
			}else{
				destroyBlockEdge(domEdge);
			}
		}
	}

	printRevDomList(blockList);

	//construct rev dom tree
	for(i=al_size(blockList)-1;i>=0;i--){
		block = (BasicBlock *)al_get(blockList,i);
		/*
		if(block->reverseImmDomPreds){
			//al_freeWithElements(block->immDomPreds);
			al_free(block->reverseImmDomPreds);
			block->reverseImmDomPreds=NULL;
		}
		if(block->reverseImmDomSuccs){
			al_freeWithElements(block->reverseImmDomSuccs);
			block->reverseImmDomSuccs=NULL;
		}

		block->reverseImmDomPreds = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
		block->reverseImmDomSuccs = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
		*/

		if(!block->reverseImmDomPreds){
			block->reverseImmDomPreds = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
		}
		if(!block->reverseImmDomSuccs){
			block->reverseImmDomSuccs = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
		}

		for(j=0;j<al_size(block->reverseDominators);j++){
			//block2 reverse dominates block
			block2 =  (BasicBlock *)al_get(block->reverseDominators,j);
			if(block2->id == block->id){
				continue;
			}
			for(k=0;k<al_size(block->reverseDominators);k++){
				block3 = (BasicBlock *)al_get(block->reverseDominators,k);
				if(block3->id == block->id){
					continue;
				}
				if(!reverseDominate(block3, block2)){
					block2 = NULL;		//then block2 cannot be the imm dominator of block, set to NULL as a flag
					break;
				}
			}
			if(!block2)
				continue;

			if(!block2->reverseImmDomSuccs){
				block2->reverseImmDomSuccs = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
			}
			//printf("block%d is imm-reverse dominator of block%d\n", block2->id, block->id);
			//then block is reveres imm dominator of block
			domEdge = createBlockEdge();
			domEdge->tail = block2;
			domEdge->head = block;
			EDGE_SET_FLAG(domEdge, EDGE_IS_REV_IMM_DOM);
			al_add(block->reverseImmDomPreds, domEdge);
			al_add(block2->reverseImmDomSuccs, domEdge);
		}
	}

	printRevImmDomList(blockList);
}



void printBasicBlockId(void *bbl){
	BasicBlock *block = (BasicBlock *)bbl;
	printf("%d", block->id);
	return;
}
/*
 * TODO: put the psudocode of algorithm here for future referenece
 */
/*
 * input: root block of a reveres dom tree
 * output: dominance frontier for each block in given reverse dom tree
 * 			(i.e. filling reverseDomFrontier field for each block in given tree)
 */
void computeReverseDominanceFrontiersHelper(BasicBlock *root){
	//use a post order travesal on the reverse dom tree
	int i;
	BasicBlock *child, *Y, *Z;
	BlockEdge *edge;
	//process children first
	for(i=0;i<al_size(root->reverseImmDomSuccs);i++){
		edge = (BlockEdge *)al_get(root->reverseImmDomSuccs, i);
		assert(EDGE_HAS_FLAG(edge, EDGE_IS_REV_IMM_DOM));
		child = edge->head;
		computeReverseDominanceFrontiersHelper(child);
	}
	//process root
	//1. DF[root] = empty
	if(root->reverseDomFrontier){
		al_free(root->reverseDomFrontier);
	}
	root->reverseDomFrontier = al_newGeneric(AL_LIST_SET, blockIdCompare, NULL, NULL);
	//2. FOREACH Y in succ(root) in reverse CFG DO:      (succ(root) in reversed CFG == preds(root) in normal CFG)
	//		IF idom(Y) != root THEN DF[root] = DF[root] union {Y} ENDIF
	//	 ENDFOR
	for(i=0;i<al_size(root->preds);i++){
		edge = (BlockEdge *)al_get(root->preds, i);
		Y = edge->tail;
		assert(al_size(Y->reverseImmDomPreds)==1);
		BlockEdge *rev_idom_Y = (BlockEdge *)al_get(Y->reverseImmDomPreds, 0);
		assert(EDGE_HAS_FLAG(rev_idom_Y, EDGE_IS_REV_IMM_DOM));
		if(rev_idom_Y->tail->id != root->id){
			al_add(root->reverseDomFrontier, Y);
		}
	}
	//3. FOREACH Z in children(root) in dom tree DO:
	//		FOREACH Y in DF[Z] DO:
	//			IF idom(Y) != root THEN DF[root] = DF[root] union {Y} ENDIF
	//	 	ENDFOR
	//	 ENDFOR

	/*
	 * ONLY children of root!!!!!!!
	 * not all the decendents of root!!!!!
	 * todo: clean up the code, remove stack operations, etc.	02/08/2013
	 */

	OpStack *stack;		//use a stack for DFS on reversed dom tree
	stack = initOpStack(printBasicBlockId);
	assert(stack);
	//first push all children of root in 'rev dom tree' to stack
	for(i=0;i<al_size(root->reverseImmDomSuccs);i++){
		edge = (BlockEdge *)al_get(root->reverseImmDomSuccs, i);
		assert(EDGE_HAS_FLAG(edge, EDGE_IS_REV_IMM_DOM));
		pushOpStack(stack, (void *)(edge->head));
	}
	assert(al_size(root->reverseImmDomSuccs)==countOpStack(stack));
	//then proccess each node in stack
	while(!isOpStackEmpty(stack)){
		Z = (BasicBlock *)popOpStack(stack);
		/*
		 * NOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
		//push Z's children
		for(i=0;i<al_size(Z->reverseImmDomSuccs);i++){
			edge = (BlockEdge *)al_get(Z->reverseImmDomSuccs, i);
			assert(EDGE_HAS_FLAG(edge, EDGE_IS_REV_IMM_DOM));
			pushOpStack(stack, (void *)(edge->head));
		}
		*/
		//processing Z
		for(i=0;i<al_size(Z->reverseDomFrontier);i++){
			Y = (BasicBlock *)al_get(Z->reverseDomFrontier, i);
			assert(al_size(Y->reverseImmDomPreds)==1);
			BlockEdge *rev_idom_Y = (BlockEdge *)al_get(Y->reverseImmDomPreds, 0);
			assert(EDGE_HAS_FLAG(rev_idom_Y, EDGE_IS_REV_IMM_DOM));
			if(rev_idom_Y->tail->id != root->id){
				al_add(root->reverseDomFrontier, Y);
			}
		}
	}
}

/*
 * this function compute dominance frontier for each block in reversed CFGs.
 */
void computeReverseDominanceFrontiers(ArrayList *blockList){
	assert(augmented!=0);
	int i;
	BasicBlock *block;
	for(i=0;i<al_size(blockList);i++){
		block = (BasicBlock *)al_get(blockList, i);
		if(block->type==BT_AUG_EXIT){	//block is the root of a reverse dom tree
			printf("processing (BT_AUG_EXIT) block %d\n", block->id);
			assert(al_size(block->reverseImmDomPreds)==0);
			computeReverseDominanceFrontiersHelper(block);
		}
	}
}

