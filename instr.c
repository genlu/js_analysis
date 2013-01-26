#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "instr.h"
#include "slicing.h"
#include "stack_sim.h"


Instruction *InstrCreate(void) {
    Instruction *new = (Instruction *)malloc(sizeof(*new));
    new->flags = 0;
    new->addr = 0;;
    new->length = 0;
    new->str = NULL;
    new->eval_str = NULL;
    new->objRef = 0;
    new->opName[0]='\0';
    new->operand.i = -1;
    new->stackUse = 0;
    new->stackUseStart = 0;
    new->stackDef = 0;
    new->stackDefStart = 0;
    new->propUseScope = 0;
    new->propUseId = 0;
    new->propDefScope = 0;;
    new->propDefId = 0;
    new->localUse = 0;
    new->localDef = 0;
    new->localDef2 = 0;
    new->jmpOffset = 0;
    new->nvars = 0;
    new->nextBlock= NULL;
    new->inBlock = NULL;
    new->inFunction = 0;
    new->inCallee = 0;
    return (new);
}

/*
 * NOTE: using strcmp instead of compare opcode directly is beacause
 * those functions might be called in GetInstrFromText(), in which opcode
 * has not been filled.
 */


void InstrDestroy(Instruction *instr) {
	if(!strcmp(instr->opName, "string")||!strcmp(instr->opName, "regexp")){
		free(instr->operand.s);
		instr->operand.s = NULL;
	}else if(!strcmp(instr->opName, "eval")){
		free(instr->eval_str);
		instr->eval_str = NULL;
	}
/*	if(instr->str){
		free(instr->str);
		instr->str=NULL;
	}*/
    free(instr);
    instr = NULL;
}

Instruction *getInstruction(InstrList *iList, int order){
	assert(iList);
	if(order<0 || order>=iList->numInstrs)
		return NULL;
	return (Instruction *) al_get(iList->iList, order);
}

InstrList *InstrListCreate(void){
	InstrList *iList = malloc(sizeof(InstrList));
	if(!iList)
		return NULL;
	iList->iList = al_new();
	iList->numInstrs = 0;
	return iList;
}

InstrList *InstrListClone(InstrList *iList, uint32_t flag){
	int i;
	int newIndex;
	Instruction *instr, *newInstr;
	InstrList *newList = InstrListCreate();
	assert(iList && newList);
	newIndex = 0;
	for(i=0;i<InstrListLength(iList);i++){
		instr = getInstruction(iList, i);
		if(INSTR_HAS_FLAG(instr, flag)){
			newInstr = (Instruction *)malloc(sizeof(Instruction));
			assert(newInstr);
			memcpy((void *)newInstr, (void *)instr, sizeof(Instruction));
			if(!strcmp(instr->opName, "string")||!strcmp(instr->opName, "regexp")){
				newInstr->operand.s = (char *)malloc(strlen(instr->operand.s)+1);
				assert(newInstr->operand.s);
				memcpy(newInstr->operand.s, instr->operand.s, strlen(instr->operand.s)+1);
			}else if(!strcmp(instr->opName, "eval")){
				newInstr->eval_str =  (char *)malloc(strlen(instr->eval_str)+1);
				assert(newInstr->eval_str);
				memcpy(newInstr->eval_str, instr->eval_str, strlen(instr->eval_str)+1);

			}
			if(instr->str){
				newInstr->str = (char *)malloc(strlen(instr->str)+1);
				assert(newInstr->str);
				memcpy(newInstr->str, instr->str, strlen(instr->str)+1);
			}
			newInstr->order = newIndex++;
			newInstr->flags = instr->flags;//0;
			//clear block related flags
			INSTR_CLR_FLAG(newInstr, INSTR_IN_SLICE);
			INSTR_CLR_FLAG(newInstr, INSTR_IS_BBL_START);
			INSTR_CLR_FLAG(newInstr, INSTR_IS_BBL_END);

			newInstr->inBlock = NULL;
			newInstr->nextBlock = NULL;
			InstrListAdd(newList, newInstr);
		}
	}
	return newList;
}

int InstrListLength(InstrList *iList){
	assert(iList);
	return iList->numInstrs;
}

void InstrListAdd(InstrList *iList, Instruction *instr){
	assert(iList && instr);
    iList->numInstrs++;
    al_add(iList->iList, instr);
}

Instruction *InstrListRemove(InstrList *iList, Instruction *instr){
	Instruction *ret = (Instruction *)(al_remove(iList->iList, instr));
	iList->numInstrs--;
	return ret;
}

void InstrListDestroy(InstrList *iList, int clearElem){
	if(clearElem)
		al_freeWithElements(iList->iList);
	else
		al_free(iList->iList);
	free(iList);
}

