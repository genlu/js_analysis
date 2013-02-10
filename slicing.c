#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "slicing.h"
#include "array_list.h"
#include "instr.h"
#include "stack_sim.h"
#include "writeSet.h"


//can't create a (0,0) property
Property *createProperty(ADDRESS obj,long id, char *name){
	assert(obj!=0 && id!=0 && name!=NULL);
	Property *prop = (Property *)malloc(sizeof(Property));
	if(!prop)
		return NULL;
	prop->obj = obj;
	prop->id = id;
	prop->name=NULL;

	prop->name = (char *)malloc(strlen(name)+1);
	assert(prop->name);
	memcpy(prop->name, name, strlen(name)+1);

	return prop;
}


void propFree(void *item) {
	Property *prop = (Property *)item;
    if(prop!=NULL){
    	if(prop->name!=NULL){
    		free(prop->name);
    		prop->name=NULL;
    	}
    	free(prop);
    }
    prop = NULL;
}

void propPrint(void *item) {
	Property *prop = (Property *)item;
    printf("obj: 0x%16lX\tid: %-10ld\t%s\n", prop->obj, prop->id, prop->name?prop->name:"");
}

int propCompare(void *o1, void *o2) {
	Property *prop1 = (Property *)o1;
	Property *prop2 = (Property *)o2;
	assert(prop1 && prop2);

	if(prop1->obj==prop2->obj){
/*		if(prop1->id==prop2->id){
			return 0;
		}else
			return (int)(prop1->id-prop2->id);*/
		return strcmp(prop1->name, prop2->name);
	}else
		return (int)(prop1->obj-prop2->obj);
}



SlicingPropSet *createPropSet(void){
	SlicingPropSet *new = (SlicingPropSet *)malloc(sizeof(SlicingPropSet));
	if(!new)
		return NULL;
	new->propSet = al_newGeneric(AL_LIST_SET, propCompare, propPrint, propFree);
	return new;
}

void destroyPropSet(SlicingPropSet *set){
	if(!set)
		return;
	al_freeWithElements(set->propSet);
	free(set);
	set = NULL;
	return;
}

SlicingState *initSlicingState(InstrList *iList, int order){
	int i;
	Property *propUsed;
	Instruction *instr;
	SlicingState *state = (SlicingState *)malloc(sizeof(SlicingState));
	if(state==0){
		fprintf(stderr, "ERROR: failed to create an state\n");
		assert(0);
	}
	//todo:
	state->stack = initOpStack(NULL);
	if(state->stack==0){
		fprintf(stderr, "ERROR: failed to create an OpStack\n");
		assert(0);
	}
	/*
	 * TODO: need to preset the frameStack according to the order of instruction given
	 * lastFrame has to be NULL
	 */
	i = InstrListLength(iList) - 1;
	while(i>order){
		instr = getInstruction(iList, i);
		// if i is a return instr
		if(INSTR_HAS_FLAG(instr, INSTR_IS_RET)){
			InstrList *newFrame = InstrListCreate();
			pushOpStack(state->stack, (void *)newFrame);
		}
		// if s is a invocation instr
		if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){
			InstrListDestroy((InstrList *)popOpStack(state->stack),0);
		}
		i--;
	}

	state->lastFrame = NULL;
	assert(iList && order>=0 && order<iList->numInstrs);
	state->iList = iList;
	state->currect_instr = order;
	state->propsLive = createPropSet();
	assert(state->propsLive);
	propUsed = getPropUse(state->iList, state->currect_instr);
	if(propUsed!=NULL)
		al_add(state->propsLive->propSet, propUsed);
	state->memLive = getMemUse(state->iList, state->currect_instr);
	assert(state->memLive);
	return state;
}

void destroySlicingState(SlicingState *state){
	if(!state)
		return;
	destroyPropSet(state->propsLive);
	if(!isOpStackEmpty(state->stack)){
		InstrList *iList;
		int i=0;
		while(!isOpStackEmpty(state->stack)){
			i++;
			iList = (InstrList *)popOpStack(state->stack);
			InstrListDestroy(iList, 0);
		}
		if(i>1)
			fprintf(stderr, "WARNING: frame stack has more than one frame left before being destroyed\n");
	}
	destroyOpStack(state->stack);
	WriteSetDestroy(state->memLive);

	free(state);
	state=NULL;
	return;
}

