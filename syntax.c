/*
 * syntax.c
 *
 *  Created on: May 17, 2011
 *      Author: genlu
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "instr.h"
#include "opcode.h"
#include "cfg.h"
#include "syntax.h"
#include "function.h"

static int syntaxBlockIDctl=0;
static int anonfunobjctl=0;
static int funobjtableentryctl=0;

char *this_str = "this";
char *anonfunobj_str = "anonfunobj";
char *arg_str = "arg";
char *local_var_str = "local_var";
char *length_str = "length";
char *null_str = "null";

#define DEBUG 0

/*
 * pair[0] is old CFG id
 * pair[1] is corresponding syntax tree node id
 */
void **createIdPair(){
	void **pair = (void **)malloc(2*sizeof(void *));
	return pair;
}

void destroyIdPair(void *p){
	assert(p);
	void **pair = (void **)p;
	free(pair);
}

int idPairCompare(void *t1, void *t2){
	assert(t1 && t2);
	return ((BasicBlock *)((void **)t1)[0])->id - ((BasicBlock *)((void **)t2)[0])->id;
}

//////////////////////////////////////

int andOrTargetCompare(void *t1, void *t2){
	assert(t1 && t2);
	return ((AndOrTarget *)t1)->targetAddr - ((AndOrTarget *)t2)->targetAddr;
}

void printAndOrTarget(void *target){
	assert(target);
	printf("targetAddr: %lx Num: %d\n", ((AndOrTarget *)target)->targetAddr, ((AndOrTarget *)target)->num);
}

void destroyAndOrTarget(void *target){
	assert(target);
	free((AndOrTarget *)target);
}

AndOrTarget *createAndOrTarget(){
	AndOrTarget *ret = (AndOrTarget *)malloc(sizeof(AndOrTarget));
	assert(ret);
	ret->targetAddr=0;
	ret->num=0;
	return ret;
}

AndOrTarget *searchForTarget(ArrayList *list, ADDRESS addr){
	assert(list);
	AndOrTarget *target;
	int i;
	for(i=0;i<al_size(list);i++){
		target = (AndOrTarget *)al_get(list, i);
		if(target->targetAddr==addr)
			return target;
	}
	return NULL;
}

StackNode *popSyntaxStack(SyntaxStack *stack){
	StackNode *temp = NULL;;
	assert(stack);
	if(stack->number==1){
		(stack->number)--;
		temp = stack->top;
		stack->top = stack->bottom = NULL;
	}else if(stack->number>1){
		(stack->number)--;
		stack->top->next->prev = NULL;
		temp = stack->top;
		stack->top = stack->top->next;
		temp->prev = temp->next = NULL;
	}
#if DEBUG
	assert(temp);
	printf("stack_size:%d  temp%p\n", stack->number, temp);
	printf("pop %s\t%d\n",
			(temp->type==SYN_NODE)?"SYN":"EXP",
					(temp->type==SYN_NODE)?(((SyntaxTreeNode *)(temp->node))->type):(((ExpTreeNode *)(temp->node))->type));
#endif
	return temp;
}

void pushSyntaxStack(SyntaxStack *stack, StackNode *node){
	assert(stack && node);
#if DEBUG
	printf("push %s\t%d\n",
			(node->type==SYN_NODE)?"SYN":"EXP",
					(node->type==SYN_NODE)?(((SyntaxTreeNode *)(node->node))->type):(((ExpTreeNode *)(node->node))->type));
#endif
	if(stack->number==0){
		(stack->number)++;
		stack->bottom = stack->top = node;
	}else{
		(stack->number)++;
		node->next = stack->top;
		stack->top->prev = node;
		stack->top = node;
	}
	//printf("stack_size:%d  temp%p\n", stack->number, node);
	return;
}

StackNode *createSyntaxStackNode(){
	StackNode *node;
	node = (StackNode *)malloc(sizeof(StackNode));
	assert(node);
	node->prev = node->next = node->node = NULL;
	return node;
}


void destroySyntaxStackNode(StackNode *node ){
	if(!node)
		return;
	//todo
	/*	if(node->type == SYN_NODE){
		destroySyntaxTreeNode((SyntaxTreeNode *)node->node);
	}
	else if(node->type == EXP_NODE){
		destroyExpTreeNode((ExpTreeNode *)node->node);
	}*/
	free(node);
}

SyntaxStack *createSyntaxStack(void){
	SyntaxStack *stack;
	stack = (SyntaxStack *)malloc(sizeof(SyntaxStack));
	assert(stack);
	stack->bottom = stack->top = NULL;
	stack->number = 0;
	return stack;
}

void stackTest(){
	SyntaxStack *stack = createSyntaxStack();
	StackNode *node;
	long x;
	long i;
	for(i=0;i<10;i++){
		node = createSyntaxStackNode();
		node->type = EXP_NODE;
		node->node = (void *)i;
		pushSyntaxStack(stack, node);
		printf("pushed: %ld\n", i);
	}
	for(i=0;i<5;i++){
		node = popSyntaxStack(stack);
		if(!node)
			break;
		x = (long)node->node;
		printf("poped: %ld\n", x);
	}
	for(i=0;i<5;i++){
		node = createSyntaxStackNode();
		node->type = EXP_NODE;
		node->node = (void *)i;
		pushSyntaxStack(stack, node);
		printf("pushed: %ld\n", i);
	}
	for(;;){
		node = popSyntaxStack(stack);
		if(!node)
			break;
		x = (long)node->node;
		printf("poped: %ld\n", x);
	}
}

/*******************************************************************/


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

ExpTreeNode *createExpTreeNode(){
	ExpTreeNode *node;
	node = (ExpTreeNode *)malloc(sizeof(ExpTreeNode));
	assert(node);
	node->flags = 0;
	memset(&(node->u), 0, sizeof(node->u));
	return node;
}

void destroyExpTreeNode(ExpTreeNode *node){
	//TODO
	if(node)
		free(node);
}


bool expIsTrue(ExpTreeNode *node){
	bool ret;
	assert(node);
	ret = false;
	if(node->type==EXP_CONST_VALUE){
		if(EXP_HAS_FLAG(node, EXP_IS_BOOL)){
			ret = node->u.const_value_bool;
		}
		//todo: other constant values equivelent to true
	}
	return ret;
}

void printExpTreeNode(ExpTreeNode *node){
	int i;
	ExpTreeNode *expNode;

	if(!node)
		return;

	char *str;
	str = (char *)malloc(16);

	//printf("@%d\n", node->type);
	switch(node->type){
	case EXP_NAME:
		if(node->u.name)
			printf("%s", node->u.name);
		else
			printf("undefined");
		break;
	case EXP_CONST_VALUE:
		if(EXP_HAS_FLAG(node, EXP_IS_INT)){
			printf("%ld", node->u.const_value_int);
		}else if(EXP_HAS_FLAG(node, EXP_IS_DOUBLE)){
			printf("%f", node->u.const_value_double);
		}else if(EXP_HAS_FLAG(node, EXP_IS_STRING)){
			printf("\"%s\"", node->u.const_value_str);
		}else if(EXP_HAS_FLAG(node, EXP_IS_REGEXP)){
			printf("%s", node->u.const_value_str);
		}else if(EXP_HAS_FLAG(node, EXP_IS_BOOL)){
			printf("%s", (node->u.const_value_bool==true)?"true":"false");
		}
		break;
	case EXP_CALL:
	case EXP_EVAL:
		if(node->type==EXP_CALL){
			printExpTreeNode(node->u.func.name);
		}else if(node->type==EXP_EVAL){
			printf("eval");
		}
		printf("( ");
		for(i=node->u.func.num_paras;i>0;i--){
			expNode = (ExpTreeNode *)al_get(node->u.func.parameters, i-1);
			printExpTreeNode(expNode);
			if(i>1)
				printf(", ");
		}
		printf(" )");
		if(node->type==EXP_EVAL){
			sprintf(str, "block_%d", node->u.func.funcEntryBlock->id);
			printf("/* eval'ed code -> %s */", str);
		}
		break;
	case EXP_NEW:
		printf("(");
		printf("new ");
		printExpTreeNode(node->u.func.name);
		printf("( ");
		for(i=node->u.func.num_paras-1;i>=0;i--){
			expNode = (ExpTreeNode *)al_get(node->u.func.parameters, i);
			printExpTreeNode(expNode);
			if(i)
				printf(", ");
		}
		printf(" )");
		printf(")");
		if(node->u.func.funcEntryBlock){
			sprintf(str, "block_%d", node->u.func.funcEntryBlock->id);
			printf("/* target function: %s */", str);
		}
		break;
	case EXP_PROP:
		printf("(");
		//print obj
		printf("(");
		printExpTreeNode(node->u.prop.objName);
		printf(")");
		//print prop
		printf(".");
		printExpTreeNode(node->u.prop.propName);
		printf(")");
		break;
	case EXP_BIN:
		printf("(");
		//print lval
		printExpTreeNode(node->u.bin_op.lval);
		//print op
		switch(node->u.bin_op.op){
		case OP_ADD:
			printf("+");
			break;
		case OP_SUB:
			printf("-");
			break;
		case OP_MUL:
			printf("*");
			break;
		case OP_DIV:
			printf("/");
			break;
		case OP_INITPROP:
			printf(":");
			break;
		case OP_GE:
			printf(">=");
			break;
		case OP_GT:
			printf(">");
			break;
		case OP_LE:
			printf("<=");
			break;
		case OP_LT:
			printf("<");
			break;
		case OP_EQ:
			printf("==");
			break;
		case OP_NE:
			printf("!=");
			break;
		case OP_AND:
			printf("&&");
			break;
		case OP_OR:
			printf("||");
			break;
		case OP_ASSIGN:
			printf("=");
			break;
		default:
			printf("?");
			break;
		}
		//print rval
		printExpTreeNode(node->u.bin_op.rval);
		printf(")");
		break;

		case EXP_UN:
		{
			printf("(");
			switch(node->u.un_op.op){
			case OP_NOT:
				printf("!");
				printExpTreeNode(node->u.un_op.lval);
			case OP_VINC:
				printf("%s",node->u.un_op.name);
				printf("++");
				break;
			case OP_VDEC:
				printf("%s",node->u.un_op.name);
				printf("--");
				break;
			case OP_INCV:
				printf("++");
				printf("%s",node->u.un_op.name);
				break;
			case OP_DECV:
				printf("--");
				printf("%s",node->u.un_op.name);
				break;
			default:
				break;
			}
			printf(")");
		}
		break;


		case EXP_ARRAY_ELM:
			printExpTreeNode(node->u.array_elm.name);
			printf("[");
			printExpTreeNode(node->u.array_elm.index);
			printf("]");
			break;

		case EXP_ARRAY_INIT:
			if(EXP_HAS_FLAG(node, EXP_IS_PROP_INIT)){
				printf("{ ");
			}else{
				printf("[ ");
			}
			assert(node->u.array_init.size == al_size(node->u.array_init.initValues));
			for(i=0;i<al_size(node->u.array_init.initValues);i++){
				expNode = (ExpTreeNode *)al_get(node->u.array_init.initValues, i);
				printExpTreeNode(expNode);
				if(i<al_size(node->u.array_init.initValues)-1)
					printf(", ");
			}
			if(EXP_HAS_FLAG(node, EXP_IS_PROP_INIT)){
				printf(" }");
			}else{
				printf(" ]");
			}
			break;

		case EXP_UNDEFINED:
		default:
			break;
	}//end switch

	free(str);
}


/*******************************************************************/


SyntaxTreeNode *createSyntaxTreeNode(){
	SyntaxTreeNode *node;
	node = (SyntaxTreeNode *)malloc(sizeof(SyntaxTreeNode));
	assert(node);
	node->flags = 0;
	node->parent = NULL;
	node->predsList = al_new();
	node->id = syntaxBlockIDctl++;
	memset(&(node->u), 0, sizeof(node->u));
	return node;
}

void destroySyntaxTreeNode(SyntaxTreeNode *node){
	//TODO
	if(node){
		if(node->predsList)
			al_free(node->predsList);
		free(node);
	}
}


int SyntaxTreeNodeCompareByBlockID(void *i1, void *i2){
	assert(i1 && i2);
	int id1, id2;
	id1 = ((SyntaxTreeNode *)i1)->id;
	id2 = ((SyntaxTreeNode *)i2)->id;

/*	assert(((SyntaxTreeNode *)i1)->type == TN_BLOCK || ((SyntaxTreeNode *)i1)->type == TN_WHILE || ((SyntaxTreeNode *)i1)->type == TN_FUNCTION);
	assert(((SyntaxTreeNode *)i2)->type == TN_BLOCK || ((SyntaxTreeNode *)i2)->type == TN_WHILE || ((SyntaxTreeNode *)i1)->type == TN_FUNCTION);

	if(((SyntaxTreeNode *)i1)->type == TN_BLOCK){
		id1 = ((SyntaxTreeNode *)i1)->u.block.cfg_id;
	}else if(((SyntaxTreeNode *)i1)->type == TN_WHILE){
		id1 = ((SyntaxTreeNode *)i1)->u.loop.header->id;
	}else
		id1 = ((SyntaxTreeNode *)i1)->u.func.funcStruct->funcEntryBlock->id;

	if(((SyntaxTreeNode *)i2)->type == TN_BLOCK){
		id2 = ((SyntaxTreeNode *)i2)->u.block.cfg_id;
	}else if(((SyntaxTreeNode *)i2)->type == TN_WHILE){
		id2 = ((SyntaxTreeNode *)i2)->u.loop.header->id;
	}else
		id2 = ((SyntaxTreeNode *)i2)->u.func.funcStruct->funcEntryBlock->id;*/
	return id1-id2;
}


