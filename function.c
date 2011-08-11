/*
 * function.c
 *
 *  Created on: May 16, 2011
 *      Author: genlu
 */

#include <stdlib.h>
#include <assert.h>
#include "al_helper.h"
#include "cfg.h"
#include "instr.h"
#include "function.h"

int functionCFGProcessed = 0;

int FuncEntryBlkIDCompare(void *i1, void *i2){
	assert(i1 && i2);
	return ((Function *)i1)->funcEntryBlock->id- ((Function *)i2)->funcEntryBlock->id;
}

int FuncEntryAddrCompare(void *i1, void *i2){
	assert(i1 && i2);
	return ((Function *)i1)->funcEntryAddr - ((Function *)i2)->funcEntryAddr;
}

void destroyFunctionNode(void *func){
	if(func!=NULL)
		free((Function *)func);
}

Function *createFunctionNode(void){
	Function *func = (Function *)malloc(sizeof(Function));
	assert(func);
	func->args = 0;
	func->flags = 0;
	func->funcEntryAddr = 0;
	func->funcObj = 0;
	func->funcEntryBlock = NULL;
	func->funcName = NULL;
	func->funcBody = al_newGeneric(AL_LIST_SORTED, blockIdCompare, NULL, NULL);
	return func;
}

void destroyFunctionCFGs(ArrayList *funcCFGs){

	if(funcCFGs){
		//printf("freeing %d\n", al_size(funcCFGs));
		al_freeWithElements(funcCFGs);

	}
}

/*
 * the recursive function used by buildFunctionCFGs
 */
static void doSearch(ArrayList *blockList, BasicBlock *n, uint32_t flag, ArrayList *functionCFGs){
	int k, i, j;
	BlockEdge *edge;
	BasicBlock *block1, *block2, *block3, *block4;
	Function *func;

	if(!n)return;

	assert(n);
	BBL_SET_FLAG(n, flag);

	if(n->type==BT_SCRIPT_INVOKE){
		//assert(al_size(n->succs)==1);
		for(k=0;k<al_size(n->succs);k++)
		{
			//get a pointer to the callee
			edge = (BlockEdge *)al_get(n->succs, k);

			if(EDGE_HAS_FLAG(edge, EDGE_IS_CALLSITE))
				continue;

			block1 = edge->head;
			BBL_SET_FLAG(block1, BBL_IS_FUNC_ENTRY);
			edge = NULL;
			//n = NULL;		//FXXX!!!
			/*
			 * Now block1 is the function entry block, use block1->preds to find all caller blocks
			 * and concat them with their next block.
			 * cut out the function from CFG, create a function node and insert it to functionCFGs
			 * XXX: this will fail if caller and returned block are not adjacent address-wise (i.e. in a slice/simpllified trace)
			 * 		need to change this later!
			 */
			//printf("@@b1:%d\n", block1->id);
			for(i=0;i<al_size(block1->preds);i++){
				edge = (BlockEdge *)al_get(block1->preds, i);
				block2 = edge->tail;
				//printf("@@  b2:%d\n", block2->id);
				//printBasicBlock(block2); //////////////////////
				//assert(block2->type==BT_SCRIPT_INVOKE);
				//assert(al_size(block2->succs)==1);


				edge = (BlockEdge *)al_get(block2->succs, 0);
				//assert(EDGE_HAS_FLAG(edge, EDGE_IS_SCRIPT_INVOKE));///////////////////////
				//remove this edge from block2->succs and block1->preds
				al_remove(block2->succs, (void *)edge);
				al_remove(block1->preds, (void *)edge);

				//hack
				i--;
				if(n->id==block2->id)
					k--;

				destroyBlockEdge((void *)edge);
				//assert(al_size(block2->succs)==0);

				block3 = findBasicBlockFromListByAddr(blockList, block2->end_addr + 3);
				if(!block3){
					//printf("block2:%d\t%lx\n",block2->id, block2->end_addr + 3);
					//assert(block3);
					continue;
				}

				//printf("    @@b3:%d\n", block3->id);
				for(j=0;j<al_size(block3->preds);j++){
					edge = (BlockEdge *)al_get(block3->preds, j);
					if(!EDGE_HAS_FLAG(edge, EDGE_IS_RET))
						continue;
					else{
						block4 = edge->tail;
						BBL_SET_FLAG(block4, BBL_IS_FUNC_RET);
						//printf("@@      b4:%d\n", block4->id);
						al_remove(block4->succs, (void *)edge);
						al_remove(block3->preds, (void *)edge);
						printf("@@@@@ b1:%d\tb2:%d\tb3:%d\tb4:%d\n", block1->id, block2->id, block3->id, block4->id);
						//hack
						j--;

						destroyBlockEdge((void *)edge);
					}
				}
				edge = createBlockEdge();
				edge->tail = block2;
				edge->head = block3;
				edge->count = block2->count;
				EDGE_SET_FLAG(edge, EDGE_IS_CALLSITE);

				int added = 0;
				if(!al_contains(block2->succs, edge)){
					al_add(block2->succs, edge);
					added++;
				}else{
					EDGE_SET_FLAG(((BlockEdge *)al_get(block2->succs, al_indexOf(block2->succs, edge))), EDGE_IS_CALLSITE);
				}
				if(!al_contains(block3->preds, edge)){
					al_add(block3->preds, edge);
					added++;
				}else{
					EDGE_SET_FLAG(((BlockEdge *)al_get(block3->preds, al_indexOf(block3->preds, edge))), EDGE_IS_CALLSITE);
				}
				if(!added){
					destroyBlockEdge(edge);
				}
			}
			assert(al_size(block1->preds)==0);

			func = createFunctionNode();
			func->funcEntryAddr = block1->addr;
			func->funcEntryBlock = block1;

			if(al_contains(functionCFGs, (void *)func))
				destroyFunctionNode((void *)func);
			else
				al_add(functionCFGs, (void *)func);

			doSearch(blockList, block1, flag, functionCFGs);

			doSearch(blockList, block3, flag, functionCFGs);
		}
	}
	else{
		for(i=0;i<al_size(n->succs);i++){
			edge = (BlockEdge *)al_get(n->succs, i);
			block1 = edge->head;
			if(!BBL_HAS_FLAG(block1, flag)){
				EDGE_SET_FLAG(edge, EDGE_IN_DFST);
				doSearch(blockList, block1, flag, functionCFGs);
			}
		}
	}
}


