/*
 * opcode.h
 *
 *  Created on: May 17, 2011
 *      Author: genlu
 */

#ifndef OPCODE_H_
#define OPCODE_H_

#include "prv_types.h"


// structure to associate instruction names with their opcodes
typedef struct {
  char *name;
  OpCode op;
} name_op;


extern name_op	instrNames[];

OpCode getOpCodeFromString(char *name);

#endif /* OPCODE_H_ */
