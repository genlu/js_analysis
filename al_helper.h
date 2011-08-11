/*
 * helper.h
 *
 *  Created on: May 28, 2009
 *      Author: kpcoogan
 */
#ifndef HELPER_H_
#define HELPER_H_

#include <stdio.h>
#include <string.h>

void *alloc(int size);
void *zalloc(int size);
void dealloc(void *ptr);
void *resize(void *address, int size);


int intCompare(void *o1, void *o2);
int refCompare(void *o1, void *o2);
int strCompare(void *o1, void *o2);
int insCompareByAddr(void *i1, void *i2);
int insCompareByOrder(void *i1, void *i2);
void intPrint(void *item);
void refPrint(void *item);
void strPrint(void *key);

#endif /* HELPER_H_ */