void concatCallSiteBlocks(BlockEdge *edge){
	int i;
	BasicBlock *block1, *block2;
	BlockEdge *e;
	Instruction *instr;
	block1 = edge->tail;
	block2 = edge->head;
	block1->type = block2->type;
	al_remove(block1->succs, edge);
	for(i=0;i<al_size(block2->succs);i++){
		e = al_get(block2->succs, i);
		e->tail = block1;
		al_add(block1->succs, e);
	}
	for(i=0;i<al_size(block2->instrs);i++){
		instr = al_get(block2->instrs, i);
		instr->inBlock = block1;
		al_add(block1->instrs, instr);
	}
	block1->end_addr = block2->end_addr;
	if(BBL_HAS_FLAG(block2, BBL_IS_HALT_NODE))
		BBL_SET_FLAG(block1, BBL_IS_HALT_NODE);
	if(BBL_HAS_FLAG(block2, BBL_IS_FUNC_RET))
		BBL_SET_FLAG(block1, BBL_IS_FUNC_RET);

	al_free(block2->instrs);
	al_free(block2->preds);
	al_free(block2->succs);
}



void doSearchFuncBody(BasicBlock *n, ArrayList *funcBody, uint32_t flag){
	int i;
	BlockEdge *edge;
	BasicBlock *block;
	assert(n);
	BBL_SET_FLAG(n, flag);

	al_add(funcBody, n);

	for(i=0;i<al_size(n->succs);i++){
		edge = al_get(n->succs, i);
		block = edge->head;
		if(!BBL_HAS_FLAG(block, flag)){
			doSearchFuncBody(block, funcBody, flag);
		}
	}
}
/*
 *	buildFunctionCFGs() rearrange CFG specified by 'blockList',
 *	"cut out" all the functions, and connect call-site and return-site (adjacent instructions)
 *	with an 'EDGE_IS_CALLSITE' edge. it also mark entry and return node of functions
 *	Finally, a list of Function nodes is returned (program entry is not considered as a function)
 */
