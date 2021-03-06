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
 * prv_types.h
 *
 *  Created on: Apr 21, 2011
 *      Author: genlu
 */

#ifndef _PRV_TYPES_H
#define _PRV_TYPES_H

#include <stdint.h>
#include "array_list.h"

#define MAX_OPNAME_LEN	15

typedef unsigned long ADDRESS;
#define	PTRSIZE	sizeof(ADDRESS)

typedef struct BasicBlock BasicBlock;

extern int functionCFGProcessed;

/*********************************************************************************/

typedef enum {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format,imp)		op,
#include "opcode.tbl"
#undef OPDEF
} OpCode;


/*********************************************************************************/

#define	INSTR_IN_SLICE				(1<<0)				// instruction is in the data dependence chain
#define	INSTR_ON_EVAL				(1<<1)				// instruction affected the value of some string being eval'ed


#define	INSTR_IS_1_BRANCH			(1<<2)
#define	INSTR_IS_2_BRANCH			(1<<3)
#define	INSTR_IS_N_BRANCH			(1<<4)
#define	INSTR_IS_SCRIPT_INVOKE		(1<<5)				//eval and doc.write as well
#define	INSTR_IS_NATIVE_INVOKE		(1<<6)
#define INSTR_IS_RET				(1<<7)				// instruction is a return(ret/stop) from the invoked frame
#define INSTR_HAS_RET_VALUE			(1<<8)				//this is a 'ret' instr

#define INSTR_IS_LVAR_ACCESS		(1<<9)				//this is for those global access instr (e.g. setname) which actually access local variables
#define INSTR_IS_LARG_ACCESS		(1<<10)				// the number of arg/var is stored in oprand.i
														//caution: if 1 of these 2 flags is set, then propUse/Def fields must be empty

#define	INSTR_BRANCH_TAKEN			(1<<11)
#define	INSTR_BRANCH_NOT_TAKEN		(1<<12)

#define INSTR_IS_FUNC_START_IN_SLICE	(1<<13)
#define INSTR_IS_FUNC_END_IN_SLICE		(1<<14)

#define	INSTR_IS_BBL_START			(1<<15)
#define INSTR_IS_BBL_END			(1<<16)

#define	INSTR_EVAL_AFFECT_SLICE		(1<<17)				// instr is eval, and generated instrs in slice
#define	INSTR_IS_DOC_WRITE			(1<<18)				// instr is the native call to document.write()
//INSTR_IS_SCRIPT_INVOKE && INSTR_IS_DOC_WRITE means a call to document.write which generate trace
//INSTR_IS_NATIVE_INVOKE && INSTR_IS_DOC_WRITE means a call to document.write which doesn't generate trace

#define INSTR_IS_ENV				(1<<19)				//instr is environment dependent (e.g. userAgent, Date(), etc)

#define INSTR_FLAG_TMP0				(1<<30)
#define INSTR_FLAG_TMP1				(1<<31)


#define	INSTR_SET_FLAG(instr, flag)		(instr->flags |= flag)
#define	INSTR_CLR_FLAG(instr, flag)		(instr->flags &= ~flag)
#define	INSTR_HAS_FLAG(instr, flag)		(instr->flags & flag)

typedef struct Instruction{
	int			order;
	ADDRESS 	addr;
	ADDRESS 	addr_origin;
	char		opName[MAX_OPNAME_LEN];
	OpCode		opCode;
	int			length;
	uint32_t	flags;
	char *		eval_str;	// actual string passed to eval
	char *		str;		// this used to store prop/global var/func name used by opcode
	ADDRESS		objRef;
	union{
		char 		*s;
		double		d;
		long		i;
	}	operand;		//this store 'constant' operand of opcodes
	int			stackUse;
	ADDRESS		stackUseStart;
	int			stackDef;
	ADDRESS		stackDefStart;
	ADDRESS		propUseScope;
	long		propUseId;
	ADDRESS		propDefScope;
	long		propDefId;
	ADDRESS		localUse;
	ADDRESS		localDef;
	ADDRESS		localDef2;
	int			jmpOffset;
	int			nvars;
	BasicBlock 	*inBlock;
	BasicBlock 	*nextBlock;			//only used when INSTR_IS_BBL_END

	ADDRESS		inCallee;
	ADDRESS		inFunction;			//filled by inbuildFunctionCFGs()
}Instruction;

typedef struct InstrList {
	int			numInstrs;
    ArrayList	*iList;
}InstrList;

/*********************************************************************************/

typedef enum {
	BT_UNKNOWN,
	BT_FALL_THROUGH,
    BT_1_BRANCH,
    BT_2_BRANCH,
    BT_N_BRANCH,
    BT_SCRIPT_INVOKE,
    //BT_EVAL,
    BT_RET,
    BT_AUG_EXIT
} BasicBlockType;

struct BasicBlock{
    int id;
    BasicBlockType type;
    ADDRESS addr;       // address of first instruction -- used for lookups
    ADDRESS end_addr;
    Instruction *lastInstr;		// last instruction in the block, for the purpose of fast lookup
    ArrayList *instrs;  // the instructions in the block: arraylist of Instruction
    ArrayList *preds, *succs;  // predecessors and successors: arraylist of Edge
    int count;

    int dfn;			//depth-first number
    ArrayList *dominators;		// this an array of BasicBlocks that dominate this block
    ArrayList *dominate;		// this is an array of EDGEs that points to the blocks dominated by this block
    ArrayList *immDomPreds;		//at most one element in this list!
    ArrayList *immDomSuccs;