Property *getPropUse(InstrList *iList, int order){
	Property *prop;
	Instruction *instr = getInstruction(iList, order);
	if(!instr)
		return NULL;
	//for (0,0) property
	if(!(instr->propUseScope && instr->propUseId))
		return NULL;
	prop = createProperty(instr->propUseScope, instr->propUseId, instr->str);
	return prop;
}

Property *getPropDef(InstrList *iList, int order){
	Property *prop;
	Instruction *instr = getInstruction(iList, order);
	if(!instr)
		return NULL;
	//for (0,0) property
	if(!(instr->propDefScope && instr->propDefId))
		return NULL;
	prop = createProperty(instr->propDefScope, instr->propDefId, instr->str);
	return prop;
}

WriteSet *getMemUse(InstrList *iList, int order){
	WriteSet *ws;
	Range *s_range = NULL, *l_range = NULL;
	Instruction *instr = getInstruction(iList, order);
	if(!instr)
		return NULL;
	//stack use is not NULL
	if(instr->stackUse && instr->stackUseStart){
		s_range = RangeCreate(instr->stackUseStart, instr->stackUseStart+ PTRSIZE*instr->stackUse-1);
		assert(s_range);
	}
	//local use is not null
	if(instr->localUse){
		l_range = RangeCreate(instr->localUse, instr->localUse+ PTRSIZE -1);
		assert(l_range);
	}
	if((ws=WriteSetCreate())){
		AddRangeToWriteSet(ws, s_range);
		AddRangeToWriteSet(ws, l_range);
	}
	return ws;
}

WriteSet *getMemDef(InstrList *iList, int order){
	WriteSet *ws;
	Range *s_range = NULL, *l_range = NULL, *l_range2 = NULL;
	Instruction *instr = getInstruction(iList, order);
	if(!instr)
		return NULL;
	//stack def is not NULL
	if(instr->stackDef && instr->stackDefStart){
		s_range = RangeCreate(instr->stackDefStart, instr->stackDefStart+ PTRSIZE*instr->stackDef-1);
		assert(s_range);
	}
	//local def is not null
	if(instr->localDef){
		l_range = RangeCreate(instr->localDef, instr->localDef+ PTRSIZE -1);
		assert(l_range);
	}
	if(instr->localDef2){
		l_range2 = RangeCreate(instr->localDef2, instr->localDef2+ PTRSIZE -1);
		assert(l_range2);
	}
	if((ws=WriteSetCreate())){
		AddRangeToWriteSet(ws, s_range);
		AddRangeToWriteSet(ws, l_range);
		AddRangeToWriteSet(ws, l_range2);
	}
	return ws;
}

bool isAND(InstrList *iList, Instruction *ins){
	assert(ins);
	if(!strcmp(ins->opName, "and"))
		return true;
	else
		return false;
}

bool isOR(InstrList *iList, Instruction *ins){
	assert(ins);
	if(!strcmp(ins->opName, "or"))
		return true;
	else
		return false;
}



bool is1WayBranch(InstrList *iList, Instruction *ins){
	assert(ins);
	if(!strcmp(ins->opName, "goto"))
		return true;
	else
		return false;
}


bool is2WayBranch(InstrList *iList, Instruction *ins){
	assert(ins);
	if(!strcmp(ins->opName, "ifeq")
			|| !strcmp(ins->opName, "ifne")
			|| !strcmp(ins->opName, "or")
			|| !strcmp(ins->opName, "and")
		)
		return true;
	else
		return false;
}


/*
 * TODO: implement this
 */
bool isNWayBranch(InstrList *iList, Instruction *ins){
	assert(ins);
	return false;
}

bool isBranch(InstrList *iList, Instruction *ins){
	if(is1WayBranch(iList, ins) || is2WayBranch(iList, ins) || isNWayBranch(iList, ins))
		return true;
	else
		return false;
}

bool isRetInstruction(InstrList *iList, Instruction *ins){
	assert(ins);
	if(!strcmp(ins->opName, "return") || !strcmp(ins->opName, "stop") )
		return true;
	else
		return false;
}


bool isInvokeInstruction(InstrList *iList, Instruction *ins){
	assert(ins);
	if(!strcmp(ins->opName,"new") || !strcmp(ins->opName,"call") || !strcmp(ins->opName,"eval") ){
		return true;
	}else
		return false;
}

/*
 * return true if the target function/constructor is native function
 * (i.e. no new frame is created )
 */
