#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include "prv_types.h"
#include "instr.h"
#include "read.h"
#include "slicing.h"
#include "stack_sim.h"
#include "cfg.h"
#include "function.h"
#include "syntax.h"



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
		f = "SLICE";
		break;
	default:
		return;
	}
	printf("\n\n/---------------------------------------------------------------------------------------/\n\
/                                    %10s                                        /\n\
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

#define BASIC		0
#define	DECOMPILE	0
#define SLICE		1

int main (int argc, char *argv[]) {

    InstrList *iList = NULL;
    int slicing_begin;
    int reducible;
    int i,j;

    if(argc != 3) {
        printf("usage: js_analysis <js_trace file> <slicing_begin>\n");
        return(-1);
    }

    initSys();
    slicing_begin = atoi(argv[2]);
    //test();

    //create instruction list
    iList = InitInstrList(argv[1]);

    /*
     * label all the important instructions
     * e.g., function calls and returns, jumps
     */
    labelInstructionList(iList);



	//////////////////////////////////////////////////////////////////////////////////

#if BASIC

    ArrayList *blockList = NULL;
    ArrayList *loopList;

    PrintInfo(0);
    /*
     * Build dynamic CFG.
     * (No edges are marked yet)
     */
    blockList = buildDynamicCFG(iList);
	//printBasicBlockList(blockList);
    if(slicing_begin==0)
    	printInstrList(iList);

    /*
     * find dominators for each node (basicBlock)
     */
    findDominators(blockList);

    /*
     * build a DFS-Tree on CFG
     * (tree edges are marked)
     */
    buildDFSTree(blockList);

    /*
     * mark all retreat edges and backedges in given CFG and DFS-Tree
     * and determin if this CFG is reducible.
     */
    reducible = findBackEdges(blockList);
    if(reducible)
    	printf("REDUCIBLE\n");
    else
    	printf("NOT REDUCIBLE\n");

   // printBasicBlockList(blockList);

    loopList = buildNaturalLoopList(blockList);
   // printNaturalLoopList(loopList);



    destroyNaturalLoopList(loopList);
	//destroyBasicBlockList(blockList);


#endif

	//////////////////////////////////////////////////////////////////////////////////

#if DECOMPILE
    PrintInfo(1);
    /*
     * create CFGs for functions
     */
    ArrayList *funcBlockList;
    ArrayList *funcLoopList;
    ArrayList *funcCFGs;
    ArrayList *funcSyntaxTree;
    SyntaxTreeNode *funcSyntaxTreeNode;

    printf("processEvaledInstr...\n");
    processEvaledInstr(iList);

    printf("building CFG...\n");
    funcBlockList = buildDynamicCFG(iList);


	//printBasicBlockList(funcBlockList);
    printInstrList(iList);
    printf("building function CFGs...\n");
    funcCFGs = buildFunctionCFGs(iList, funcBlockList);

	//printBasicBlockList(funcBlockList);
    //printInstrList(iList);
    /*
     * find dominators for each node (basicBlock)
     */
    findDominators(funcBlockList);
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



    //printInstrList(iList);
    //printBasicBlockList(funcBlockList);
    printf("Building syntax tree...\n");
    funcSyntaxTree = buildSyntaxTree(iList, funcBlockList, funcLoopList, funcCFGs);

	//print all function info
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
    	printSyntaxTreeNode(funcSyntaxTreeNode);
    }


    destroyFunctionCFGs(funcCFGs);
    destroyNaturalLoopList(funcLoopList);
	destroyBasicBlockList(funcBlockList);

#endif


	//////////////////////////////////////////////////////////////////////////////////

    /*
     * create CFG for slicing
     */
#if SLICE

    PrintInfo(2);

    ArrayList *blockList, *sliceBlockList;
    ArrayList *funcCFGs, *sliceFuncCFGs;
    ArrayList *loopList, *sliceLoopList;
    InstrList *sliceInstrList;


    ArrayList *sliceSyntaxTree;
    SyntaxTreeNode *sliceSyntaxTreeNode;


    printf("processEvaledInstr...\n");
    processEvaledInstr(iList);

    //Build dynamic CFG.
    printf("build CFG for entire trace...\n");
    blockList = buildDynamicCFG(iList);
    //create functions list (separate them from main procedure)
    printf("construct functions list from the CFG of entire trace...\n");
    funcCFGs = buildFunctionCFGs(iList, blockList);
    //find dominators for each node (basicBlock)
    findDominators(blockList);
    //build a DFS-Tree on CFG(tree edges are marked)
    buildDFSTree(blockList);

    printInstrList(iList);


	//do the dynamic slicing based on instruction specified by 'slicing_begin'
    //todo: need to do d-slicing on all the native calls not contribute to eval
    SlicingState *state = initSlicingState(iList, slicing_begin/*30 53*/);
    //testUD(iList,state);
    //todo
    markUDchain(iList, state);
    //printSlicingState(state);
    checkSlice(iList);

    /*todo: big change here
    1. find all function entry instrs: 1st instr after a non-native call (1 per function/eval)
    2. find all function exit instrs: 1st instr before a func entry instr (could be more than 1)
    3. create a function obj table for slice
		- with funcObj, entryAddr
	4. collecting info for branch instruction
		- target address for 1 way branch
		- branch and adj address for 2 way branch
		- todo: don't worry about and/or for now...
    5. unlabel all instructions which:
		- added by checkSlice(), and
		- belong to some func/eval, and
		- the invoke instr is not in the slice

    */

    //after above steps, we could get a list of headers, and construct slice CFG based on them
    printf("InstrListClone\n");
    sliceInstrList = InstrListClone(iList, INSTR_IN_SLICE);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    printf("build CFG for slice..\n");
    //labelInstructionList(sliceList);		//no need for this if we keep all the instruction type flags
    sliceBlockList = buildDynamicCFG(sliceInstrList);


    //printInstrList(sliceList);
   // printBasicBlockList(sliceBlockList);

    printf("build function CFGs for slice...\n");
    sliceFuncCFGs = buildFunctionCFGs(sliceInstrList, sliceBlockList);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////




    //printInstrList(iList);

    printf("calculate dominator information...\n");
    findDominators(sliceBlockList);
    buildDFSTree(sliceBlockList);
    reducible = findBackEdges(sliceBlockList);
    if(reducible)
    	printf("REDUCIBLE\n");
    else
    	printf("NOT REDUCIBLE\n");

    printInstrList(sliceInstrList);
    //printBasicBlockList(sliceBlockList);
    sliceLoopList = buildNaturalLoopList(sliceBlockList);
    // printNaturalLoopList(sliceLoopList);

    printf("build syntax tree for slice...\n");
    sliceSyntaxTree=buildSyntaxTree(sliceInstrList,sliceBlockList, sliceLoopList, sliceFuncCFGs);

    printf("\nTransform syntax tree...\n");
    transformSyntaxTree(sliceSyntaxTree);

    printf("\nRecovered Source Code:\n");
    for(i=0;i<al_size(sliceSyntaxTree);i++){
    	sliceSyntaxTreeNode = al_get(sliceSyntaxTree, i);
    	printSyntaxTreeNode(sliceSyntaxTreeNode);
    }

    destroySlicingState(state);
    destroyNaturalLoopList(sliceLoopList);
	destroyBasicBlockList(sliceBlockList);
    InstrListDestroy(sliceInstrList, 1);

    /*
     * done slice CFG ^^^
     */


	destroyBasicBlockList(blockList);
#endif


	//////////////////////////////////////////////////////////////////////////////////
    InstrListDestroy(iList, 1);
    return 0;
}
