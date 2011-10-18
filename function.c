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

static int anonfunobjctl=0;
static int funobjtableentryctl=0;

char *anonfunobj_str = "anonfunobj";

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
	if(func!=NULL){
		if(((Function *)func)->funcObj)
			al_free(((Function *)func)->funcObj);
		free((Function *)func);
	}
}

Function *createFunctionNode(void){
	Function *func = (Function *)malloc(sizeof(Function));
	assert(func);
	func->args = 0;
	func->flags = 0;
	func->funcEntryAddr = 0;
	func->funcObj = al_newInt(AL_LIST_SET);;
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
						//printf("@@@@@ b1:%d\tb2:%d\tb3:%d\tb4:%d\n", block1->id, block2->id, block3->id, block4->id);
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
				//printf("dealing: %d\t block1:%d\t block3:%d\n", n->id, block1->id, block3->id);
				doSearch(blockList, block3, flag, functionCFGs);
			}
			assert(al_size(block1->preds)==0);

			func = createFunctionNode();
			func->funcEntryAddr = block1->addr;
			func->funcEntryBlock = block1;

			if(al_contains(functionCFGs, (void *)func))
				destroyFunctionNode((void *)func);
			else
				al_add(functionCFGs, (void *)func);

			//printBasicBlockList(blockList);

			doSearch(blockList, block1, flag, functionCFGs);

			//doSearch(blockList, block3, flag, functionCFGs);
		}
	}
	else{
		for(i=0;i<al_size(n->succs);i++){
			edge = (BlockEdge *)al_get(n->succs, i);
			block1 = edge->head;
			if(!BBL_HAS_FLAG(block1, flag)){
				//EDGE_SET_FLAG(edge, EDGE_IN_DFST);
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
	block1->lastInstr = block2->lastInstr;
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



void doSearchFuncBody(Function *func, BasicBlock *n, ArrayList *funcBody, uint32_t flag){
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
			doSearchFuncBody(func, block, funcBody, flag);
		}
	}
}


void traverseFuncBody(Function *func, BasicBlock *n, uint32_t flag){
	int i;
	BlockEdge *edge;
	BasicBlock *block;
	assert(n);
	BBL_SET_FLAG(n, flag);

	n->inFunction = func->funcEntryAddr;

	for(i=0;i<al_size(n->succs);i++){
		edge = al_get(n->succs, i);
		block = edge->head;
		if(!BBL_HAS_FLAG(block, flag)){
			traverseFuncBody(func, block, flag);
		}
	}
}

/*
 *	buildFunctionCFGs() rearrange CFG specified by 'blockList',
 *	"cut out" all the functions, and connect call-site and return-site (adjacent instructions)
 *	with an 'EDGE_IS_CALLSITE' edge. it also mark entry and return node of functions
 *	Finally, a list of Function nodes is returned (program entry is not considered as a function)
 */
ArrayList *buildFunctionCFGs(InstrList *iList, ArrayList *blockList, ArrayList **funcObjTable){
	int i,j,k;
	BasicBlock *block;
	BlockEdge *edge;
	BasicBlock *n0;
	ArrayList *functionCFGs;
	Instruction *instr;

	functionCFGProcessed++;

	functionCFGs = al_newGeneric(AL_LIST_SET, FuncEntryBlkIDCompare, NULL, destroyFunctionNode);
	assert(functionCFGs);

	n0=NULL;
	//make sure flag TMP0 is cleared
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
		if(BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE))
			n0 = block;
	}

	//printf("n0: %lx\n", n0);
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
					edge=NULL;
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
		doSearchFuncBody(func, func->funcEntryBlock, func->funcBody, BBL_FLAG_TMP0);
	}
	//print all function info
	printf("\nFunctionCFGs:\t");
	printf("%d\n", al_size(functionCFGs));
	for(i=0;i<al_size(functionCFGs);i++){
		func = al_get(functionCFGs, i);
		printf("func %s\t", func->funcName);
		printf("func_entry_addr %lx\t", func->funcEntryAddr);
		for(j=0;j<al_size(func->funcBody);j++){
			printf("%d  ",((BasicBlock*)al_get(func->funcBody, j))->id);
		}
		printf("\n");
	}
	printf("**********************\n");
	//clear flag TMP0
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}

	//try to find the total number of arguments of each function
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


	//create funcObjTable first, this is needed for slice processing
    *funcObjTable = constructFuncObjTable(iList, functionCFGs);
	printFuncObjTable(*funcObjTable);

	//fill the block->inFunction field (which is from func_obj)
	for(i=0;i<al_size(functionCFGs);i++){
		func = al_get(functionCFGs, i);
		traverseFuncBody(func, func->funcEntryBlock, BBL_FLAG_TMP0);
	}
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}
	//for each instr, find which function this instr is in. (0 means not in any function, or in main procedure )
	for(i=0;i<InstrListLength(iList);i++){
		instr = getInstruction(iList, i);
		instr->inFunction = instr->inBlock->inFunction;
	}


	return functionCFGs;
}

