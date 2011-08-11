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


ArrayList *buildFunctionCFGs(InstrList *iList, ArrayList *blockList);
void destroyFunctionCFGs(ArrayList *funcCFGs);

Function *findFunctionByObjAddress(ArrayList *functionCFGs, ADDRESS funcObjAddr);
Function *findFunctionByEntryBlockID(ArrayList *functionCFGs, int id);
Function *findFunctionByEntryAddress(ArrayList *functionCFGs, ADDRESS addr);

BasicBlock *isBlockInTheFunction(Function *func, int id);
#endif /* FUNCTION_H_ */
