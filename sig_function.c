/***********************************************************************************
Copyright 2014 Arizona Board of Regents; all rights reserved.

This software is being provided by the copyright holders under the
following license.  By obtaining, using and/or copying this software, you
agree that you have read, understood, and will comply with the following
terms and conditions:

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee or royalty is hereby granted,
provided that the full text of this notice appears on all copies of the
software and documentation or portions thereof, including modifications,
that you make.

This software is provided "as is," and copyright holders make no
representations or warranties, expressed or implied.  By way of example, but
not limitation, copyright holders make no representations or warranties of
merchantability or fitness for any particular purpose or that the use of the
software or documentation will not infringe any third party patents,
copyrights, trademarks or other rights.  Copyright holders will bear no
liability for any use of this software or documentation.

The name and trademarks of copyright holders may not be used in advertising
or publicity pertaining to the software without specific, written prior
permission.  Title to copyright in this software and any associated
documentation will at all times remain with copyright holders.
***********************************************************************************/

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
SyntaxTreeNode *createThrow(char *expStr, int inSlice){
	SyntaxTreeNode *throwNode;
	ExpTreeNode *expNode1;

	throwNode = createSyntaxTreeNode();
	assert(throwNode);
	if(inSlice==1)
		TN_SET_FLAG(throwNode, TN_IN_SLICE);
	throwNode->type = TN_THROW;
	TN_SET_FLAG(throwNode, TN_IS_SIG_FUNCTION);
	TN_SET_FLAG(throwNode, TN_IS_CHECK_POINT);


	expNode1 = createExpTreeNode();
	if(inSlice==1)
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
 * in array, 0 and 1 represents branch not-taken/taken, and integer represents invoked function's entry address
 * TODO: switch
 */
SyntaxTreeNode *createDecisionVector(InstrList *iList, char *vecName, ArrayList *funcObjTable){

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
			//EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
			expNode1->type = EXP_CONST_VALUE;
		}
		assert(expNode1);

		instr = getInstruction(iList, i);
		/*
		if(!(!sliceFlag || (sliceFlag && INSTR_HAS_FLAG(instr, INSTR_IN_SLICE))))
			continue;
		 */
		if(!strcmp(instr->opName, "ifeq") || !strcmp(instr->opName, "ifne")){	// a 2-way branch
			expNode1->u.const_value_int = INSTR_HAS_FLAG(instr, INSTR_BRANCH_TAKEN)?1:0;
			EXP_SET_FLAG(expNode1,EXP_IS_INT);
			if(INSTR_HAS_FLAG(instr, INSTR_IN_SLICE)){
				EXP_SET_FLAG(expNode1,EXP_IN_SLICE);
			}
			al_add(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.initValues, (void *)expNode1);
			(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.size)++;
			expNode1=NULL;
		}
		else if(INSTR_HAS_FLAG(instr, INSTR_IS_N_BRANCH)){	//  n-way branch
			assert(0);
		}
		else if(!strcmp(instr->opName, "call") && INSTR_HAS_FLAG(instr, INSTR_IS_SCRIPT_INVOKE)){	// script function call
			EXP_SET_FLAG(expNode1,EXP_IS_STRING);
			if(INSTR_HAS_FLAG(instr, INSTR_IN_SLICE)){
				EXP_SET_FLAG(expNode1,EXP_IN_SLICE);
			}
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
SyntaxTreeNode *createCheckPoint(char *vecName, char *indexName, char *expStr, ExpTreeNode *decision, int inSlice){

	SyntaxTreeNode *checkNode, *syntaxNode1;
	ExpTreeNode *expNode1 ,*expNode2, *expNode3;

	assert(vecName && indexName && expStr && decision);
	if(inSlice==1)
		assert(EXP_HAS_FLAG(decision, EXP_IN_SLICE));
	else if(inSlice==0)
		assert(!EXP_HAS_FLAG(decision, EXP_IN_SLICE));

	//if-else node
	checkNode = createSyntaxTreeNode();
	assert(checkNode);
	if(inSlice==1)
		TN_SET_FLAG(checkNode, TN_IN_SLICE);
	checkNode->type = TN_IF_ELSE;
	checkNode->u.if_else.if_path = al_new();
	checkNode->u.if_else.else_path = al_new();
	TN_SET_FLAG(checkNode, TN_IS_SIG_FUNCTION);
	TN_SET_FLAG(checkNode, TN_IS_CHECK_POINT);

	//indexName++
	expNode1 = createExpTreeNode();
	assert(expNode1);
	if(inSlice==1)
		EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
	expNode1->type = EXP_UN;
	expNode1->u.un_op.op = OP_VINC;
	expNode1->u.un_op.name = (char *)malloc(strlen(indexName)+1);
	assert(expNode1->u.un_op.name);
	strcpy(expNode1->u.un_op.name, indexName);

	//vecName[indexName++]
	expNode2 = createExpTreeNode();
	assert(expNode2);
	if(inSlice==1)
		EXP_SET_FLAG(expNode2, EXP_IN_SLICE);
	expNode2->type = EXP_ARRAY_ELM;

	expNode3 = createExpTreeNode();
	assert(expNode3);
	if(inSlice==1)
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
	if(inSlice==1)
		EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
	expNode1->type = EXP_BIN;
	expNode1->u.bin_op.op = OP_NE;
	expNode1->u.bin_op.lval = expNode2;
	assert(decision->type==EXP_CONST_VALUE);
	expNode1->u.bin_op.rval = decision;

	expNode2 = NULL;

	//throw expStr;
	syntaxNode1 = createThrow(expStr, inSlice);

	//put them together
	checkNode->u.if_else.cond = expNode1;
	al_add(checkNode->u.if_else.if_path, (void*)syntaxNode1);

	return checkNode;
}

/*
 * insert check point in an if-else (or while-loop after transformation) and function body
 * also mark slice-flag for insert check point if corresponding if-else/function node is labeled
 */
/*
 * correction: use a lazy approach, only label syntaxTreeNodes with flag TN_HAS_CHECKPOINT,
 * and let printSyntaxTreeNode() print the chcek point code directly, based on info stored in node.
 */
void insertCheckPoints(SyntaxTreeNode *node, char *vecName, char *indexName, char *expStr){

	//_ifeq==1: then if-path = 0 and else-path = 1
	//_ifeq==0, then if-path = 1 and else-path = 0

	assert(node);
		int i;
		SyntaxTreeNode *tempNode1;
		switch(node->type){
		case TN_BLOCK:
			for(i=0;i<al_size(node->u.block.statements);i++){
				tempNode1 = al_get(node->u.block.statements, i);
				insertCheckPoints(tempNode1, vecName, indexName, expStr);
			}
			break;
		case TN_FUNCTION:
			for(i=0;i<al_size(node->u.func.funcBody);i++){
				tempNode1 = al_get(node->u.func.funcBody, i);
				insertCheckPoints(tempNode1, vecName, indexName, expStr);
			}
			//add a check point at the begining of funcBody
			TN_SET_FLAG(node, TN_HAS_CHECKPOINT);
			break;
		case TN_WHILE:
			for(i=0;i<al_size(node->u.loop.loopBody);i++){
				tempNode1 = al_get(node->u.loop.loopBody, i);
				insertCheckPoints(tempNode1, vecName, indexName, expStr);
			}
			//if not infinite-loop, add check-point
			if(node->u.loop._bodyBranchType!=-1)
				TN_SET_FLAG(node, TN_HAS_CHECKPOINT);
			break;
		case TN_IF_ELSE:
			for(i=0;i<al_size(node->u.if_else.if_path);i++){
				tempNode1 = al_get(node->u.if_else.if_path, i);
				insertCheckPoints(tempNode1, vecName, indexName, expStr);
			}
			for(i=0;i<al_size(node->u.if_else.else_path);i++){
				tempNode1 = al_get(node->u.if_else.else_path, i);
				insertCheckPoints(tempNode1, vecName, indexName, expStr);
			}
			TN_SET_FLAG(node, TN_HAS_CHECKPOINT);
			break;
		case TN_TRY_CATCH:
		case TN_THROW:
			assert(0);
			break;
		case TN_GOTO:
		case TN_RETURN:
		case TN_RETEXP:
		case TN_EXP:
		case TN_DEFVAR:
		default:
			break;
		}//end switch
		return;

}


SyntaxTreeNode *createSigFunction(InstrList *iList, ArrayList *syntaxTrees, ArrayList *funcObjTable){

	//printf("Creating signature function on %s...\n", sliceFlag?"slice":"entire trace");

	SyntaxTreeNode *decisionVec, *sigFunctionNode, *syntaxNode1;
	SyntaxTreeNode *n1, *n2, *n3;
	ExpTreeNode *expNode1, *expNode2, *expNode3;
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
	printf("createDecisionVector\n");
	decisionVec = createDecisionVector(iList, vectorStr, funcObjTable);


	//2. go through syntax tree nodes and insert check points
	//SyntaxTreeNode *createCheckPoint(char *vecName, char *indexName, char *expStr, ExpTreeNode *decision, int inSlice)

	printf("insertCheckPoints\n");
	for(i=0;i<al_size(syntaxTrees);i++){
		syntaxNode1 = al_get(syntaxTrees, i);
		insertCheckPoints(syntaxNode1, vectorStr, vectorIndex, exceptionStr);
	}

	//3. create try-catch and insert all nodes in try-body

	printf("finishing...\n");
	n1 = createTryCatchSigFunc();
	for(i=0;i<al_size(syntaxTrees);i++){
		//printf("\nadding...");
		//printSyntaxTreeNode(al_get(syntaxTrees, i), 1);
		al_add(n1->u.try_catch.try_body, al_get(syntaxTrees, i));
	}
	//printSyntaxTreeNode(n1, 1);

	n2 = createReturnBool(1);


	n3 = createSyntaxTreeNode();
	assert(n3);
	TN_SET_FLAG(n3, TN_IN_SLICE);
	n3->type = TN_EXP;
	TN_SET_FLAG(n3, TN_IS_SIG_FUNCTION);

	expNode1 = createExpTreeNode();
	EXP_SET_FLAG(expNode1, EXP_IN_SLICE);
	expNode1->type = EXP_NAME;
	expNode1->u.name = (char *)malloc(strlen(vectorIndex)+1);
	assert(expNode1->u.name);
	strcpy(expNode1->u.name, vectorIndex);

	expNode2 = createExpTreeNode();
	EXP_SET_FLAG(expNode2, EXP_IN_SLICE);
	expNode2->type = EXP_CONST_VALUE;
	EXP_SET_FLAG(expNode2,EXP_IS_INT);
	expNode2->u.const_value_int = 0;

	expNode3 = createExpTreeNode();
	EXP_SET_FLAG(expNode3, EXP_IN_SLICE);
	expNode3->type = EXP_BIN;
	expNode3->u.bin_op.op = OP_ASSIGN;
	expNode3->u.bin_op.lval = expNode1;
	expNode3->u.bin_op.rval = expNode2;

	n3->u.expNode = expNode3;


	al_add(sigFunctionNode->u.func.funcBody, n3);
	al_add(sigFunctionNode->u.func.funcBody, decisionVec);
	al_add(sigFunctionNode->u.func.funcBody, n1);
	al_add(sigFunctionNode->u.func.funcBody, n2);

	//printf("------------------------------------------\n");
	//printSyntaxTreeNode(sigFunctionNode, 1);
	//printf("------------------------------------------\n");

	return sigFunctionNode;
}
