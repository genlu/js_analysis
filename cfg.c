/*
 * cfg.c
 *
 *  Created on: May 3, 2011
 *      Author: genlu
 */

#include <stdlib.h>
#include <assert.h>
#include "al_helper.h"
#include "cfg.h"

//set to 0 in buildBasicBlockList()
static int blockIdCtr;
static int edgeIdCtr;
//initialized in buildDFSTree()
static int DFSTreeNodeCount;
//set to 0 in buildNaturalLoopList()
static int loopIdCtr;

/*
 * compare 2 ADDRESS, return 0 is equal
 */
int addressCompare(void *a1, void *a2){
	return (ADDRESS) a1 - (ADDRESS) a2;
}

/*
 * compare 2 BasicBlock by id, return 0 is equal
 */
int blockIdCompare(void *i1, void *i2){
	assert(i1 && i2);
	return ((BasicBlock *)i1)->id - ((BasicBlock *)i2)->id;
}

/*
 * compare 2 BasicBlock by the address of their first instruction
 * return 0 if equal
 */
int blockAddrCompare(void *i1, void *i2){
	assert(i1 && i2);
	return ((BasicBlock *)i1)->addr - ((BasicBlock *)i2)->addr;
}

/*
 * compare 2 BlockEdge by id
 */
int edgeIdCompare(void *i1, void *i2){
	assert(i1 && i2);
	return ((BlockEdge *)i1)->id - ((BlockEdge *)i2)->id;
}

/*
 * compare 2 blockEdge by their head and tail block
 * return 0 if they have the same end nodes (and direction)
 */
int edgeBlockIdCompare(void *i1, void *i2){
	assert(i1 && i2);
	if(((BlockEdge *)i1)->tail->id==((BlockEdge *)i2)->tail->id){
		return ((BlockEdge *)i1)->head->id - ((BlockEdge *)i2)->head->id;
	}else
		return ((BlockEdge *)i1)->tail->id - ((BlockEdge *)i2)->tail->id;
}

/*
 * InstrIsEqual
 *  takes two instructions, returns true if they are the same, false ow
 *  "same" defined as same address and same name.
 */
bool instrIsEqual(Instruction *instr1, Instruction *instr2) {
	bool isEqual = false;
	if(instr1->addr == instr2->addr) {
		if(strcmp(instr1->opName, instr2->opName) == 0) {
			isEqual = true;
		}
	}

	return (isEqual);
}

/*
 * print info of Edges in/out given BasicBlock
 */
void printEdges(BasicBlock *block){
	BlockEdge *edge = NULL;
	int i;
	printf("inbound Edges of block #%d:\n", block->id);
	for(i=0;i<al_size(block->preds);i++){
		edge = al_get(block->preds, i);
		printf("%c%c%c%c%c %d -> %d  : %d\n",
				EDGE_HAS_FLAG(edge, EDGE_IN_DFST)?'*':'-',
				EDGE_HAS_FLAG(edge, EDGE_IS_RETREAT)?'R':'-',
				EDGE_HAS_FLAG(edge, EDGE_IS_BACK_EDGE)?'B':'-',
				EDGE_HAS_FLAG(edge, EDGE_IS_BRANCHED_PATH)?'J':'-',
				EDGE_HAS_FLAG(edge, EDGE_IS_ADJACENT_PATH)?'A':'-',
				edge->tail->id, edge->head->id, edge->count);
	}
	printf("outbound Edges of block #%d:\n", block->id);
	for(i=0;i<al_size(block->succs);i++){
		edge = al_get(block->succs, i);
		printf("%c%c%c%c%c %d -> %d  : %d\n",
				EDGE_HAS_FLAG(edge, EDGE_IN_DFST)?'*':'-',
				EDGE_HAS_FLAG(edge, EDGE_IS_RETREAT)?'R':'-',
				EDGE_HAS_FLAG(edge, EDGE_IS_BACK_EDGE)?'B':'-',
				EDGE_HAS_FLAG(edge, EDGE_IS_BRANCHED_PATH)?'J':'-',
				EDGE_HAS_FLAG(edge, EDGE_IS_ADJACENT_PATH)?'A':'-',

				edge->tail->id, edge->head->id, edge->count);
	}
}

/*
 * print detailed info of given BasicBlock
 */
void printBasicBlock(BasicBlock *block) {
  int i, size;

  printf("=================================================================\n");
  printf("BBL: %3d [ ADDR %lX ]\tdfn = %3d\tCount = %3d ]\n", block->id, block->addr, block->dfn, block->count);
  printf("%s\t%s\n", BBL_HAS_FLAG(block, BBL_IS_FUNC_ENTRY)?"FUNC_ENTRY":"-", BBL_HAS_FLAG(block, BBL_IS_FUNC_RET)?"FUNC_RET":"-");
  printf("TYPE: ");
  switch(block->type){
  	  case BT_FALL_THROUGH:
		  printf("FALL_THROUGH");
		  break;
	  case BT_1_BRANCH:
		  printf("1_WAY_BRANCH");
		  break;
	  case BT_2_BRANCH:
		  printf("2_WAY_BRANCH");
		  break;
	  case BT_N_BRANCH:
		  printf("N_WAY_BRANCH");
		  break;
	  case BT_SCRIPT_INVOKE:
		  printf("SCRIPT_INVOKE");
		  break;
	  case BT_RET:
		  if(BBL_HAS_FLAG(block, BBL_IS_HALT_NODE)){
			  assert(al_size(block->succs)==0);
			  printf("HALT");
		  }
		  else
			  printf("RETURN");
		  break;
	  default:
		  printf("ERROR: unknown block type");
  }
  printf("\n");
  printf("Last Instruction:\t%lx  %s\n", block->lastInstr->addr, block->lastInstr->opName);


  /********************* PREDECESSORS ********************/
  printf("PREDS:  ");
  size = al_size(block->preds);
  for (i = 0; i < size; i++) {
	  BasicBlock *next = (BasicBlock *)((BlockEdge *)al_get(block->preds, i))->tail;
      printf("%d; ", next->id);
  }
  printf("\n");

  /********************* SUCCESSORS *********************/

  printf("SUCCS:  ");

  size = al_size(block->succs);
  for (i = 0; i < size; i++) {
	  BasicBlock *next = (BasicBlock *)((BlockEdge *)al_get(block->succs, i))->head;
      printf("%d; ", next->id);
  }
  printf("\n");
  printEdges(block);
}