void printSlicingState(SlicingState *state){
	assert(state);
	int i;
	printf("current instr: %d\n", state->currect_instr);
	printf("-- propLive:\n");
	for(i=0;i<state->propsLive->propSet->al_size;i++)
		propPrint(al_get(state->propsLive->propSet, i));
	printf("-- memLive:\n"); PrintWriteSet(state->memLive);
}


void testUD(InstrList *iList, SlicingState *state){
	int i=0;
	Instruction *instr;
	Property *prop1, *prop2;
	while(i<iList->numInstrs){
		instr=getInstruction(iList, i);
		prop1 = getPropUse(iList, i);
		prop2 = getPropDef(iList, i);

		i++;
		if(!prop1 && !prop2)
			continue;
		if(prop1){
			if(!al_contains(state->propsLive->propSet, prop1)){
				al_add(state->propsLive->propSet, prop1);
				printf("dup prop @instr %d\n", i-1);
			}else
				propFree((void *)prop1);
		}
		if(prop2){
			if(!al_contains(state->propsLive->propSet, prop2)){
				al_add(state->propsLive->propSet, prop2);
				printf("dup prop @instr %d\n", i-1);
			}else
				propFree((void *)prop2);
		}
	}
}



#define INCLUDE_DATA_DEP	1
#define INCLUDE_CTRL_DEP	1


#define UDPRINTF(a) 		//printf a

void forwardUDchain(InstrList *iList, int order){
	assert(iList && order>=0 && order<InstrListLength(iList));
	Instruction *instr;
	WriteSet *memLive, *memDef, *memUsed, *memTemp;
	Property *propUsed, *propDef, *propTemp;
	SlicingPropSet *propsLive;
	int add_flag, i;


	propsLive = createPropSet();
	assert(propsLive);
	propDef = getPropDef(iList, order);
	if(propDef!=NULL)
		al_add(propsLive->propSet, propDef);
	memLive = getMemDef(iList, order);
	assert(memLive);

	printf("forward DU chain of instr %d\n", order);

	i = order;
	instr=getInstruction(iList,i);
	//Starting instruction should be included in UD chain
	printInstruction(instr);

	//start from the instruction state->currect_instr-1
	while(++i < InstrListLength(iList)){
		add_flag=0;

		//get previous instruction
		instr=getInstruction(iList,i);

		// get use/def info about instruction i
		memUsed = getMemUse(iList, i);
		memDef = getMemDef(iList, i);
		propUsed = getPropUse(iList, i);
		propDef = getPropDef(iList, i);


		//calculate data dependency of i
		if(WriteSetsOverlap(memLive, memUsed)){
			add_flag++;
		}
		if(propUsed){
			if(al_contains(propsLive->propSet, propUsed)){
				add_flag++;
			}
		}

		if(add_flag){
			printInstruction(instr);
			//add all xxxDef into live set
			MergeWriteSets(memLive, memDef);
            if(propDef)
            	al_add(propsLive->propSet, propDef);
		}
		else{
			//remove xxxDef from live set
			memTemp = SubtractWriteSets(memLive, memDef);
            WriteSetDestroy(memLive);
            memLive = NULL;
            memLive = memTemp;
            if(propDef){
            	if(al_contains(propsLive->propSet, propDef)){
            		propTemp = al_remove(propsLive->propSet, propDef);
            		propFree(propTemp);
            	}
            }
		}
		WriteSetDestroy(memUsed);
		WriteSetDestroy(memDef);
	}
}

