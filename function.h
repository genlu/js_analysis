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