/*
 * print detail info of given BasicBlock list
 */
void printBasicBlockList(ArrayList *blockList) {
  int i, sz = al_size(blockList);
  BasicBlock *block;

  printf("\nBasic Block List:\n");
  for (i = 0; i < sz; i++) {
	  block = (BasicBlock *)al_get(blockList, i);
    printBasicBlock(block);
  }
  printf("=================================================================\n");
}

/*
 * find all the leaders (instruction) in the InstrList given,
 * leaders returned as an ArrayList(set) of Instruction*
 */
ArrayList *findLeaders(InstrList *iList){
	Instruction *temp_ins;
	ArrayList *leaders = al_newGeneric(AL_LIST_SET, addressCompare, refPrint, NULL);
	int i=0;

	//add first instr into leader list
	Instruction *instr = getInstruction(iList, i);
	al_add(leaders, (void *)(instr->addr));

	for(i=0;i<iList->numInstrs;i++){
		Instruction *instr = getInstruction(iList, i);

		//ignore non-jump instructions and native invocations
		if(!isBranch(iList, instr) &&
				!isRetInstruction(iList, instr) &&
				!(isInvokeInstruction(iList, instr) && INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE))
		)
			continue;

		/* add:
		 * 		instr as target of jump
		 * 		instr immediately after jump
		 */
		if(isBranch(iList, instr)){
			if(!instr->jmpOffset){
				printInstruction(instr);
			}
			assert(instr->jmpOffset);
			al_add(leaders, (void *)(instr->addr + instr->jmpOffset));
			assert(instr->length>0);
			al_add(leaders, (void *)(instr->addr + instr->length));
		}
		else if(isRetInstruction(iList, instr)){
			if(i < iList->numInstrs-1){
				temp_ins = getInstruction(iList, i+1);
				al_add(leaders, (void *)(temp_ins->addr));
			}
		}
		else{	//non-native invoke and document.write() which generate code

			if(i < iList->numInstrs-1){
				assert(isInvokeInstruction(iList, instr) && INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE));
				temp_ins = getInstruction(iList, i+1);
				al_add(leaders, (void *)(temp_ins->addr));
			}
		}
	}
	return leaders;
}

/*
 * create an edge
 */
BlockEdge *createBlockEdge(void){
	BlockEdge *newEdge = (BlockEdge *)malloc(sizeof(BlockEdge));
	assert(newEdge);
	newEdge->id = edgeIdCtr++;
	newEdge->flags = 0;
	newEdge->count = 0;
	newEdge->tail = newEdge->head = NULL;
	return newEdge;
}

/*
 * destroy an edge, but won't touch the BasicBlocks pointed by head and tail
 */
void destroyBlockEdge(void *edge){
	if(edge!=NULL)
		free((BlockEdge *)edge);
	edge = NULL;
}

/*
 * mark basicBlock start and end, based on the given list of leaders.
 */
void markBasicBlockBoundary(InstrList *iList, ArrayList *leaders){
	Instruction *instr;
	int i;
	for(i=0;i<iList->numInstrs;i++){
		instr = getInstruction(iList, i);
		if(al_contains(leaders, (void *)(instr->addr))){
			INSTR_SET_FLAG(instr, INSTR_IS_BBL_START);
			if(i>0){
				instr = getInstruction(iList, i-1);
				INSTR_SET_FLAG(instr, INSTR_IS_BBL_END);
			}
		}
	}
	instr = getInstruction(iList, iList->numInstrs-1);
	INSTR_SET_FLAG(instr, INSTR_IS_BBL_END);
	//printInstruction(instr);
}


/*
 * create a basicBlock
 * dominator list is set to NULL at this moment
 */
BasicBlock *createBasicBlock(void){
	BasicBlock *new = (BasicBlock *)malloc(sizeof(BasicBlock));
	assert(new);
	if(!new)
		return NULL;
	new->id = blockIdCtr++;
	new->addr = new->end_addr = new->flags = new->count = 0;

	new->calltarget = NULL;
	new->instrs = al_newGeneric(AL_LIST_SET, insCompareByAddr, refPrint, dealloc);
/*	new->preds = al_newGeneric(AL_LIST_SET, blockIdCompare, NULL, NULL);
	new->succs = al_newGeneric(AL_LIST_SET, blockIdCompare, NULL, NULL);*/
	new->preds = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
	new->succs = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
	new->dfn = -1;			//depth-first number
	new->dominators = NULL;//al_newGeneric(AL_LIST_SET, blockIdCompare, NULL, NULL);
	new->dominate = NULL;
	new->immDomPreds = NULL;
	new->immDomSuccs = NULL;
	new->type = BT_UNKNOWN;
	new->flags = 0;
	new->inFunction = 0;
	assert(new->preds && new->succs);
	return new;
}

/*
 * DestroyBbl(bbl) -- deallocate the block bbl as well as its components.
 * CAUTION: all the blocks and edges will be freed after this call!
 */
void destroyBasicBlock(void *block) {
	BasicBlock *bbl;

	if(!block)
		return;

	bbl = (BasicBlock *)block;

	if(bbl->instrs)
		al_free(bbl->instrs);

	if(bbl->preds)
		al_free(bbl->preds);

	if(bbl->succs)
		al_freeWithElements(bbl->succs);

	bbl->instrs = bbl->preds = bbl->succs = NULL;

	if(bbl->dominators)
		al_free(bbl->dominators);

	if(bbl->dominate)
		al_freeWithElements(bbl->dominate);

	if(bbl->immDomPreds){
		al_freeWithElements(bbl->immDomPreds);
		bbl->immDomPreds=NULL;
	}

	if(bbl->immDomSuccs){
		al_freeWithElements(bbl->immDomSuccs);
		bbl->immDomSuccs=NULL;
	}

	bbl->id = -1;

	free(bbl);
	bbl=NULL;

	return;
}


/*
 * if flag==1, then don't free pred, if flag==2, then don't free succs,
 * else free both
 */