void markUDchain(InstrList *iList, SlicingState *state, uint32_t flag){
	assert(iList && state);
	Instruction *instr;
	WriteSet *memDef, *memUsed, *memTemp;
	Property *propUsed, *propDef, *propTemp;
	InstrList *currentFrame;
	int add_flag;



	currentFrame = (InstrList *)peekOpStack(state->stack);
	assert(currentFrame);
	int *i = &(state->currect_instr);
	instr=getInstruction(iList,(*i));
	//Starting instruction should be included in UD chain
	INSTR_SET_FLAG(instr, flag);

	/*
	 * to make decompiler work correctly, we have to
	 * include all remaining none branch instrs in this
	 * block into the slice but we DON'T use them to do the slicing!
	 */
	//xxx don't need this step, since we don't decompile the slice anymore
	/*
	int tempInt = *i;
	//while(!INSTR_HAS_FLAG(instr, INSTR_IS_BBL_END)){
	while(!INSTR_HAS_FLAG(instr, INSTR_IS_BBL_END)){
		//printf("lsls: %d\n", tempInt);
		instr=getInstruction(iList,++tempInt);
		if(!isBranch(iList, instr) && !isInvokeInstruction(iList, instr)){
			INSTR_SET_FLAG(instr, flag);
		}else{
			break;
		}
	}
	*/

	instr=getInstruction(iList,(*i));

	// put instr into currentFrame
	InstrListAdd(currentFrame, instr);
	//printf("Mark UD for instr#%d\n", (*i));

	//start from the instruction state->currect_instr-1
	while((*i)-- > 0){
		add_flag=0;

		//get previous instruction
		instr=getInstruction(iList,(*i));

		//printf("processing instr %d\n", instr->order);

/*		if(isInvokeInstruction(iList, instr)){
			printf("invoke: %d\t", instr->order);
			if(isNativeInvokeInstruction(iList, instr))
				printf("***");
			printf("\n");
		}else if(isRetInstruction(iList, instr)){
			printf("ret: %d\n", instr->order);
		}*/

		// get use/def info about instruction i
		memUsed = getMemUse(iList, (*i));
		memDef = getMemDef(iList, (*i));
		propUsed = getPropUse(iList, (*i));
		propDef = getPropDef(iList, (*i));

		// if i is a return instr
		if(INSTR_HAS_FLAG(instr, INSTR_IS_RET)){
			InstrList *newFrame = InstrListCreate();
			pushOpStack(state->stack, (void *)newFrame);
			//INSTR_SET_FLAG(instr, INSTR_IS_RET);
		}

		// if s is a invocation instr
		if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) || INSTR_HAS_FLAG(instr, INSTR_IS_NATIVE_INVOKE)){
			if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){
				UDPRINTF(("script invoke #%d\n", instr->order));
				state->lastFrame = (InstrList *)popOpStack(state->stack);
				assert(state->lastFrame);
			}
		}else{
			if(state->lastFrame!=NULL){
				InstrListDestroy(state->lastFrame,0);
				state->lastFrame = NULL;
			}
		}

		currentFrame = peekOpStack(state->stack);
		assert(currentFrame);


#if INCLUDE_CTRL_DEP
		// calculate control dependency of i
		//1. inter-procedural control dependency (ignore 'eval')
		if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){
			assert(state->lastFrame);
			if(InstrListLength(state->lastFrame)>0){
				if(instr->opCode!=JSOP_EVAL&&!INSTR_HAS_FLAG(instr, INSTR_IS_DOC_WRITE)){
					UDPRINTF(("adding %d as a control dep(invoke)\n", instr->order));
					int in;
					for(in=0;in<InstrListLength(state->lastFrame);in++){
						UDPRINTF(("#%d ", getInstruction(state->lastFrame,in)->order));
					}
					UDPRINTF(("\n"));
					add_flag++;
					//put instr into currentFrame
					InstrListAdd(currentFrame, instr);
				}else
					INSTR_SET_FLAG(instr,INSTR_EVAL_AFFECT_SLICE);
			}
		}
		//2. intra-procedural control dependency
		if(INSTR_HAS_FLAG(instr,INSTR_IS_1_BRANCH)||
				INSTR_HAS_FLAG(instr,INSTR_IS_2_BRANCH)||
				INSTR_HAS_FLAG(instr,INSTR_IS_N_BRANCH)){
			//printf("instr:%lx\tnextBlock:%lx\n", instr->addr, instr->nextBlock );
			int nextBlock = instr->nextBlock->id;
			//printf("nextBlock: %lx\n");
			assert(nextBlock>=0);
			int jj;
			for(jj=0;jj<InstrListLength(currentFrame);jj++){
				Instruction *tempInstr = getInstruction(currentFrame, jj);
				if(tempInstr->inBlock->id == nextBlock){
					if(!add_flag)
						UDPRINTF(("adding %d as a control dep(jump)\n", instr->order));
					add_flag++;
					InstrListRemove(currentFrame, tempInstr);
					jj--;
				}
			}
		}