ArrayList *tempSynTree=NULL;
bool print_flag = false;

void printSyntaxTreeNode(SyntaxTreeNode *node){


	int i;
	char *str;
	SyntaxTreeNode 	*sTreeNode;

	//print an empty statement
	if(!node){
		printf(";\t//this branch's likely missing from dynamic trace\n");
		return;
	}

	if(print_flag)
		printf("\n//id:%d\n", node->id);

	str = (char *)malloc(16);
	switch(node->type){

	case TN_FUNCTION:
		//eval'ed code block still use CFG id instead of SyntaxTreeNode ID!!!!!!
		if(BBL_HAS_FLAG(node->u.func.funcStruct->funcEntryBlock, BBL_IS_EVAL_ENTRY)){
			sprintf(str, "block_%d:", node->u.func.funcStruct->funcEntryBlock->id);
			printf("//eval'ed code\n%s", str);
			printf("{\n");
		}
		else{
			printf("function %s ", node->u.func.funcStruct->funcName);
			printf("(");
			for(i=0;i<=node->u.func.funcStruct->args;i++){
				printf("%s%d", arg_str, i);
				if(i<node->u.func.funcStruct->args)
					printf(", ");
			}
			printf(") {\n");
		}
		for(i=0;i<al_size(node->u.func.funcBody);i++){
			sTreeNode = (SyntaxTreeNode *)al_get(node->u.func.funcBody, i);
			assert(sTreeNode);
			printSyntaxTreeNode(sTreeNode);
			printf("\n");
		}
		printf("}\n");
		break;


	case TN_WHILE:
		//printf("while(true) {\n");
		printf("block_%d:\n", node->id);
		printf("while( "); printExpTreeNode(node->u.loop.cond);printf(" ){\n");
		for(i=0;i<al_size(node->u.loop.loopBody);i++){
			sTreeNode = (SyntaxTreeNode *)al_get(node->u.loop.loopBody, i);
			assert(sTreeNode);
			printSyntaxTreeNode(sTreeNode);
			printf("\n");
		}
		printf("}\t//end loop\n");
		break;

	case TN_BLOCK:
		if(TN_HAS_FLAG(node, TN_NOT_SHOW_LABEL)){
			sprintf(str, "\n");
		}else if(TN_HAS_FLAG(node, TN_AFTER_RELINK_GOTO))
			sprintf(str, "block_%d:", node->id);
		else
			sprintf(str, "block_%d:", node->u.block.cfg_id);
		printf("\n%s\n", str);
		for(i=0;i<al_size(node->u.block.statements);i++){
			sTreeNode = (SyntaxTreeNode *)al_get(node->u.block.statements, i);
			assert(sTreeNode);
			printSyntaxTreeNode(sTreeNode);
			printf(";\n");
		}
		printf("\n");
		break;

	case TN_EXP:
		printExpTreeNode(node->u.expNode);
		break;

	case TN_GOTO:
		//printf("\n//goto id:%d\n", node->id);
		if(TN_HAS_FLAG(node, TN_IS_GOTO_BREAK)){
			printf("break;");
		}else if(TN_HAS_FLAG(node, TN_IS_GOTO_CONTINUE)){
			printf("continue;");
		}else{
			if(TN_HAS_FLAG(node, TN_AFTER_RELINK_GOTO))
				sprintf(str, "block_%d", node->u.go_to.synTargetBlock->id);
			else
				sprintf(str, "block_%d", node->u.go_to.targetBlock->id);
			printf("goto %s ", str);
		}
		break;

	case TN_DEFVAR:
		printf("var ");
		printExpTreeNode(node->u.expNode);
		break;

	case TN_RETURN:
		printf("return");
		break;

	case TN_RETEXP:
		printf("return ");
		printExpTreeNode(node->u.expNode);
		break;

	case TN_IF_ELSE:
		printf("if");
		printf("( ");
		printExpTreeNode(node->u.if_else.cond);

		printf(" ) {\n");
		for(i=0;i<al_size(node->u.if_else.if_path);i++){
			sTreeNode = (SyntaxTreeNode *)al_get(node->u.if_else.if_path, i);
			printSyntaxTreeNode(sTreeNode);
		}
		printf(";");

		printf(" \n}\nelse {\n");
		for(i=0;i<al_size(node->u.if_else.else_path);i++){
			sTreeNode = (SyntaxTreeNode *)al_get(node->u.if_else.else_path, i);
			printSyntaxTreeNode(sTreeNode);
		}
		printf(";");

		printf("\n}");
		break;

	default:
		break;
	}//end switch

	free(str);


	if(tempSynTree && node->type!=TN_FUNCTION && print_flag){
		if(node->type!=TN_BLOCK ){
			sTreeNode = findAdjacentStatement(tempSynTree, node);
			printf("\n/*****************************************\tadj statement:%d\t", sTreeNode?sTreeNode->id:-1);
			printf("***************************************/\n");
			sTreeNode = findNextStatement(tempSynTree, node);
			printf("\n/*****************************************\tnext statement:%d\t", sTreeNode?sTreeNode->id:-1);
			printf("***************************************/\n");
		}
		if(al_size(node->predsList)==1){
			sTreeNode = al_get(node->predsList, 0);
			printf("\n/*****************************************\tonly ONE pred, should move to:%d\t", sTreeNode->id);
			printf("***************************************/\n");
		}
	}

	if(print_flag)
		printf("\n//end id:%d\n", node->id);

}

/*******************************************************************/

void buildBlockSyntaxTreeFromSyntaxStack(SyntaxTreeNode *syntaxBlockNode, SyntaxStack *stack){
	assert(syntaxBlockNode && stack);
	assert(syntaxBlockNode->type==TN_BLOCK);
	assert(syntaxBlockNode->u.block.statements);

	StackNode 		*stackNode1;
	SyntaxTreeNode 	*sTreeNode;
	ExpTreeNode 	*expTreeNode;

	if(stack->number==0)
		return;

	stackNode1 = stack->bottom;
	while(stackNode1){

		//add syntax tree node into statement list directly.
		if(stackNode1->type == SYN_NODE){
			al_add(syntaxBlockNode->u.block.statements, stackNode1->node);
		}

		//convert EXP_CALL, EXP_NEW, assignment and inc/dec expNode into a SyntaxTree node with type TN_EXP
		//then add converted node into list, ignore all other ExpTreeNode
		else{
			assert(stackNode1->type = EXP_NODE);
			expTreeNode = (ExpTreeNode *)(stackNode1->node);
			if(expTreeNode->type==EXP_CALL || expTreeNode->type==EXP_NEW || expTreeNode->type==EXP_EVAL ||
					(expTreeNode->type==EXP_BIN && expTreeNode->u.bin_op.op==OP_ASSIGN) ||
					(expTreeNode->type==EXP_UN && (expTreeNode->u.un_op.op==OP_VINC ||
							expTreeNode->u.un_op.op==OP_INCV ||
							expTreeNode->u.un_op.op==OP_VDEC ||
							expTreeNode->u.un_op.op==OP_DECV ))
			){
				//XXX this will cause memory leaks in stack
				// 	  those ignored expNode will not be freed when stack node is destroyed.
				sTreeNode = createSyntaxTreeNode();
				sTreeNode->type = TN_EXP;
				sTreeNode->u.expNode = expTreeNode;
				al_add(syntaxBlockNode->u.block.statements, (void *)sTreeNode);
			}
		}
		stackNode1 = stackNode1->prev;
	}//end while-loop

	return;
}