void destroyBasicBlockForConCat(BasicBlock *block, int flag) {
	BasicBlock *bbl;

	if(!block)
		return;

	bbl = block;

	if(bbl->instrs)
		al_free(bbl->instrs);

	if(bbl->preds && flag!=1)
		al_free(bbl->preds);

	if(bbl->succs && flag!=2)
		al_freeWithElements(bbl->succs);

	bbl->instrs = bbl->preds = bbl->succs = NULL;

	if(bbl->dominators)
		al_free(bbl->dominators);

	if(bbl->dominate)
		al_freeWithElements(bbl->dominate);

	bbl->id = -1;

	free(bbl);
	bbl=NULL;

	return;
}

/*
 * Blocks will be considered same, if they have the same instrs (decided by its address)
 * in the same order
 */
BasicBlock *getBlockFromInstrs(ArrayList *bbls, ArrayList *instrs) {
	BasicBlock *returnBbl = NULL;
	int i, bblSize = al_size(bbls);
	int iSize = al_size(instrs);

	for(i = 0; i < bblSize; i++) {
		bool sameBbl = true;
		BasicBlock *nextBbl = (BasicBlock *)al_get(bbls, i);
		int j, bblInstrSize = al_size(nextBbl->instrs);

		if(iSize != bblInstrSize) {
			//instr lists cannot be equal
			continue;
		}

		for(j = 0; j < bblInstrSize; j++) {
			Instruction *nextBblInstr = (Instruction *)al_get(nextBbl->instrs, j);
			Instruction *nextInstr = (Instruction *)al_get(instrs, j);
			if(!instrIsEqual(nextBblInstr, nextInstr)) {
				sameBbl = false;
				break;
			}
		}

		if(sameBbl) {
			returnBbl = nextBbl;
			break;
		}
	}

	return (returnBbl);
}


BasicBlock *findBasicBlockFromListByAddr(ArrayList *blockList, ADDRESS addr){
	assert(blockList);
	int i;
	BasicBlock *block;
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(block->addr == addr)
			return block;
	}
	return NULL;
}


BasicBlock *findBasicBlockFromListByID(ArrayList *blockList, int id){
	assert(blockList);
	int i;
	BasicBlock *block;
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(block->id == id)
			return block;
	}
	return NULL;
}

/*
 * main routine for create blocks and CFG
 * most of work is done here, requires
 * the block boundary marked in advance
 */
ArrayList *buildBasicBlockList(InstrList *iList){
	//TODO sub dealloc
	blockIdCtr=0;
	edgeIdCtr=0;

	ArrayList *blockList = al_newGeneric(AL_LIST_SORTED, blockIdCompare, NULL, destroyBasicBlock);
	int i = 0;
	int size = iList->numInstrs;
	Instruction *instr = NULL, *lastInstr = NULL;
	BasicBlock *block = createBasicBlock();
	BBL_SET_FLAG(block, BBL_IS_ENTRY_NODE);
	BasicBlock *prev = NULL;
	BlockEdge *edge = NULL;



	for (i = 0; i < size; i++) {
		instr = getInstruction(iList, i);

		al_add(block->instrs, instr);
		instr->inBlock = block;
		if(INSTR_HAS_FLAG(instr,INSTR_IS_BBL_END)) {
			/*
			 * End of bbl, add to list and create "edges"
			 */
			BasicBlock *existingBbl = getBlockFromInstrs(blockList, block->instrs);
			if(existingBbl) {
				//use the existing block
				int jj;
				for(jj=0;jj<al_size(block->instrs);jj++){
					Instruction *tempInstr = (Instruction *)al_get(block->instrs, jj);
					tempInstr->inBlock = existingBbl;
				}
				destroyBasicBlock((void *)block);
				block = existingBbl;
				block->count++;
			} else {
				//we found a new block, add it to the list
				al_add(blockList, block);
				block->count = 1;
				block->addr = ((Instruction *)(al_get(block->instrs, 0)))->addr;
				block->end_addr = instr->addr;
				block->lastInstr = instr;
				if(INSTR_HAS_FLAG(instr,INSTR_IS_1_BRANCH))
					block->type = BT_1_BRANCH;
				else if(INSTR_HAS_FLAG(instr,INSTR_IS_2_BRANCH))
					block->type = BT_2_BRANCH;
				else if(INSTR_HAS_FLAG(instr,INSTR_IS_N_BRANCH))
					block->type = BT_N_BRANCH;
				else if(INSTR_HAS_FLAG(instr,INSTR_IS_SCRIPT_INVOKE)){
						block->type = BT_SCRIPT_INVOKE;
				}
				/*
				 * it is possible in the slice the last instr is not stop/ret
				 * we still have to set it to BT_RET
				 */
				else if(INSTR_HAS_FLAG(instr,INSTR_IS_RET) || i==size-1)
					block->type = BT_RET;
				else
					block->type = BT_FALL_THROUGH;
			}



			//add "edges" to structs
			if(prev) {
				int added = 0;
				//create an edge: prev->block
				edge = createBlockEdge();
				edge->tail = prev;
				edge->head = block;
				edge->count = 1;
				if(prev->type == BT_SCRIPT_INVOKE){
					prev->calltarget = block;
					EDGE_SET_FLAG(edge, EDGE_IS_SCRIPT_INVOKE);
				}
				else if(prev->type == BT_2_BRANCH){
					assert(lastInstr != NULL);
					assert(lastInstr->jmpOffset!=0);
					//printf("lastInstr->jmpOffset %d\n", lastInstr->jmpOffset);
					if(lastInstr->jmpOffset < 0){
						if(block->addr > lastInstr->addr){
							//then this block is not branched block
							// XXX changed, but might be wrong here -08/10/2011
							EDGE_SET_FLAG(edge, EDGE_IS_ADJACENT_PATH);
						}else{
							EDGE_SET_FLAG(edge, EDGE_IS_BRANCHED_PATH);
						}
					}else{
						if(block->addr < lastInstr->addr + lastInstr->jmpOffset){
							//then this block is not branched block
							EDGE_SET_FLAG(edge, EDGE_IS_ADJACENT_PATH);
						}else{
							EDGE_SET_FLAG(edge, EDGE_IS_BRANCHED_PATH);
						}
					}
				}
				else if(prev->type == BT_RET)
					EDGE_SET_FLAG(edge, EDGE_IS_RET);


				if(!al_contains(prev->succs, edge)){
					al_add(prev->succs, edge);
					added++;
				}
				if(!al_contains(block->preds, edge)){
					al_add(block->preds, edge);
					added++;
				}
				if(!added){
					BlockEdge *e = (BlockEdge *)al_get(block->preds, al_indexOf(block->preds, edge));
					e->count++;
					destroyBlockEdge(edge);
				}
			}

			if(lastInstr != NULL) {
				lastInstr->nextBlock = block;
			}
			lastInstr = instr;
			if(i < (size-1)) {
				//there are more instructions left, so at least one more block
				//set new block to be used by subsequent instructions
				prev = block;
				block = createBasicBlock();
			} //else we're done
			else{
				BBL_SET_FLAG(block, BBL_IS_HALT_NODE);
			}
		}
	}

	/*
	 * try to discover all complement edges
	 */
	for(i=0;i<al_size(blockList);i++){
		block = (BasicBlock *)al_get(blockList, i);
		if(block->type==BT_2_BRANCH){
			//at least one branch target have to be reached.
			//If only one is reached, we need to add a complement edge to the other
			if(al_size(block->succs)!=2){
				assert(al_size(block->succs)==1);
				BasicBlock *branchTakenBlock;
				BasicBlock *branchMissingBlock;
				BlockEdge *branchTakenEdge;
				BlockEdge *branchMissingEdge;
				ADDRESS branchMissingAddress;
				fprintf(stderr, "block%d missing an edge\n", block->id);
				//get the branch instr (last instr in block)
				int temp_i = al_size(block->instrs)-1;
				instr = al_get(block->instrs, temp_i);
				assert(INSTR_HAS_FLAG(instr,INSTR_IS_2_BRANCH));
				//calculate missing branch target address
				branchTakenEdge = al_get(block->succs,0);
				branchTakenBlock = branchTakenEdge->head;
				if(EDGE_HAS_FLAG(branchTakenEdge, EDGE_IS_ADJACENT_PATH)){
					branchMissingAddress = instr->addr + instr->jmpOffset;
				}else{
					branchMissingAddress = instr->addr + instr->length;
				}
				//find the missing target block by address
				branchMissingBlock = findBasicBlockFromListByAddr(blockList, branchMissingAddress);
				if(branchMissingBlock){
					//add the complement-edge to block->succ
					branchMissingEdge = createBlockEdge();
					branchMissingEdge->count = 0;
					branchMissingEdge->tail = block;
					branchMissingEdge->head = branchMissingBlock;
					EDGE_SET_FLAG(branchMissingEdge, EDGE_IS_COMPLEMENT);
					if(EDGE_HAS_FLAG(branchTakenEdge, EDGE_IS_ADJACENT_PATH)){
						EDGE_SET_FLAG(branchMissingEdge, EDGE_IS_BRANCHED_PATH);
					}else{
						EDGE_SET_FLAG(branchMissingEdge, EDGE_IS_ADJACENT_PATH);
					}
					al_add(block->succs, branchMissingEdge);
					al_add(branchMissingBlock->preds, branchMissingEdge);
				}else
					continue;
			}
			else{
				continue;
			}
		}else if(block->type==BT_N_BRANCH){
			//TODO
			assert(0);
		}
	}

	return (blockList);
}

