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