SyntaxTreeNode *buildSyntaxTreeForBlock(BasicBlock *block, uint32_t flag, ArrayList *funcObjTable, ArrayList *funcCFGs){

	int i,j;

	int tempInt;
	char *str;
	Instruction 	*instr, *lastInstr;
	BlockEdge		*edge;
	SyntaxStack 	*stack;
	SyntaxTreeNode 	*syntaxBlockNode, *sTreeNode1, *sTreeNode2;
	ExpTreeNode		*expTreeNode1, *expTreeNode2, *expTreeNode3, *expTreeNode4;
	StackNode 		*stackNode1, *stackNode2, *stackNode3, *stackNode4;
	ArrayList *targetTable;
	AndOrTarget *andOrTarget;

	syntaxBlockNode = createSyntaxTreeNode();
	syntaxBlockNode->type = TN_BLOCK;
	syntaxBlockNode->u.block.cfg_id = block->id;
	syntaxBlockNode->u.block.cfgBlock = block;
	syntaxBlockNode->u.block.statements = al_new();

	instr = lastInstr = NULL;

	//printf("created a syntax node #%d for basic block #%d\n",syntaxBlockNode->id, block->id);

	stack = createSyntaxStack();

	//we need a target table for processing AND and OR instructions
	targetTable = al_newGeneric(AL_LIST_SET, andOrTargetCompare, printAndOrTarget, destroyAndOrTarget);

	//start_over:
#if DEBUG
	printf("~~~~~~~~~~~processing block %d\n", block->id);
	printBasicBlock(block);
#endif
	BBL_SET_FLAG(block, flag);

	for(i=0;i<al_size(block->instrs);i++){

		lastInstr = instr;

		instr = al_get(block->instrs, i);

#if DEBUG
		printf("processing..  %lx  %s\n", instr->addr, instr->opName);
#endif
		/*		if(instr->opCode==JSOP_CALL || instr->opCode==JSOP_NEW){
			printInstruction(instr);
			if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){
				printf("scripted\n");
				assert(i==al_size(block->instrs)-1);
			}
			else
				printf("not!\n");
		}*/

		assert(instr->inBlock->id == block->id);

		//avoid any dangling pointer problem may occur
		tempInt = 0;
		str = NULL;
		edge = NULL;
		sTreeNode1 = sTreeNode2 = NULL;
		expTreeNode1 = expTreeNode2 = expTreeNode3 = expTreeNode4 = NULL;
		stackNode1 = stackNode2 = stackNode3 = stackNode4 = NULL;
		andOrTarget = NULL;



		//if any missing code due to short circuit, we add an boolean exp as a place holder.
		// here we made an assumption that AND/OR either jump to short circuit target or fallthough to next expression
		// if branch, then entire AND expression is false (threfore having a true at last won't affect result)
		//                        OR expression is true (threfore having a false at last won't affect result)
		if(lastInstr){
			if(lastInstr->opCode == JSOP_AND && instr->addr != lastInstr->addr + lastInstr->length){
				expTreeNode1 = createExpTreeNode();
				expTreeNode1->type = EXP_CONST_VALUE;
				expTreeNode1->u.const_value_bool = true;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_BOOL);
				stackNode1 = createSyntaxStackNode();
				stackNode1->type = EXP_NODE;
				stackNode1->node = (void *)expTreeNode1;
				pushSyntaxStack(stack, stackNode1);
			}else if(lastInstr->opCode == JSOP_OR && instr->addr != lastInstr->addr + lastInstr->length){
				expTreeNode1 = createExpTreeNode();
				expTreeNode1->type = EXP_CONST_VALUE;
				expTreeNode1->u.const_value_bool = false;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_BOOL);
				stackNode1 = createSyntaxStackNode();
				stackNode1->type = EXP_NODE;
				stackNode1->node = (void *)expTreeNode1;
				pushSyntaxStack(stack, stackNode1);
			}
			expTreeNode1 = NULL;
			stackNode1 = NULL;
		}
		//check if this instruction is a AND/OR branch target.
		//TODO
		andOrTarget = searchForTarget(targetTable, instr->addr);
		if(andOrTarget && andOrTarget->num>0){
			printf("found one! %lx, %d\n", andOrTarget->targetAddr, andOrTarget->num);
			//printf("number of synbol in stack: %d\n", stack->number);
			assert(stack->number>=2*andOrTarget->num);
			StackNode *ptr = NULL;
			for(j=0;j<andOrTarget->num;j++){
				//pop 2nd exp
				ptr = popSyntaxStack(stack);
				assert(ptr->type==EXP_NODE);
				expTreeNode1 = (ExpTreeNode *)ptr->node;	//b in a&&b
				destroySyntaxStackNode(ptr);
				//pop and/or token
				ptr = popSyntaxStack(stack);
				assert(ptr->type==EXP_NODE);
				expTreeNode2 = (ExpTreeNode *)ptr->node;	//&& in a&&b
				assert(expTreeNode2->type==EXP_TOKEN);
				if(expTreeNode2->u.tok.targetAddr!=andOrTarget->targetAddr)
					j--;
				destroySyntaxStackNode(ptr);
				//pop 1st exp
				ptr = popSyntaxStack(stack);
				assert(ptr->type==EXP_NODE);
				expTreeNode3 = (ExpTreeNode *)ptr->node;	//a in a&&b
				destroySyntaxStackNode(ptr);
				//create a binary exp
				expTreeNode4 = createExpTreeNode();
				expTreeNode4->type = EXP_BIN;
				expTreeNode4->u.bin_op.lval = expTreeNode3;
				expTreeNode4->u.bin_op.rval = expTreeNode1;
				expTreeNode4->u.bin_op.op = expTreeNode2->u.tok.op;
				//push it into stack
				ptr = createSyntaxStackNode();
				ptr->type = EXP_NODE;
				ptr->node = (void *)expTreeNode4;
				pushSyntaxStack(stack, ptr);
				//clean-up
				expTreeNode1 = expTreeNode2 = expTreeNode3 = expTreeNode4 = NULL;
				ptr = NULL;
			}//end of for
			al_remove(targetTable, (void *)andOrTarget);
		}


		/*
		 * main dispatching
		 */
		switch(instr->opCode){

		/*****************************************************************************
		 *
		 * following cases will generate a ExpTreeNode node and push it into stack
		 *
		 *****************************************************************************/
		//  1. type == EXP_CONST_VALUE
		case JSOP_UINT16:
		case JSOP_INT8:
		case JSOP_INT32:
		case JSOP_ZERO:
		case JSOP_ONE:
		case JSOP_STRING:
		case JSOP_DOUBLE:
		case JSOP_REGEXP:
		case JSOP_NULL:
		case JSOP_TRUE:
		case JSOP_FALSE:
			//printf("-- EXP_CONST_VALUE  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_CONST_VALUE;
			if(instr->opCode==JSOP_INT8 || instr->opCode==JSOP_INT32 || instr->opCode==JSOP_UINT16){
				expTreeNode1->u.const_value_int = instr->operand.i;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_INT);
			}
			else if(instr->opCode==JSOP_ZERO){
				expTreeNode1->u.const_value_int = 0;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_INT);
			}
			else if(instr->opCode==JSOP_ONE){
				expTreeNode1->u.const_value_int = 1;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_INT);
			}
			else if(instr->opCode==JSOP_DOUBLE){
				expTreeNode1->u.const_value_double = instr->operand.d;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_DOUBLE);
			}
			else if(instr->opCode==JSOP_STRING){
				str = (char *)malloc(strlen(instr->operand.s)+1);
				assert(str);
				memcpy(str, instr->operand.s, strlen(instr->operand.s)+1);
				expTreeNode1->u.const_value_str = str;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_STRING);
			}
			else if(instr->opCode==JSOP_REGEXP){
				str = (char *)malloc(strlen(instr->operand.s)+1);
				assert(str);
				memcpy(str, instr->operand.s, strlen(instr->operand.s)+1);
				expTreeNode1->u.const_value_str = str;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_REGEXP);
			}
			else if(instr->opCode==JSOP_TRUE){
				expTreeNode1->u.const_value_bool = true;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_BOOL);
			}
			else if(instr->opCode==JSOP_FALSE){
				expTreeNode1->u.const_value_bool = false;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_BOOL);
			}
			else if(instr->opCode==JSOP_NULL){
				str = (char *)malloc(strlen(null_str)+1);
				assert(str);
				memcpy(str, null_str, strlen(null_str)+1);
				expTreeNode1->u.const_value_str = str;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_STRING);
				EXP_SET_FLAG(expTreeNode1, EXP_IS_NULL);
			}
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = EXP_NODE;
			stackNode1->node = (void *)expTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;

			//  2. type == EXP_NAME
		case JSOP_GETGVAR:
		case JSOP_GETARG:
		case JSOP_GETVAR:
		case JSOP_NAME:
		case JSOP_CALLNAME:
		case JSOP_CALLARG:
		case JSOP_THIS:
		case JSOP_ANONFUNOBJ:
		case JSOP_PUSH:
			// call(name/prop/etc) actually push 2 elements into stack,
			// but we don't care the calling obj when decompiling, therefore ignored
			//printf("-- EXP_NAME  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_NAME;
			if(instr->opCode == JSOP_GETARG){
				str = (char *)malloc(strlen(arg_str)+5);
				assert(str);
				sprintf(str, "%s%ld", arg_str, instr->operand.i);
			}
			else if(instr->opCode == JSOP_CALLARG){
				str = (char *)malloc(strlen(arg_str)+5);
				assert(str);
				sprintf(str, "%s%ld", arg_str, instr->operand.i);
			}
			else if(instr->opCode == JSOP_GETVAR){
				str = (char *)malloc(strlen(arg_str)+5);
				assert(str);
				sprintf(str, "%s%ld", local_var_str, instr->operand.i);
			}
			else if(instr->opCode == JSOP_THIS){
				str = (char *)malloc(strlen(this_str)+1);
				assert(str);
				memcpy(str, this_str, strlen(this_str)+1);
			}
			else if(instr->opCode == JSOP_PUSH){
				str = NULL;
			}
			else if(instr->opCode == JSOP_ANONFUNOBJ){
				//todo
				EXP_SET_FLAG(expTreeNode1, EXP_IS_FUNCTION_OBJ);
				FuncObjTableEntry *funcObjTableEntry1=NULL;
				funcObjTableEntry1 = findFuncObjTableEntryByFuncObj(funcObjTable, instr->objRef);
				str = (char *)malloc(strlen(funcObjTableEntry1->func_name)+1);
				assert(str);
				memcpy(str, funcObjTableEntry1->func_name, strlen(funcObjTableEntry1->func_name)+1);
			}
			else{
				if(INSTR_HAS_FLAG(instr, INSTR_IS_LARG_ACCESS)){
					str = (char *)malloc(strlen(arg_str)+5);
					assert(str);
					sprintf(str, "%s%ld", arg_str, instr->operand.i);
				}
				else if(INSTR_HAS_FLAG(instr, INSTR_IS_LVAR_ACCESS)){
					str = (char *)malloc(strlen(local_var_str)+5);
					assert(str);
					sprintf(str, "%s%ld", local_var_str, instr->operand.i);
				}
				else{
					str = (char *)malloc(strlen(instr->str)+1);
					assert(str);
					memcpy(str, instr->str, strlen(instr->str)+1);
				}
			}
			expTreeNode1->u.name = str;
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = EXP_NODE;
			stackNode1->node = (void *)expTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;


			//  3. type == EXP_UN
		case JSOP_NOT:
			stackNode1 = popSyntaxStack(stack);			//val
			assert(stackNode1);
			assert(stackNode1->type == EXP_NODE);
			expTreeNode1 = (ExpTreeNode *)stackNode1->node;

			expTreeNode2 = createExpTreeNode();
			expTreeNode2->type = EXP_UN;
			expTreeNode2->u.un_op.op = OP_NOT;
			expTreeNode2->u.un_op.lval = expTreeNode1;

			stackNode2 = createSyntaxStackNode();
			stackNode2->type = EXP_NODE;
			stackNode2->node = (void *)expTreeNode2;
			pushSyntaxStack(stack, stackNode2);
			destroySyntaxStackNode(stackNode1);
			break;

		case JSOP_GVARINC:
		case JSOP_GVARDEC:
		case JSOP_INCGVAR:
		case JSOP_DECGVAR:
		case JSOP_NAMEINC:
		case JSOP_NAMEDEC:
		case JSOP_INCNAME:
		case JSOP_DECNAME:
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_UN;

			if(INSTR_HAS_FLAG(instr, INSTR_IS_LARG_ACCESS)){
				str = (char *)malloc(strlen(arg_str)+5);
				assert(str);
				sprintf(str, "%s%ld", arg_str, instr->operand.i);
			}
			else if(INSTR_HAS_FLAG(instr, INSTR_IS_LVAR_ACCESS)){
				str = (char *)malloc(strlen(local_var_str)+5);
				assert(str);
				sprintf(str, "%s%ld", local_var_str, instr->operand.i);
			}
			else{
				str = (char *)malloc(strlen(instr->str)+1);
				assert(str);
				memcpy(str, instr->str, strlen(instr->str)+1);
			}
			expTreeNode1->u.un_op.name = str;

			if(instr->opCode == JSOP_GVARINC || instr->opCode == JSOP_NAMEINC){
				expTreeNode1->u.un_op.op = OP_VINC;
			}
			if(instr->opCode == JSOP_GVARDEC || instr->opCode == JSOP_NAMEDEC){
				expTreeNode1->u.un_op.op = OP_VDEC;
			}
			if(instr->opCode == JSOP_INCGVAR || instr->opCode == JSOP_INCNAME){
				expTreeNode1->u.un_op.op = OP_INCV;
			}
			if(instr->opCode == JSOP_DECGVAR || instr->opCode == JSOP_DECNAME){
				expTreeNode1->u.un_op.op = OP_DECV;
			}
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = EXP_NODE;
			stackNode1->node = (void *)expTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;

		case JSOP_ARGINC:
		case JSOP_INCARG:
		case JSOP_ARGDEC:
		case JSOP_DECARG:
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_UN;
			str = (char *)malloc(strlen(arg_str)+5);
			assert(str);
			sprintf(str, "%s%ld", arg_str, instr->operand.i);
			expTreeNode1->u.un_op.name = str;
			if(instr->opCode == JSOP_ARGINC){
				expTreeNode1->u.un_op.op  = OP_VINC;
			}
			if(instr->opCode == JSOP_ARGDEC){
				expTreeNode1->u.un_op.op  = OP_VDEC;
			}
			if(instr->opCode == JSOP_INCARG){
				expTreeNode1->u.un_op.op  = OP_INCV;
			}
			if(instr->opCode == JSOP_DECARG){
				expTreeNode1->u.un_op.op  = OP_DECV;
			}
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = EXP_NODE;
			stackNode1->node = (void *)expTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;

		case JSOP_VARINC:
		case JSOP_INCVAR:
		case JSOP_VARDEC:
		case JSOP_DECVAR:
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_UN;
			str = (char *)malloc(strlen(local_var_str)+5);
			assert(str);
			sprintf(str, "%s%ld", local_var_str, instr->operand.i);
			expTreeNode1->u.un_op.name = str;
			if(instr->opCode == JSOP_VARINC){
				expTreeNode1->u.un_op.op = OP_VINC;
			}
			if(instr->opCode == JSOP_VARDEC){
				expTreeNode1->u.un_op.op = OP_VDEC;
			}
			if(instr->opCode == JSOP_INCVAR){
				expTreeNode1->u.un_op.op = OP_INCV;
			}
			if(instr->opCode == JSOP_DECVAR){
				expTreeNode1->u.un_op.op = OP_DECV;
			}
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = EXP_NODE;
			stackNode1->node = (void *)expTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;


			//  4. type == EXP_BIN
		case JSOP_ADD:
		case JSOP_SUB:
		case JSOP_MUL:
		case JSOP_DIV:
		case JSOP_GE:
		case JSOP_GT:
		case JSOP_LE:
		case JSOP_LT:
		case JSOP_EQ:
		case JSOP_NE:
			//printf("-- EXP_BIN  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			stackNode1 = popSyntaxStack(stack);			//rval
			stackNode2 = popSyntaxStack(stack);			//lval
			assert(stackNode1 && stackNode2);
			assert(stackNode1->type == EXP_NODE);
			assert(stackNode2->type == EXP_NODE);
			expTreeNode1 = (ExpTreeNode *)stackNode1->node;
			expTreeNode2 = (ExpTreeNode *)stackNode2->node;
			expTreeNode3 = createExpTreeNode();
			expTreeNode3->type = EXP_BIN;
			if(instr->opCode == JSOP_ADD){
				expTreeNode3->u.bin_op.op = OP_ADD;
			}
			else if(instr->opCode == JSOP_SUB){
				expTreeNode3->u.bin_op.op = OP_SUB;
			}
			else if(instr->opCode == JSOP_MUL){
				expTreeNode3->u.bin_op.op = OP_MUL;
			}
			else if(instr->opCode == JSOP_DIV){
				expTreeNode3->u.bin_op.op = OP_DIV;
			}
			else if(instr->opCode == JSOP_GE){
				expTreeNode3->u.bin_op.op = OP_GE;
			}
			else if(instr->opCode == JSOP_GT){
				expTreeNode3->u.bin_op.op = OP_GT;
			}
			else if(instr->opCode == JSOP_LE){
				expTreeNode3->u.bin_op.op = OP_LE;
			}
			else if(instr->opCode == JSOP_LT){
				expTreeNode3->u.bin_op.op = OP_LT;
			}
			else if(instr->opCode == JSOP_EQ){
				expTreeNode3->u.bin_op.op = OP_EQ;
			}
			else if(instr->opCode == JSOP_NE){
				expTreeNode3->u.bin_op.op = OP_NE;
			}
			expTreeNode3->u.bin_op.lval = expTreeNode2;
			expTreeNode3->u.bin_op.rval = expTreeNode1;
			stackNode3 = createSyntaxStackNode();
			stackNode3->type = EXP_NODE;
			stackNode3->node = (void *)expTreeNode3;
			pushSyntaxStack(stack, stackNode3);
			destroySyntaxStackNode(stackNode1);
			destroySyntaxStackNode(stackNode2);
			break;



		case JSOP_SETGVAR:
		case JSOP_SETNAME:
		case JSOP_SETVAR:
		case JSOP_SETELEM:
		case JSOP_SETARG:
			//printf("-- TN_ASSIGN  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			stackNode1 = popSyntaxStack(stack);			//rval
			assert(stackNode1);
			assert(stackNode1->type == EXP_NODE);

			expTreeNode1 = (ExpTreeNode *)stackNode1->node;

			expTreeNode2 = createExpTreeNode();			//lval
			expTreeNode2->type =  EXP_NAME;
			if(instr->opCode == JSOP_SETGVAR || instr->opCode ==  JSOP_SETNAME){
				if(INSTR_HAS_FLAG(instr, INSTR_IS_LARG_ACCESS)){
					str = (char *)malloc(strlen(arg_str)+5);
					assert(str);
					sprintf(str, "%s%ld", arg_str, instr->operand.i);
				}
				else if(INSTR_HAS_FLAG(instr, INSTR_IS_LVAR_ACCESS)){
					str = (char *)malloc(strlen(local_var_str)+5);
					assert(str);
					sprintf(str, "%s%ld", local_var_str, instr->operand.i);
				}
				else{
					str = (char *)malloc(strlen(instr->str)+1);
					assert(str);
					memcpy(str, instr->str, strlen(instr->str)+1);
				}
				expTreeNode2->u.name = str;
			}else if(instr->opCode == JSOP_SETVAR){
				str = (char *)malloc(strlen(local_var_str)+5);
				assert(str);
				sprintf(str, "%s%ld", local_var_str, instr->operand.i);
				expTreeNode2->u.name = str;
			}else if(instr->opCode == JSOP_SETARG){
				str = (char *)malloc(strlen(arg_str)+5);
				assert(str);
				sprintf(str, "%s%ld", arg_str, instr->operand.i);
				expTreeNode2->u.name = str;
			}else if(instr->opCode == JSOP_SETELEM){
				expTreeNode2->type = EXP_ARRAY_ELM;
				stackNode3 = popSyntaxStack(stack);			//index
				assert(stackNode3);
				assert(stackNode3->type == EXP_NODE);
				expTreeNode3 = (ExpTreeNode *)stackNode3->node;
				stackNode4 = popSyntaxStack(stack);			//array name
				assert(stackNode4);
				assert(stackNode4->type == EXP_NODE);
				expTreeNode4 = (ExpTreeNode *)stackNode4->node;
				expTreeNode2->u.array_elm.name = expTreeNode4;
				expTreeNode2->u.array_elm.index = expTreeNode3;
				expTreeNode3 = expTreeNode4 = NULL;
			}
			expTreeNode3 = createExpTreeNode();
			expTreeNode3->type = EXP_BIN;
			expTreeNode3->u.bin_op.lval = expTreeNode2;
			expTreeNode3->u.bin_op.rval = expTreeNode1;
			expTreeNode3->u.bin_op.op = OP_ASSIGN;
			stackNode2 = createSyntaxStackNode();
			stackNode2->type = EXP_NODE;
			stackNode2->node = (void *)expTreeNode3;
			pushSyntaxStack(stack, stackNode2);
			destroySyntaxStackNode(stackNode1);
			break;

		case JSOP_SETPROP:
			//printf("-- TN_ASSIGN  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			stackNode1 = popSyntaxStack(stack);			//rval
			stackNode2 = popSyntaxStack(stack);			//obj name of lval (propname is in instr.str)
			assert(stackNode1&&stackNode2);
			assert(stackNode1->type == EXP_NODE);
			assert(stackNode2->type == EXP_NODE);

			expTreeNode1 = (ExpTreeNode *)stackNode1->node;
			expTreeNode2 = (ExpTreeNode *)stackNode2->node;
			//first create a EXP_NAME node for lval's prop name
			expTreeNode3 = createExpTreeNode();
			expTreeNode3->type =  EXP_NAME;
			str = (char *)malloc(strlen(instr->str)+1);
			assert(str);
			memcpy(str, instr->str, strlen(instr->str)+1);
			expTreeNode3->u.name = str;
			//then create a EXP_PROP node for lval as a whole
			expTreeNode4 = createExpTreeNode();
			expTreeNode4->type =  EXP_PROP;
			expTreeNode4->u.prop.objName = expTreeNode2;
			expTreeNode4->u.prop.propName = expTreeNode3;
			expTreeNode2 = expTreeNode3 = NULL;
			//finally, create a EXP_BIN node for assignment
			expTreeNode2 = createExpTreeNode();
			expTreeNode2->type = EXP_BIN;
			expTreeNode2->u.bin_op.lval = expTreeNode4;
			expTreeNode2->u.bin_op.rval = expTreeNode1;
			expTreeNode2->u.bin_op.op = OP_ASSIGN;
			//push it into stack
			stackNode3 = createSyntaxStackNode();
			stackNode3->type = EXP_NODE;
			stackNode3->node = (void *)expTreeNode2;
			pushSyntaxStack(stack, stackNode3);
			destroySyntaxStackNode(stackNode1);
			destroySyntaxStackNode(stackNode2);
			break;

		case JSOP_AND:
		case JSOP_OR:
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_TOKEN;
			if(instr->opCode == JSOP_AND){
				expTreeNode1->u.tok.op = OP_AND;
			}else{
				expTreeNode1->u.tok.op = OP_OR;
			}
			//add AndOrTarget into the targetTable (if it's not in there)
			// or increment the existing target's num by 1
			//todo
			andOrTarget = createAndOrTarget();
			andOrTarget->targetAddr = expTreeNode1->u.tok.targetAddr = instr->addr+instr->jmpOffset;
			andOrTarget->num = 1;
			if(al_contains(targetTable,(void *)andOrTarget)){
				int targetIndex = al_indexOf(targetTable, (void *)andOrTarget);
				destroyAndOrTarget((void *)andOrTarget);
				andOrTarget = (AndOrTarget *)al_get(targetTable,targetIndex );
				assert(andOrTarget->targetAddr==instr->addr+instr->jmpOffset);
				(andOrTarget->num)++;
			}else{
				al_add(targetTable,(void *)andOrTarget);
			}
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = EXP_NODE;
			stackNode1->node = (void *)expTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;

			//  5. type == EXP_CALL
			//  6. type == EXP_NEW
		case JSOP_CALL:
		case JSOP_NEW:
		case JSOP_EVAL:
