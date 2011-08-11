/*
 * writeSet.h
 *
 *  Created on: Apr 21, 2011
 *      Author: genlu
 */


#ifndef _WRITESET_H
#define	_WRITESET_H

#include <stdbool.h>
#include "prv_types.h"

Range *RangeCreate(ADDRESS min, ADDRESS max);
void RangeDestroy(Range *r);
Range *IntersectRange(Range *r1, Range *r2);
WriteSet *WriteSetCreate(void);
WriteSet *CloneWriteSet(WriteSet *orig);
void WriteSetDestroy(WriteSet *ws);
void AddRangeToWriteSet(WriteSet *ws, Range *r);
WriteSet *SubtractRangeFromWriteSet(WriteSet *ws, Range *r);
bool ValInWriteSet(WriteSet *ws, ADDRESS val);
WriteSet *MergeWriteSets(WriteSet *ws1, WriteSet *ws2);
bool WriteSetIsEmpty(WriteSet *ws);
bool WriteSetEquals(WriteSet *ws1, WriteSet *ws2);
WriteSet *IntersectWriteSets(WriteSet *ws1, WriteSet *ws2);
WriteSet *UnionWriteSets(WriteSet *ws1, WriteSet *ws2);
bool WriteSetsOverlap(WriteSet *ws1, WriteSet *ws2);
bool RangeIsEmpty(Range *r);
void ScrubWriteSet(WriteSet *ws);
WriteSet *SubtractWriteSets(WriteSet *ws1, WriteSet *ws2);
bool RangeInWriteSet(WriteSet *ws, Range *r);
void fPrintWriteSet(FILE *fp, WriteSet *ws);
void PrintWriteSet(WriteSet *ws);
bool WriteSetSubsetOfWriteSet(WriteSet *ws_sub, WriteSet *ws_super);


#endif
