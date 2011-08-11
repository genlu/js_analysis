/*
 * writeSet.c
 *
 *  Created on: Apr 21, 2011
 *      Author: genlu
 */
#include <stdio.h>
#include <stdlib.h>
#include "al_helper.h"
#include "writeSet.h"


Range *RangeCreate(ADDRESS min, ADDRESS max) {
    Range *new = malloc(sizeof(Range));
    if(!new)
    	return NULL;
    new->min = min;
    new->max = max;
    return (new);
}


void RangeDestroy(Range *r) {
    if (r) {
        free(r);
        r = NULL;
    }
}


Range *IntersectRange(Range *r1, Range *r2) {
    Range *new;
    ADDRESS newMin = r1->min;
    ADDRESS newMax = r1->max;

    if(r2->min > newMin) {
        newMin = r2->min;
    }

    if(r2->max < newMax) {
        newMax = r2->max;
    }

    if(newMax < newMin) {
        new = RangeCreate(1,0);
    } else {
        new = RangeCreate(newMin, newMax);
    }
    return (new);
}



WriteSet *WriteSetCreate() {
    WriteSet *new = malloc(sizeof(WriteSet));
    new->ranges = al_new();

    return (new);
}



WriteSet *CloneWriteSet(WriteSet *orig) {
    if(orig == NULL) {
        return NULL;
    }

    WriteSet *clone = NULL;

    int i = 0;
    int size = al_size(orig->ranges);

    if(size > 0) {
        clone = WriteSetCreate();

        for (i = 0; i < size; i++) {
            Range *next = al_get(orig->ranges, i);
            Range *new = RangeCreate(next->min, next->max);
            AddRangeToWriteSet(clone, new);
        }
    }

    return (clone);
}


void WriteSetDestroy(WriteSet *ws) {
    if(ws != NULL) {
        al_freeWithElements(ws->ranges);
        free(ws);
    }
    ws = NULL;
}


void AddRangeToWriteSet(WriteSet *ws, Range *r) {
    //check for "empty" range
	if(r == NULL) {
		return;
	}
    if(r->max < r->min) {
    	free(r);
        return;
    }

    int i;
    ADDRESS min, max;
    int len = al_size(ws->ranges);
    Range *temp = r;

    i = 0;
    while(i < len) {
        Range *next = (Range *)al_get(ws->ranges, i);

        // look for overlaps with existing ranges, we use tempmax +1 and
        // tempmin - 1 so that adjacent ranges are merged.
        // e.g. 1 to 5, and 6 to 10 --> 1 to 10
        if( ((temp->max+1) >= next->min) && ((temp->min-1) <= next->max) ) {
            // set new min and max for combined range
            min = temp->min;
            if(next->min < min) {
                min = next->min;
            }

            max = temp->max;
            if(next->max > max) {
                max = next->max;
            }

            // create new temp range
            temp->min = min;
            temp->max = max;

            // remove old range from list
            al_removeAt(ws->ranges, i);
            free(next);
            len = al_size(ws->ranges);
        } else {
            i++;
        }
    }
    al_add(ws->ranges, temp);
}


/*
 * SubtractRangeFromWriteSet(ws, r) -- remove range r from writeset ws
 */

WriteSet *SubtractRangeFromWriteSet(WriteSet *ws, Range *r) {
  WriteSet *wtmp0, *wtmp1;

  wtmp0 = WriteSetCreate();
  AddRangeToWriteSet(wtmp0, r);

  wtmp1 = SubtractWriteSets(ws, wtmp0);

  WriteSetDestroy(wtmp0);

  return wtmp1;
}


bool ValInWriteSet(WriteSet *ws, ADDRESS val) {
    bool valInWS = false;
    Iterator *itr = al_iterator(ws->ranges);

    while (!valInWS && itr->it_hasNext(itr)) {
        Range *r = (Range *)itr->it_next(itr);
        if ((val >= r->min) && (val <= r->max) ) {
            valInWS = true;
        }
    }
    free(itr);
    return (valInWS);
}





/* MergeWriteSets
 *  adds all ranges in write set 2 to write set 1, and returns write set 1
 *  NOTE: write set 1 is changed as a result of this function.
 */
WriteSet *MergeWriteSets(WriteSet *ws1, WriteSet *ws2) {
    Range *temp;
    int sz, i;

    sz = al_size(ws2->ranges);
    for (i = 0; i < sz; i++) {
        temp = (Range *)al_get(ws2->ranges, i);
        Range *new = RangeCreate(temp->min, temp->max);
        AddRangeToWriteSet(ws1, new);
    }

    return (ws1);
}



