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

static int syntaxBlockIDctl;
static int anonfunobjctl;

char *this_str = "this";
char *anonfunobj_str = "anonfunobj";
char *arg_str = "arg";
char *local_var_str = "local_var";
char *length_str = "length";
char *null_str = "null";

#define DEBUG 0

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
	entry->flag = 0;
	entry->funcName = NULL;
	entry->funcObjAddr = 0;
	entry->funcStruct = NULL;
	return entry;
}

void destroyFuncObjTableEntry(void *e){
	assert(e);
	FuncObjTableEntry *entry = (FuncObjTableEntry *)e;
	if(entry->funcName){
		free(entry->funcName);
		entry->funcName = NULL;
	}
	free(entry);
}

int FuncObjTableEntryCompare(void *a1, void *a2){
	assert(a1 && a2);
	return ((FuncObjTableEntry *)a1)->funcObjAddr - ((FuncObjTableEntry *)a2)->funcObjAddr;
}

FuncObjTableEntry *findFuncObjTableEntry(ArrayList *FuncObjTable, ADDRESS funcObj){
	assert(FuncObjTable);
	assert(funcObj);
	int i;
	FuncObjTableEntry *entry;
	for(i=0;i<al_size(FuncObjTable);i++){
		entry = al_get(FuncObjTable, i);
		if(entry->funcObjAddr == funcObj){
			return entry;
		}
	}
	return NULL;
}

