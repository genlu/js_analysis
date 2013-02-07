/*
 * sig_function.c
 *
 *		all the syntaxTreeNode and expTreeNode are label as in_slice
 *
 *
 *  Created on: Feb 5, 2013
 *      Author: genlu
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "instr.h"
#include "prv_types.h"
#include "syntax.h"
#include "function.h"
#include "sig_function.h"


char *vectorStr = "decisionVector";
char *vectorIndex = "vecIndex";
char *exceptionStr = "wrong path";
char *sigFuncStr = "signatureFunction";

SyntaxTreeNode *createReturnBool(int b){
	SyntaxTreeNode *retNode;
	ExpTreeNode *expNode1;

	retNode = createSyntaxTreeNode();
	assert(retNode);
	TN_SET_FLAG(retNode, TN_IN_SLICE);
	retNode->type = TN_RETEXP;
	TN_SET_FLAG(retNode, TN_IS_SIG_FUNCTION);

	expNode1 = createExpTreeNode();
	assert(expNode1);
	EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
	expNode1->type = EXP_CONST_VALUE;
	EXP_SET_FLAG(expNode1,EXP_IS_BOOL);
	expNode1->u.const_value_bool = b?true:false;
	retNode->u.expNode = expNode1;

	return retNode;
}


/*this function create a try-catch syntax tree node like this
 * try{
 * 		//empty
 * }catch(e){
 * 		return false;
 * }
*/
SyntaxTreeNode *createTryCatchSigFunc(){
	SyntaxTreeNode *tryCatchNode, *syntaxNode1;

	tryCatchNode = createSyntaxTreeNode();
	assert(tryCatchNode);
	TN_SET_FLAG(tryCatchNode, TN_IN_SLICE);
	tryCatchNode->type = TN_TRY_CATCH;
	TN_SET_FLAG(tryCatchNode, TN_IS_SIG_FUNCTION);
	tryCatchNode->u.try_catch.try_body = al_new();
	tryCatchNode->u.try_catch.catch_body = al_new();

	syntaxNode1 = createReturnBool(0);
	TN_SET_FLAG(syntaxNode1, TN_IN_SLICE);
	assert(syntaxNode1);

	al_add(tryCatchNode->u.try_catch.catch_body, (void*)syntaxNode1);

	return tryCatchNode;
}

/*
 * this function creates a throw syntax tree node like this
 * throw ""
 * which will be used for check point
 */
SyntaxTreeNode *createThrow(char *expStr){
	SyntaxTreeNode *throwNode;
	ExpTreeNode *expNode1;

	throwNode = createSyntaxTreeNode();
	assert(throwNode);
	TN_SET_FLAG(throwNode, TN_IN_SLICE);
	throwNode->type = TN_THROW;
	TN_SET_FLAG(throwNode, TN_IS_SIG_FUNCTION);
	TN_SET_FLAG(throwNode, TN_IS_CHECK_POINT);


	expNode1 = createExpTreeNode();
	EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
	expNode1->type = EXP_CONST_VALUE;
	EXP_SET_FLAG(expNode1,EXP_IS_STRING);
	expNode1->u.const_value_str = (char *)malloc(strlen(expStr)+1);
	assert(expNode1->u.const_value_str);
	strcpy(expNode1->u.const_value_str, expStr);

	throwNode->u.expNode = expNode1;

	return throwNode;
}



/*
 * create a array init syntax tree node as decision vector
 * if slice_flag==0, then all branch/function-call decisions are stored
 * if slice_flag==1, then only branch/function-called in INSTR_IN_SLICE are stored
 * in array, 0 and 1 represents branch not-taken/taken, and integer represents invoked function's entry address
 * TODO: switch
 */
