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
 * opcode.c
 *
 *  Created on: May 17, 2011
 *      Author: genlu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "opcode.h"

name_op	instrNames[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format,imp)		{name, op},
#include "opcode.tbl"
#undef OPDEF
};

int size = 0;

OpCode getOpCodeFromString(char *name){
	int i;//, size;
	int found;
	OpCode op;
	assert(name);
	if(!size)
		size = sizeof(instrNames)/sizeof(name_op);
	found = 0;
	for(i=0;i<size;i++){
		if(!strcmp(name, instrNames[i].name)){
			op = instrNames[i].op;
			found++;
			break;
		}
	}

	if(!found){
		printf("ERROR: can't find opcode %s\n", name);
		abort();
	}

	return op;
}
