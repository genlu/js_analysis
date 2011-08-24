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

void labelInstructionList(InstrList *iList);

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
