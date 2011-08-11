#ifndef _READ_H
#define _READ_H


#include "prv_types.h"
#include "instr.h"



InstrList 	*InitInstrList(char *inFileName);
InstrList 	*BuildIListFromFile(FILE *file);
Instruction	*GetInstrFromText(char *buffer);

#endif
