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