ArrayList *buildFunctionCFGs(InstrList *iList, ArrayList *blockList){
	int i,j,k;
	BasicBlock *block;
	BlockEdge *edge;
	BasicBlock *n0;
	ArrayList *functionCFGs;
	Instruction *instr;

	functionCFGProcessed++;

	functionCFGs = al_newGeneric(AL_LIST_SET, FuncEntryBlkIDCompare, NULL, destroyFunctionNode);
	assert(functionCFGs);

	//make sure flag TMP0 is cleared
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
		if(BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE))
			n0 = block;
	}

	//printf("n0:%p\n", n0);
	doSearch(blockList, n0, BBL_FLAG_TMP0, functionCFGs);

	//clear flag TMP0
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}

	//concat all script call and returned block as one block
	int change;
	while(1){
		change=0;
		for(i=0;i<al_size(blockList);i++){
			block = al_get(blockList, i);
			for(j=0;j<al_size(block->succs);j++){
				edge = al_get(block->succs, j);
				if(EDGE_HAS_FLAG(edge, EDGE_IS_CALLSITE)){
					//assert(0);
					printf("concat callsite: %d->%d\n", edge->tail->id, edge->head->id);
					assert(al_size(edge->tail->succs)==1);
					assert(al_size(edge->head->preds)==1);
					concatCallSiteBlocks(edge);
					al_remove(blockList, edge->head);
					for(k=0;k<InstrListLength(iList);k++){
						instr = getInstruction(iList, k);
						if(instr->inBlock==edge->head){
							instr->inBlock = edge->tail;
						}
					}
					destroyBlockEdge(edge);
					change++;
					break;
				}
			}//end inner for-loop
			if(change)
				break;
		}//end outer for-loop
		if(!change)
			break;
	}

	Function *func;

	//find all the function bodies
	for(i=0;i<al_size(functionCFGs);i++){
		func = al_get(functionCFGs, i);
		doSearchFuncBody(func->funcEntryBlock, func->funcBody, BBL_FLAG_TMP0);
	}
	//print all function info
	for(i=0;i<al_size(functionCFGs);i++){
		func = al_get(functionCFGs, i);
		printf("func %s\t", func->funcName);
		for(j=0;j<al_size(func->funcBody);j++){
			printf("%d  ",((BasicBlock*)al_get(func->funcBody, j))->id);
		}
		printf("\n");
	}

	//clear flag TMP0
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}

	for(i=0;i<al_size(functionCFGs);i++){
		int args = 0;
		func = al_get(functionCFGs, i);
		for(j=0;j<al_size(func->funcBody);j++){
			block = al_get(func->funcBody, j);
			for(k=0;k<al_size(block->instrs);k++){
				instr = al_get(block->instrs, k);
				if(instr->opCode==JSOP_GETARG || instr->opCode==JSOP_SETARG || instr->opCode==JSOP_CALLARG ||
						instr->opCode==JSOP_ARGINC || instr->opCode==JSOP_INCARG || instr->opCode==JSOP_ARGDEC || instr->opCode==JSOP_DECARG ||
						INSTR_HAS_FLAG(instr, INSTR_IS_LARG_ACCESS)){
					if(instr->operand.i > args)
						args = instr->operand.i;
				}
			}
		}
		func->args = args;
	}

	return functionCFGs;
}

Function *findFunctionByObjAddress(ArrayList *functionCFGs, ADDRESS funcObjAddr){
	assert(functionCFGs);
	Function *func;
	int i;
	for(i=0;i<al_size(functionCFGs);i++){
		func = al_get(functionCFGs, i);
		if(func->funcObj==funcObjAddr){
			return func;
		}
	}
	printf("Error: cannot find function object with id %lx\n", funcObjAddr);
	return NULL;
}


Function *findFunctionByEntryAddress(ArrayList *functionCFGs, ADDRESS addr){
	assert(functionCFGs);
	Function *func;
	int i;
	for(i=0;i<al_size(functionCFGs);i++){
		func = al_get(functionCFGs, i);
		if(func->funcEntryBlock->addr==addr){
			return func;
		}
	}
	printf("Error: cannot find function object with address %lx\n", addr);
	return NULL;
}

Function *findFunctionByEntryBlockID(ArrayList *functionCFGs, int id){
	assert(functionCFGs);
	Function *func;
	int i;
	for(i=0;i<al_size(functionCFGs);i++){
		func = al_get(functionCFGs, i);
		if(func->funcEntryBlock->id==id){
			return func;
		}
	}
	printf("Error: cannot find function object with id %d\n", id);
	return NULL;
}



BasicBlock *isBlockInTheFunction(Function *func, int id){
	int i;
	BasicBlock *block;
	for(i=0;i<al_size(func->funcBody);i++){
		block = al_get(func->funcBody, i);
		if(block->id == id)
			return block;
	}
	return NULL;
}