void destroyBasicBlockList(ArrayList *blockList){
	if(!blockList)
		return;
/*	int i;
	BasicBlock *bbl;
	for(i=0;i<al_size(blockList);i++){
		bbl = al_get(blockList, i);
		destroyBasicBlock(bbl);
		bbl = NULL;
	}
	free(blockList->al_elements);
	blockList->al_elements = NULL;
	free(blockList);
	blockList = NULL;*/
	al_freeWithElements(blockList);
}

/*
 * do-it-all:
 * find leaders-> mark blocks-> create blocks and CFG
 * return a list of BasicBlocks
 */
ArrayList *buildDynamicCFG(InstrList *iList){
	ArrayList *leaders;
	ArrayList *blockList;
	leaders = findLeaders(iList);
	printf("\nLeader list:\n");
	al_print(leaders);
	printf("\n");
	markBasicBlockBoundary(iList, leaders);
	al_free(leaders);
	blockList = buildBasicBlockList(iList);
	return blockList;
}


void printDominators(ArrayList *blockList){
	int i,j;
	BasicBlock *b1, *b2;
	printf("\nDominators for each block\n");
	for(i=0;i<al_size(blockList);i++){
		b1 = (BasicBlock *)al_get(blockList, i);
		printf(" Dominators of block %3d:\t", b1->id);
		for(j=0;j<al_size(b1->dominators);j++){
			b2 = (BasicBlock *)al_get(b1->dominators, j);
			printf("%d\t", b2->id);
		}
		printf("\n");
	}
}



void printDomList(ArrayList *blockList){
	int i,j;
	BasicBlock *b1;
	BlockEdge *e;
	printf("\nDomListfor each block\n");
	for(i=0;i<al_size(blockList);i++){
		b1 = (BasicBlock *)al_get(blockList, i);
		printf(" blocks dominated by block %3d:\t", b1->id);
		for(j=0;j<al_size(b1->dominate);j++){
			e = (BlockEdge *)al_get(b1->dominate, j);
			assert(EDGE_HAS_FLAG(e, EDGE_IS_DOMINATE));
			printf("\t%d\t", e->head->id);
		}
		printf("\n");
	}
}

void printImmDomList(ArrayList *blockList){
	int i,j;
	BasicBlock *b1;
	BlockEdge *e;
	printf("\nImm Dom List for each block\n");
	for(i=0;i<al_size(blockList);i++){
		b1 = (BasicBlock *)al_get(blockList, i);
		printf(" blocks immediately dominated by block %3d:\t", b1->id);
		for(j=0;j<al_size(b1->immDomSuccs);j++){
			e = (BlockEdge *)al_get(b1->immDomSuccs, j);
			printf("\t%d\t", e->head->id);
		}
		printf("\n");
	}
}


