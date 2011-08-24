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
		f = "FUNCTIONS";
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

#define BASIC	0
#define	FUNCS	1
#define SLICE	BASIC && 0

int main (int argc, char *argv[]) {

    InstrList *iList = NULL;
    int slicing_begin;
    int reducible;
    int i;

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

#if FUNCS
    PrintInfo(1);
    /*
     * create CFGs for functions
     */
    ArrayList *funcBlockList;
    ArrayList *funcLoopList;
    ArrayList *funcCFGs;
    ArrayList *funcSyntaxTree;
    SyntaxTreeNode *funcSyntaxTreeNode;

    printf("processEvaledInstr\n");
    processEvaledInstr(iList);

    funcBlockList = buildDynamicCFG(iList);


	printBasicBlockList(funcBlockList);
    printInstrList(iList);
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

    //process all the AND/OR expressions
    //concatenate involved basicblocks in the order of the address
    processANDandORExps(iList, funcBlockList);

    findDominators(funcBlockList);
    buildDFSTree(funcBlockList);

    /*
     * mark all retreat edges and backedges in given CFG and DFS-Tree
     * and determin if this CFG is reducible.
     */
    reducible = findBackEdges(funcBlockList);
    if(reducible)
    	printf("REDUCIBLE\n");
    else
    	printf("NOT REDUCIBLE\n");

	printBasicBlockList(funcBlockList);

    funcLoopList = buildNaturalLoopList(funcBlockList);
    printNaturalLoopList(funcLoopList);

    //printInstrList(iList);
    printf("Building syntax tree...\n");
    funcSyntaxTree = buildSyntaxTree(iList, funcBlockList, funcLoopList, funcCFGs);
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

	/*
	 * do the dynamic slicing based on instruction specified by 'slicing_begin'
	 */
    SlicingState *state = initSlicingState(iList, slicing_begin/*30 53*/);
    //testUD(iList,state);
    // printSlicingState(state);
    markUDchain(iList, state);
    //printSlicingState(state);
    checkSlice(iList);

    //printInstrList(iList);

    ArrayList *sliceSyntaxTree;
    SyntaxTreeNode *sliceSyntaxTreeNode;

    ArrayList *sliceBlockList = NULL;
    ArrayList *sliceLoopList;
    InstrList *sliceList;
    ArrayList *sliceFuncCFGs;
    printf("processEvaledInstr\n");
    processEvaledInstr(iList);
    //printInstrList(iList);
    printf("InstrListClone\n");
    sliceList = InstrListClone(iList, INSTR_IN_SLICE);

    //printInstrList(iList);
    //printInstrList(sliceList);


    //printInstrList(sliceList);

    labelInstructionList(sliceList);
    sliceBlockList = buildDynamicCFG(sliceList);


    //printInstrList(sliceList);
   // printBasicBlockList(sliceBlockList);

    sliceFuncCFGs = buildFunctionCFGs(sliceList, sliceBlockList);




    //printInstrList(iList);

    findDominators(sliceBlockList);
    buildDFSTree(sliceBlockList);
    reducible = findBackEdges(sliceBlockList);
    if(reducible)
    	printf("REDUCIBLE\n");
    else
    	printf("NOT REDUCIBLE\n");

    //printInstrList(sliceList);
    //printBasicBlockList(sliceBlockList);
    sliceLoopList = buildNaturalLoopList(sliceBlockList);
    // printNaturalLoopList(sliceLoopList);


    sliceSyntaxTree=buildSyntaxTree(sliceList,sliceBlockList, sliceLoopList, sliceFuncCFGs);
    printf("\nRecovered Source Code:\n");
    for(i=0;i<al_size(sliceSyntaxTree);i++){
    	sliceSyntaxTreeNode = al_get(sliceSyntaxTree, i);
    	printSyntaxTreeNode(sliceSyntaxTreeNode);
    }

    destroySlicingState(state);
    destroyNaturalLoopList(sliceLoopList);
	destroyBasicBlockList(sliceBlockList);
    InstrListDestroy(sliceList, 1);

    /*
     * done slice CFG ^^^
     */


	destroyBasicBlockList(blockList);
#endif


	//////////////////////////////////////////////////////////////////////////////////
    InstrListDestroy(iList, 1);
    return 0;
}