#if DEBUG
			printf("-- EXP_CALL  block#%d: %lx %s\n", instr->inBlock->id, instr->addr, instr->opName);
#endif
			//this time, we first create a EXP_CALL node
			expTreeNode1 = createExpTreeNode();
			if(instr->opCode == JSOP_CALL){
				expTreeNode1->type = EXP_CALL;
			}else if(instr->opCode == JSOP_NEW){
				expTreeNode1->type = EXP_NEW;
			}else if(instr->opCode == JSOP_EVAL){
				expTreeNode1->type = EXP_EVAL;
			}
			expTreeNode1->u.func.num_paras = instr->operand.i;
			if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){
				expTreeNode1->u.func.funcEntryBlock = instr->nextBlock;//block->calltarget;
			}else{
				expTreeNode1->u.func.funcEntryBlock = NULL;
			}
			expTreeNode1->u.func.parameters = al_new();
			//then we pop parameters one by one
			for(tempInt=0;tempInt<expTreeNode1->u.func.num_paras;tempInt++){
				stackNode1 = popSyntaxStack(stack);
				assert(stackNode1);
				assert(stackNode1->type == EXP_NODE);
				expTreeNode2 = (ExpTreeNode *)stackNode1->node;
				// and push them into parameter list, in the order reverse to the call
				// so we need to print them reversely when generating sourse code
				al_add(expTreeNode1->u.func.parameters, expTreeNode2);
				destroySyntaxStackNode(stackNode1);
			}
			//get function name from stack
			stackNode1 = popSyntaxStack(stack);
			assert(stackNode1);
			assert(stackNode1->type == EXP_NODE);
			expTreeNode2 = (ExpTreeNode *)stackNode1->node;
			if(EXP_HAS_FLAG(expTreeNode2, EXP_IS_NULL)){
				stackNode3 = popSyntaxStack(stack);
				assert(stackNode3);
				assert(stackNode3->type == EXP_NODE);
				expTreeNode2 = (ExpTreeNode *)stackNode3->node;
				assert(EXP_HAS_FLAG(expTreeNode2, EXP_IS_FUNCTION_OBJ));
				destroySyntaxStackNode(stackNode3);
			}
			expTreeNode1->u.func.name = expTreeNode2;
			//push this into stack
			stackNode2 = createSyntaxStackNode();
			stackNode2->type = EXP_NODE;
			stackNode2->node = (void *)expTreeNode1;
			pushSyntaxStack(stack, stackNode2);
			destroySyntaxStackNode(stackNode1);
			break;

			//  7. type == EXP_PROP
		case JSOP_GETTHISPROP:
			//printf("-- EXP_PROP  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			expTreeNode4 = createExpTreeNode();
			expTreeNode4->type = EXP_NAME;
			str = (char *)malloc(strlen(this_str)+1);
			assert(str);
			memcpy(str, this_str, strlen(this_str)+1);
			expTreeNode4->u.name = str;
			stackNode3 = createSyntaxStackNode();
			stackNode3->type = EXP_NODE;
			stackNode3->node = (void *)expTreeNode4;
			pushSyntaxStack(stack, stackNode3);
			//XXX caution, non-break, fall-through to JSOP_GETPROP
		case JSOP_GETPROP:
		case JSOP_CALLPROP:
		case JSOP_LENGTH:
			//printf("-- EXP_PROP  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			stackNode1 = popSyntaxStack(stack);			//obj
			assert(stackNode1->type == EXP_NODE);
			expTreeNode1 = (ExpTreeNode *)stackNode1->node;
			//create a EXP_NAME node fpr prop name
			expTreeNode2 = createExpTreeNode();
			expTreeNode2->type =  EXP_NAME;
			if(instr->opCode==JSOP_GETPROP || instr->opCode==JSOP_CALLPROP || instr->opCode==JSOP_GETTHISPROP){
				str = (char *)malloc(strlen(instr->str)+1);
				assert(str);
				memcpy(str, instr->str, strlen(instr->str)+1);
			}else if(instr->opCode==JSOP_LENGTH){
				str = (char *)malloc(strlen(length_str)+1);
				assert(str);
				memcpy(str, length_str, strlen(length_str)+1);
			}
			expTreeNode2->u.name = str;
			//create a EXP_PROP node
			expTreeNode3 = createExpTreeNode();
			expTreeNode3->type =  EXP_PROP;
			expTreeNode3->u.prop.objName = expTreeNode1;
			expTreeNode3->u.prop.propName = expTreeNode2;
			//push this into stack
			stackNode2 = createSyntaxStackNode();
			stackNode2->type = EXP_NODE;
			stackNode2->node = (void *)expTreeNode3;
			pushSyntaxStack(stack, stackNode2);
			destroySyntaxStackNode(stackNode1);
			break;

			//  8. type == EXP_UNDEFINED

			//  9. type = EXP_ARRAY_ELM
		case JSOP_CALLELEM:
		case JSOP_GETELEM:
#if DEBUG
			printf("-- EXP_ARRAY_ELM  block#%d: %lx %s\n", instr->inBlock->id, instr->addr, instr->opName);
#endif
			stackNode1 = popSyntaxStack(stack);			//index
			stackNode2 = popSyntaxStack(stack);			//array
			assert(stackNode1 && stackNode2);
			assert(stackNode1->type == EXP_NODE);
			assert(stackNode2->type == EXP_NODE);
			expTreeNode1 = (ExpTreeNode *)stackNode1->node;
			expTreeNode2 = (ExpTreeNode *)stackNode2->node;
			expTreeNode3 = createExpTreeNode();
			expTreeNode3->type = EXP_ARRAY_ELM;

			expTreeNode3->u.array_elm.name = expTreeNode2;
			expTreeNode3->u.array_elm.index = expTreeNode1;

			stackNode3 = createSyntaxStackNode();
			stackNode3->type = EXP_NODE;
			stackNode3->node = (void *)expTreeNode3;
			pushSyntaxStack(stack, stackNode3);
			destroySyntaxStackNode(stackNode1);
			destroySyntaxStackNode(stackNode2);
			break;


			//  10. type = EXP_ARRAY_INIT
		case JSOP_NEWINIT:
			// create a init_node
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_ARRAY_INIT;
			expTreeNode1->u.array_init.initValues = al_new();
			expTreeNode1->u.array_init.size = 0;
			EXP_SET_FLAG(expTreeNode1, EXP_IS_PROP_INIT);
			//push it
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = EXP_NODE;
			stackNode1->node = (void *)expTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;


		case JSOP_INITELEM:
		case JSOP_INITPROP:
			stackNode1 = popSyntaxStack(stack);			//value
			stackNode2 = popSyntaxStack(stack);			//index
			stackNode3 = popSyntaxStack(stack);			//EXP_ARRAY_INIT node
			assert(stackNode1 && stackNode2 && stackNode3);
			assert(stackNode1->type == EXP_NODE);
			assert(stackNode2->type == EXP_NODE);
			assert(stackNode3->type == EXP_NODE);
			expTreeNode1 = (ExpTreeNode *)stackNode1->node;
			expTreeNode2 = (ExpTreeNode *)stackNode2->node;
			expTreeNode3 = (ExpTreeNode *)stackNode3->node;
			assert(expTreeNode3->type==EXP_ARRAY_INIT);
			if(instr->opCode==JSOP_INITELEM){
				//this is a array
				EXP_CLR_FLAG(expTreeNode1, EXP_IS_PROP_INIT);
				//assuming init ALL elements sequentially
				al_add(expTreeNode3->u.array_init.initValues, expTreeNode1);
			}
			else if(instr->opCode==JSOP_INITPROP){
				EXP_SET_FLAG(expTreeNode1, EXP_IS_PROP_INIT);
				expTreeNode4 = createExpTreeNode();
				expTreeNode4->type = EXP_BIN;
				expTreeNode4->u.bin_op.lval = expTreeNode3;
				expTreeNode4->u.bin_op.rval = expTreeNode2;
				expTreeNode4->u.bin_op.op = OP_INITPROP;
				al_add(expTreeNode3->u.array_init.initValues, expTreeNode1);
			}
			expTreeNode3->u.array_init.size++;
			//push it
			stackNode4 = createSyntaxStackNode();
			stackNode4->type = EXP_NODE;
			stackNode4->node = (void *)expTreeNode3;
			pushSyntaxStack(stack, stackNode4);
			destroySyntaxStackNode(stackNode1);
			destroySyntaxStackNode(stackNode2);
			destroySyntaxStackNode(stackNode3);
			break;


			/*****************************************************************************
			 *
			 * following cases will generate a SyntaxTreeNode node and push it into stack
			 *
			 *****************************************************************************/

			//  1. type == TN_IF_ELSE
		case JSOP_IFEQ:
		case JSOP_IFNE:
#if DEBUG
			printf("-- TN_IF_ELSE  block#%d: %lx %s\n", instr->inBlock->id, instr->addr, instr->opName);
#endif
			stackNode1 = popSyntaxStack(stack);			//cond
			assert(stackNode1);
			assert(stackNode1->type == EXP_NODE);
			// create a TN_IF_ELSE node
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_IF_ELSE;

			expTreeNode1 = (ExpTreeNode *)stackNode1->node;
			sTreeNode1->u.if_else.cond = expTreeNode1;

			sTreeNode1->u.if_else.if_path = al_new();
			sTreeNode1->u.if_else.else_path = al_new();
			//look through outbound edges, and create branch/adjacent path accoedingly
			//printf("num_edges: %d\n", al_size(block->succs));
			for(tempInt=0;tempInt<al_size(block->succs);tempInt++){
				edge = (BlockEdge *)al_get(block->succs, tempInt);
				/*
				 * one of the paths could be missing from dynamic trace
				 * if that's the case, corresponding pointer stays NULL
				 */
				if(EDGE_HAS_FLAG(edge, EDGE_IS_ADJACENT_PATH)){
					sTreeNode2 = createSyntaxTreeNode();
					sTreeNode2->type = TN_GOTO;
					sTreeNode2->u.go_to.targetBlock = edge->head;
					if(instr->opCode==JSOP_IFEQ){
						al_add(sTreeNode1->u.if_else.if_path, (void *)sTreeNode2);
					}
					else if(instr->opCode==JSOP_IFNE){
						al_add(sTreeNode1->u.if_else.else_path, (void *)sTreeNode2);
					}
				}
				else if(EDGE_HAS_FLAG(edge, EDGE_IS_BRANCHED_PATH)){
					sTreeNode2 = createSyntaxTreeNode();
					sTreeNode2->type = TN_GOTO;
					sTreeNode2->u.go_to.targetBlock = edge->head;
					if(instr->opCode==JSOP_IFEQ){
						al_add(sTreeNode1->u.if_else.else_path, (void *)sTreeNode2);
					}
					else if(instr->opCode==JSOP_IFNE){
						al_add(sTreeNode1->u.if_else.if_path, (void *)sTreeNode2);
					}
				}
			}
			//push it
			stackNode2 = createSyntaxStackNode();
			stackNode2->type = SYN_NODE;
			stackNode2->node = (void *)sTreeNode1;
			pushSyntaxStack(stack, stackNode2);
			destroySyntaxStackNode(stackNode1);
			break;

			//  3. type == TN_GOTO
		case JSOP_GOTO:
			//printf("-- TN_GOTO  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_GOTO;
			assert(al_size(block->succs)==1);
			sTreeNode1->u.go_to.targetBlock = ((BlockEdge *)al_get(block->succs, 0))->head;
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = SYN_NODE;
			stackNode1->node = (void *)sTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;

			//  4. type == TN_RETURN
		case JSOP_STOP:
			//only produce a return node when it's in a function
			if(BBL_HAS_FLAG(block, BBL_IS_FUNC_RET)){
				//printf("-- TN_RETURN  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
				sTreeNode1 = createSyntaxTreeNode();
				sTreeNode1->type = TN_RETURN;
				stackNode1 = createSyntaxStackNode();
				stackNode1->type = SYN_NODE;
				stackNode1->node = (void *)sTreeNode1;
				pushSyntaxStack(stack, stackNode1);
			}
			break;

			//  5. type == TN_EXP

			//  6. type == TN_RETEXP
		case JSOP_RETURN:
			//get retval
			stackNode1 = popSyntaxStack(stack);
			assert(stackNode1);
			assert(stackNode1->type == EXP_NODE);
			expTreeNode1 = (ExpTreeNode *)stackNode1->node;
			//create ret node
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_RETEXP;
			sTreeNode1->u.expNode = expTreeNode1;
			//push it
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = SYN_NODE;
			stackNode1->node = (void *)sTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;





			//  7.  type ==TN_DEFVAR
		case JSOP_DEFVAR:
			// create a name node
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_NAME;
			str = (char *)malloc(strlen(instr->str)+1);
			assert(str);
			memcpy(str, instr->str, strlen(instr->str)+1);
			expTreeNode1->u.un_op.name = str;
			//create a defvar node
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_DEFVAR;
			sTreeNode1->u.expNode = expTreeNode1;
			//push it
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = SYN_NODE;
			stackNode1->node = (void *)sTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;

			// deffun/closure will not create any syntax related nodes, just try to set up funcObjTable
		case JSOP_DEFFUN:
		case JSOP_CLOSURE:

		case JSOP_BINDNAME:
		case JSOP_ENDINIT:
		case JSOP_NOP:
		case JSOP_POP:
		case JSOP_POPV:
		case JSOP_LINENO:
		case JSOP_GROUP:
			break;
		default:
			printf("unhandled JSOP: %s\n", instr->opName);
			assert(0);
#if DEBUG
			printf("-- ignore block#%d: %lx %s\n", instr->inBlock->id, instr->addr, instr->opName);
#endif
			break;
		}//end switch
	}//end for-loop

	/*
	if(block->type==BT_SCRIPT_INVOKE){
		BlockEdge *callEdge = (BlockEdge *)al_get(block->succs, 0);
		assert(callEdge && EDGE_HAS_FLAG(callEdge, EDGE_IS_CALLSITE));
#if DEBUG
		printf("//this is a interpreted function call.\n");
#endif
		block = callEdge->head;

		goto start_over;
	}


	else*/
	if(block->type==BT_FALL_THROUGH ){

		if(al_size(block->succs)!=1){
			printBasicBlock(block);
		}
		assert(al_size(block->succs)==1);
		edge = (BlockEdge *)al_get(block->succs, 0);

		//need to explicitly add an goto node at the end of fall-through block??
		//xxx this doesn't work for if(a&&b), since ifeq will be the start of a new block, but the top of the stack at that moment
		//		is goto node, the condition node is just below the goto node......
		sTreeNode1 = createSyntaxTreeNode();
		sTreeNode1->type = TN_GOTO;
		sTreeNode1->u.go_to.targetBlock = edge->head;
		stackNode1 = createSyntaxStackNode();
		stackNode1->type = SYN_NODE;
		stackNode1->node = (void *)sTreeNode1;
		pushSyntaxStack(stack, stackNode1);


		/*		//xxx
		block = edge->head;

		//add label for next block
		sTreeNode2 = createSyntaxTreeNode();
		sTreeNode2->type = TN_BLOCK;
		sTreeNode2->u.block.cfg_id = block->id;
		sTreeNode2->id = syntaxBlockIDctl++;
		sTreeNode2->u.block.statements = al_new();
		stackNode2 = createSyntaxStackNode();
		stackNode2->type = SYN_NODE;
		stackNode2->node = (void *)sTreeNode2;
		pushSyntaxStack(stack, stackNode2);

		goto start_over;*/
	}


	/*
	 * at this point, all the syntaxTreeNodes and expTreeNodes are stored in syntaxStack,
	 * with the the node corresponding to 1st statement at the bottom of the stack.
	 * we need to clean up the nodes by converting call/new expTreeNode into SyntaxTreeNode,
	 * and eliminating all other remaining expTreeNode in the stack. Then add them into
	 * syntaxBlockNode->u.block.statements
	 */
	buildBlockSyntaxTreeFromSyntaxStack(syntaxBlockNode, stack);

	//assert(0);

	al_freeWithElements(targetTable);
	return syntaxBlockNode;
}// end buildSyntaxTreeForBlock()


SyntaxTreeNode *findBlockNodeByOldID(ArrayList *syntaxTree, int oldID){
	assert(syntaxTree);
	int i;
	SyntaxTreeNode *node;
	for(i=0;i<al_size(syntaxTree);i++){
		node = al_get(syntaxTree, i);
		//assert(node->type == TN_BLOCK);
		if(node->type == TN_BLOCK && node->u.block.cfg_id==oldID){
			fprintf(stderr, "block old id:%d\n", node->u.block.cfg_id);
			return node;
		}
		if(node->type == TN_WHILE && node->u.loop.header->id==oldID ){
			fprintf(stderr, "while old id:%d\n", node->u.loop.header->id);
			return node;
		}
	}
	printf("ERROR: cannot find blockNode in syntax tree with old ID %d\n", oldID);
	return NULL;
}

/*
 * the recursive function DFS
 */
static void doSearch(BasicBlock *n, uint32_t flag, ArrayList *syntaxTree, ArrayList *funObjTable, ArrayList *funcCFGs){
	int i;
	BlockEdge *edge;
	BasicBlock *block;
	SyntaxTreeNode *syntaxBlockNode;

	assert(n);
#if DEBUG
	printf("searching to :%d\n", n->id);
#endif
	BBL_SET_FLAG(n, flag);

	syntaxBlockNode = buildSyntaxTreeForBlock(n, flag, funObjTable, funcCFGs);
	al_add(syntaxTree, syntaxBlockNode);

	for(i=0;i<al_size(n->immDomSuccs);i++){
		edge = al_get(n->immDomSuccs, i);
		block = edge->head;
		if(!BBL_HAS_FLAG(block, flag)){
			doSearch(block, flag, syntaxTree, funObjTable, funcCFGs);
		}
	}

	/*	for(i=0;i<al_size(n->dominate);i++){
		edge = al_get(n->dominate, i);
		block = edge->head;
		if(!BBL_HAS_FLAG(block, flag)){
			doSearch(block, flag, syntaxTree, funObjTable, funcCFGs);
		}
	}*/
}

void transformGOTOsInLoopBlock(SyntaxTreeNode *loopBlock, NaturalLoop *loop){
	//XXX need rewrite!!!
	return;

	if(!loopBlock)
		return;
	assert(loop);
	int i;
	SyntaxTreeNode *sTreeNode;


	switch(loopBlock->type){
	case TN_IF_ELSE:
		//printf("dealing if-else\n");
		//printf("dealing if\n");
		//printSyntaxTreeNode(loopBlock->u.if_else.if_path);
		for(i=0;i<al_size(loopBlock->u.if_else.if_path);i++){
			sTreeNode = al_get(loopBlock->u.if_else.if_path, i);
			transformGOTOsInLoopBlock(sTreeNode, loop);
		}
		//printf("dealing else\n");
		//printSyntaxTreeNode(loopBlock->u.if_else.else_path);
		for(i=0;i<al_size(loopBlock->u.if_else.else_path);i++){
			sTreeNode = al_get(loopBlock->u.if_else.else_path, i);
			transformGOTOsInLoopBlock(sTreeNode, loop);
		}
		break;
	case TN_GOTO:
		//if goto loop header

		//printf("transform goto block %d\n", loopBlock->u.go_to.targetBlock->id);
		if(isBlockTheLoopHeader(loop, loopBlock->u.go_to.targetBlock->id)){
			//printf("to CONTINUE\n");
			TN_SET_FLAG(loopBlock, TN_IS_GOTO_CONTINUE);
		}
		//else goto outside of loop
		else if(!isBlockInTheLoop(loop, loopBlock->u.go_to.targetBlock->id)){
			//printf("to BREAK\n");
			TN_SET_FLAG(loopBlock, TN_IS_GOTO_BREAK);
		}
		break;
	case TN_BLOCK:
		//printf("CAUTION: transformGOTOsInLoopBlock() called on a TN_BLOCK:%d\n", loopBlock->u.block.cfg_id);
		for(i=0;i<al_size(loopBlock->u.block.statements);i++){
			sTreeNode = al_get(loopBlock->u.block.statements, i);
			//printSyntaxTreeNode(sTreeNode);
			transformGOTOsInLoopBlock(sTreeNode, loop);
		}
		break;
	case TN_RETURN:
	case TN_RETEXP:
	case TN_EXP:
	case TN_DEFVAR:
	case TN_WHILE:
	case TN_FUNCTION:
		break;
	}
	/*
	 * if_path and else_path only includes a GOTO node
	 */

}