/*
 * return true if block1 dominate block2 in CFG
 */
bool dominate(BasicBlock *block1, BasicBlock *block2){
	assert(block1&&block2);
	assert(block1->dominate);
	int i;
	BlockEdge *edge;
	for(i=0;i<al_size(block1->dominate);i++){
		edge = al_get(block1->dominate, i);
		if(edge->head->id == block2->id)
			return true;
	}
	return false;
}

/*
 * auxiliary function used by findDominators()
 */
ArrayList *findDominatorHelper(BasicBlock *block){
	//printf("findDominatorHelper: id %d\n", block->id);

	assert(!BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE));
	assert(!BBL_HAS_FLAG(block, BBL_IS_FUNC_ENTRY));

	int i;
	ArrayList *dominators, *dominatorsTemp;
	BasicBlock *predNode;

	/*
	 * D(n) = {n} 'union' ('intersection' D(p) | for each p 'in' preds(n))
	 */
	predNode = (BasicBlock *)((BlockEdge *)al_get(block->preds, 0))->tail;
	dominators = dominatorsTemp = al_clone(predNode->dominators);
	assert(dominators && dominatorsTemp);

	for(i=1;i<al_size(block->preds);i++){
		predNode = (BasicBlock *)((BlockEdge *)al_get(block->preds, i))->tail;
		dominators = al_intersection(dominatorsTemp, predNode->dominators);
		al_free(dominatorsTemp);
		dominatorsTemp = NULL;
		dominatorsTemp = dominators;
	}

	assert(dominators);
	al_add(dominators, (void *)block);

	return dominators;
}

/*
 * find all the dominators for each basicBlock in the Dynamic CFG
 * this function would create an ArrayList(set) dominators for each BasicBlock
 */
void findDominators(ArrayList *blockList){
	assert(blockList);
	BasicBlock *block, *block2, *block3;
	ArrayList *domTemp;
	BlockEdge *domEdge;
	int change;
	int i,j,k;

	/* Initialize D(n) for each node n.
	 * D(n) = {n0}, if n==n0
	 * D(n) = N (all nodes), otherwise
	 */
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(block->dominators){
			al_free(block->dominators);
		}
		//printf("block#%d\t", block->id);
		//block 0 is garanteed to be the entry node
		if(BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE)){
			//assert(BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE));
			//printf("entry\n");
			block->dominators = al_newGeneric(AL_LIST_SET, blockIdCompare, NULL, NULL);
			al_add(block->dominators, (void *)block);
		}
		else if(al_size(block->preds)==0){
			assert(BBL_HAS_FLAG(block, BBL_IS_FUNC_ENTRY));
			//printf("func_entry\n");
			block->dominators = al_newGeneric(AL_LIST_SET, blockIdCompare, NULL, NULL);
			al_add(block->dominators, (void *)block);
		}
		else{
			//printf("reg\n");
			block->dominators = al_clone(blockList);
			block->dominators->al_type = AL_LIST_SET;
			block->dominators->al_print = NULL;
			block->dominators->al_free = NULL;
		}
	}

	//printDominators(blockList);

	do{
		change = 0;
		//for each node != n0
		for(i=0;i<al_size(blockList);i++){
			block = (BasicBlock *)al_get(blockList,i);
			if(BBL_HAS_FLAG(block, BBL_IS_FUNC_ENTRY)||BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE))
				continue;
			domTemp = findDominatorHelper(block);
			assert(domTemp);

			if(al_setEquals(block->dominators, domTemp)){
				al_free(domTemp);
				domTemp = NULL;
			}else{
				al_free(block->dominators);
				block->dominators = NULL;
				block->dominators = domTemp;
				change++;
			}

		}//end for-loop
	}while(change);

	//printDominators(blockList);

	//calculate block->dominate
	for(i=0;i<al_size(blockList);i++){
		block = (BasicBlock *)al_get(blockList,i);
		if(block->dominate){
			al_freeWithElements(block->dominate);
		}
		block->dominate = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);

		for(j=0;j<al_size(block->dominators);j++){
			//block2 dominates block
			block2 =  (BasicBlock *)al_get(block->dominators,j);
			domEdge = createBlockEdge();
			domEdge->tail = block2;
			domEdge->head = block;
			EDGE_SET_FLAG(domEdge, EDGE_IS_DOMINATE);
			if(!al_contains(block2->dominate, domEdge)){
				al_add(block2->dominate, domEdge);
			}else{
				destroyBlockEdge(domEdge);
			}
		}
	}

	//printDomList(blockList);

	//construct dom-tree
	for(i=0;i<al_size(blockList);i++){
		block = (BasicBlock *)al_get(blockList,i);
		if(block->immDomPreds){
			//al_freeWithElements(block->immDomPreds);
			al_free(block->immDomPreds);
			block->immDomPreds=NULL;
		}
		if(block->immDomSuccs){
			al_freeWithElements(block->immDomSuccs);
			block->immDomSuccs=NULL;
		}
		block->immDomPreds = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);
		block->immDomSuccs = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, destroyBlockEdge);

		for(j=0;j<al_size(block->dominators);j++){
			//block2 dominates block
			block2 =  (BasicBlock *)al_get(block->dominators,j);
			if(block2->id == block->id){
				continue;
			}
			for(k=0;k<al_size(block->dominators);k++){
				block3 = (BasicBlock *)al_get(block->dominators,k);
				if(block3->id == block->id){
					continue;
				}
				if(!dominate(block3, block2)){
					block2 = NULL;		//then block2 cannot be the imm dominator of block, set to NULL as a flag
					break;
				}
			}
			if(!block2)
				continue;
			//printf("block%d is imm-dominator of block%d\n", block2->id, block->id);
			//then block is imm dominator of block
			domEdge = createBlockEdge();
			domEdge->tail = block2;
			domEdge->head = block;
			EDGE_SET_FLAG(domEdge, EDGE_IS_IMM_DOM);
			al_add(block->immDomPreds, domEdge);
			al_add(block2->immDomSuccs, domEdge);
		}
	}
	//printImmDomList(blockList);
}





/*
 * the recursive function used by buildDFSTree
 */
