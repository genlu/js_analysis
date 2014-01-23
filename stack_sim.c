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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "stack_sim.h"

/*
 * Illustration of the stack
 * sp alway points to the first empty slot in the
 * (underlyding) list, so if sp==0, stack is empty
 *
 * |  a  |
 *  -----
 * |  b  |
 *  -----   <-sp
 *
 */
OpStack *initOpStack(void (* printFunc)(void *)){
	OpStack *s = (OpStack *)malloc(sizeof(*s));
	if(!s)
		return NULL;
	s->size = 32;
	s->sp = 0;
	s->stack = (void *)malloc(s->size * sizeof(void *));
	if(!(s->stack)){
		free(s);
		return NULL;
	}
	s->printFunc = printFunc?printFunc:NULL;
	return s;
}

void destroyOpStack(OpStack *s){
	if(!s)
		return;
	free(s->stack);
	free(s);
	s=NULL;
	return;
}

void pushOpStack(OpStack *s, void *item){
	assert(s);
	void *temp;
	if(s->sp >= s->size){
		//fprintf(stderr, "resize OpStack\n");
		temp = realloc((void *)(s->stack), 2*(s->size)*sizeof(void *));
		if(temp==NULL){
			fprintf(stderr, "ERROR: Memory shortage while resize OpStack\n");
			assert(0);
		}
		s->size = s->size*2;
		s->stack = temp;
	}
	s->stack[s->sp++] = item;
	return;
}

void *popOpStack(OpStack *s){
	assert(s);
	if(s->sp==0){
		fprintf(stderr, "ERROR: Pop empty stack\n");
		return NULL;
	}
	return s->stack[--s->sp];
}

void *peekOpStack(OpStack *s){
	assert(s);
	if(s->sp==0)
		return NULL;
	else
		return s->stack[s->sp-1];
}

bool isOpStackEmpty(OpStack *s){
	assert(s);
	if(s->sp==0)
		return true;
	else
		return false;
}

void printOpStack(OpStack *s){
	assert(s);
	int i;
	for(i=0;i<s->sp;i++){
		if(s->printFunc==NULL)
			printf("#%-4d  |_%16p_|\n", i, s->stack[i]);
		else{
			printf("#%-4d  |", i);
			s->printFunc(s->stack[i]);
			printf("\n");
		}
	}
}


int countOpStack(OpStack *s){
	assert(s);
	return s->sp;
}