void createLoopsInSynaxTree(ArrayList *syntaxTree, ArrayList *loopList){
	int i;
	int header_index;
	NaturalLoop *loop;
	SyntaxTreeNode *loopNode;
	SyntaxTreeNode *sTreeNode1;
	ExpTreeNode *expTreeNode1;

	for(i=0;i<al_size(loopList);i++){
		loop = al_get(loopList, i);
		LOOP_CLR_FLAG(loop, LOOP_FLAG_TMP0);
	}


	loop = findSmallestUnprocessedLoop(loopList, LOOP_FLAG_TMP0);

	while(loop){
		loopNode = createSyntaxTreeNode();
		loopNode->type = TN_WHILE;
		loopNode->u.loop.loopStruct = loop;
		loopNode->u.loop.header = loop->header;
		expTreeNode1 = createExpTreeNode();
		expTreeNode1->type = EXP_CONST_VALUE;
		expTreeNode1->u.const_value_bool = true;
		EXP_SET_FLAG(expTreeNode1, EXP_IS_BOOL);
		loopNode->u.loop.cond = expTreeNode1;
		loopNode->u.loop.loopBody = al_newGeneric(AL_LIST_SET, SyntaxTreeNodeCompareByBlockID, NULL, NULL);
		loopNode->u.loop.loopBody2 = al_newGeneric(AL_LIST_SET, SyntaxTreeNodeCompareByBlockID, NULL, NULL);
		//printNaturalLoop(loop);
		// find loop header Node, and add it into loop node
		sTreeNode1 = findBlockNodeByOldID(syntaxTree, loop->header->id);
		assert(sTreeNode1);
		loopNode->u.loop.synHeader = sTreeNode1;
		TN_SET_FLAG(sTreeNode1, TN_IS_LOOP_HEADER);
		//printSyntaxTreeNode(sTreeNode1);

		header_index = al_indexOf(syntaxTree, sTreeNode1);
		al_remove(syntaxTree, sTreeNode1);
		al_add(loopNode->u.loop.loopBody, sTreeNode1);
		al_add(loopNode->u.loop.loopBody2, sTreeNode1);

		al_insertAt(syntaxTree, loopNode, header_index);
		//transformGOTOsInLoopBlock(sTreeNode1, loop);

		//then we iterate through the syntaxTree list, and find nodes in loop
		for(i=0;i<al_size(syntaxTree);i++){
			int move_flag=0;
			sTreeNode1 = al_get(syntaxTree, i);
			assert(sTreeNode1->type==TN_BLOCK || sTreeNode1->type==TN_WHILE);
			if(sTreeNode1->type==TN_BLOCK){
				//if block is in the loop
				if(isBlockInTheLoop(loop, sTreeNode1->u.block.cfg_id)){
					assert(!isBlockTheLoopHeader(loop, sTreeNode1->u.block.cfg_id));
					al_remove(syntaxTree, sTreeNode1);
					al_add(loopNode->u.loop.loopBody, sTreeNode1);
					al_add(loopNode->u.loop.loopBody2, sTreeNode1);
					move_flag++;
				}
			}

			else{
				//printf("dealing loop %d\t encounter smaller loop %d\n", loop->header->id, sTreeNode1->u.loop.header->id);
				assert(sTreeNode1->type==TN_WHILE);
				//if block is not the loop we are processing
				if( isBlockInTheLoop(loop, sTreeNode1->u.loop.header->id) &&
						sTreeNode1->u.loop.header->id != loopNode->u.loop.header->id){
					al_remove(syntaxTree, sTreeNode1);
					al_add(loopNode->u.loop.loopBody, sTreeNode1);
					al_add(loopNode->u.loop.loopBody2, sTreeNode1);
					move_flag++;
				}
			}
			if(move_flag){
				//transformGOTOsInLoopBlock(sTreeNode1, loop);
				i--;
			}
		}//end for-loop



		//printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
		loop = findSmallestUnprocessedLoop(loopList, LOOP_FLAG_TMP0);
	}//end while-loop

	for(i=0;i<al_size(loopList);i++){
		loop = al_get(loopList, i);
		LOOP_CLR_FLAG(loop, LOOP_FLAG_TMP0);
	}
}


void createFuncsInSynaxTree(ArrayList *syntaxTree, ArrayList *funcCFGs){
	int i,j;
	Function *func;
	SyntaxTreeNode *funcNode;
	SyntaxTreeNode *sTreeNode1;
	BasicBlock *block;

	//printf("num of func: %d\n", al_size(funcCFGs));

	for(i=0;i<al_size(funcCFGs);i++){
		func = al_get(funcCFGs,  i);

		printf("processing function %s\n", func->funcName);
		for(j=0;j<al_size(func->funcBody);j++){
			block = al_get(func->funcBody, j);
			printf("%d\t", block->id);
		}
		printf("\n");


		funcNode = createSyntaxTreeNode();
		funcNode->type = TN_FUNCTION;
		funcNode->u.func.funcStruct = func;
		funcNode->u.func.funcBody = al_newGeneric(AL_LIST_SET, SyntaxTreeNodeCompareByBlockID, NULL, NULL);


		sTreeNode1 = findBlockNodeByOldID(syntaxTree, func->funcEntryBlock->id);
		assert(sTreeNode1);
		funcNode->u.func.synFuncEntryNode = sTreeNode1;
		TN_SET_FLAG(sTreeNode1, TN_NOT_SHOW_LABEL);
		al_remove(syntaxTree, sTreeNode1);
		al_add(funcNode->u.func.funcBody, sTreeNode1);
		//alway indert function at the beginning
		al_insertAt(syntaxTree, funcNode, 0);

		//then we iterate through the syntaxTree list, and find nodes in function
		for(j=0;j<al_size(syntaxTree);j++){
			sTreeNode1 = al_get(syntaxTree, j);
			assert(sTreeNode1->type==TN_BLOCK || sTreeNode1->type==TN_WHILE || sTreeNode1->type==TN_FUNCTION);
			if(sTreeNode1->type==TN_BLOCK){
				//if block is in the loop
				if(isBlockInTheFunction(func, sTreeNode1->u.block.cfg_id)){
					al_remove(syntaxTree, sTreeNode1);
					al_add(funcNode->u.func.funcBody,  sTreeNode1);
					j--;
				}
			}
			else if(sTreeNode1->type==TN_WHILE){
				//printf("dealing loop %d\t encounter smaller loop %d\n", loop->header->id, sTreeNode1->u.loop.header->id);
				assert(sTreeNode1->type==TN_WHILE);
				//if block is not the loop we are processing
				if( isBlockInTheFunction(func, sTreeNode1->u.loop.header->id)){
					al_remove(syntaxTree, sTreeNode1);
					al_add(funcNode->u.func.funcBody,  sTreeNode1);
					j--;
				}
			}
		}//end for-loop
	}
}

/*
 * this function search the parent block node for the given syntax node.
 * return 2 values:
 * 		1. an ArrayList where the given 'node' locates, NULL if can't find 'node in its parent node's list
 * 		2. ret_index contains the index of 'node' in the returned ArrayList, -1 if not found
 *
 *  XXX: actually, the program abort if can't find 'node' in its parent! -- Gen
 */
ArrayList *findSyntaxTreeNodeLocation(ArrayList *syntaxTree, SyntaxTreeNode *node, int *ret_index){
	int i;
	int list_len;
	SyntaxTreeNode 	*tempSynNode;
	ArrayList 		*ret_list;

	assert(syntaxTree && node && ret_index);

	ret_list = NULL;
	*ret_index = -1;

	//node is at the top-level of syntax tree
	if(node->parent==NULL){
		list_len = al_size(syntaxTree);
		for(i=0;i<list_len;i++){
			tempSynNode = al_get(syntaxTree, i);
			if(tempSynNode->id==node->id){
				ret_list = syntaxTree;
				*ret_index = i;
				break;	//break for-loop
			}
		}//end-for
	}
	else{
		switch(node->parent->type){
		case TN_BLOCK:
			list_len = al_size(node->parent->u.block.statements);
			for(i=0;i<list_len;i++){
				tempSynNode = al_get(node->parent->u.block.statements, i);
				if(tempSynNode->id==node->id){
					ret_list = node->parent->u.block.statements;
					*ret_index = i;
					break;	//break for-loop
				}
			}//end-for
			break;
		case TN_FUNCTION:
			list_len = al_size(node->parent->u.func.funcBody);
			for(i=0;i<list_len;i++){
				tempSynNode = al_get(node->parent->u.func.funcBody, i);
				if(tempSynNode->id==node->id){
					ret_list = node->parent->u.func.funcBody;
					*ret_index = i;
					break;	//break for-loop
				}
			}//end-for
			break;
		case TN_WHILE:
			list_len = al_size(node->parent->u.loop.loopBody);
			for(i=0;i<list_len;i++){
				tempSynNode = al_get(node->parent->u.loop.loopBody, i);
				if(tempSynNode->id==node->id){
					ret_list = node->parent->u.loop.loopBody;
					*ret_index = i;
					break;	//break for-loop
				}
			}//end-for
			break;
		case TN_IF_ELSE:
			//if part first
			list_len = al_size(node->parent->u.if_else.if_path);
			for(i=0;i<list_len;i++){
				tempSynNode = al_get(node->parent->u.if_else.if_path, i);
				if(tempSynNode->id==node->id){
					ret_list = node->parent->u.if_else.if_path;
					*ret_index = i;
					break;	//break for-loop
				}
			}//end-for
			if(ret_list && *ret_index!=-1)	//if we found the node, break and return it
				break;
			//then else part
			list_len = al_size(node->parent->u.if_else.else_path);
			for(i=0;i<list_len;i++){
				tempSynNode = al_get(node->parent->u.if_else.else_path, i);
				if(tempSynNode->id==node->id){
					ret_list = node->parent->u.if_else.else_path;
					*ret_index = i;
					break;	//break for-loop
				}
			}//end-for
			break;
			/*
			 * following are not block type, can't be a parent node
			 * treated the same way
			 */
		case TN_GOTO:
		case TN_RETURN:
		case TN_RETEXP:
		case TN_EXP:
		case TN_DEFVAR:
		default:
			assert(0);
			break;
		}	//end-switch
	}// end if-else
	assert(ret_list && *ret_index!=-1);
	return ret_list;
}

//XXX for non-GOTO nodes, physically and logically adj node are the same
/*
 * this function returns physically adjacent syntax tree node of a given syntax node 'node' in the syntax tree
 * e.g. in this code snippet:
 *
 * 		if(x>0)
 * 			goto label_1;	// stmt0
 * 		else
 * 			x++;
 * 		print(x);		// stmt1
 * 	label_1:
 * 		x = 10;			// stmt2
 *
 * 	the physically adjacent statement of the stmt0 is stmt 1, NOT stmt2
 *
 * return value NULL means node is actually the last statement in the code (or in a function),
 * or the node is a function node (adj-statement doesn't make sense for a function node)
 */
SyntaxTreeNode *findAdjacentStatement(ArrayList *syntaxTree, SyntaxTreeNode *node){
	int i;
	int list_len;
	SyntaxTreeNode *tempSynNode;
	ArrayList *list;

	assert(syntaxTree && node);

	i = -1;
	list = NULL;

	if(node->type==TN_FUNCTION)
		return NULL;

	list = findSyntaxTreeNodeLocation(syntaxTree, node, &i);
	if(list==NULL || i==-1){
		fprintf(stderr, "can't find syntax node#%d in it's parent node! Exiting...\n", node->id);
		assert(0);
	}

	list_len = al_size(list);
	assert(i>=0 && i<list_len);
	tempSynNode = al_get(list, i);
	assert(tempSynNode->id == node->id);
	if(i==list_len-1){	// if node is the last statement in the parent node
		if(node->parent==NULL){
			tempSynNode = NULL;
		}
		//then the adj-statement is the next of its parent node for non-loop
		// or 1st statement in the loop
		else{
			if(node->parent->type==TN_WHILE)
				tempSynNode = node->parent->u.loop.synHeader;
			else
				tempSynNode = findAdjacentStatement(syntaxTree, node->parent);
		}
	}
	else
		//otherwise, we find the next statement
		tempSynNode = al_get(list, i+1);

	return tempSynNode;
}

/*
 * this function returns logically adjacent syntax tree node of a given syntax node 'node' in the syntax tree
 * e.g. in this code snippet:
 *
 * 		if(x>0)
 * 			goto label_1;	// stmt0
 * 		else
 * 			x++;
 * 		print(x);		// stmt1
 * 	label_1:
 * 		x = 10;			// stmt2
 *
 * 	next statement of the stmt0 is stmt2, NOT stmt1, because the goto statement
 *
 * 	for return values:
 * 		- TN_RETURN & TN_RETEXP: NULL
 * 		- TN_GOTO:	jump target node
 * 		- TN_FUNCTION: NULL
 * 		- all other node: same as findAdjacentStatement()
 */
SyntaxTreeNode *findNextStatement(ArrayList *syntaxTree, SyntaxTreeNode *node){
	SyntaxTreeNode *tempSynNode;

	assert(syntaxTree && node);

	switch(node->type){
	case TN_GOTO:
		tempSynNode = node->u.go_to.synTargetBlock;
		break;
	case TN_FUNCTION:
	case TN_RETURN:
	case TN_RETEXP:
		tempSynNode = NULL;
		break;
	case TN_BLOCK:
	case TN_WHILE:
	case TN_IF_ELSE:
	case TN_EXP:
	case TN_DEFVAR:
		tempSynNode = findAdjacentStatement(syntaxTree, node);
		break;
	default:
		assert(0);
		break;
	}	//end-switch

	return tempSynNode;
}


/*
 * this function returns the inner-most loop node which this node belongs to
 * returns NULL is not in any loop
 */
SyntaxTreeNode *syntaxTreeNodeInLoop(SyntaxTreeNode *node){
	assert(node);
	while(node->parent!=NULL){
		if(node->parent->type==TN_WHILE)
			break;
		node = node->parent;
	}
	return node->parent;
}

/*
 * insert a syntax node into a given block-list at the specified index
 */
SyntaxTreeNode *syntaxTreeNodeInsertAt(ArrayList *list, SyntaxTreeNode *node, int index){
	int list_size;
	assert(list && node);
	list_size = al_size(list);
	assert(list_size>=index && index>=0);
	return (SyntaxTreeNode *)al_insertAt(list, (void *)node, index);
}


/*
 * for the following functions:
 * 		insertBefore
 * 		insertAfter
 * 		moveToBefore
 * 		moveToAfter
 *
 * after the function call, the inserted/moved node and the target node have the same parent Node!
 */