Function *findFunctionByObjAddress(ArrayList *functionCFGs, ADDRESS funcObjAddr){
	assert(functionCFGs);
	Function *func;
	ADDRESS funcObj;
	int i,j;
	for(i=0;i<al_size(functionCFGs);i++){
		func = al_get(functionCFGs, i);
		for(j=0;j<al_size(func->funcObj);j++){
			funcObj = (ADDRESS) al_get(func->funcObj, j);
			if(funcObj==funcObjAddr){
				return func;
			}
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




FuncObjTableEntry *createFuncObjTableEntry(void){
	FuncObjTableEntry *entry;
	entry = (FuncObjTableEntry *)malloc(sizeof(FuncObjTableEntry));
	assert(entry);
	entry->id = funobjtableentryctl++;
	entry->flags = 0;
	entry->func_name = NULL;
	entry->func_addr = 0;
	entry->anonfunobj_instr_addr = 0;
	entry->func_struct = NULL;
	entry->func_objs = al_newInt(AL_LIST_SET);
	return entry;
}

void destroyFuncObjTableEntry(void *e){
	assert(e);
	FuncObjTableEntry *entry = (FuncObjTableEntry *)e;
	if(entry->func_name){
		free(entry->func_name);
		entry->func_name = NULL;
	}
	if(entry->func_objs){
		al_free(entry->func_objs);
		entry->func_objs=NULL;
	}
	free(entry);
}

//function's starting address is unique, rather than the function's obj address (i.e. anonymous functions)
int FuncObjTableEntryCompare(void *a1, void *a2){
	assert(a1 && a2);
	return ((FuncObjTableEntry *)a1)->func_addr - ((FuncObjTableEntry *)a2)->func_addr;
}

//find function table entry by address
FuncObjTableEntry *findFuncObjTableEntryByFuncAddr(ArrayList *FuncObjTable, ADDRESS funcAddr){
	assert(FuncObjTable);
	assert(funcAddr);
	int i;
	FuncObjTableEntry *entry;
	for(i=0;i<al_size(FuncObjTable);i++){
		entry = (FuncObjTableEntry*)al_get(FuncObjTable, i);
		if(entry->func_addr == funcAddr){
			return entry;
		}
	}
	return NULL;
}

//find function table entry by address
FuncObjTableEntry *findFuncObjTableEntryByFuncObj(ArrayList *FuncObjTable, ADDRESS funcObj){
	assert(FuncObjTable);
	assert(funcObj);
	int i,j;
	ADDRESS tempAddr = 0;
	FuncObjTableEntry *entry = NULL;
	for(i=0;i<al_size(FuncObjTable);i++){
		entry = al_get(FuncObjTable, i);
		for(j=0;j<al_size(entry->func_objs);j++){
			tempAddr = (ADDRESS)al_get(entry->func_objs, j);
			if(tempAddr == funcObj){
				return entry;
			}
		}
	}
	return NULL;
}

void printFuncObjTableEntry(FuncObjTableEntry *entry){
	int i;
	printf("id: %d\tfuncName: %16s\tfunc_addr:%lx\ttargetBlock:%d\n", entry->id,
			entry->func_name?entry->func_name:"(null)", entry->func_addr, entry->func_struct?entry->func_struct->funcEntryBlock->id:-1);
	printf("\t-- func_objs:\t");
	for(i=0;i<al_size(entry->func_objs);i++){
		printf("  %lx  ", (ADDRESS)al_get(entry->func_objs, i));
	}
	printf("\n");
}

void printFuncObjTable(ArrayList *funcObjTable){
	assert(funcObjTable);
	int i;
	FuncObjTableEntry *entry;
	printf("\nFuncObj Table:\n");
	for(i=0;i<al_size(funcObjTable);i++){
		entry = al_get(funcObjTable, i);
		printFuncObjTableEntry(entry);
	}
	printf("\n");
}


/*******************************************************************/

/*
 * go though the trace to create functionObjTable.
 * function def instructions (JSOP_DEFFUN, JSOP_CLOSURE) will add a new entry in table, fill in the name and funcObj
 * 		JSOP_ANONFUNOBJ will check if the function is already added (by using instr's addr) before create a new entry
 * Function calling instructions (JSOP_NEW & JSOP_CALL) will find the existing function entry,
 * 		and fill in the the address of function's first instruction ()
 */
ArrayList *constructFuncObjTable(InstrList *iList, ArrayList *funcCFGs){

	anonfunobjctl = 0;

#define TABLE_DEBUG 0

	int i,j, size;
	char *str=NULL;
	Instruction *instr=NULL;
	Instruction *next=NULL;
	Function *funcStruct=NULL;
	FuncObjTableEntry *funcObjTableEntry1=NULL;
	ArrayList *funcObjTable;

	funcObjTable = al_newGeneric(AL_LIST_UNSORTED, NULL, NULL, destroyFuncObjTableEntry);

	size = InstrListLength(iList);
	for(i=0;i<size;i++){
		instr = getInstruction(iList, i);
		if(i<size-1)
			next = getInstruction(iList, i+1);
		else
			next = NULL;
		//function calling instrs
		if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) && (instr->opCode == JSOP_NEW || instr->opCode == JSOP_CALL)){
			assert(next);

#if TABLE_DEBUG
		fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~instr: %lx\tfind func instr->objRef: %lx\n", instr->addr, instr->objRef);
#endif

			funcStruct = findFunctionByEntryAddress(funcCFGs, next->addr);
			funcObjTableEntry1 = findFuncObjTableEntryByFuncObj(funcObjTable, instr->objRef);
			assert(funcObjTableEntry1);
			assert(funcStruct);
			assert(funcObjTableEntry1->func_name);

			if(funcObjTableEntry1->func_struct!=NULL){
				assert(funcObjTableEntry1->func_struct==funcStruct);
			}else{
				funcObjTableEntry1->func_struct = funcStruct;
			}
			if(funcObjTableEntry1->func_addr!=0){
				assert(funcObjTableEntry1->func_addr==next->addr);
			}else{
				funcObjTableEntry1->func_addr = next->addr;
			}
			if(funcStruct->funcName==NULL){
				funcStruct->funcName = (char *)malloc(strlen(funcObjTableEntry1->func_name)+1);
				assert(funcStruct->funcName);
				memcpy(funcStruct->funcName, funcObjTableEntry1->func_name, strlen(funcObjTableEntry1->func_name)+1);
				al_add(funcStruct->funcObj,(void *)instr->objRef);
			}else{
				assert(!strcmp(funcStruct->funcName, funcObjTableEntry1->func_name));
			}
		}
		//eval, do nothing, just set flag (since eval'ed code doesn't need a name)
		else if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) && (instr->opCode == JSOP_EVAL)){
			assert(next);
			funcStruct = findFunctionByEntryAddress(funcCFGs, next->addr);
			if(funcStruct)
				BBL_SET_FLAG(funcStruct->funcEntryBlock, BBL_IS_EVAL_ENTRY);
			else
				assert(0);
		}
		//named function def
		else if(instr->opCode==JSOP_DEFFUN || instr->opCode==JSOP_CLOSURE){
#if TABLE_DEBUG
		fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~instr: %lx\tadd func %s instr->objRef: %lx\n", instr->addr,
				instr->str?instr->str:"(null)", instr->objRef);
#endif
			str = (char *)malloc(strlen(instr->str)+1);
			assert(str);
			memcpy(str, instr->str, strlen(instr->str)+1);
			funcObjTableEntry1 = createFuncObjTableEntry();
			funcObjTableEntry1->func_name = str;
			al_add(funcObjTableEntry1->func_objs, (void *)(instr->objRef));
			assert(!findFuncObjTableEntryByFuncObj(funcObjTable, instr->objRef));
			al_add(funcObjTable, funcObjTableEntry1);
			str=NULL;
#if TABLE_DEBUG
		printFuncObjTable(funcObjTable);
#endif
		}
		//anonymous
		else if(instr->opCode==JSOP_ANONFUNOBJ){
			//first, check if there's any entry for the same anonymous function already been added
			//we have to use anonfunobj instruction's addr since every time this instr will use a different obj_ref
			for(j=0;j<al_size(funcObjTable);j++){
				funcObjTableEntry1 = (FuncObjTableEntry *)al_get(funcObjTable, j);
				if(FE_HAS_FLAG(funcObjTableEntry1, FE_IS_ANONFUNOBJ) && funcObjTableEntry1->anonfunobj_instr_addr==instr->addr){
					break;
				}
				funcObjTableEntry1=NULL;
			}
			//if we find one, then just add the obj_entry in to the list
			if(funcObjTableEntry1){
#if TABLE_DEBUG
		fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~instr: %lx\tadd anon-func instr->objRef: %lx(only obj_ref)\n", instr->addr, instr->objRef);
		fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~ to entry %d\n", funcObjTableEntry1->id);
#endif
				al_add(funcObjTableEntry1->func_objs, (void *)(instr->objRef));
#if TABLE_DEBUG
		printFuncObjTable(funcObjTable);
#endif
			}
			//otherwise, create and add the new entry (and create a name for the function)
			else{
#if TABLE_DEBUG
		fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~instr: %lx\tadd anon-func #%d instr->objRef: %lx\n", instr->addr, anonfunobjctl, instr->objRef);
#endif
				str = (char *)malloc(strlen(anonfunobj_str)+5);
				assert(str);
				sprintf(str, "%s%d", anonfunobj_str, anonfunobjctl++);
				funcObjTableEntry1 = createFuncObjTableEntry();
				funcObjTableEntry1->func_name = str;
				funcObjTableEntry1->anonfunobj_instr_addr = instr->addr;
				al_add(funcObjTableEntry1->func_objs, (void *)(instr->objRef));
				assert(!findFuncObjTableEntryByFuncObj(funcObjTable, instr->objRef));
				FE_SET_FLAG(funcObjTableEntry1, FE_IS_ANONFUNOBJ);
				al_add(funcObjTable, funcObjTableEntry1);
				str=NULL;
#if TABLE_DEBUG
		printFuncObjTable(funcObjTable);
#endif
			}
		}
	}//end for-loop
	return funcObjTable;
}