static void doSearch(BasicBlock *n, uint32_t flag){
	int i;
	BlockEdge *edge;
	BasicBlock *block;
	assert(n);
	BBL_SET_FLAG(n, flag);

	for(i=0;i<al_size(n->succs);i++){
		edge = al_get(n->succs, i);
		block = edge->head;
		if(!BBL_HAS_FLAG(block, flag)){
			EDGE_SET_FLAG(edge, EDGE_IN_DFST);
			doSearch(block, flag);
		}
	}
	n->dfn = DFSTreeNodeCount--;
}


/*
 * given a list of basic blocks, build a DFS tree of it (a list of edges)
 * and assign a depth-first number to each block.
 */
void buildDFSTree(ArrayList *blockList){
	int i;
	BasicBlock *block;
	BasicBlock *n0;
	DFSTreeNodeCount = al_size(blockList);
	//make sure flag TMP0 is cleared
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
		if(BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE))
			n0 = block;
	}
	doSearch(n0, BBL_FLAG_TMP0);

	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(BBL_HAS_FLAG(block, BBL_IS_FUNC_ENTRY)){
			doSearch(block, BBL_FLAG_TMP0);
			//printBasicBlock(block);
		}
	}
	//clear flag TMP0
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}
	return;
}

/*
 * mark all retreating and back edge, and determine if CFG is reducible (return 0 if not)
 */
int findBackEdges(ArrayList *blockList){
	int i,j;
	int reducible = 0;
	BasicBlock *block, *next;
	BlockEdge *edge;
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		for(j=0;j<al_size(block->succs);j++){
			edge = al_get(block->succs, j);
			next = edge->head;
			//an edge m->n is retreating iff dfn(m)>=dfn(n)
			if(block->dfn>=next->dfn){
				EDGE_SET_FLAG(edge, EDGE_IS_RETREAT);
			}
			//check if 'next' dominate 'block'
			if(al_contains(block->dominators, next)){
				EDGE_SET_FLAG(edge, EDGE_IS_BACK_EDGE);
			}
			//if edge is !(both RETREAT and BACK, or neither)
			if(!((EDGE_HAS_FLAG(edge, EDGE_IS_RETREAT) && EDGE_HAS_FLAG(edge, EDGE_IS_BACK_EDGE)) ||
					!((EDGE_HAS_FLAG(edge, EDGE_IS_RETREAT) || EDGE_HAS_FLAG(edge, EDGE_IS_BACK_EDGE)))))
				reducible++;
		}
	}
	if(reducible>0)
		return 0;
	else
		return 1;
}

void destroyNaturalLoop(void *loop){
	if(!loop)
		return;
	NaturalLoop *lp = (NaturalLoop *)loop;
	al_free(lp->backEdges);
	al_free(lp->nodes);
	free(lp);
	return;
}

NaturalLoop *createNaturalLoop(BlockEdge *backEdge){
	if(!EDGE_HAS_FLAG(backEdge, EDGE_IS_BACK_EDGE)){
		fprintf(stderr, "Error - createNaturalLoop(): edge %3d->%3d is not a back-edge!\n", backEdge->tail->id, backEdge->head->id);
		return NULL;
	}
	NaturalLoop *loop = (NaturalLoop *)malloc(sizeof(NaturalLoop));
	assert(loop);
	loop->flags = 0;
	loop->id = loopIdCtr++;
	loop->backEdges = al_newGeneric(AL_LIST_SET, edgeBlockIdCompare, NULL, NULL);
	al_add(loop->backEdges, backEdge);
	loop->header = backEdge->head;
	loop->nodes = al_newGeneric(AL_LIST_SET, blockIdCompare, NULL, NULL);

	return loop;
}

int loopHeaderCompare(void *l1, void *l2){
	assert(l1 && l2);
	return ((NaturalLoop *)l1)->header->id - ((NaturalLoop *)l2)->header->id;
}


void printNaturalLoop(NaturalLoop *loop){
	assert(loop);
	int i;
	BlockEdge *edge;
	BasicBlock *block;

	printf("=================================================================\n");
	printf("HEADER: %3d\tID: %3d\n", loop->header->id, loop->id);
	printf("BACK-EDGE(S):\n");
	for(i=0;i<al_size(loop->backEdges);i++){
		edge = (BlockEdge *)al_get(loop->backEdges, i);
		printf(" - %3d->%3d\n", edge->tail->id, edge->head->id);
	}
	printf("NODES:\t[ ");
	for(i=0;i<al_size(loop->nodes);i++){
		block = (BasicBlock *)al_get(loop->nodes, i);
		printf(" %3d ", block->id);
	}
	printf(" ]\n");
}

void printNaturalLoopList(ArrayList *loopList){
	int i;
	NaturalLoop *loop;
	printf("\nNATURAL-LOOP LIST:\n");
	for(i=0;i<al_size(loopList);i++){
		loop = al_get(loopList, i);
		printNaturalLoop(loop);
	}
	printf("=================================================================\n");
}

void doReverseSearch(BasicBlock *n, uint32_t flag){
	int i;
	BlockEdge *edge;
	BasicBlock *block;
	assert(n);
	BBL_SET_FLAG(n, flag);
	//printf("add block:%d\n", n->id);
	for(i=0;i<al_size(n->preds);i++){
		edge = al_get(n->preds, i);
		block = edge->tail;
		if(!BBL_HAS_FLAG(block, flag)){
			doReverseSearch(block, flag);
		}
	}
}


NaturalLoop *buildLoop(ArrayList *blockList, NaturalLoop *loop){
	assert(blockList && loop);
	int i;
	BasicBlock *block;

	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}

	block = ((BlockEdge *)al_get(loop->backEdges,0))->tail;
	if(block->id == loop->header->id)
		al_add(loop->nodes, block);
	else{
		BBL_SET_FLAG(loop->header, BBL_FLAG_TMP0);
		doReverseSearch(block, BBL_FLAG_TMP0);
	}

	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(BBL_HAS_FLAG(block, BBL_FLAG_TMP0)){
			al_add(loop->nodes, block);
			BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
		}
	}
	return loop;
}

/*
 * try to combine all nodes in loop1 and loop2 into loop1
 * keep loop2 untouched, caller's responsible to free loop2.
 */