SyntaxTreeNode *insertBefore(ArrayList *syntaxTree, SyntaxTreeNode *node_ins, SyntaxTreeNode *node_target){
	int index_target;
	ArrayList *list_target;
	SyntaxTreeNode *tempNode;

	assert(syntaxTree && node_ins && node_target);
	index_target = -1;
	list_target = NULL;
	tempNode = NULL;

	//find the list and index for the node_target
	list_target = findSyntaxTreeNodeLocation(syntaxTree, node_target, &index_target);
	assert(list_target);

	//insert node_ins AFTER node_target in the same list
	tempNode = syntaxTreeNodeInsertAt(list_target, node_ins, index_target);
	assert(tempNode->id==node_ins->id);
	tempNode->parent=node_target->parent;

	return tempNode;
}

SyntaxTreeNode *insertAfter(ArrayList *syntaxTree, SyntaxTreeNode *node_ins, SyntaxTreeNode *node_target){
	int index_target;
	ArrayList *list_target;
	SyntaxTreeNode *tempNode;

	assert(syntaxTree && node_ins && node_target);
	index_target = -1;
	list_target = NULL;
	tempNode = NULL;

	//find the list and index for the node_target
	list_target = findSyntaxTreeNodeLocation(syntaxTree, node_target, &index_target);
	assert(list_target);

	//insert node_ins AFTER node_target in the same list
	tempNode = syntaxTreeNodeInsertAt(list_target, node_ins, index_target+1);
	assert(tempNode->id==node_ins->id);
	tempNode->parent=node_target->parent;

	return tempNode;
}

SyntaxTreeNode *moveToBefore(ArrayList *syntaxTree, SyntaxTreeNode *node_move, SyntaxTreeNode *node_target){
	int index_move;
	ArrayList *list_move;
	SyntaxTreeNode *tempNode;
	assert(syntaxTree && node_move && node_target);

	index_move = -1;
	list_move = NULL;
	tempNode = NULL;

	//find the location of node_move inits parent's list
	list_move = findSyntaxTreeNodeLocation(syntaxTree, node_move, &index_move);
	assert(list_move);

	//remove node_move from its parent
	tempNode = (SyntaxTreeNode *)al_removeAt(list_move, index_move);
	assert(tempNode->id==node_move->id);
	tempNode->parent=NULL;

	//insert node_move immediately after node_target (which meansthey have the SAME parent)
	return insertBefore(syntaxTree, tempNode, node_target);
}

SyntaxTreeNode *moveToAfter(ArrayList *syntaxTree, SyntaxTreeNode *node_move, SyntaxTreeNode *node_target){
	int index_move;
	ArrayList *list_move;
	SyntaxTreeNode *tempNode;
	assert(syntaxTree && node_move && node_target);

	index_move = -1;
	list_move = NULL;
	tempNode = NULL;

	//find the location of node_move inits parent's list
	list_move = findSyntaxTreeNodeLocation(syntaxTree, node_move, &index_move);
	assert(list_move);

	//remove node_move from its parent
	tempNode = (SyntaxTreeNode *)al_removeAt(list_move, index_move);
	assert(tempNode->id==node_move->id);
	tempNode->parent=NULL;

	//insert node_move immediately after node_target (which meansthey have the SAME parent)
	return insertAfter(syntaxTree, tempNode, node_target);
}


/*
 * called after function and loop syntax nodes are created
 * this function also construct a arrayList of cfg_id and syntaxNode id conversion.
 */
void fillParentSyntaxNode(SyntaxTreeNode *parent, SyntaxTreeNode *node, ArrayList *list){
	assert(node && list);
	int i;
	SyntaxTreeNode *tempNode;

	//set the parent of node first
	node->parent = parent;

	//then recursively set each child (for block node)
	switch(node->type){
	case TN_BLOCK:
		for(i=0;i<al_size(node->u.block.statements);i++){
			tempNode = al_get(node->u.block.statements, i);
			fillParentSyntaxNode(node, tempNode, list);
		}
		void **pair = createIdPair();
		pair[0] = (void *)(node->u.block.cfgBlock);
		pair[1] = (void *)node;
		al_add(list, (void *)pair);
		break;
	case TN_FUNCTION:
		for(i=0;i<al_size(node->u.func.funcBody);i++){
			tempNode = al_get(node->u.func.funcBody, i);
			fillParentSyntaxNode(node, tempNode, list);
		}
		break;
	case TN_WHILE:
		for(i=0;i<al_size(node->u.loop.loopBody);i++){
			tempNode = al_get(node->u.loop.loopBody, i);
			fillParentSyntaxNode(node, tempNode, list);
		}
		break;
	case TN_IF_ELSE:
		for(i=0;i<al_size(node->u.if_else.if_path);i++){
			tempNode = al_get(node->u.if_else.if_path, i);
			fillParentSyntaxNode(node, tempNode, list);
		}
		for(i=0;i<al_size(node->u.if_else.else_path);i++){
			tempNode = al_get(node->u.if_else.else_path, i);
			fillParentSyntaxNode(node, tempNode, list);
		}
		break;

	case TN_GOTO:
	case TN_RETURN:
	case TN_RETEXP:
	case TN_EXP:
	case TN_DEFVAR:
	default:
		break;
	}
}

void relinkGotos(SyntaxTreeNode *parent, SyntaxTreeNode *node, ArrayList *list){
	assert(node && list);
	int i;
	SyntaxTreeNode *tempNode;
	void **tempPair;

	TN_SET_FLAG(node, TN_AFTER_RELINK_GOTO);
	//then recursively ssearch each child (for block node)
	switch(node->type){
	case TN_BLOCK:
		for(i=0;i<al_size(node->u.block.statements);i++){
			tempNode = al_get(node->u.block.statements, i);
			relinkGotos(node, tempNode, list);
		}
		break;
	case TN_FUNCTION:
		for(i=0;i<al_size(node->u.func.funcBody);i++){
			tempNode = al_get(node->u.func.funcBody, i);
			relinkGotos(node, tempNode, list);
		}
		break;
	case TN_WHILE:
		for(i=0;i<al_size(node->u.loop.loopBody);i++){
			tempNode = al_get(node->u.loop.loopBody, i);
			relinkGotos(node, tempNode, list);
		}
		break;
	case TN_IF_ELSE:
		for(i=0;i<al_size(node->u.if_else.if_path);i++){
			tempNode = al_get(node->u.if_else.if_path, i);
			relinkGotos(node, tempNode, list);
		}
		for(i=0;i<al_size(node->u.if_else.else_path);i++){
			tempNode = al_get(node->u.if_else.else_path, i);
			relinkGotos(node, tempNode, list);
		}
		break;
	case TN_GOTO:
		tempPair = createIdPair();
		tempPair[0] = (void *)(node->u.go_to.targetBlock);
		int index = al_indexOf(list, (void *)tempPair);
		destroyIdPair((void *)tempPair);
		if(index>=0){
			tempPair = (void **)al_get(list, index);
			tempNode = (SyntaxTreeNode *)(tempPair[1]);
			node->u.go_to.synTargetBlock = tempNode;
			assert(tempNode->predsList);
			//if goto target node is a loop header, then link goto to the loop instead
			if(tempNode->parent && tempNode->parent->type==TN_WHILE &&
					tempNode->id == tempNode->parent->u.loop.synHeader->id){
				al_add(tempNode->parent->predsList, (void *)node);
				node->u.go_to.synTargetBlock = tempNode->parent;
				fprintf(stderr,"synNode#%d: preds %d\n", tempNode->parent->id,
						al_size(tempNode->parent->predsList));
			}else{
			//also set precsList
				al_add(tempNode->predsList, (void *)node);
				fprintf(stderr,"synNode#%d: preds %d\n", tempNode->id,
						al_size(tempNode->predsList));
			}
		}else{
			node->u.go_to.synTargetBlock = NULL;
			assert(0);
		}
		break;
	case TN_RETURN:
	case TN_RETEXP:
	case TN_EXP:
	case TN_DEFVAR:
	default:
		break;
	}
}

/*
 * go though the trace to create functionObjTable.
 * function def instructions (JSOP_DEFFUN, JSOP_CLOSURE) will add a new entry in table, fill in the name and funcObj
 * 		JSOP_ANONFUNOBJ will check if the function is already added (by using instr's addr) before create a new entry
 * Function calling instructions (JSOP_NEW & JSOP_CALL) will find the existing function entry,
 * 		and fill in the the address of function's first instruction ()
 */
ArrayList *constructFuncObjTable(InstrList *iList, ArrayList *funcCFGs, ArrayList *funcObjTable){

#define TABLE_DEBUG 1

	int i,j, size;
	char *str=NULL;
	Instruction *instr=NULL;
	Instruction *next=NULL;
	Function *funcStruct=NULL;
	FuncObjTableEntry *funcObjTableEntry1=NULL;

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
				funcStruct->funcObj = instr->objRef;	//todo make funcStruct->funcObj a List!
			}else{
				assert(!strcmp(funcStruct->funcName, funcObjTableEntry1->func_name));
			}
		}
		//eval, do nothing, just set flag (since eval'ed code doesn't need a name)
		else if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) && (instr->opCode == JSOP_EVAL)){
			assert(next);
			funcStruct = findFunctionByEntryAddress(funcCFGs, next->addr);
			BBL_SET_FLAG(funcStruct->funcEntryBlock, BBL_IS_EVAL_ENTRY);
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



ArrayList *buildSyntaxTree(InstrList *iList, ArrayList *blockList, ArrayList *loopList, ArrayList *funcCFGs){
	int i;
	BasicBlock *block;
	BasicBlock *n0;
	SyntaxTreeNode *tempNode;

	ArrayList *syntaxTree;
	ArrayList *funcObjTable;

	if(!functionCFGProcessed){
		printf("ERROR: please call buildFunctionCFGs() before call buildSyntaxTree()!\n");
		abort();
	}

	syntaxTree = al_newGeneric(AL_LIST_SET, SyntaxTreeNodeCompareByBlockID, NULL, NULL);
	funcObjTable = al_newGeneric(AL_LIST_UNSORTED, NULL, NULL, destroyFuncObjTableEntry);

	syntaxBlockIDctl = 0;
	anonfunobjctl = 0;
	n0=NULL;

	//create funcObjTable first, before the construction of syntax tree;
	constructFuncObjTable(iList, funcCFGs, funcObjTable);
	printFuncObjTable(funcObjTable);

	//make sure flag TMP0 is cleared
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
		if(BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE))
			n0 = block;

#if DEBUG
		if(block->type==BT_SCRIPT_INVOKE)
			printf("block %d calltarget:%d\n", block->id,block->calltarget->id);
#endif
	}

	//build AST for main routine and every function
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		if(BBL_HAS_FLAG(block, BBL_IS_ENTRY_NODE) || BBL_HAS_FLAG(block, BBL_IS_FUNC_ENTRY)){
			//printf("``````````starting with block#%d\n", block->id);
			n0 = block;
			doSearch(n0, BBL_FLAG_TMP0, syntaxTree, funcObjTable, funcCFGs);
		}
	}

	//clear flag TMP0
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}

	//loop finder has to be called before function finder
	createLoopsInSynaxTree(syntaxTree,loopList);

	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}

	createFuncsInSynaxTree(syntaxTree,funcCFGs);

	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}

	al_freeWithElements(funcObjTable);

	//fill parent field of each syntax node
	ArrayList *idPairList = al_newGeneric(AL_LIST_SET, idPairCompare, NULL, destroyIdPair);
	for(i=0;i<al_size(syntaxTree);i++){
		tempNode = al_get(syntaxTree, i);
		fillParentSyntaxNode(NULL, tempNode, idPairList);
	}
	for(i=0;i<al_size(syntaxTree);i++){
		tempNode = al_get(syntaxTree, i);
		relinkGotos(NULL, tempNode, idPairList);
	}
	al_freeWithElements(idPairList);

	tempSynTree = syntaxTree;
	return syntaxTree;
}

/*
 * return if the target of given goto node is in the given loop.
 */
bool isGotoTargetInThisLoop(SyntaxTreeNode *gotoNode, SyntaxTreeNode *whileNode){
	int i;
	bool ret;
	SyntaxTreeNode *tempNode1;
	assert(gotoNode && whileNode);
	assert(gotoNode->type==TN_GOTO);

	if(gotoNode->u.go_to.synTargetBlock->id == whileNode->id){
		return true;
	}

	ret = false;

	for(i=0;i<al_size(whileNode->u.loop.loopBody2);i++){
		tempNode1 = al_get(whileNode->u.loop.loopBody2, i);
		if(tempNode1->id==gotoNode->u.go_to.synTargetBlock->id){
			ret = true;
			break;
		}else if(tempNode1->type==TN_WHILE){
			ret = isGotoTargetInThisLoop(gotoNode, tempNode1);
		}
		if(ret)
			break;
	}
	return ret;
}

/*
 * return the last statement in a given node, if node is a block node
 * else, return the node itself.
 * NOTE: IF_ELSE node is treated as a non-block node, since you cannot say which statement is THE final statement due to the 2 way choice
 */
SyntaxTreeNode *lastStatementInBlock(SyntaxTreeNode *node){
	assert(node);
	SyntaxTreeNode *ret, *tempNode1;

	switch(node->type){
	case TN_BLOCK:
		tempNode1 = al_get(node->u.block.statements, al_size(node->u.block.statements)-1);
		ret = lastStatementInBlock(tempNode1);
		break;
	case TN_FUNCTION:
		tempNode1 = al_get(node->u.func.funcBody, al_size(node->u.func.funcBody)-1);
		ret = lastStatementInBlock(tempNode1);
		break;
	case TN_WHILE:
		tempNode1 = al_get(node->u.loop.loopBody, al_size(node->u.loop.loopBody)-1);
		ret = lastStatementInBlock(tempNode1);
		break;
	case TN_IF_ELSE:
	case TN_GOTO:
	case TN_RETURN:
	case TN_RETEXP:
	case TN_EXP:
	case TN_DEFVAR:
	default:
		ret = node;
		break;
	}//end switch
	return ret;
}