bool WriteSetIsEmpty(WriteSet *ws) {
	if(ws == NULL) {
		return (true);
	}
    int len = al_size(ws->ranges);
    if(len == 0) {
        return (true);
    }

    bool isEmpty = false;
    Range *r = al_get(ws->ranges,0);
    if(r->max < r->min) {
        isEmpty = true;
    }

    return (isEmpty);
}



bool WriteSetEquals(WriteSet *ws1, WriteSet *ws2) {
    int i = 0;
    ADDRESS j = 0;
    bool same = true;
    int sz1 = al_size(ws1->ranges);
    int sz2 = al_size(ws2->ranges);

    for (i = 0; i < sz1; i++) {
        Range *next1 = (Range *) al_get(ws1->ranges, i);
        for(j = next1->min; j <= next1->max; j++) {
            if(!ValInWriteSet(ws2, j)) {
                same = false;
            }
        }
    }

    for (i = 0; i < sz2; i++) {
        Range *next2 = (Range *) al_get(ws2->ranges, i);
        for(j = next2->min; j <= next2->max; j++) {
            if(!ValInWriteSet(ws1, j)) {
                same = false;
            }
        }
    }

    return (same);
}



WriteSet *IntersectWriteSets(WriteSet *ws1, WriteSet *ws2) {
    WriteSet *new = WriteSetCreate();
    int sz1, sz2, i, j;

    sz1 = al_size(ws1->ranges);
    sz2 = al_size(ws2->ranges);

    for (i = 0; i < sz1; i++) {
        Range *next1 = (Range *) al_get(ws1->ranges, i);
	for (j = 0; j < sz2; j++) {
            Range *next2 = (Range *) al_get(ws2->ranges, j);
            AddRangeToWriteSet(new, IntersectRange(next1, next2));
        }
    }

    return(new);
}


WriteSet *UnionWriteSets(WriteSet *ws1, WriteSet *ws2) {
    WriteSet *new = WriteSetCreate();

    new = MergeWriteSets(new, ws1);
    new = MergeWriteSets(new, ws2);

    return new;
}

/*
 * WriteSetsOverlap(ws1, ws2) -- returns true if the intersection of
 * the ranges of ws1 and ws2 is nonempty; false otherwise.
 */
bool WriteSetsOverlap(WriteSet *ws1, WriteSet *ws2)
{
  Range *r1, *r2;
  int sz1 = al_size(ws1->ranges), sz2 = al_size(ws2->ranges);
  int i, j;
  ADDRESS max1, min1, max2, min2, newMax, newMin;

    for (i = 0; i < sz1; i++) {
      r1 = (Range *) al_get(ws1->ranges, i);
      max1 = r1->max;
      min1 = r1->min;

      for (j = 0; j < sz2; j++) {
        r2 = (Range *) al_get(ws2->ranges, j);
        max2 = r2->max;
        min2 = r2->min;

        newMax = (max1 < max2 ? max1 : max2);
        newMin = (min1 > min2 ? min1 : min2);

        if (newMin <= newMax) {
          return true;
        }
      }
    }

    return false;
}


/* RangeIsEmpty
 *  returns true is r is null, or if r represents a null range, e.g. (1,0)
 *  return false o.w.
 */
bool RangeIsEmpty(Range *r) {
    bool isEmpty = true;

    if(r && r->max >= r->min) {
        isEmpty = false;
    }

    return (isEmpty);
}

/* ScrubWriteSet
 *  For given write set, removes any empty ranges to help improve speed of
 *  later calculations.  This function should *never need* to be called.
 */

void ScrubWriteSet(WriteSet *ws) {
    int size = al_size(ws->ranges);
    int cur = 0;

    while(cur < size) {
        Range *next = (Range *)al_get(ws->ranges, cur);
        if(RangeIsEmpty(next)) {
            al_removeAt(ws->ranges, cur);
            free(next);
            size = al_size(ws->ranges);
        } else {
            cur++;
        }
    }
}

/* SubtractWriteSets
 *  calculates ws1 - ws2
 *
 *  Algorithm is currently somewhat inefficient.  Subtracts ranges in ws2
 *  from each range in ws1.  In one case, this may result in two separate
 *  ranges.  If this happens, we modify existing range to account for first,
 *  create a new range for second, then add new range to the write set.
 *
 *  This requires that we reset the iterator to the beginning.  However, since
 *  write sets do not contain overlapping ranges, this is safe and will
 *  terminate.  It just causes some redundant calculation
 */