    //similar to above lists, but for this block in reverse CFG.
    ArrayList *reverseDominators;
    ArrayList *reverseDominate;
    ArrayList *reverseImmDomPreds;		//again, at most one element in this list!
    ArrayList *reverseImmDomSuccs;

    ArrayList *reverseDomFrontier;

    uint32_t flags;
    struct BasicBlock *calltarget; // this is assigned in several cases but not actually used

    ADDRESS inFunction;			//filled in traverseFuncBody()
};


#define BBL_IS_ENTRY_NODE			(1<<0)
#define BBL_IS_HALT_NODE			(1<<1)
#define BBL_IS_LOOP_HEADER			(1<<2)
#define BBL_IS_FUNC_ENTRY			(1<<3)
#define BBL_IS_FUNC_RET				(1<<4)
#define BBL_IS_EVAL_ENTRY			(1<<5)
#define BBL_IS_EVAL_RET				(1<<6)
#define BBL_IS_IN_SLICE				(1<<7)
#define BBL_IS_AUG_EXIT				(1<<8)

#define BBL_FLAG_TMP0				(1<<30)
#define BBL_FLAG_TMP1				(1<<31)

#define	BBL_SET_FLAG(bbl, flag)		(bbl->flags |= flag)
#define	BBL_CLR_FLAG(bbl, flag)		(bbl->flags &= ~flag)
#define	BBL_HAS_FLAG(bbl, flag)		(bbl->flags & flag)

/*********************************************************************************/

typedef struct BlockEdge{
	int id;
	uint32_t flags;
	int count;
	BasicBlock *tail;
	BasicBlock *head;
} BlockEdge;

#define EDGE_IN_DFST				(1<<0)			//this edge is part of DFS tree
#define EDGE_IS_RETREAT				(1<<1)
#define EDGE_IS_BACK_EDGE			(1<<2)
#define EDGE_IS_SCRIPT_INVOKE		(1<<3)
#define EDGE_IS_RET					(1<<4)
#define EDGE_IS_CALLSITE			(1<<5)			//this edge connects a script-call block and it's returned to block
#define EDGE_IS_DOMINATE			(1<<6)			//this edge is an dominate-edge, i.e. in block->dominate and tail dom head
#define EDGE_IS_COMPLEMENT			(1<<7)			//this edge is added as a complement edge, which is not existed in dynamic trace but should be in static CFG
#define EDGE_IS_IMM_DOM				(1<<8)
#define EDGE_IS_TO_AUG_EXIT			(1<<9)			//an augmented egde, points to augmented exit block for each function CFG from all BBL_IS_FUNC_RET blocks
#define EDGE_IS_REV_DOMINATE		(1<<10)
#define EDGE_IS_REV_IMM_DOM			(1<<11)

#define EDGE_IS_BRANCHED_PATH					(1<<16)
#define EDGE_IS_ADJACENT_PATH					(1<<17)


#define EDGE_FLAG_TMP0				(1<<30)
#define EDGE_FLAG_TMP1				(1<<31)

#define	EDGE_SET_FLAG(edge, flag)		(edge->flags |= flag)
#define	EDGE_CLR_FLAG(edge, flag)		(edge->flags &= ~flag)
#define	EDGE_HAS_FLAG(edge, flag)		(edge->flags & flag)

/*********************************************************************************/

typedef struct NaturalLoop{
	int id;
	uint32_t flags;
	BasicBlock *header;
	ArrayList *backEdges;
	ArrayList *nodes;
}NaturalLoop;


#define LOOP_FLAG_TMP0				(1<<31)

#define	LOOP_SET_FLAG(loop, flag)		(loop->flags |= flag)
#define	LOOP_CLR_FLAG(loop, flag)		(loop->flags &= ~flag)
#define	LOOP_HAS_FLAG(loop, flag)		(loop->flags & flag)

/*********************************************************************************/

typedef struct Function{
	//int id;
	uint32_t flags;
	int		args;
	int		loc_vars;
	ADDRESS	funcEntryAddr;
	//ADDRESS	funcObj;
	ArrayList *funcObj;
	BasicBlock *funcEntryBlock;
	char *funcName;
	ArrayList *funcBody;		//a CFG of this function, which is a list of BasicBlocks belongs to this function

	//BasicBlock *augmentedExit;
									// all the BBL_IS_FUNC_RET blocks preceed this augmented block in each funtion CFG.
									// the only purpose for this is for domination analysis on reversed CFGs right now.
									// and this block is added AFTER function CFGs are seperated from main CFG,
									// so buildFunctionCFGs should not worry about this extra field.
}Function;


#define FUNC_IN_SLICE					(1<<0)

#define	FUNC_SET_FLAG(func, flag)		(func->flags |= flag)
#define	FUNC_CLR_FLAG(func, flag)		(func->flags &= ~flag)
#define	FUNC_HAS_FLAG(func, flag)		(func->flags & flag)

/*********************************************************************************/

typedef struct Range {
    ADDRESS	max;
    ADDRESS	min;
} Range;

typedef struct WriteSet {
    ArrayList           *ranges;
} WriteSet;


/*********************************************************************************/

typedef struct Property{
	ADDRESS	obj;
	long	id;
	char *name;
}Property;

typedef struct OpStack{
	int size;
	int sp;
	void **stack;
	void (*printFunc)(void *);
}OpStack;




#endif /* _PRV_TYPES_H */