void combineLoops(NaturalLoop *loop1, NaturalLoop* loop2){
	assert(loop1 && loop2);
	assert(loop1->header->id == loop2->header->id );
	al_addAll(loop1->nodes, loop2->nodes);
	al_addAll(loop1->backEdges, loop2->backEdges);
}

ArrayList *buildNaturalLoopList(ArrayList *blockList){
	assert(blockList);
	int i,j;
	ArrayList *loopList;
	BasicBlock *block;
	BlockEdge *edge;
	NaturalLoop *loop;

	loopIdCtr = 0;

	loopList = al_newGeneric(AL_LIST_SET, loopHeaderCompare, NULL, destroyNaturalLoop);

	for(i=0;i<al_size(blockList);i++){
		block = (BasicBlock *)al_get(blockList, i);
		for(j=0;j<al_size(block->succs);j++){
			edge = (BlockEdge *)al_get(block->succs, j);
			if(!EDGE_HAS_FLAG(edge, EDGE_IS_BACK_EDGE))
				continue;
			loop = createNaturalLoop(edge);

			//reverse DFS search for entire loop
			buildLoop(blockList, loop);

			//check is there's other loop with same header in the list
			if(al_contains(loopList, (void *)loop)){
				//try to combine 2 loop with same header
				NaturalLoop *loopTmp = (NaturalLoop *)al_get(loopList, al_indexOf(loopList,(void *)loop ));
				combineLoops(loopTmp, loop);
				destroyNaturalLoop(loop);
				loop=NULL;
			}
			else{
				al_add(loopList, (void *)loop);
			}
		}
	}//end for-loop

	return loopList;
}

void destroyNaturalLoopList(ArrayList *loopList){
	if(!loopList)
		return;
	al_freeWithElements(loopList);
}

//return NULL if block is not in the loop
//return pointer to the block  if block is in the loop or is the loop header
BasicBlock *isBlockInTheLoop(NaturalLoop *loop, int block){
	assert(loop && block>=0);
	int i;
	BasicBlock *node;
	for(i=0;i<al_size(loop->nodes);i++){
		node = al_get(loop->nodes,i);
		if(node->id == block){
			//assert(node==block);
			return node;
		}
	}
	return NULL;
}

BasicBlock *isBlockTheLoopHeader(NaturalLoop *loop, int block){
	assert(loop && block>=0);
	if(loop->header->id == block){
		//assert(loop->header==block);
		return loop->header;
	}else
		return NULL;
}


NaturalLoop *findSmallestUnprocessedLoop(ArrayList *loopList, uint32_t flag){
	assert(loopList);
	int i;
	int min = -1;
	NaturalLoop *min_loop=NULL;
	NaturalLoop *loop;
	for(i=0;i<al_size(loopList);i++){
		loop = al_get(loopList, i);
		if(LOOP_HAS_FLAG(loop, flag)){
			continue;
		}
		if(min<0){
			min = al_size(loop->nodes);
			min_loop = loop;
		}
		else{
			if(min > al_size(loop->nodes)){
				min = al_size(loop->nodes);
				min_loop = loop;
			}
		}
	}
	if(!min_loop){
		//printf("no loop unprocessed\n");
		return NULL;
	}
	//printf("min loop is #%d. size : %d\n", min_loop->header->id, min);
	LOOP_SET_FLAG(min_loop, flag);
	return min_loop;
}


/*
 * this helper function returns an basicblock end with AND/OR opCode if there's any,
 * or NULL otherwise
 */
BasicBlock *findANDandORBlock(InstrList *iList, ArrayList *blockList){
	int i;
	BasicBlock *block;
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(isAND(iList, block->lastInstr)||isOR(iList, block->lastInstr))
			return block;
	}
	return NULL;
}


/*
 * the recursive function used by processANDandORExp
 */
static void markANDandORExp(BasicBlock *n, uint32_t flag){
	int i;
	BlockEdge *edge;
	BasicBlock *block;
	assert(n);
	BBL_SET_FLAG(n, flag);

	for(i=0;i<al_size(n->preds);i++){
		edge = al_get(n->preds, i);
		block = edge->tail;
		if(!BBL_HAS_FLAG(block, flag)){
			markANDandORExp(block, flag);
		}
	}
	printf("block %d marked\n", n->id);
}


/*
 * this function would fail if any block label by flag but not endFlag
 * has an edge point to a block not labeled by flag
 */
void checkExpValidity(InstrList *iList, ArrayList *blockList, uint32_t flag, uint32_t endFlag){
	int i, j;
	BlockEdge *edge;
	BasicBlock *block;
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(!BBL_HAS_FLAG(block, flag) || BBL_HAS_FLAG(block, endFlag))
			continue;
		assert((block->type==BT_FALL_THROUGH)||isAND(iList, block->lastInstr)||isOR(iList, block->lastInstr));
		for(j=0;j<al_size(block->succs);j++){
			edge = al_get(block->succs, j);
			assert(BBL_HAS_FLAG(edge->head, flag));
		}
	}
}


BasicBlock *findBasicBlockInFunctionCFGs(ArrayList *funcCFGs, int id, Function **ret_func){
	int i,j;
	BasicBlock *block, *ret;
	Function *func;
	assert(funcCFGs && id>=0);
	ret = NULL;
	printf("searching for block#%d..\t", id);

	for(i=0;i<al_size(funcCFGs);i++){
		func = al_get(funcCFGs, i);
		for(j=0;j<al_size(func->funcBody);j++){
			block = (BasicBlock*)al_get(func->funcBody, j);
			if(block->id==id){
				printf("found!\n");
				ret = block;
				*ret_func = func;
				//printf("%lx\n", *ret_func);
				return ret;
			}
		}
	}
	printf("not found!\n");
	return ret;
}

/*
 * process all the AND/OR expressions
 * concatenate involved basicblocks in the order of the address
 */
