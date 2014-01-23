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
 * function.h
 *
 *  Created on: May 16, 2011
 *      Author: genlu
 */

#ifndef FUNCTION_H_
#define FUNCTION_H_

#include <stdint.h>
#include "prv_types.h"
#include "instr.h"
#include "array_list.h"
/*********************************************************************************/

typedef struct FuncObjTableEntry{
	int				id;
	uint32_t 		flags;
	ADDRESS			func_addr;
	ADDRESS			anonfunobj_instr_addr;
	char 			*func_name;
	Function		*func_struct;
	ArrayList 		*func_objs;
}FuncObjTableEntry;

#define	FE_IS_ANONFUNOBJ			(1<<0)

#define	FE_SET_FLAG(fe, flag)		(fe->flags |= flag)
#define	FE_CLR_FLAG(fe, flag)		(fe->flags &= ~flag)
#define	FE_HAS_FLAG(fe, flag)		(fe->flags & flag)

Function *createFunctionNode(void);
FuncObjTableEntry *createFuncObjTableEntry(void);
void destroyFuncObjTableEntry(void *entry);
ArrayList *constructFuncObjTable(InstrList *iList, ArrayList *funcCFGs);
FuncObjTableEntry *findFuncObjTableEntryByFuncAddr(ArrayList *FuncObjTable, ADDRESS funcAddr);
FuncObjTableEntry *findFuncObjTableEntryByFuncObj(ArrayList *FuncObjTable, ADDRESS funcObj);
void printFuncObjTable(ArrayList *funcObjTable);
/*********************************************************************************/

ArrayList *buildFunctionCFGs(InstrList *iList, ArrayList *blockList, ArrayList **funcObjTable);
void destroyFunctionCFGs(ArrayList *funcCFGs);

Function *findFunctionByObjAddress(ArrayList *functionCFGs, ADDRESS funcObjAddr);
Function *findFunctionByEntryBlockID(ArrayList *functionCFGs, int id);
Function *findFunctionByEntryAddress(ArrayList *functionCFGs, ADDRESS addr);

BasicBlock *isBlockInTheFunction(Function *func, int id);
#endif /* FUNCTION_H_ */