#endif	//end #if INCLUDE_CTRL_DEP



#if INCLUDE_DATA_DEP
		//calculate data dependency of i
		if(WriteSetsOverlap(state->memLive, memDef)){
			add_flag++;
			memTemp = SubtractWriteSets(state->memLive, memDef);
            WriteSetDestroy(state->memLive);
            state->memLive = NULL;
            state->memLive = memTemp;
            UDPRINTF(("adding %d as a data dep\n", instr->order));
		}
		if(propDef){
			//instr i define some property in liveSet, remove it from liveSet
			if(al_contains(state->propsLive->propSet, propDef)){
				add_flag++;
				propTemp = al_remove(state->propsLive->propSet, propDef);
				propFree(propTemp);
				UDPRINTF(("adding %d as a data dep\n", instr->order));
			}
		}
#endif   //end #if INCLUDE_DATA_DEP

		if(add_flag){
			//put this instr in UD chain
			INSTR_SET_FLAG(instr, flag);
			// put instr into currentFrame
			InstrListAdd(currentFrame, instr);

			state->memLive = MergeWriteSets(state->memLive, memUsed);
			if(propUsed){
				if(!al_contains(state->propsLive->propSet, propUsed)){
					al_add(state->propsLive->propSet, propUsed);
				}else
					propFree(propUsed);
			}
		}
		// free propUsed if it's not add the the liveSet
		else
			propFree(propUsed);
		//end of data dependency calculation

		propFree(propDef);
		WriteSetDestroy(memUsed);
		WriteSetDestroy(memDef);
	}
}

void checkSlice(InstrList *iList){

	int i,j;
	Instruction *instr, *instr2;

	for(i=0;i<InstrListLength(iList);i++){
		instr = getInstruction(iList, i);
		INSTR_CLR_FLAG(instr, INSTR_FLAG_TMP0);
	}
	//this is only for recovering source code!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//extra instruction will be included in the dynamic slice!!!!!!!!!!!!
	//BE CAREFUL XXX
	for(i=0;i<InstrListLength(iList);i++){
		instr = getInstruction(iList, i);
		if(INSTR_HAS_FLAG(instr, INSTR_IN_SLICE) && !INSTR_HAS_FLAG(instr, INSTR_FLAG_TMP0) ){
			for(j=0;j<InstrListLength(iList);j++){
				instr2 = getInstruction(iList, j);
				if(instr2->addr == instr->addr){
					INSTR_SET_FLAG(instr2, INSTR_IN_SLICE);
					INSTR_SET_FLAG(instr2, INSTR_FLAG_TMP0);
				}
			}
		}
	}
}


#define DEOBFSLICING_PRINT 0
void deobfSlicing(InstrList *iList){

	int i;
	Instruction *instr;

	//1. label all the native calls contribute to eval'ed/document.write() string using INSTR_ON_EVAL flag

	 for(i=InstrListLength(iList)-1;i>=0;i--){
		instr = getInstruction(iList, i);
		if(instr->opCode!=JSOP_EVAL && !(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) && INSTR_HAS_FLAG(instr, INSTR_IS_DOC_WRITE))){
			//printf("#%d is INSTR_ON_EVAL\n", instr->order);
			continue;
		}
	    SlicingState *state = initSlicingState(iList,i);
	    markUDchain(iList, state, INSTR_ON_EVAL);	//it doestn't matter that markUDchian does't trace on eval()
	    											//since we are actually tracing back on each eval() in this loop
	    destroySlicingState(state);
	}
#if DEOBFSLICING_PRINT
	printf("\n*********************************************************************************\n");
	printf("instrs after INSTR_ON_EVAL\n");
	printf("*********************************************************************************\n");
    printInstrList(iList);

    printf("###############################slicing#################################\n");
