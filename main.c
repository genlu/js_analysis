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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>

#include<time.h>
#include<sys/time.h>


#include "prv_types.h"
#include "instr.h"
#include "read.h"
#include "slicing.h"
#include "stack_sim.h"
#include "cfg.h"
#include "function.h"
#include "syntax.h"
#include "df.h"
#include "sig_function.h"



void PrintInfo(int func){
	char *f;
	switch(func){
	case 0:
		f = "BASIC";
		break;
	case 1:
		f = "DECOMPILATION";
		break;
	case 2:
		f = "DEOBF";
		break;
	case 3:
		f = "SIG-FUNCTION";
		break;
	default:
		return;
	}
	printf("\n\n\
/---------------------------------------------------------------------------------------/\n\
/                                    %15s                                    /\n\
/---------------------------------------------------------------------------------------/\n", f);
}

void initSys(void){

}

void test(void){
	printf("begin test()\n");

	OpStack *stack = initOpStack(NULL);
	int *a = (int *)malloc(sizeof(int));
	*a=100;
	pushOpStack(stack, a);
	printf("sp:%d\n", stack->sp);
	printf("%d\n", *((int *)popOpStack(stack)));
	printf("sp:%d\n", stack->sp);

	printf("end test()\n");
}

//#define	DECOMPILE	1
//#define SLICE		0

