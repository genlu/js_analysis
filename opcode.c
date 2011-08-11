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
	int i, size;
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