void printFuncObjTableEntry(FuncObjTableEntry *entry){
	printf("funcName: %16s\t funcObj:%lx\ttargetBlock:%d\n", entry->funcName, entry->funcObjAddr, entry->funcStruct?entry->funcStruct->funcEntryBlock->id:-1);
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
		for(i=node->u.func.num_paras-1;i>=0;i--){
			expNode = (ExpTreeNode *)al_get(node->u.func.parameters, i);
			printExpTreeNode(expNode);
			if(i)
				printf(", ");
		}
		printf(" )");
		if(node->u.func.funcEntryBlock){
			sprintf(str, "block_%d", node->u.func.funcEntryBlock->id);
			printf("/* function/eval'ed code -> %s */", str);
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
			switch(node->u.un_op.op){
			case OP_NOT:
				printf("!");
				printExpTreeNode(node->u.un_op.lval);
			default:
				break;
			}
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
			assert(node->u.array_init.size = al_size(node->u.array_init.initValues));
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
	memset(&(node->u), 0, sizeof(node->u));
	return node;
}

void destroySyntaxTreeNode(SyntaxTreeNode *node){
	//TODO
	if(node)
		free(node);
}


int SyntaxTreeNodeCompareByBlockID(void *i1, void *i2){
	assert(i1 && i2);
	assert(((SyntaxTreeNode *)i1)->type == TN_BLOCK || ((SyntaxTreeNode *)i1)->type == TN_WHILE || ((SyntaxTreeNode *)i1)->type == TN_FUNCTION);
	assert(((SyntaxTreeNode *)i2)->type == TN_BLOCK || ((SyntaxTreeNode *)i2)->type == TN_WHILE || ((SyntaxTreeNode *)i1)->type == TN_FUNCTION);
	int id1, id2;
	if(((SyntaxTreeNode *)i1)->type == TN_BLOCK){
		id1 = ((SyntaxTreeNode *)i1)->u.block.old_id;
	}else if(((SyntaxTreeNode *)i1)->type == TN_WHILE){
		id1 = ((SyntaxTreeNode *)i1)->u.loop.header->id;
	}else
		id1 = ((SyntaxTreeNode *)i1)->u.func.funcStruct->funcEntryBlock->id;

	if(((SyntaxTreeNode *)i2)->type == TN_BLOCK){
		id2 = ((SyntaxTreeNode *)i2)->u.block.old_id;
	}else if(((SyntaxTreeNode *)i2)->type == TN_WHILE){
		id2 = ((SyntaxTreeNode *)i2)->u.loop.header->id;
	}else
		id2 = ((SyntaxTreeNode *)i2)->u.func.funcStruct->funcEntryBlock->id;
	return id1-id2;
}


void printSyntaxTreeNode(SyntaxTreeNode *node){


	int i;
	char *str;
	SyntaxTreeNode 	*sTreeNode;

	//print an empty statement
	if(!node){
		printf(";\t//this branch's likely missing from dynamic trace\n");
		return;
	}

	str = (char *)malloc(16);
	switch(node->type){

	case TN_FUNCTION:
		if(BBL_HAS_FLAG(node->u.func.funcStruct->funcEntryBlock, BBL_IS_EVAL_ENTRY)){
			printf("//eval''ed block\n");
			printf("{\n");
		}
		else{
			printf("function %s ", node->u.func.funcStruct->funcName);
			printf("(");
			for(i=0;i<=node->u.func.funcStruct->args;i++){
				printf("%s%d", arg_str, i);
				if(i<node->u.func.funcStruct->args-1)
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
		/*		sprintf(str, "block_%d:", node->u.loop.header->id);
		printf("\n%s\n", str);*/
		printf("while(true) {\n");
		for(i=0;i<al_size(node->u.loop.loopBody);i++){
			sTreeNode = (SyntaxTreeNode *)al_get(node->u.loop.loopBody, i);
			assert(sTreeNode);
			printSyntaxTreeNode(sTreeNode);
			printf("\n");
		}
		printf("}\n");
		break;
	case TN_BLOCK:
		sprintf(str, "block_%d:", node->u.block.old_id);
		printf("\n%s\n", str);
		for(i=0;i<al_size(node->u.block.statements);i++){
			sTreeNode = (SyntaxTreeNode *)al_get(node->u.block.statements, i);
			assert(sTreeNode);
			printSyntaxTreeNode(sTreeNode);
			printf(";\n");
		}
		printf("\n");
		break;

	case TN_ASSIGN:
		printExpTreeNode(node->u.assign.lval);
		printf(" = ");
		printExpTreeNode(node->u.assign.rval);
		break;

	case TN_EXP:
		printExpTreeNode(node->u.expNode);
		break;

	case TN_GOTO:
		if(TN_HAS_FLAG(node, TN_IS_GOTO_BREAK)){
			printf("break;");
		}else if(TN_HAS_FLAG(node, TN_IS_GOTO_CONTINUE)){
			printf("continue;");
		}else{
			sprintf(str, "block_%d", node->u.go_to.targetBlock->id);
			printf("goto %s ", str);
		}
		break;

	case TN_DEFVAR:
		printf("var ");
		printExpTreeNode(node->u.expNode);
		break;

	case TN_INC_DEC:
		printf("( ");
		switch(node->u.inc_dec.op){
		case OP_VINC:
			printExpTreeNode(node->u.inc_dec.name);
			printf("++");
			break;
		case OP_VDEC:
			printExpTreeNode(node->u.inc_dec.name);
			printf("--");
			break;
		case OP_INCV:
			printf("++");
			printExpTreeNode(node->u.inc_dec.name);
			break;
		case OP_DECV:
			printf("--");
			printExpTreeNode(node->u.inc_dec.name);
			break;
		default:
			break;
		}
		printf(" )");
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

			printSyntaxTreeNode(node->u.if_else.if_path);
			printf(";");

			printf(" \n}\nelse {\n");

			printSyntaxTreeNode(node->u.if_else.else_path);
			printf(";");

			printf("\n}");
			break;

		default:
			break;
	}//end switch

	free(str);
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

		//convert EXP_CALL and EXP_NEW expNode into a SyntaxTree node with type TN_EXP
		//then add converted node into list, ignore all other ExpTreeNode
		else{
			assert(stackNode1->type = EXP_NODE);
			expTreeNode = (ExpTreeNode *)(stackNode1->node);
			if(expTreeNode->type==EXP_CALL || expTreeNode->type==EXP_NEW || expTreeNode->type==EXP_EVAL){
				//XXX this will cause memory leaks in stack
				// 	  those ignored expNode will not be freed whrn stack node is destroyed.
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

	int i;

	int tempInt;
	char *str;
	Instruction 	*instr;
	BlockEdge		*edge;
	SyntaxStack 	*stack;
	SyntaxTreeNode 	*syntaxBlockNode, *sTreeNode1, *sTreeNode2;
	ExpTreeNode		*expTreeNode1, *expTreeNode2, *expTreeNode3, *expTreeNode4;
	StackNode 		*stackNode1, *stackNode2, *stackNode3, *stackNode4;
	FuncObjTableEntry *funcObjTableEntry1;

	syntaxBlockNode = createSyntaxTreeNode();
	syntaxBlockNode->type = TN_BLOCK;
	syntaxBlockNode->u.block.old_id = block->id;
	syntaxBlockNode->u.block.id = syntaxBlockIDctl++;
	syntaxBlockNode->u.block.statements = al_new();


	stack = createSyntaxStack();

	start_over:
#if DEBUG
	printf("~~~~~~~~~~~start-over@\n");
	printBasicBlock(block);
#endif
	BBL_SET_FLAG(block, flag);

	for(i=0;i<al_size(block->instrs);i++){
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

		// main dispatching
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
		case JSOP_NULL:
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
			else if(instr->opCode==JSOP_NULL){
				str = (char *)malloc(strlen(null_str)+1);
				assert(str);
				memcpy(str, null_str, strlen(null_str)+1);
				expTreeNode1->u.const_value_str = str;
				EXP_SET_FLAG(expTreeNode1, EXP_IS_STRING);
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
				EXP_SET_FLAG(expTreeNode1, EXP_IS_FUNCTION_OBJ);
				str = (char *)malloc(strlen(anonfunobj_str)+5);
				assert(str);
				sprintf(str, "%s%d", anonfunobj_str, anonfunobjctl++);
				//memcpy(str, anonfunobj_str, strlen(anonfunobj_str)+1);
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
			//  1. type == TN_ASSIGN
			//xxx right now rval can only be exp, threfore 'a=x++' is not supported yet;
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
			// if rval is an function obj (the flag is set by JSOP_ANONFUNCOBJ only for now)
			// we need to set up an entry in funcObjTable
			if(EXP_HAS_FLAG(expTreeNode1, EXP_IS_FUNCTION_OBJ)){
				str = (char *)malloc(strlen(expTreeNode1->u.name)+1);
				assert(str);
				memcpy(str, expTreeNode1->u.name, strlen(expTreeNode1->u.name)+1);
				funcObjTableEntry1 = createFuncObjTableEntry();
				funcObjTableEntry1->funcName = str;
				funcObjTableEntry1->funcObjAddr = instr->objRef;
				assert(!findFuncObjTableEntry(funcObjTable, funcObjTableEntry1->funcObjAddr));
				al_add(funcObjTable, funcObjTableEntry1);
				str = NULL;
			}
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
			}
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_ASSIGN;
			sTreeNode1->u.assign.lval = expTreeNode2;
			sTreeNode1->u.assign.rval = expTreeNode1;
			stackNode2 = createSyntaxStackNode();
			stackNode2->type = SYN_NODE;
			stackNode2->node = (void *)sTreeNode1;
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
			// if rval is an function obj (the flag is set by JSOP_ANONFUNCOBJ only for now)
			// we need to set up an entry in funcObjTable
			if(EXP_HAS_FLAG(expTreeNode1, EXP_IS_FUNCTION_OBJ)){
				str = (char *)malloc(strlen(expTreeNode1->u.name)+1);
				assert(str);
				memcpy(str, expTreeNode1->u.name, strlen(expTreeNode1->u.name)+1);
				funcObjTableEntry1 = createFuncObjTableEntry();
				funcObjTableEntry1->funcName = str;
				funcObjTableEntry1->funcObjAddr = instr->objRef;
				assert(!findFuncObjTableEntry(funcObjTable, funcObjTableEntry1->funcObjAddr));
				al_add(funcObjTable, funcObjTableEntry1);
				str = NULL;
				//printFuncObjTable(funcObjTable);
			}
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
			//finally, create a TN_ASSIGN node
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_ASSIGN;
			sTreeNode1->u.assign.lval = expTreeNode4;
			sTreeNode1->u.assign.rval = expTreeNode1;
			//push it into stack
			stackNode3 = createSyntaxStackNode();
			stackNode3->type = SYN_NODE;
			stackNode3->node = (void *)sTreeNode1;
			pushSyntaxStack(stack, stackNode3);
			destroySyntaxStackNode(stackNode1);
			destroySyntaxStackNode(stackNode2);
			break;

			//  2. type == TN_IF_ELSE
		case JSOP_IFEQ:
		case JSOP_IFNE:
		case JSOP_AND:
		case JSOP_OR:
#if DEBUG
			printf("-- TN_GOTO  block#%d: %lx %s\n", instr->inBlock->id, instr->addr, instr->opName);
#endif
			stackNode1 = popSyntaxStack(stack);			//cond
			assert(stackNode1);
			assert(stackNode1->type == EXP_NODE);
			expTreeNode1 = (ExpTreeNode *)stackNode1->node;
			// create a TN_IF_ELSE node
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_IF_ELSE;
			sTreeNode1->u.if_else.cond = expTreeNode1;

			sTreeNode1->u.if_else.if_path = sTreeNode1->u.if_else.else_path = NULL;
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
						sTreeNode1->u.if_else.if_path = sTreeNode2;
					}
					else if(instr->opCode==JSOP_IFNE){
						sTreeNode1->u.if_else.else_path = sTreeNode2;
					}
				}
				else if(EDGE_HAS_FLAG(edge, EDGE_IS_BRANCHED_PATH)){
					sTreeNode2 = createSyntaxTreeNode();
					sTreeNode2->type = TN_GOTO;
					sTreeNode2->u.go_to.targetBlock = edge->head;
					if(instr->opCode==JSOP_IFEQ){
						sTreeNode1->u.if_else.else_path = sTreeNode2;
					}
					else if(instr->opCode==JSOP_IFNE){
						sTreeNode1->u.if_else.if_path = sTreeNode2;
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


			//  7. type == TN_INC_DEC
		case JSOP_GVARINC:
		case JSOP_GVARDEC:
		case JSOP_INCGVAR:
		case JSOP_DECGVAR:
		case JSOP_NAMEINC:
		case JSOP_NAMEDEC:
		case JSOP_INCNAME:
		case JSOP_DECNAME:
			//printf("-- TN_INC_DEC  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_NAME;

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
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_INC_DEC;
			sTreeNode1->u.inc_dec.name = expTreeNode1;

			if(instr->opCode == JSOP_GVARINC || instr->opCode == JSOP_NAMEINC){
				sTreeNode1->u.inc_dec.op = OP_VINC;
			}
			if(instr->opCode == JSOP_GVARDEC || instr->opCode == JSOP_NAMEDEC){
				sTreeNode1->u.inc_dec.op = OP_VDEC;
			}
			if(instr->opCode == JSOP_INCGVAR || instr->opCode == JSOP_INCNAME){
				sTreeNode1->u.inc_dec.op = OP_INCV;
			}
			if(instr->opCode == JSOP_DECGVAR || instr->opCode == JSOP_DECNAME){
				sTreeNode1->u.inc_dec.op = OP_DECV;
			}
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = SYN_NODE;
			stackNode1->node = (void *)sTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;

		case JSOP_ARGINC:
		case JSOP_INCARG:
		case JSOP_ARGDEC:
		case JSOP_DECARG:
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_NAME;
			str = (char *)malloc(strlen(arg_str)+5);
			assert(str);
			sprintf(str, "%s%ld", arg_str, instr->operand.i);
			expTreeNode1->u.un_op.name = str;
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_INC_DEC;
			sTreeNode1->u.inc_dec.name = expTreeNode1;
			if(instr->opCode == JSOP_ARGINC){
				sTreeNode1->u.inc_dec.op = OP_VINC;
			}
			if(instr->opCode == JSOP_ARGDEC){
				sTreeNode1->u.inc_dec.op = OP_VDEC;
			}
			if(instr->opCode == JSOP_INCARG){
				sTreeNode1->u.inc_dec.op = OP_INCV;
			}
			if(instr->opCode == JSOP_DECARG){
				sTreeNode1->u.inc_dec.op = OP_DECV;
			}
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = SYN_NODE;
			stackNode1->node = (void *)sTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;

		case JSOP_VARINC:
		case JSOP_INCVAR:
		case JSOP_VARDEC:
		case JSOP_DECVAR:
			expTreeNode1 = createExpTreeNode();
			expTreeNode1->type = EXP_NAME;
			str = (char *)malloc(strlen(local_var_str)+5);
			assert(str);
			sprintf(str, "%s%ld", local_var_str, instr->operand.i);
			expTreeNode1->u.un_op.name = str;
			sTreeNode1 = createSyntaxTreeNode();
			sTreeNode1->type = TN_INC_DEC;
			sTreeNode1->u.inc_dec.name = expTreeNode1;
			if(instr->opCode == JSOP_VARINC){
				sTreeNode1->u.inc_dec.op = OP_VINC;
			}
			if(instr->opCode == JSOP_VARDEC){
				sTreeNode1->u.inc_dec.op = OP_VDEC;
			}
			if(instr->opCode == JSOP_INCVAR){
				sTreeNode1->u.inc_dec.op = OP_INCV;
			}
			if(instr->opCode == JSOP_DECVAR){
				sTreeNode1->u.inc_dec.op = OP_DECV;
			}
			stackNode1 = createSyntaxStackNode();
			stackNode1->type = SYN_NODE;
			stackNode1->node = (void *)sTreeNode1;
			pushSyntaxStack(stack, stackNode1);
			break;


			//  8.  type ==TN_DEFVAR
		case JSOP_DEFVAR:
			//printf("-- TN_INC_DEC  block#%d: %lx %s\n", instr->inBlock, instr->addr, instr->opName);
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

			// deffun will not create any syntax related nodes, just try to set up funcObjTable
		case JSOP_DEFFUN:
		case JSOP_CLOSURE:
			str = (char *)malloc(strlen(instr->str)+1);
			assert(str);
			memcpy(str, instr->str, strlen(instr->str)+1);
			funcObjTableEntry1 = createFuncObjTableEntry();
			funcObjTableEntry1->funcName = str;
			funcObjTableEntry1->funcObjAddr = instr->objRef;
			assert(!findFuncObjTableEntry(funcObjTable, funcObjTableEntry1->funcObjAddr));
			al_add(funcObjTable, funcObjTableEntry1);
			break;


		case JSOP_BINDNAME:
		case JSOP_POP:
		case JSOP_POPV:
		case JSOP_LINENO:
		default:
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


	else*/ if(block->type==BT_FALL_THROUGH ){

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
		sTreeNode2->u.block.old_id = block->id;
		sTreeNode2->u.block.id = syntaxBlockIDctl++;
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

	return syntaxBlockNode;
}// end buildSyntaxTreeForBlock()


SyntaxTreeNode *findBlockNodeByOldID(ArrayList *syntaxTree, int oldID){
	assert(syntaxTree);
	int i;
	SyntaxTreeNode *node;
	for(i=0;i<al_size(syntaxTree);i++){
		node = al_get(syntaxTree, i);
		//assert(node->type == TN_BLOCK);
		if(node->type == TN_BLOCK && node->u.block.old_id==oldID)
			return node;
		if(node->type == TN_WHILE && node->u.loop.header->id==oldID )
			return node;
	}
	printf("ERROR: cannot find blockNode in syntax tree with old ID %d\n", oldID);
	return NULL;
}

/*ArrayList *buildFunctionsInSyntaxTree(ArrayList *syntaxTree, ArrayList *funcObjTable){
	int i,j,k;
	FuncObjTableEntry *entry;
	Function *func;
	SyntaxTreeNode *sTreeNode1, *sTreeNode2;
	SyntaxTreeNode *funcNode;

	funcNode = createSyntaxTreeNode();
	funcNode->type = TN_FUNCTION;
	funcNode->u.func.funcBody = al_new();
	funcNode->u.func.funcStruct = NULL;

	for(i=0;i<al_size(funcObjTable);i++){
		entry = al_get(funcObjTable, i);
		func = entry->funcStruct;
	}

}*/


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

	for(i=0;i<al_size(n->dominate);i++){
		edge = al_get(n->dominate, i);
		block = edge->head;
		if(!BBL_HAS_FLAG(block, flag)){
			doSearch(block, flag, syntaxTree, funObjTable, funcCFGs);
		}
	}
}

void transformGOTOsInLoopBlock(SyntaxTreeNode *loopBlock, NaturalLoop *loop){
	assert(loopBlock && loop);
	int i;
	SyntaxTreeNode *sTreeNode;


	switch(loopBlock->type){
	case TN_IF_ELSE:
		//printf("dealing if-else\n");
		//printSyntaxTreeNode(loopBlock->u.if_else.if_path);
		transformGOTOsInLoopBlock(loopBlock->u.if_else.if_path, loop);
		//printSyntaxTreeNode(loopBlock->u.if_else.else_path);
		transformGOTOsInLoopBlock(loopBlock->u.if_else.else_path, loop);
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
		//printf("CAUTION: transformGOTOsInLoopBlock() called on a TN_BLOCK:%d\n", loopBlock->u.block.old_id);
		for(i=0;i<al_size(loopBlock->u.block.statements);i++){
			sTreeNode = al_get(loopBlock->u.block.statements, i);
			transformGOTOsInLoopBlock(sTreeNode, loop);
		}
		break;
	case TN_INC_DEC:
	case TN_ASSIGN:
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
		loopNode->u.loop.loopBody = al_newGeneric(AL_LIST_SORTED, SyntaxTreeNodeCompareByBlockID, NULL, NULL);
		//printNaturalLoop(loop);
		// find loop header Node, and add it into loop node
		sTreeNode1 = findBlockNodeByOldID(syntaxTree, loop->header->id);
		//printSyntaxTreeNode(sTreeNode1);

		header_index = al_indexOf(syntaxTree, sTreeNode1);
		al_remove(syntaxTree, sTreeNode1);
		al_add(loopNode->u.loop.loopBody, sTreeNode1);

		al_insertAt(syntaxTree, loopNode, header_index);
		transformGOTOsInLoopBlock(sTreeNode1, loop);

		//then we iterate through the syntaxTree list, and find nodes in loop
		for(i=0;i<al_size(syntaxTree);i++){
			int move_flag=0;
			sTreeNode1 = al_get(syntaxTree, i);
			assert(sTreeNode1->type==TN_BLOCK || sTreeNode1->type==TN_WHILE);
			if(sTreeNode1->type==TN_BLOCK){
				//if block is in the loop
				if(isBlockInTheLoop(loop, sTreeNode1->u.block.old_id)){
					assert(!isBlockTheLoopHeader(loop, sTreeNode1->u.block.old_id));
					al_remove(syntaxTree, sTreeNode1);
					al_add(loopNode->u.loop.loopBody, sTreeNode1);
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
					move_flag++;
				}
			}
			if(move_flag){
				transformGOTOsInLoopBlock(sTreeNode1, loop);
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
		funcNode->u.func.funcBody = al_newGeneric(AL_LIST_SORTED, SyntaxTreeNodeCompareByBlockID, NULL, NULL);


		sTreeNode1 = findBlockNodeByOldID(syntaxTree, func->funcEntryBlock->id);
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
				if(isBlockInTheFunction(func, sTreeNode1->u.block.old_id)){
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


ArrayList *buildSyntaxTree(InstrList *iList, ArrayList *blockList, ArrayList *loopList, ArrayList *funcCFGs){
	int i;
	Instruction *instr;
	BasicBlock *block;
	BasicBlock *n0;
	Function *funcStruct1;
	FuncObjTableEntry *funcObjTableEntry1;

	ArrayList *syntaxTree;
	ArrayList *funcObjTable;

	if(!functionCFGProcessed){
		printf("ERROR: please call buildFunctionCFGs() before call buildSyntaxTree()!\n");
		abort();
	}

	syntaxTree = al_newGeneric(AL_LIST_SORTED, SyntaxTreeNodeCompareByBlockID, NULL, NULL);
	funcObjTable = al_newGeneric(AL_LIST_SET, FuncObjTableEntryCompare, NULL, destroyFuncObjTableEntry);

	syntaxBlockIDctl = 0;
	anonfunobjctl = 0;
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
			n0 = block;
			doSearch(n0, BBL_FLAG_TMP0, syntaxTree, funcObjTable, funcCFGs);
		}
	}

	//clear flag TMP0
	for(i=0;i<al_size(blockList);i++){
		block = al_get(blockList, i);
		BBL_CLR_FLAG(block, BBL_FLAG_TMP0);
	}

	for(i=0;i<al_size(funcObjTable);i++){
		funcObjTableEntry1 = al_get(funcObjTable, i);
		printf("funcName:%s\tobj:%lx\n", funcObjTableEntry1->funcName,  funcObjTableEntry1->funcObjAddr);
	}

	/*
	 * base on the FuncObjTable created during syntax trees construction, we
	 * fill the name and obj fields in FunctionCFGs
	 */
	for(i=0;i<InstrListLength(iList);i++){
		instr = getInstruction(iList, i);
		if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) && (instr->opCode == JSOP_NEW || instr->opCode == JSOP_CALL)){
			funcStruct1 = findFunctionByEntryBlockID(funcCFGs, instr->nextBlock->id);
			funcObjTableEntry1 = findFuncObjTableEntry(funcObjTable, instr->objRef);
			assert(funcObjTableEntry1);
			assert(funcStruct1);

			if(funcObjTableEntry1->funcStruct==NULL || funcStruct1->funcName==NULL){
				if(funcObjTableEntry1->funcStruct==NULL){
					funcObjTableEntry1->funcStruct = funcStruct1;
				}
				if(funcStruct1->funcName==NULL){
					funcStruct1->funcName = (char *)malloc(strlen(funcObjTableEntry1->funcName)+1);
					assert(funcStruct1->funcName);
					memcpy(funcStruct1->funcName, funcObjTableEntry1->funcName, strlen(funcObjTableEntry1->funcName)+1);
					funcStruct1->funcObj = funcObjTableEntry1->funcObjAddr;
				}
			}
		}
		else if(INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE) && (instr->opCode == JSOP_EVAL)){
			funcStruct1 = findFunctionByEntryBlockID(funcCFGs, instr->nextBlock->id);
			BBL_SET_FLAG(funcStruct1->funcEntryBlock, BBL_IS_EVAL_ENTRY);
		}

	}
	printFuncObjTable(funcObjTable);
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
	return syntaxTree;
}