bool isNativeInvokeInstruction(InstrList *iList, Instruction *ins){
	assert(iList);
	assert(ins);
	Instruction *next;
	//eval will create a new frame
	if(!isInvokeInstruction(iList,ins))
		return false;
	ADDRESS nextInstrAddr = ins->addr + ins->length;
	if(!(next=getInstruction(iList, ins->order+1)))
		return true;
	if(next->addr==nextInstrAddr)
		return true;
	else{
		//if a call to document.write generate trace, then it's still a native call
		//INSTR_IS_DOC_WRITE && INSTR_IS_NATIVE_INVOKE would identify such instr
		if(INSTR_HAS_FLAG(ins, INSTR_IS_DOC_WRITE)){
			return true;
		}
		else
			return false;
	}
	//return INSTR_HAS_FLAG(instr, INSTR_IS_NATIVE_INVOKE);
}


/*
 * set the instruction flags for different category
 * return number of native calls
 */
int labelInstructionList(InstrList *iList){
	int i;
	int sum=0;
	Instruction *instr;
	for(i=0;i<iList->numInstrs;i++){
		instr = getInstruction(iList, i);
		if(isRetInstruction(iList, instr)){
			INSTR_SET_FLAG(instr, INSTR_IS_RET);
			if(!strcmp(instr->opName, "return"))
				INSTR_SET_FLAG(instr, INSTR_HAS_RET_VALUE);
		}
		// if s is a invocation instr
		if(isInvokeInstruction(iList, instr)){
			if(instr->opCode==JSOP_EVAL){
				sum++;
				INSTR_SET_FLAG(instr, INSTR_IS_SCRIPT_INVOKE);
			}
			else if(!isNativeInvokeInstruction(iList, instr)){
				//printInstruction(instr);
				assert(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE));
				//printf("#%d is non-native call\n", instr->order);
			}else{
				sum++;
				//a native call to doc.write() and generate trace
				if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){
					assert(INSTR_HAS_FLAG(instr, INSTR_IS_DOC_WRITE));
					//printf("#%d is document.write() and generate code\n", instr->order);
				}else{
				//printInstruction(instr);
					assert(INSTR_HAS_FLAG(instr, INSTR_IS_NATIVE_INVOKE));
					//printf("#%d is native call\n", instr->order);
				}
			}
		}
		if(is1WayBranch(iList, instr)){
			INSTR_SET_FLAG(instr, INSTR_IS_1_BRANCH);
		}else if(is2WayBranch(iList, instr)){
			INSTR_SET_FLAG(instr, INSTR_IS_2_BRANCH);
		}else if(isNWayBranch(iList, instr)){
			INSTR_SET_FLAG(instr, INSTR_IS_N_BRANCH);
		}
	}
	return sum;
}


void printInstruction(Instruction *ins){
	assert(ins);
/*	printf("%c #%u\t0x%lX\t%-15s\tS_USE: %d\tS_DEF: %d\tL_USE: %-16lX\tL_DEF: %-16lX\tP_USE_SCOPE: %-16lX\tP_USE_ID: %-10ld\tP_DEF_SCOPE: %-16lX\tP_DEF_ID: %-10ld\n", \
			INSTR_HAS_FLAG(ins,INSTR_IN_SLICE)?'*':' ', ins->order, ins->addr,	ins->opName,\
			ins->stackUse, ins->stackDef, ins->localUse, ins->localDef, \
			ins->propUseScope, ins->propUseId, ins->propDefScope,ins->propDefId
	);*/
	printf("%c%c%c%c%c%c $%d$ #%-4u\t0x%lX\t%-15s\tS_USE: %16lX--%-16lX\tS_DEF: %16lX-%-16lX\tL_USE: %-16lX\tL_DEF: %-16lX\tL_DEF2: %-16lX\tP_USE_SCOPE: %-16lX\tP_USE_ID: %-10ld\tP_DEF_SCOPE: %-16lX\tP_DEF_ID: %-10ld JMP_OFFSET: %d\t",
			INSTR_HAS_FLAG(ins,INSTR_IS_ENV)?'!':' ',
			INSTR_HAS_FLAG(ins,INSTR_ON_EVAL)?'e':' ',
			INSTR_HAS_FLAG(ins,INSTR_IN_SLICE)?'*':' ',
			INSTR_HAS_FLAG(ins,INSTR_IS_BBL_START)?'S':' ',
			INSTR_HAS_FLAG(ins,INSTR_IS_BBL_END)?'E':' ',
			INSTR_HAS_FLAG(ins,INSTR_BRANCH_TAKEN)?'1':INSTR_HAS_FLAG(ins,INSTR_BRANCH_NOT_TAKEN)?'0':' ',
			ins->inBlock?ins->inBlock->id:-1,
			//ins->inFunction,
			ins->order, ins->addr,	ins->opName,
			ins->stackUseStart, ins->stackUseStart==0?0:ins->stackUseStart+(PTRSIZE * ins->stackUse)-1,
			ins->stackDefStart, ins->stackDefStart==0?0:ins->stackDefStart+(PTRSIZE * ins->stackDef)-1,
			ins->localUse, ins->localDef, ins->localDef2,
			ins->propUseScope, ins->propUseId, ins->propDefScope,ins->propDefId, ins->jmpOffset
	);
	if(!strcmp(ins->opName, "string")||!strcmp(ins->opName, "regexp")){
		printf("OP_S:%s\t", ins->operand.s);
	}else if(!strcmp(ins->opName, "double")){
		printf("OP_D:%f\t", ins->operand.d);
	}else
		printf("OP_I:%ld\t", ins->operand.i);
	if(ins->str)
		printf("STR: %s\t", ins->str);
	if(ins->nextBlock)
		printf("nextBlock:%d\t", ins->nextBlock->id);
/*	if(INSTR_HAS_FLAG(ins,INSTR_IS_SCRIPT_INVOKE)||INSTR_HAS_FLAG(ins,INSTR_IS_NATIVE_INVOKE)){
		printf("\tPARAS_NUM: %d", ins->parameterNum);
	}*/
	printf("\n");
}