#endif

	//2. d-slicing on all native calls not labeled by INSTR_ON_EVAL
    //right now, we don't slicing on document.write()
	for(i=InstrListLength(iList)-1;i>=0;i--){
		instr = getInstruction(iList, i);
		if(!INSTR_HAS_FLAG(instr, INSTR_IS_NATIVE_INVOKE) || INSTR_HAS_FLAG(instr, INSTR_ON_EVAL) || INSTR_HAS_FLAG(instr, INSTR_IS_DOC_WRITE))
			continue;
#if DEOBFSLICING_PRINT
		printf("d-slicing on instr# %d\n", i);
#endif
	    SlicingState *state = initSlicingState(iList,i);
	    markUDchain(iList, state, INSTR_IN_SLICE);
	    destroySlicingState(state);
	}
#if DEOBFSLICING_PRINT
	printf("\n*********************************************************************************\n");
	printf("instrs after INSTR_IN_SLICE\n");
	printf("*********************************************************************************\n");
    printInstrList(iList);
#endif
    //printInstrList(iList);
    checkSlice(iList);
}


/*
 * for each function, find its starting addr (the addr of its first instr in slice)
 * based on the fact that the very first instruction in the function  we encountered in the slice
 * has to be the 1st instruction in that function.
 */
ArrayList *findFuncStartInstrsInSlice(InstrList *iList){
	int i, size;
	Instruction *instr, *lastInstr;
	ArrayList *funcObjList = al_newInt(AL_LIST_SET);
	ArrayList *retList = al_newInt(AL_LIST_SET);

	size = InstrListLength(iList);
	lastInstr=NULL;
	for(i=0;i<size;i++){
		instr = getInstruction(iList, i);
		if(!INSTR_HAS_FLAG(instr, INSTR_IN_SLICE))
			continue;
		if(!lastInstr){
			al_add(funcObjList, (void *) instr->inFunction);
			al_add(retList, (void *) instr->addr);
			lastInstr = instr;
			continue;
		}
		if(lastInstr->inFunction!= instr->inFunction){
			if(!al_contains(funcObjList, (void *) instr->inFunction)){
				al_add(funcObjList, (void *) instr->inFunction);
				al_add(retList, (void *) instr->addr);
			}
			lastInstr = instr;
			continue;
		}
	}
	al_free(funcObjList);

	printf("func Start Instrs In Slice:\t");
	for(i=0;i<al_size(retList);i++){
		ADDRESS temp = (ADDRESS)al_get(retList,i);
		printf("%lx\t", temp);
	}
	printf("\n");

	return retList;
}


























