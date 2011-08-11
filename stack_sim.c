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


