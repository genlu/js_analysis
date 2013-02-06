/*
 * sig_function.c
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

SyntaxTreeNode *createReturnBool(int b){
	SyntaxTreeNode *retNode;
	ExpTreeNode *expNode1;

	retNode = createSyntaxTreeNode;
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
SyntaxTreeNode *createTryCatch(){
	SyntaxTreeNode *tryCatchNode, syntaxNode1;

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

	al_add(tryCatchNode->u.try_catch.catch_body, syntaxNode1);

	return tryCatchNode;
}

/*
 * this function creates a throw syntax tree node like this
 * throw ""
 */
SyntaxTreeNode *createThrow(char *expStr){
	SyntaxTreeNode *throwNode;
	ExpTreeNode *expNode1;

	throwNode = createSyntaxTreeNode();
	assert(throwNode);
	TN_SET_FLAG(throwNode, TN_IN_SLICE);
	throwNode->type = TN_THROW;
	TN_SET_FLAG(throwNode, TN_IS_SIG_FUNCTION);


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
	//	1. 1/0: wether a branch is taken of not
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
			al_add(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.initValues, expNode1);
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
			al_add(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.initValues, expNode1);
			(vecInitNode->u.expNode->u.bin_op.rval->u.array_init.size)++;
			expNode1=NULL;
		}
		else
			continue;
	}

	return vecInitNode;
}

SyntaxTreeNode *createCheckPoint(char *vecName, char *indexName, ExpTreeNode *decision){

	SyntaxTreeNode *checkNode;

	return NULL;
}

/*
 * insert check points in each if-else and function body
 * also mark slice-flag for insert check point if corresponding if-else/function node is labeled
 */
void insertCheckPoints(ArrayList *syntaxTrees){}


SyntaxTreeNode *createSigFunction(InstrList *iList, ArrayList *syntaxTrees, char *vecName, char *indexName){
	//1. go through instr list and generate decision vector

	//2. go through syntax tree nodes and insert check points

	//3. create signature function node, insert decision vec at the beginning and
	//      original ASTs into the try-body (functions and main routine)

	return NULL;
}