/*
 * Move block to after its only GOTO predecessor.
 *		- don't move loop header
 *		- don't move if already located immediately after predecessor goto node,
 *		 	set flag TN_HIDE on goto node, and TN_NOT_SHOW_LABEL onthe target node
 *		- don't move node into a loop from outside
 */
bool trans_goto_helper_move(ArrayList *syntaxTree, SyntaxTreeNode *node){

	assert(node && syntaxTree);
	int i,j;
	bool flag;
	SyntaxTreeNode *tempNode1, *tempNode2, *tempNode3;
	ArrayList *tempList1, *tempList2;


	//if node has only one predecessor
	if(al_size(node->predsList)==1 && !TN_HAS_FLAG(node, TN_IS_LOOP_HEADER)){
		printf("blk#%d with only one pred\n", node->id);
		tempNode1 = (SyntaxTreeNode *)al_get(node->predsList,0);
		assert(tempNode1->type==TN_GOTO);

		tempList1 = findSyntaxTreeNodeLocation(syntaxTree, tempNode1, &i);
		tempList2 = findSyntaxTreeNodeLocation(syntaxTree, node, &j);

		//we have to check if node is already in place
		//(immediately after the onlu predecess goto node)
		if(tempList1!=tempList2 || i!=j-1){

			//check if we are move a node into a loop which doesn't belong to
			tempNode2 = syntaxTreeNodeInLoop(tempNode1);
			tempNode3 = syntaxTreeNodeInLoop(node);

			if((tempNode2==NULL && tempNode3==NULL) ||
					((tempNode2!=NULL && tempNode3!=NULL) && (tempNode2->id == tempNode3->id))
				){
				printf("move blk#%d to after blk%d\n", node->id, tempNode1->id);
				tempNode2 = moveToAfter(syntaxTree, node, tempNode1);
				assert(tempNode2->parent == tempNode1->parent);
				return true;
			}
			else{
				printf("blk#%d deon't belong to the blk#%d's loop, stop moving\n", node->id, tempNode1->id);
			}
		}
		else{
			printf("succ of blk#%d (blk#%d) is already in place\n", tempNode1->id, node->id);
		}
	}

	//then recursively ssearch each child (for block node)
	flag=false;
	switch(node->type){
	case TN_BLOCK:
		for(i=0;i<al_size(node->u.block.statements);i++){
			tempNode1 = al_get(node->u.block.statements, i);
			flag=trans_goto_helper_move(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_FUNCTION:
		for(i=0;i<al_size(node->u.func.funcBody);i++){
			tempNode1 = al_get(node->u.func.funcBody, i);
			flag=trans_goto_helper_move(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_WHILE:
		for(i=0;i<al_size(node->u.loop.loopBody);i++){
			tempNode1 = al_get(node->u.loop.loopBody, i);
			flag=trans_goto_helper_move(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_IF_ELSE:
		for(i=0;i<al_size(node->u.if_else.if_path);i++){
			tempNode1 = al_get(node->u.if_else.if_path, i);
			flag=trans_goto_helper_move(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		if(flag)
			break;
		for(i=0;i<al_size(node->u.if_else.else_path);i++){
			tempNode1 = al_get(node->u.if_else.else_path, i);
			flag=trans_goto_helper_move(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_GOTO:
	case TN_RETURN:
	case TN_RETEXP:
	case TN_EXP:
	case TN_DEFVAR:
	default:
		break;
	}//end switch
	return flag;
}

bool trans_goto_helper_continue(ArrayList *syntaxTree, SyntaxTreeNode *node){

	assert(node && syntaxTree);
	int i;
	bool flag;
	SyntaxTreeNode *tempNode1;

	//then recursively search each child (for block node)
	flag=false;
	switch(node->type){
	case TN_GOTO:
		tempNode1 = syntaxTreeNodeInLoop(node);
		if(tempNode1){
			if(!isGotoTargetInThisLoop(node, tempNode1)){
				printf("goto #%d target #%d NOT in the same loop\n", node->id, node->u.go_to.synTargetBlock->id);
			}else{
				printf("goto #%d target #%d in the same loop\n",  node->id, node->u.go_to.synTargetBlock->id);
			}
		}else{
			printf("goto #%d (target#%d) is not in a loop\n",  node->id, node->u.go_to.synTargetBlock->id);
		}
		if(TN_HAS_FLAG(node, TN_IS_GOTO_CONTINUE))
			break;
		if(node->u.go_to.synTargetBlock->type==TN_WHILE){
			tempNode1 = syntaxTreeNodeInLoop(node);
			//if this is a goto to a different loop-header (which node doesn't belong to)
			if(tempNode1==NULL || tempNode1->id!=node->u.go_to.synTargetBlock->id)
				break;
			TN_SET_FLAG(node, TN_IS_GOTO_CONTINUE);
			al_remove(node->u.go_to.synTargetBlock->predsList, (void *)node);
			flag = true;
		}
		break;

	case TN_BLOCK:
		for(i=0;i<al_size(node->u.block.statements);i++){
			tempNode1 = al_get(node->u.block.statements, i);
			flag=trans_goto_helper_continue(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_FUNCTION:
		for(i=0;i<al_size(node->u.func.funcBody);i++){
			tempNode1 = al_get(node->u.func.funcBody, i);
			flag=trans_goto_helper_continue(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_WHILE:
		for(i=0;i<al_size(node->u.loop.loopBody);i++){
			tempNode1 = al_get(node->u.loop.loopBody, i);
			flag=trans_goto_helper_continue(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_IF_ELSE:
		for(i=0;i<al_size(node->u.if_else.if_path);i++){
			tempNode1 = al_get(node->u.if_else.if_path, i);
			flag=trans_goto_helper_continue(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		if(flag)
			break;
		for(i=0;i<al_size(node->u.if_else.else_path);i++){
			tempNode1 = al_get(node->u.if_else.else_path, i);
			flag=trans_goto_helper_continue(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_RETURN:
	case TN_RETEXP:
	case TN_EXP:
	case TN_DEFVAR:
	default:
		break;
	}//end switch
	return flag;
}


bool trans_goto(ArrayList *syntaxTree){
	int i;
	bool ret;
	SyntaxTreeNode *tempNode;

	assert(syntaxTree);

	ret = false;

	//loop until no movement
	while(true){
		bool change = false;
		for(i=0;i<al_size(syntaxTree);i++){
			tempNode = al_get(syntaxTree, i);
			change = change || trans_goto_helper_move(syntaxTree, tempNode);
			change = change || trans_goto_helper_continue(syntaxTree, tempNode);
			if(change)	//do it over if we made a movement, to avoid list length issue
				break;
		}
		ret = ret || change;
		//if after a full loop, no movement, then we done
		if(!change)
			break;
	}
	return ret;
}




bool trans_while_helper_infWhile(ArrayList *syntaxTree, SyntaxTreeNode *node){
	assert(node && syntaxTree);
	int i;
	bool flag;
	SyntaxTreeNode *tempNode1, *tempNode2, *tempNode3;
	ArrayList *tempList;
	//then recursively search each child (for block node)
	flag=false;
	switch(node->type){

	case TN_WHILE:
		//not infinite loop
		if(!expIsTrue(node->u.loop.cond)){
			printf("+++ loop #%d node not infinite\n", node->id);
			for(i=0;i<al_size(node->u.loop.loopBody);i++){
				tempNode1 = al_get(node->u.loop.loopBody, i);
				flag=trans_while_helper_infWhile(syntaxTree, tempNode1);
				if(flag)
					break;
			}
		}
		else{
			//if the first statement in loop is if-else
			if((tempNode1=node)->u.loop.synHeader->type==TN_IF_ELSE ||
					(node->u.loop.synHeader->type==TN_BLOCK &&
							(tempNode1=((SyntaxTreeNode *)al_get(node->u.loop.synHeader->u.block.statements, 0)))->type==TN_IF_ELSE
					)
				){
				printf("+++ first statement in inf-loop #%d is if-else\n", node->id);
				tempNode2 = (SyntaxTreeNode *)al_get(tempNode1->u.if_else.if_path,0);
				tempNode3 = (SyntaxTreeNode *)al_get(tempNode1->u.if_else.else_path,0);
				//if if branch is a goto out of loop
				if(tempNode2 && tempNode2->type==TN_GOTO && !isGotoTargetInThisLoop(tempNode2, node)){
					printf("+++ combine else and loop#%d\n", node->id);
					//set loop condition
					node->u.loop.cond = createExpTreeNode();
					node->u.loop.cond->type=EXP_UN;
					node->u.loop.cond->u.un_op.op=OP_NOT;
					node->u.loop.cond->u.un_op.lval=tempNode1->u.if_else.cond;
					//move goto after loop
					moveToAfter(syntaxTree, tempNode2, node);
					//move all the statements in else branch before if-else node
					while(al_size(tempNode1->u.if_else.else_path)>0){
						moveToBefore(syntaxTree, (SyntaxTreeNode *)al_get(tempNode1->u.if_else.else_path,0), tempNode1);
					}
					//remove if-else node entirely
					int if_else_index=-1;
					tempList = findSyntaxTreeNodeLocation(syntaxTree, tempNode1, &if_else_index);
					assert(tempList);
					al_removeAt(tempList, if_else_index);

					flag = true;
				}
				//if else branch is a goto out of loop
				else if(tempNode3 && tempNode3->type==TN_GOTO && !isGotoTargetInThisLoop(tempNode3, node)){
					printf("+++ combine if and loop#%d\n", node->id);
					//set loop condition
					node->u.loop.cond = tempNode1->u.if_else.cond;
					//move goto after loop
					moveToAfter(syntaxTree, tempNode3, node);
					//move all the statements in else branch before if-else node
					while(al_size(tempNode1->u.if_else.if_path)>0){
						moveToBefore(syntaxTree, (SyntaxTreeNode *)al_get(tempNode1->u.if_else.if_path,0), tempNode1);
					}
					//remove if-else node entirely
					int if_else_index=-1;
					tempList = findSyntaxTreeNodeLocation(syntaxTree, tempNode1, &if_else_index);
					assert(tempList);
					al_removeAt(tempList, if_else_index);

					flag = true;
				}
				//none goto is out of loop
				else{
					for(i=0;i<al_size(node->u.loop.loopBody);i++){
						tempNode1 = al_get(node->u.loop.loopBody, i);
						flag=trans_while_helper_infWhile(syntaxTree, tempNode1);
						if(flag)
							break;
					}
				}
			}
			//1st statement is not if-else
			else{
				printf("+++ first statement in inf-loop #%d is NOT if-else\n", node->id);
			}
		}
		break;
	case TN_BLOCK:
		for(i=0;i<al_size(node->u.block.statements);i++){
			tempNode1 = al_get(node->u.block.statements, i);
			flag=trans_while_helper_infWhile(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_FUNCTION:
		for(i=0;i<al_size(node->u.func.funcBody);i++){
			tempNode1 = al_get(node->u.func.funcBody, i);
			flag=trans_while_helper_infWhile(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_IF_ELSE:
		for(i=0;i<al_size(node->u.if_else.if_path);i++){
			tempNode1 = al_get(node->u.if_else.if_path, i);
			flag=trans_while_helper_infWhile(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		if(flag)
			break;
		for(i=0;i<al_size(node->u.if_else.else_path);i++){
			tempNode1 = al_get(node->u.if_else.else_path, i);
			flag=trans_while_helper_infWhile(syntaxTree, tempNode1);
			if(flag)
				break;
		}
		break;
	case TN_RETURN:
	case TN_RETEXP:
	case TN_EXP:
	case TN_DEFVAR:
	case TN_GOTO:
	default:
		break;
	}//end switch
	return flag;

}

bool trans_while(ArrayList *syntaxTree){
	int i;
	bool ret;
	SyntaxTreeNode *tempNode;

	assert(syntaxTree);
	ret = false;
	//loop until no movement
	while(true){
		bool change = false;
		for(i=0;i<al_size(syntaxTree);i++){
			tempNode = al_get(syntaxTree, i);
			change = change || trans_while_helper_infWhile(syntaxTree, tempNode);
			if(change)	//do it over if we made a movement, to avoid list length issue
				break;
		}
		ret = ret || change;
		if(!change)
			break;
	}
	return ret;
}


bool trans_if_else(ArrayList *syntaxTree){
	int i;
	bool ret;
	SyntaxTreeNode *tempNode;

	assert(syntaxTree);
	ret = false;
	//loop until no movement
	while(true){
		bool change = false;
		for(i=0;i<al_size(syntaxTree);i++){
			tempNode = al_get(syntaxTree, i);
			//change = change || trans_if_else_helper_commGotos(syntaxTree, tempNode);
			if(change)	//do it over if we made a movement, to avoid list length issue
				break;
		}
		ret = ret || change;
		if(!change)
			break;
	}
	return ret;
}

void transformSyntaxTree(ArrayList *syntaxTree){
	assert(syntaxTree);
	//loop until no movement
	while(true){
		bool change = false;
		change = change || trans_goto(syntaxTree);
		change = change || trans_while(syntaxTree);
		change = change || trans_if_else(syntaxTree);
		if(!change)
			break;
	}
	return;
}







