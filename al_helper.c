/*
 *  helper.c -- helper functions for data structure code.
 */
#include <stdlib.h>
#include "al_helper.h"
#include "instr.h"

void *alloc(int size) {
    void *new = malloc(size);
    if(new == NULL) {
        fprintf(stderr,"ALLOC: Out of memory\n");
    }

    return (new);
}

void *zalloc(int size) {
    int i;
    char *new = (char *)alloc(size);

    if(new == NULL) {
        fprintf(stderr,"ZALLOC: Out of memory\n");
    }

    for(i = 0; i < size; i++) {
        new[i]= 0x0;
    }

    return (new);
}


void dealloc(void *ptr) {
    free(ptr);
}


void *resize(void *address, int newsize) {
    void *mem;

    mem = realloc(address, newsize);
    if (mem == NULL) {
        fprintf(stderr,"RESIZE: Out of memory\n");
    }

    return (mem);
}

/*
 *  intCompare(o1,o2) -- compare ints for sorting.
 *
 *  #%#% Does not work in case of overflow?
 */
int intCompare(void *o1, void *o2) {
    return refCompare(o1, o2);
}



/*
 *  refCompare(o1,o2) -- sorting comparison for ints and pointers.
 *
 *  Subtracts value of o1 from o2.
 *  #%#% Does not work in case of overflow?
 */
int refCompare(void *o1, void *o2) {
    return (long int) o1 - (long int) o2;
}



/*
 *  strCompare(o1,o2) -- compare strings for sorting.
 */
int strCompare(void *o1, void *o2) {
    return strcmp(o1, o2);
}



/*
 * insCompareByAddr(i1, i2) -- compare instructions by address for sorting
 */
int insCompareByAddr(void *i1, void *i2) {
  Instruction *ins1, *ins2;

  ins1 = (Instruction *)i1;
  ins2 = (Instruction *)i2;

  if (ins1->addr < ins2->addr) {
    return -1;
  }

  if (ins1->addr > ins2-> addr) {
    return 1;
  }

  return 0;
}


/*
 * insCompareByOrder(i1, i2) -- compare instructions by order # for sorting
 */
int insCompareByOrder(void *i1, void *i2) {
  Instruction *ins1, *ins2;

  ins1 = (Instruction *)i1;
  ins2 = (Instruction *)i2;

  if (ins1->order < ins2->order) {
    return -1;
  }

  if (ins1->order > ins2->order) {
    return 1;
  }

  return 0;
}

/*
 *  intPrint(item) -- print an integer.
 */
void intPrint(void *item) {
    printf("%ld", (long int) item);
}



/*
 *  refPrint(item) -- print memory addresses.
 */
void refPrint(void *item) {
    printf("%16lX", (unsigned long int) item);
}



/*
 *  strPrint(key) -- print a string.
 */
void strPrint(void *key) {
    printf("\"%s\"", (char *) key);
}