void backwardSlicing(InstrList *iList, int order, ArrayList *blocksList, uint32_t flag){

	assert(iList && blocksList);
	Instruction *instr;
	WriteSet *memDef, *memUsed, *memTemp;
	Property *propUsed, *propDef, *propTemp;
	InstrList *currentFrame;
	SlicingState *state;
	int add_flag;


	/*
	 * create a slicingState for backward slicing
	 */
    state = initSlicingState(iList,order);

	currentFrame = (InstrList *)peekOpStack(state->stack);
	assert(currentFrame);
	int *i = &(state->currect_instr);
	instr=getInstruction(iList,(*i));
	//Starting instruction should be included in slice
	INSTR_SET_FLAG(instr, flag);


	instr=getInstruction(iList,(*i));

	// put instr into currentFrame
	InstrListAdd(currentFrame, instr);
	//printf("Mark backward slice for instr#%d\n", (*i));

	//start from the instruction state->currect_instr-1
	while((*i)-- > 0){
		add_flag=0;

		//get previous instruction
		instr=getInstruction(iList,(*i));

		//printf("processing instr %d\n", instr->order);

/*		if(isInvokeInstruction(iList, instr)){
			printf("invoke: %d\t", instr->order);
			if(isNativeInvokeInstruction(iList, instr))
				printf("***");
			printf("\n");
		}else if(isRetInstruction(iList, instr)){
			printf("ret: %d\n", instr->order);
		}*/

		// get use/def info about instruction i
		memUsed = getMemUse(iList, (*i));
		memDef = getMemDef(iList, (*i));
		propUsed = getPropUse(iList, (*i));
		propDef = getPropDef(iList, (*i));

		// if i is a return instr
		if(INSTR_HAS_FLAG(instr, INSTR_IS_RET)){
			InstrList *newFrame = InstrListCreate();
			pushOpStack(state->stack, (void *)newFrame);
		}

		// if s is a invocation instr
		if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) || INSTR_HAS_FLAG(instr, INSTR_IS_NATIVE_INVOKE)){
			if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){
				UDPRINTF(("script invoke #%d\n", instr->order));
				state->lastFrame = (InstrList *)popOpStack(state->stack);
				assert(state->lastFrame);
			}
		}else{
			if(state->lastFrame!=NULL){
				InstrListDestroy(state->lastFrame,0);
				state->lastFrame = NULL;
			}
		}

		currentFrame = peekOpStack(state->stack);
		assert(currentFrame);

		// calculate control dependency of i
		//1. inter-procedural control dependency
		if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){
			assert(state->lastFrame);
			if(InstrListLength(state->lastFrame)>0){
//				if(instr->opCode!=JSOP_EVAL&&!INSTR_HAS_FLAG(instr, INSTR_IS_DOC_WRITE)){
					UDPRINTF(("adding %d as a control dep(invoke)\n", instr->order));
					int in;
					for(in=0;in<InstrListLength(state->lastFrame);in++){
						UDPRINTF(("#%d ", getInstruction(state->lastFrame,in)->order));
					}
					UDPRINTF(("\n"));
					add_flag++;
					//put instr into currentFrame
					InstrListAdd(currentFrame, instr);
//				}else
//					INSTR_SET_FLAG(instr,INSTR_EVAL_AFFECT_SLICE);
			}
		}
		//2. intra-procedural control dependency
		if(	INSTR_HAS_FLAG(instr,INSTR_IS_2_BRANCH)||
				INSTR_HAS_FLAG(instr,INSTR_IS_N_BRANCH)){
			//printf("instr:%lx\tnextBlock:%lx\n", instr->addr, instr->nextBlock );
			int jj;
			for(jj=0;jj<InstrListLength(currentFrame);jj++){
				Instruction *tempInstr = getInstruction(currentFrame, jj);
				assert(tempInstr->inBlock->reverseDomFrontier);
				if(findBasicBlockFromListByID(tempInstr->inBlock->reverseDomFrontier, instr->inBlock->id)){
					if(!add_flag)
						UDPRINTF(("adding %d as a control dep\n", instr->order));
					add_flag++;
					InstrListRemove(currentFrame, tempInstr);
					jj--;
				}
			}
		}

		//calculate data dependency of i
		if(WriteSetsOverlap(state->memLive, memDef)){
			add_flag++;
			memTemp = SubtractWriteSets(state->memLive, memDef);
            WriteSetDestroy(state->memLive);
            state->memLive = NULL;
            state->memLive = memTemp;
            UDPRINTF(("adding %d as a data dep\n", instr->order));
		}
		if(propDef){
			//instr i define some property in liveSet, remove it from liveSet
			if(al_contains(state->propsLive->propSet, propDef)){
				add_flag++;
				propTemp = al_remove(state->propsLive->propSet, propDef);
				propFree(propTemp);
				UDPRINTF(("adding %d as a data dep\n", instr->order));
			}
		}

		if(add_flag){
			//put this instr in slice
			INSTR_SET_FLAG(instr, flag);
			// put instr into currentFrame
			InstrListAdd(currentFrame, instr);

			state->memLive = MergeWriteSets(state->memLive, memUsed);
			if(propUsed){
				if(!al_contains(state->propsLive->propSet, propUsed)){
					al_add(state->propsLive->propSet, propUsed);
				}else
					propFree(propUsed);
			}
		}
		// free propUsed if it's not add the the liveSet
		else
			propFree(propUsed);
		//end of data dependency calculation

		propFree(propDef);
		WriteSetDestroy(memUsed);
		WriteSetDestroy(memDef);
	}
}





void forwardSlicing(InstrList *iList, int order, ArrayList *blocksList, uint32_t flag){


	//need a list for possible control dependency blocks for coming blocks

	//a new list is required for each function invocation, equivalent to the frameStack for instr

	//when a CFG transition occurs, any block in the list is removed if it is control dependent on the new block

	// and the new block is added to the list if the block is a branch block  && the branch instr is in slice
	//    which means the block is added to list only the we encounter its last instr

	//control dep:
	//for every instr, check the list to see if the instr is dependent on any of the blocks
	//label instr if it is.

	//data dep:
	//same UD computation as before
}

