void processANDandORExps(InstrList *iList, ArrayList *blockList, ArrayList *funcCFGs){
	assert(blockList);

	int i,j;
	BasicBlock *startBlock, *endBlock, *tempBlock, *tempBlock2;

	ArrayList *concatList;

	printf("\n******************************************************************************\n");
	printf("process all the AND/OR expressions\n");

	//while there exists an block end with AND/OR opCode, loop
	while((startBlock=findANDandORBlock(iList, blockList))){
		//find the corresponding endBlock for this startBlock (there must be one in the trace!)
		endBlock = findBasicBlockFromListByAddr(blockList, startBlock->lastInstr->addr + startBlock->lastInstr->jmpOffset);

		printf("===============start %d end %d dom %d\n", startBlock->id, endBlock->id, dominate(startBlock, endBlock));

		/*
		 * we use BBL_FLAG_TMP1 to label the endBlock
		 * BBL_FLAG_TMP0 to label all blocks between startBlock and endBlock in CFG(including endBlock)
		 */
		for(i=0;i<al_size(blockList);i++){
			tempBlock = al_get(blockList,i);
			BBL_CLR_FLAG(tempBlock, BBL_FLAG_TMP0);
			BBL_CLR_FLAG(tempBlock, BBL_FLAG_TMP1);
		}
		BBL_SET_FLAG(startBlock, BBL_FLAG_TMP0);
		BBL_SET_FLAG(endBlock, BBL_FLAG_TMP1);
		markANDandORExp(endBlock, BBL_FLAG_TMP0);
		checkExpValidity(iList, blockList, BBL_FLAG_TMP0, BBL_FLAG_TMP1);

		/*
		 * now sort those labeled blocks in order, and concat them together into one basic block
		 * in other words, treat AND/OR as a non-branch instruction
		 */
		concatList = al_newGeneric(AL_LIST_SORTED, blockAddrCompare, NULL, NULL);
		for(i=0;i<al_size(blockList);i++){
			tempBlock = al_get(blockList, i);
			if(BBL_HAS_FLAG(tempBlock, BBL_FLAG_TMP0)){
				assert(dominate(startBlock, tempBlock));
				al_add(concatList, (void *)tempBlock);
			}
		}

		printf("Concat list:\n/");
		for(i=0;i<al_size(concatList);i++){
			tempBlock = (BasicBlock *)al_get(concatList, i);
			printf(" %d ", tempBlock->id);
		}
		printf("\n");

		//create a new BBL, with all instructions in concatList.
		//block id and preds are from startBlock, type and succs are from endBlock
		//count from the max, and flags are the union of all block flags
		BasicBlock *newBlock = createBasicBlock();
		al_free((void *)(newBlock->preds));
		al_free((void *)(newBlock->succs));
		Instruction *instr;
		BlockEdge *tempEdge;
		int listSize = al_size(concatList);
		for(i=0;i<listSize;i++){
			tempBlock = (BasicBlock *)al_get(concatList, i);
			if(i==0){
				assert(tempBlock==startBlock);
				newBlock->id = startBlock->id;
				newBlock->preds = startBlock->preds;
				newBlock->dfn = startBlock->dfn;
				newBlock->addr = startBlock->addr;
				for(j=0;j<al_size(newBlock->preds);j++){
					tempEdge = al_get(newBlock->preds, j);
					tempEdge->head = newBlock;
				}
			}else if(i==listSize-1){
				assert(tempBlock==endBlock);
				newBlock->type = endBlock->type;
				newBlock->succs = endBlock->succs;
				newBlock->end_addr = endBlock->end_addr;
				newBlock->lastInstr = endBlock->lastInstr;
				for(j=0;j<al_size(newBlock->succs);j++){
					tempEdge = al_get(newBlock->succs, j);
					tempEdge->tail = newBlock;
				}
			}
			if(tempBlock->count > newBlock->count){
				newBlock->count = tempBlock->count;
			}

			newBlock->flags = newBlock->flags | tempBlock->flags;

			//process each instruction
			for(j=0;j<al_size(tempBlock->instrs);j++){
				instr = al_get(tempBlock->instrs, j);
				instr->inBlock = newBlock;
				if(INSTR_HAS_FLAG(instr, INSTR_IS_2_BRANCH) && (isAND(iList, instr) || isOR(iList, instr))){
					INSTR_CLR_FLAG(instr, INSTR_IS_2_BRANCH);
				}
				if(i!=0 && INSTR_HAS_FLAG(instr, INSTR_IS_BBL_START)){
					INSTR_CLR_FLAG(instr, INSTR_IS_BBL_START);
				}
				else if(i!=listSize-1 && INSTR_HAS_FLAG(instr, INSTR_IS_BBL_END)){
					INSTR_CLR_FLAG(instr, INSTR_IS_BBL_END);
				}

				al_add(newBlock->instrs, (void *)instr);
			}
			//remove concatenated block from CFG
			al_remove(blockList, (void *)tempBlock);
			//add newBlock into CFG
			if(i==0){
				al_add(blockList,newBlock);
			}
		}

		Function *func = NULL;
		for(i=0;i<listSize;i++){
			tempBlock = (BasicBlock *)al_get(concatList, i);
			tempBlock2 = findBasicBlockInFunctionCFGs(funcCFGs, tempBlock->id, &func);
			//printf("%lx\n", func);
			assert(tempBlock2);assert(func);
			al_remove(func->funcBody, (void *)tempBlock2);
			assert(tempBlock == tempBlock2);
			if(i==0){
				destroyBasicBlockForConCat(tempBlock, 1);
			}else if(i==listSize-1){
				destroyBasicBlockForConCat(tempBlock, 2);
			}else{
				destroyBasicBlockForConCat(tempBlock, 0);
			}
			tempBlock=NULL;
		}
		assert(func);
		al_add(func->funcBody, newBlock);

		al_free(concatList);
		concatList = NULL;

/*		//make sure all the flags are cleared
		for(i=0;i<al_size(blockList);i++){
			tempBlock = al_get(blockList,i);
			BBL_CLR_FLAG(tempBlock, BBL_FLAG_TMP0);
			BBL_CLR_FLAG(tempBlock, BBL_FLAG_TMP1);
		}*/
		//printBasicBlockList(blockList);

	    //XXX memory leakage (dom edges)
	    findDominators(blockList);
	    buildDFSTree(blockList);

	}
	printf("\n******************************************************************************\n");
	printf("\n******************************************************************************\n");
}