int main (int argc, char *argv[]) {

	  /* used to measure the execution time */
	  struct timeval tv;
	  struct timezone tz;
	  struct tm *tm;
	  long int t1s, t1m, t2s, t2m;


	InstrList *iList = NULL;
	int reducible;
	int i,j;

	int parameter;

	if(argc != 3) {
		printf("usage: js_analysis <js_trace file> <flag>\n\tflag:\n"
				"\t 0: decompile entire trace\n"
				"\t 1: recover simplified code\n"
				"\t 2: generate sig function code\n");
		return(-1);
	}

	parameter = atoi(argv[2]);
	if(parameter!=0 && parameter!=1 && parameter!=2){
		printf("%d\n", parameter);
		printf("usage: js_analysis <js_trace file> <flag>\n\tflag:\n"
				"\t 0: decompile entire trace\n"
				"\t 1: recover simplified code\n"
				"\t 2: generate sig function code\n");
		return(-1);
	}

	  /* get current time(before executing) */
	  gettimeofday(&tv, &tz);
	  tm=localtime(&tv.tv_sec);
	  t1s=tv.tv_sec;
	  t1m=tv.tv_usec;

	initSys();
	//test();

	//create instruction list
	iList = InitInstrList(argv[1]);

	/*
	 * label all the important instructions
	 * e.g., function calls and returns, jumps
	 */
	int ncalls = labelInstructionList(iList);

	//forwardUDchain(iList, 9);

	//printInstrList(iList);

	//#if DECOMPILE
	if(parameter==0){
		PrintInfo(1);
		/*
		 * create CFGs for functions
		 */
		ArrayList *funcBlockList;
		ArrayList *funcLoopList;
		ArrayList *funcCFGs;
		ArrayList *funcSyntaxTree;
		SyntaxTreeNode *funcSyntaxTreeNode;
		ArrayList *funcObjTable;

		printf("processEvaledInstr...\n");
		processEvaledInstr(iList);
		printf("%d\n",iList->numInstrs);
		//printInstrList(iList);

		printf("building CFG...\n");
		funcBlockList = buildDynamicCFG(iList);


		//printBasicBlockList(funcBlockList);
		//printInstrList(iList);
		printf("building function CFGs...\n");
		funcCFGs = buildFunctionCFGs(iList, funcBlockList, &funcObjTable);
		printBasicBlockList(funcBlockList);

		printf("adding augmented exit blocks\n");
		ArrayList *augExits = addAugmentedExitBlocks(funcBlockList);

		//printBasicBlockList(funcBlockList);
		//printInstrList(iList);

		/*
		 * find dominators for each node (basicBlock)
		 */
		printf("finding dominators\n");
		findDominators(funcBlockList);

		printf("finding reverse dominators\n");
		findReverseDominators(funcBlockList);

		printf("computing reverse dominance frontiers...\n");
		computeReverseDominanceFrontiers(funcBlockList);
		printRevDominanceFrontiers(funcBlockList);

		printf("removing augmented exit blocks\n");
		removeAugmentedExitBlocks(funcBlockList, augExits);
		/*
		 * build a DFS-Tree on CFG
		 * (tree edges are marked)
		 */
		buildDFSTree(funcBlockList);

		//printBasicBlockList(funcBlockList);
		//printInstrList(iList);

		//process all the AND/OR expressions
		//concatenate involved basicblocks in the order of the address
		processANDandORExps(iList, funcBlockList, funcCFGs);


		//printBasicBlockList(funcBlockList);

		/*
		 * mark all retreat edges and backedges in given CFG and DFS-Tree
		 * and determin if this CFG is reducible.
		 */
		reducible = findBackEdges(funcBlockList);
		if(reducible)
			printf("REDUCIBLE\n");
		else
			printf("NOT REDUCIBLE\n");

		//printBasicBlockList(funcBlockList);

		funcLoopList = buildNaturalLoopList(funcBlockList);
		printNaturalLoopList(funcLoopList);


		printInstrList(iList);
		printBasicBlockList(funcBlockList);
		printf("Building syntax tree...\n");
		funcSyntaxTree = buildSyntaxTree(iList, funcBlockList, funcLoopList, funcCFGs, funcObjTable, parameter);

		//print all function info
		printf("\nprint all function info:\n");
		Function *func;
		for(i=0;i<al_size(funcCFGs);i++){
			func = al_get(funcCFGs, i);
			printf("func %s\t", func->funcName);
			for(j=0;j<al_size(func->funcBody);j++){
				printf("%d  ",((BasicBlock*)al_get(func->funcBody, j))->id);
			}
			printf("\n");
		}

		printf("\nTransform syntax tree...\n");
		transformSyntaxTree(funcSyntaxTree);

		printf("\nRecovered Source Code:\n");
		for(i=0;i<al_size(funcSyntaxTree);i++){
			funcSyntaxTreeNode = al_get(funcSyntaxTree, i);
			printSyntaxTreeNode(funcSyntaxTreeNode, parameter);
		}

		destroyFunctionCFGs(funcCFGs);
		destroyNaturalLoopList(funcLoopList);
		destroyBasicBlockList(funcBlockList);
	}
	//#endif


	//////////////////////////////////////////////////////////////////////////////////

	/*
	 * create CFG for slicing
	 */
	else if(parameter==1){
		//#if SLICE

		PrintInfo(2);

		ArrayList *blockList;
		ArrayList *funcCFGs;
		ArrayList *loopList;
		ArrayList *funcObjTable;

		ArrayList *sliceSyntaxTree;
		SyntaxTreeNode *sliceSyntaxTreeNode;

		printf("processEvaledInstr...\n");
		processEvaledInstr(iList);

		//Build dynamic CFG.
		printf("build CFG for entire trace...\n");
		blockList = buildDynamicCFG(iList);

		//create functions list (separate them from main procedure)
		printf("construct functions list from the CFG of entire trace...\n");
		funcCFGs = buildFunctionCFGs(iList, blockList, &funcObjTable);
		//find dominators for each node (basicBlock)
		findDominators(blockList);
		//build a DFS-Tree on CFG(tree edges are marked)
		buildDFSTree(blockList);
		//printInstrList(iList);

		printBasicBlockList(blockList);

		/*
		 * now do the d-slicing
		 */
		deobfSlicing(iList);

		//printInstrList(iList);

		processANDandORExps(iList, blockList, funcCFGs);
		reducible = findBackEdges(blockList);
		if(reducible)
			printf("REDUCIBLE\n");
		else
			printf("NOT REDUCIBLE\n");
		loopList = buildNaturalLoopList(blockList);
		printNaturalLoopList(loopList);

		printf("Building syntax tree...\n");
		sliceSyntaxTree = buildSyntaxTree(iList, blockList, loopList, funcCFGs, funcObjTable, parameter);

		printf("\nTransform syntax tree...\n");
		transformSyntaxTree(sliceSyntaxTree);

		printf("\nRecovered Source Code:\n");
		for(i=0;i<al_size(sliceSyntaxTree);i++){
			sliceSyntaxTreeNode = al_get(sliceSyntaxTree, i);
			printSyntaxTreeNode(sliceSyntaxTreeNode, parameter);
		}


		al_freeWithElements(funcObjTable);
		destroyNaturalLoopList(loopList);
		destroyFunctionCFGs(funcCFGs);
		//destroyBasicBlockList(blockList);
		//#endif
	}

	else if(parameter==2){		//signature function generation

		PrintInfo(3);
		parameter = 1;

		/*
		 * create CFGs for functions
		 */
		ArrayList *funcBlockList;
		ArrayList *funcLoopList;
		ArrayList *funcCFGs;
		ArrayList *funcSyntaxTree;
		SyntaxTreeNode *funcSyntaxTreeNode;
		ArrayList *funcObjTable;

		printf("processEvaledInstr...\n");
		processEvaledInstr(iList);
		printf("%d\n",iList->numInstrs);
		//printInstrList(iList);

		printf("building CFG...\n");
		funcBlockList = buildDynamicCFG(iList);


		//printBasicBlockList(funcBlockList);
		//printInstrList(iList);
		printf("building function CFGs...\n");
		funcCFGs = buildFunctionCFGs(iList, funcBlockList, &funcObjTable);
		printBasicBlockList(funcBlockList);

		printf("adding augmented exit blocks\n");
		ArrayList *augExits = addAugmentedExitBlocks(funcBlockList);

		//printBasicBlockList(funcBlockList);
		//printInstrList(iList);

		/*
		 * find dominators for each node (basicBlock)
		 */
		printf("finding dominators\n");
		findDominators(funcBlockList);

		printf("finding reverse dominators\n");
		findReverseDominators(funcBlockList);

		printf("computing reverse dominance frontiers...\n");
		computeReverseDominanceFrontiers(funcBlockList);
		printRevDominanceFrontiers(funcBlockList);

		printf("removing augmented exit blocks\n");
		removeAugmentedExitBlocks(funcBlockList, augExits);

		//todo: environment-dependent branches slicing here
		printf("\nforward slicing\n");
		//forwardSlicing(iList, 622, funcBlockList, INSTR_IN_SLICE);
		//backwardSlicing(iList, 645, funcBlockList, INSTR_IN_SLICE);
		envBranchSlicing(iList, funcBlockList, INSTR_IN_SLICE);

		/*
		 * build a DFS-Tree on CFG
		 * (tree edges are marked)
		 */
		buildDFSTree(funcBlockList);

		//printBasicBlockList(funcBlockList);
		//printInstrList(iList);

		//process all the AND/OR expressions
		//concatenate involved basicblocks in the order of the address
		processANDandORExps(iList, funcBlockList, funcCFGs);


		//printBasicBlockList(funcBlockList);

		/*
		 * mark all retreat edges and backedges in given CFG and DFS-Tree
		 * and determin if this CFG is reducible.
		 */
		reducible = findBackEdges(funcBlockList);
		if(reducible)
			printf("REDUCIBLE\n");
		else
			printf("NOT REDUCIBLE\n");

		//printBasicBlockList(funcBlockList);

		funcLoopList = buildNaturalLoopList(funcBlockList);
		printNaturalLoopList(funcLoopList);


		printInstrList(iList);
		printBasicBlockList(funcBlockList);
		printf("Building syntax tree...\n");
		funcSyntaxTree = buildSyntaxTree(iList, funcBlockList, funcLoopList, funcCFGs, funcObjTable, parameter);

		//print all function info
		printf("\nprint all function info:\n");
		Function *func;
		for(i=0;i<al_size(funcCFGs);i++){
			func = al_get(funcCFGs, i);
			printf("func %s\t", func->funcName);
			for(j=0;j<al_size(func->funcBody);j++){
				printf("%d  ",((BasicBlock*)al_get(func->funcBody, j))->id);
			}
			printf("\n");
		}

		printf("\nTransform syntax tree...\n");
		transformSyntaxTree(funcSyntaxTree);


		//todo: add check points into transform syntax tree here

		printf("\nRecovered Source Code:\n");
		for(i=0;i<al_size(funcSyntaxTree);i++){
			funcSyntaxTreeNode = al_get(funcSyntaxTree, i);
			printSyntaxTreeNode(funcSyntaxTreeNode, parameter);
		}

		printf("create signature function\n");
		SyntaxTreeNode *sigFunc = createSigFunction(iList, funcSyntaxTree, funcObjTable);
		printf("\nsignature function:\n");

		printf("alsize %d\n", al_size(sigFunc->u.func.funcBody));
		printSyntaxTreeNode(sigFunc, parameter);

		destroyFunctionCFGs(funcCFGs);
		destroyNaturalLoopList(funcLoopList);
		destroyBasicBlockList(funcBlockList);

	}
	/* get current time(after seq finished) */
	gettimeofday(&tv, &tz);
	tm=localtime(&tv.tv_sec);
	t2s=tv.tv_sec;
	t2m=tv.tv_usec;

	double ttt=(t2s-t1s)*1000000+(t2m-t1m);
	printf("ilist:%d\t ncalls: %d\t time:%fms\n", iList->numInstrs, ncalls, ttt);
	printf("avg/line: %f\n", ttt/iList->numInstrs);
	printf("avg/ncall: %f\n", ttt/ncalls);

	//////////////////////////////////////////////////////////////////////////////////
	InstrListDestroy(iList, 1);
	return 0;
}
