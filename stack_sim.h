#ifndef _STACK_SIM_H
#define	_STACK_SIM_H

#include <stdbool.h>

#include "prv_types.h"


OpStack *initOpStack(void (* printFunc)(void *));
void destroyOpStack(OpStack *s);
void pushOpStack(OpStack *s, void *item);
void *popOpStack(OpStack *s);
void *peekOpStack(OpStack *s);
void printOpStack(OpStack *s);
bool isOpStackEmpty(OpStack *s);
int countOpStack(OpStack *s);

#endif