WriteSet *SubtractWriteSets(WriteSet *ws1, WriteSet *ws2) {
    WriteSet *new = WriteSetCreate();
    int sz1, i, sz2, j;
    sz1 = al_size(ws1->ranges);
    sz2 = al_size(ws2->ranges);

    for (i = 0; i < sz1; i++) {
        Range *next1 = (Range *)al_get(ws1->ranges, i);
        if(next1->min <= next1->max) {
        	AddRangeToWriteSet(new, RangeCreate(next1->min, next1->max));
        }
    }

    Iterator *itr1 = al_iterator(new->ranges);
    while(itr1->it_hasNext(itr1)) {
        Range *next1 = (Range *)itr1->it_next(itr1);

        for (j = 0; j < sz2; j++) {
            Range *next2 = (Range *) al_get(ws2->ranges, j);
            ADDRESS newMin = 1, newMax = 0;
            Range *newRange = NULL;
            if((next1->max < next2->min) || (next1->min > next2->max)) {
                //then no intersection, return next1
                //AddRangeToWriteSet(new, RangeCreate(next1->min, next1->max));
            } else if((next1->min <= next2->min) && (next1->max < next2->max)) {
                //   |------|
                //       |------|
                //AddRangeToWriteSet(new, RangeCreate(next1->min, next2->min-1));
                //next1->min unchanged
                next1->max = next2->min-1;
            } else if((next1->min > next2->min) && (next1->max >= next2->max)) {
                //       |------|
                //   |------|
                //AddRangeToWriteSet(new, RangeCreate(next2->max+1, next1->max));
                next1->min = next2->max+1;
                //next1->max unchanged
            } else if((next1->min <= next2->min) && (next1->max >= next2->max)) {
                //   |---------|
                //      |---|
                // result of subtract is two ranges
                //AddRangeToWriteSet(new, RangeCreate(next1->min, next2->min-1));
                //first create new range, bc uses next1->max value
                newMin = next2->max+1;
                newMax = next1->max;
                //AddRangeToWriteSet(new, RangeCreate(next2->max+1, next1->max));
                //then adjust next1 with new max
                //next1->min unchanged
                next1->max = next2->min-1;
            } else if((next1->min > next2->min) && (next1->max < next2->max)) {
                //      |---|
                //   |---------|
                // result of subtract is empty, use empty range notation
                next1->min = 1;
                next1->max = 0;
            } else {
                printf("write set 1\n");
                PrintWriteSet(ws1);
                printf("write set 2\n");
                PrintWriteSet(ws2);
                fprintf(stderr,"UNKNOWN RANGE SUBTRACT CASE\n");
            }

            if(newMin <= newMax) {
                //add range
            	newRange = RangeCreate(newMin, newMax);
                AddRangeToWriteSet(new,newRange);

                //reset iterator to start over
                free(itr1);
                itr1 = al_iterator(new->ranges);
            }
        }
    }
    free(itr1);
    ScrubWriteSet(new);

    return(new);
}


bool RangeInWriteSet(WriteSet *ws, Range *r) {
  int i, sz;

    if((r == NULL) || (r->max < r->min)) {
        return (false);
    }

    bool inWS = false;

    sz = al_size(ws->ranges);
    for (i = 0; i < sz && !inWS; i++) {
      Range *next = (Range *) al_get(ws->ranges, i);
        if( (r->max >= next->min) && (r->min <= next->max) ) {
            inWS = true;
        }
    }

    return (inWS);
}



void fPrintWriteSet(FILE *fp, WriteSet *ws) {
    int sz, i;
    Range *r;

    sz = al_size(ws->ranges);
    for (i = 0; i < sz; i++) {
        r = (Range *) al_get(ws->ranges, i);
        fprintf(fp, "%16lX--%-16lX ", r->min, r->max);
    }
}


void PrintWriteSet(WriteSet *ws) {
    fPrintWriteSet(stdout, ws);
    fprintf(stdout, "\n");

}


/*
 * WriteSetSubsetOfWriteSet(ws_sub, ws_super) -- returns true if
 * ws_sub is contained in ws_super, false o/w.
 */
bool WriteSetSubsetOfWriteSet(WriteSet *ws_sub, WriteSet *ws_super) {
  int i, sz;

  if (WriteSetIsEmpty(ws_sub)) {
    return true;
  }

  if (WriteSetIsEmpty(ws_super)) {
    return false;  // since ws_sub is nonempty
  }

  sz = al_size(ws_sub->ranges);
  for (i = 0; i < sz; i++) {
    Range *r = (Range *) al_get(ws_sub->ranges, i);
    if (!RangeInWriteSet(ws_super, r)) {
      return false;
    }
  }

  return true;
}