SyntaxTreeNode *createDecisionVector(InstrList *iList, int sliceFlag, char *vecName, ArrayList *funcObjTable){

	//char *charBuf;
	int i;
	Instruction *instr;
	SyntaxTreeNode *vecInitNode;
	ExpTreeNode *expNode1, *expNode2, *expNode3;

	//create a EXP AST node for vector initialization
	vecInitNode = createSyntaxTreeNode();
	assert(vecInitNode);
	TN_SET_FLAG(vecInitNode, TN_IN_SLICE);
	vecInitNode->type = TN_EXP;
	TN_SET_FLAG(vecInitNode, TN_IS_SIG_FUNCTION);

	expNode1 = createExpTreeNode();
	EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
	expNode1->type = EXP_NAME;
	expNode1->u.name = (char *)malloc(strlen(vecName)+1);
	assert(expNode1->u.name);
	strcpy(expNode1->u.name, vecName);

	expNode3 = createExpTreeNode();
	EXP_SET_FLAG(expNode3, EXP_IN_SLICE);
	expNode3->type = EXP_ARRAY_INIT;
	expNode3->u.array_init.size=0;
	expNode3->u.array_init.initValues = al_new();

	expNode2 = createExpTreeNode();
	EXP_SET_FLAG(expNode2, EXP_IN_SLICE);
	expNode2->type = EXP_BIN;
	expNode2->u.bin_op.op = OP_ASSIGN;
	expNode2->u.bin_op.lval = expNode1;
	expNode2->u.bin_op.rval = expNode3;

	vecInitNode->u.expNode = expNode2;
	expNode1 = expNode2 = expNode3 = NULL;

	//go through instr list, and identify each branch and function invocation, log:
	//	1. 1/0: wether a branch is taken(else) of not
	//	2. and string represents the name of invoked function
	for(i=0;i<InstrListLength(iList);i++){

		if(expNode1==NULL){
			expNode1 = createExpTreeNode();
			EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
			expNode1->type = EXP_CONST_VALUE;
		}
		assert(expNode1);

		instr = getInstruction(iList, i);
		if(!(!sliceFlag || (sliceFlag && INSTR_HAS_FLAG(instr, INSTR_IN_SLICE))))
			continue;

		if(!strcmp(instr->opName, "ifeq") || !strcmp(instr->opName, "ifne")){	// a 2-way branch
			expNode1->u.const_value_int = INSTR_HAS_FLAG(instr, INSTR_BRANCH_TAKEN)?1:0;
			EXP_SET_FLAG(expNode1,EXP_IS_INT);
			al_add(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.initValues, (void *)expNode1);
			(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.size)++;
			expNode1=NULL;
		}
		else if(INSTR_HAS_FLAG(instr, INSTR_IS_N_BRANCH)){	//  n-way branch
			assert(0);
		}
		else if(!strcmp(instr->opName, "call") && INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){	// script function call
			EXP_SET_FLAG(expNode1,EXP_IS_STRING);
			FuncObjTableEntry *funcEntry = findFuncObjTableEntryByFuncObj(funcObjTable, instr->objRef);
			assert(funcEntry);
			expNode1->u.const_value_str = (char *)malloc(strlen(funcEntry->func_name)+1);
			assert(expNode1->u.const_value_str);
			strcpy(expNode1->u.const_value_str, funcEntry->func_name);
			al_add(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.initValues, (void*)expNode1);
			(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.size)++;
			expNode1=NULL;
		}
		else
			continue;
	}

	return vecInitNode;
}

/*
 * decision is a expTreeNode represents current path
 * (the execution path in which the newly created checkPoint will be inserted)
 *
 * crate a syntaxTreeNode for:
 * 	if(vecName[indexName++]!=decision)
 * 		throw expStr;
 */