void printInstructionOrder(InstrList *iList, int order){
	assert(iList && order<iList->numInstrs && order >=0);
	printInstruction(getInstruction(iList, order));
}

void printInstrList(InstrList *iList){
	assert(iList);
	printf("\nInstruction List:\n");
	int i;
	for(i=0;i<iList->numInstrs;i++){
		printInstructionOrder(iList, i);
	}
}

typedef struct EvalEntry{
	char 	*str;
	ADDRESS	base;
} EvalEntry;

EvalEntry *createEvalEntry(char *str, ADDRESS base){
	EvalEntry *entry = (EvalEntry *)malloc(sizeof(EvalEntry));
	assert(entry);
	entry->base = base;
	entry->str = (char *)malloc(strlen(str)+1);
	memcpy(entry->str, str, strlen(str)+1);
	return entry;
}

void destroyEvalEntry(void *entry){
	EvalEntry *e = (EvalEntry *)entry;
	assert(e);
	free(e->str);
	free(e);
}

int evalEntryCompareByStr(void *i1, void *i2){
	assert(i1 && i2);
	return strcmp(((EvalEntry *)i1)->str, ((EvalEntry *)i2)->str);
}

void printEvalEntry(void *i1){
	EvalEntry *entry = (EvalEntry *)i1;
	assert(entry);
	printf("str: %s\t, base: %lx\n", entry->str, entry->base);
}
/*this function take a list of EvalEntry's and a string as parameter
 *if the string has been eval'ed before(i.e. an entry 'e' in the evalList such that e.str==string),
 *then it returns e
 *else it return NULL
 */
EvalEntry *isEvalEntryInList(ArrayList *evalList, char *str){
	int i;
	EvalEntry *entry;
	for(i=0;i<al_size(evalList);i++){
		entry = (EvalEntry *)al_get(evalList, i);
		if(!strcmp(entry->str, str)){
			return entry;
		}
	}
	return NULL;
}

/*
 * Process the instr list to adjust the address eval'ed instructions.
 * instructions resulted from the same string would have the same base address after prcessing.
 *
 * XXX: probably should also use the address of 'eval' instruction instead of eval'ed string to determine
 *      if 2 eval generated code snippets are the same - 08/10/2011
 */
void processEvaledInstr(InstrList *iList){
	int i;
	OpStack *stack;
	Instruction *instr, *next;
	EvalEntry *entry1;
	ADDRESS		offset;


	stack = initOpStack(printEvalEntry);
	ArrayList *evalList = al_newGeneric(AL_LIST_SORTED, evalEntryCompareByStr, NULL, destroyEvalEntry);
	assert(evalList && stack);
	offset = 0;

	for(i=0;i<InstrListLength(iList);i++){
		instr = getInstruction(iList, i);
		instr->addr_origin = instr->addr;
		instr->addr = instr->addr - offset;
		if(instr->opCode==JSOP_EVAL){
			//string already get evaled once
			if((entry1 = isEvalEntryInList(evalList, instr->eval_str))){
				next = getInstruction(iList, i+1);
				pushOpStack(stack, (void *)offset);
				offset = next->addr - entry1->base;
			}else{
				next = getInstruction(iList, i+1);
				printInstruction(instr);
				assert(instr->eval_str);
				entry1 = createEvalEntry(instr->eval_str, next->addr);
				al_add(evalList, entry1);
				pushOpStack(stack, (void *)offset);
				offset = 0;
			}
		}else if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) && (instr->opCode==JSOP_CALL ||instr->opCode==JSOP_NEW )){
			pushOpStack(stack, (void *)offset);
			offset = 0;
		}else if(instr->opCode==JSOP_STOP || instr->opCode==JSOP_RETURN ){
			offset = (ADDRESS)popOpStack(stack);
		}else
			continue;
	}

}


