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
#ifndef _INSTR_H
#define	_INSTR_H

#include <stdint.h>
#include "prv_types.h"
#include "array_list.h"
#include "writeSet.h"

Instruction 	*InstrCreate(void);
void 			InstrDestroy(Instruction *instr);

InstrList 		*InstrListCreate(void);
void 			InstrListDestroy(InstrList *iList, int clearElem);
InstrList *InstrListClone(InstrList *iList, uint32_t flag);

int 			InstrListLength(InstrList *iList);
void 			InstrListAdd(InstrList *iList, Instruction *instr);
Instruction 	*InstrListRemove(InstrList *iList, Instruction *instr);

int labelInstructionList(InstrList *iList);

Instruction 	*getInstruction(InstrList *iList, int order);

bool isAND(InstrList *iList, Instruction *ins);
bool isOR(InstrList *iList, Instruction *ins);
bool is1WayBranch(InstrList *iList, Instruction *ins);
bool is2WayBranch(InstrList *iList, Instruction *ins);
bool isNWayBranch(InstrList *iList, Instruction *ins);
bool isBranch(InstrList *iList, Instruction *ins);
bool isRetInstruction(InstrList *iList, Instruction *ins);
bool isInvokeInstruction(InstrList *iList, Instruction *ins);
bool isNativeInvokeInstruction(InstrList *iList, Instruction *ins);

Property *getPropUse(InstrList *iList, int order);
Property *getPropDef(InstrList *iList, int order);
WriteSet *getMemUse(InstrList *iList, int order);
WriteSet *getMemDef(InstrList *iList, int order);

void 			printInstruction(Instruction *ins);
void 			printInstructionOrder(InstrList *iList, int order);
void 			printInstrList(InstrList *iList);

void processEvaledInstr(InstrList *iList);

#endif