SyntaxTreeNode *createCheckPoint(char *vecName, char *indexName, char *expStr, ExpTreeNode *decision){

	SyntaxTreeNode *checkNode, *syntaxNode1;
	ExpTreeNode *expNode1 ,*expNode2, *expNode3;

	assert(vecName && indexName && expStr && decision);

	//if-else node
	checkNode = createSyntaxTreeNode();
	assert(checkNode);
	TN_SET_FLAG(checkNode, TN_IN_SLICE);
	checkNode->type = TN_IF_ELSE;
	checkNode->u.if_else.if_path = al_new();
	checkNode->u.if_else.else_path = al_new();
	TN_SET_FLAG(checkNode, TN_IS_SIG_FUNCTION);
	TN_SET_FLAG(checkNode, TN_IS_CHECK_POINT);

	//indexName++
	expNode1 = createExpTreeNode();
	assert(expNode1);
	EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
	expNode1->type = EXP_UN;
	expNode1->u.un_op.op = OP_VINC;
	expNode1->u.un_op.name = (char *)malloc(strlen(indexName)+1);
	assert(expNode1->u.un_op.name);
	strcpy(expNode1->u.un_op.name, indexName);

	//vecName[indexName++]
	expNode2 = createExpTreeNode();
	assert(expNode2);
	EXP_SET_FLAG(expNode2, EXP_IN_SLICE);
	expNode2->type = EXP_ARRAY_ELM;

	expNode3 = createExpTreeNode();
	assert(expNode3);
	EXP_SET_FLAG(expNode3, EXP_IN_SLICE);
	expNode3->type = EXP_NAME;
	expNode3->u.name = (char *)malloc(strlen(vecName)+1);
	assert(expNode3->u.name);
	strcpy(expNode3->u.name, vecName);

	expNode2->u.array_elm.name = expNode3;
	expNode2->u.array_elm.index = expNode1;

	expNode1 = expNode3 = NULL;

	//vecName[indexName++]!=decision
	expNode1 = createExpTreeNode();
	assert(expNode1);
	EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
	expNode1->type = EXP_BIN;
	expNode1->u.bin_op.op = OP_NE;
	expNode1->u.bin_op.lval = expNode2;
	assert(decision->type==EXP_CONST_VALUE);
	EXP_SET_FLAG(decision, EXP_IN_SLICE);
	expNode1->u.bin_op.rval = decision;

	expNode2 = NULL;

	//throw expStr;
	syntaxNode1 = createThrow(expStr);

	//put them together
	checkNode->u.if_else.cond = expNode1;
	al_add(checkNode->u.if_else.if_path, (void*)syntaxNode1);

	return checkNode;
}

/*
 * insert check point in an if-else (or while-loop after transformation) and function body
 * also mark slice-flag for insert check point if corresponding if-else/function node is labeled
 */
void insertCheckPointAtBeginning(ArrayList *syntaxTrees){}


SyntaxTreeNode *createSigFunction(InstrList *iList, ArrayList *syntaxTrees, ArrayList *funcObjTable, int sliceFlag){

	printf("Creating signature function on %s...\n", sliceFlag?"slice":"entire trace");

	SyntaxTreeNode *decisionVec, *sigFunctionNode;
	SyntaxTreeNode *n1, *n2;
	ExpTreeNode *expNode1;
	Function 	*sigFuncStruct;
	int i;

	//create signature function syntaxtree node
	sigFunctionNode = createSyntaxTreeNode();
	assert(sigFunctionNode);
	TN_SET_FLAG(sigFunctionNode, TN_IN_SLICE);
	TN_SET_FLAG(sigFunctionNode, TN_IS_SIG_FUNCTION);
	sigFunctionNode->type = TN_FUNCTION;
	sigFunctionNode->u.func.funcBody = al_new();

	sigFuncStruct = createFunctionNode();
	assert(sigFuncStruct);
	FUNC_SET_FLAG(sigFuncStruct, FUNC_IN_SLICE);
	sigFuncStruct->funcName = (char*)malloc(strlen(sigFuncStr)+1);
	assert(sigFuncStruct->funcName);
	strcpy(sigFuncStruct->funcName, sigFuncStr);
	sigFuncStruct->args = -1;

	sigFunctionNode->u.func.funcStruct = sigFuncStruct;


	//1. go through instr list and generate decision vector
	decisionVec = createDecisionVector(iList, sliceFlag, vectorStr, funcObjTable);


	//2. go through syntax tree nodes and insert check points


	//3. create try-catch and insert all nodes in try-body

	n1 = createTryCatchSigFunc();
	for(i=0;i<al_size(syntaxTrees);i++){
		al_add(n1->u.try_catch.try_body, al_get(syntaxTrees, i));
	}
	n2 = createReturnBool(1);
	al_add(sigFunctionNode->u.func.funcBody, decisionVec);
	al_add(sigFunctionNode->u.func.funcBody, n1);
	al_add(sigFunctionNode->u.func.funcBody, n2);



	printSyntaxTreeNode(sigFunctionNode, sliceFlag);




	return NULL;
}
