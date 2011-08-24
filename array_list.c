/*
 *  ArrayList.c -- A fully functional, sortable ArrayList with set operations.
 *  Also supports iteration.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array_list.h"
#include "al_helper.h"

#define AL_LOCK()
#define AL_UNLOCK()



/* STATIC HELPER FUNCTIONS */



/*
 *  al_itrHasNext(itr) --
 */
static int al_itrHasNext(Iterator *itr) {
    ArrayList *list;
    int index;

    assert(itr);

    list = (ArrayList *) itr->it_dataStructure;
    assert(list);

    index = (long int) itr->it_opaque1;
    return index < al_size(list);
}



/*
 *  al_itrNext(itr) --
 */
static void *al_itrNext(Iterator *itr) {
    ArrayList *list;

    assert(itr);

    list = (ArrayList *) itr->it_dataStructure;
    assert(list);

    itr->it_opaque1 = (void *) ((long int) itr->it_opaque1 + 1);
    return itr->it_current = al_get(list, (long int) itr->it_opaque1 - 1);
}



/*
 *  al_itrRemove(itr) --
 */
static void *al_itrRemove(Iterator *itr) {
    ArrayList *list;
    long int index;
    void *result;

    assert(itr);

    list = (ArrayList *) itr->it_dataStructure;
    assert(list);
    index = (long int) itr->it_opaque1 - 1;

    result = al_get(list, index);

    al_removeAt(list, index);                   // remove node
    itr->it_opaque1 = (void *) index;           // correct index in iterator

    return result;
}



/* CONSTRUCTORS / DESTRUCTORS */



/*
 *  al_new(void) --
 *
 *  Initializes a new empty unsorted linked list of addresses.
 */
ArrayList *al_new(void) {
    return al_newPtr(AL_LIST_UNSORTED);
}



/*
 *  al_newT(type) -- initializes a new empty linked list of addresses.
 */
ArrayList *al_newT(ATYPE type) {
    return al_newPtr(type);
}



/*
 *  al_newInt(type) -- initializes a new empty linked list of integers.
 */
ArrayList *al_newInt(ATYPE type) {
    return al_newGeneric(type, refCompare, intPrint, NULL);
}



/*
 *  al_newPtr(type) -- initializes a new empty linked list of addresses.
 */
ArrayList *al_newPtr(ATYPE type) {
    return al_newGeneric(type, refCompare, refPrint, dealloc);
}



/*
 *  al_newStr(type) -- initializes a new empty linked list.
 */
ArrayList *al_newStr(ATYPE type) {
    return al_newGeneric(type, strCompare, strPrint, dealloc);
}



/*
 *  al_newGeneric(type,compareFunc)printFunc)freeFunc) --
 *
 *  Initializes a new empty linked list of given type and
 *  with the given function to pairwise compare/sort its elements.
 *
 *  The function is expected to return:
 *  -1 if the first argument is 'less than' the second argument,
 *  0 if they are equal,
 *  1 if the first argument is 'greater than' the second argument.
 */
ArrayList *al_newGeneric(ATYPE type, int (*compareFunc) (void *, void *),
    void (*printFunc) (void *), void (*freeFunc) (void *)) {

    ArrayList *list = zalloc(sizeof(ArrayList));
    list->al_size = 0;
    list->al_capacity = 10;
    list->al_elements = zalloc(list->al_capacity * sizeof(void *));
    list->al_type = type;
    list->al_lock = 1;
    list->al_compare = compareFunc ? compareFunc : refCompare;
    list->al_print = printFunc ? printFunc : refPrint;
    list->al_free = freeFunc ? freeFunc : NULL;

    return list;
}



/*
 *  al_free(list) --
 *
 *  Frees the given list (doesn't free the elements themselves).
 */
void al_free(ArrayList *list) {
    AL_LOCK();
    dealloc(list->al_elements);
    dealloc(list);
    AL_UNLOCK();
}



/*
 *  al_freeWithElements(list) --
 *
 *  Frees the given list, and calls its free function to free every element
 *  contained inside it.
 */
void al_freeWithElements(ArrayList *list) {
    al_clearAndFreeElements(list);
    al_free(list);
}



/*
 *  al_add(list,val) -- add an element to the end of the list.
 */
void *al_add(ArrayList *list, void *val) {
    assert(list);
    return (list->al_type == AL_LIST_SORTED)
        ? al_insertSorted(list, val)
        : al_insertAt(list, val, al_size(list));
}



/*
 *  al_addAll(list,list2) -- append list2 to list1.
 */
void al_addAll(ArrayList *list, ArrayList *list2) {
    int ii, size;

    assert(list && list2);

    // check for invalid arguments
    if (!list || !list2) {
        return;
    }

    AL_LOCK();

    // iterate through list2, adding each piece to list
    size = al_size(list2);
    for (ii = 0; ii < size; ii++) {
        al_add(list, al_get(list2, ii));
    }
    AL_UNLOCK();
}



/*
 *  al_backmap(list,func) --
 *
 *  This function applies the function func on every element
 *  in the list but in reverse order
 */
void al_backmap(ArrayList *list, void (*func) (void **)) {
    int ii;

    assert(list && func);

    // check for invalid arguments
    if (!list || !func) {
        return;
    }

    // iterate through list, applying func to each element
    for (ii = al_size(list) - 1; ii >= 0; ii--) {
        func(al_get(list, ii));
    }
}



/*
 *  al_clear(list) -- remove all elements from the given list.
 */
void al_clear(ArrayList *list) {
    AL_LOCK();
    assert(list);
    memset(list->al_elements, 0, list->al_size * sizeof(void *));
    list->al_size = 0;
    AL_UNLOCK();
}



/*
 *  al_clearAndFreeElements(list) --
 *
 *  Calls given list's free function to free every element
 *  contained inside it.  Also clears list to have 0 elements.
 */
void al_clearAndFreeElements(ArrayList *list) {
    int ii;

    if (!list) {
      assert(list);
    }

    // free each node
    AL_LOCK();
    for (ii = 0; ii < list->al_size; ii++) {
        if (list->al_free) {
            list->al_free(list->al_elements[ii]);
        }
    }
    AL_UNLOCK();

    al_clear(list);
}



/*
 *  al_clone(list) -- returns an exact duplicate of the given list.
 */
/*ArrayList *al_clone(ArrayList *list) {
    ArrayList *newList = alloc(sizeof(ArrayList));
    memcpy(newList, list, sizeof(ArrayList));
    return newList;
}*/

ArrayList *al_clone(ArrayList *list){
		ArrayList *newList = al_newGeneric(list->al_type, list->al_compare, list->al_print, list->al_free);
		int i;
		void *elm;
		for(i=0;i<al_size(list);i++){
			elm = al_get(list, i);
			al_add(newList, elm);
		}
		assert(list->al_size = newList->al_size);
		return newList;
}

/*
 *  al_contains(list,val) --
 *
 *  Returns whether the list contains the given value.
 *  Compares by the list's given comparison function.
 */
int al_contains(ArrayList *list, void *val) {
    assert(list);
    return al_indexOf(list, val) >= 0;
}



/*
 *  al_equals(list1,list2) --
 *
 *  Returns whether the two given linked lists are memberwise equal,
 *  using the comparison function of list1 to compare elements pairwise.
 *
 *  If the lists are sets, the order is not considered; only the elements.
 */
int al_equals(ArrayList *list1, ArrayList *list2) {
    int ii, size;

    assert(list1 && list2);

    size = al_size(list1);
    if (size != al_size(list2)) {
        return 0;
    }

    if (list1->al_type == AL_LIST_SET && list2->al_type == AL_LIST_SET) {
        // do not have to be in same order, just have to be present
        for (ii = 0; ii < size; ii++) {
            if (!al_contains(list1, al_get(list2, ii))
                    || !al_contains(list2, al_get(list1, ii))) {
                return 0;
            }
        }
    } else {
        for (ii = 0; ii < size; ii++) {
            if (list1->al_compare(al_get(list1, ii), al_get(list2, ii))) {
                return 0;
            }
        }
    }

    return 1;
}



/*
 *  al_first(list) -- returns the first element of the list.
 */
void *al_first(ArrayList *list) {
    return al_get(list, 0);
}



/*
 *  al_get(list,index) --
 *
 *  Return the element at index index
 *  Must check that index < numElements head
 */
void *al_get(ArrayList *list, int index) {
  if (!(list && 0 <= index && index < al_size(list))) {
	return 0;
    assert(0);
  }

    return list->al_elements[index];
}



/*
 *  al_indexOf(list,val) --
 *
 *  Lookup val in list using list's compare function for equality test
 *  Return index if present, otherwise -1
 */
int al_indexOf(ArrayList *list, void *val) {
    int ii, size;
    assert(list);

    // check for invalid arguments
    if (!list) {
        return -1;
    }

    // iterate through list to find val, if present
    size = al_size(list);
    for (ii = 0; ii < size; ii++) {
        if (!list->al_compare(al_get(list, ii), val)) {
            return ii;
        }
    }

    return -1;
}



/*
 *  al_insert(list,val) -- add an element to the start of the list.
 */
void *al_insert(ArrayList *list, void *val) {
    assert(list);
    return (list->al_type == AL_LIST_SORTED)
        ? al_insertSorted(list, val)
        : al_insertAt(list, val, 0);
}



/*
 *  al_insertAt(list,val,index) -- add an element at the given list index.
 */
void *al_insertAt(ArrayList *list, void *val, int index) {
    int ii;

    assert(list && 0 <= index && index <= al_size(list));

    // check for invalid parameters
    // (includes duplicate check if list is a set)
    if (!(0 <= index && index <= al_size(list))
        || (list->al_type == AL_LIST_SET && al_contains(list, val)))
        return val;

    AL_LOCK();

    // may have to reallocate elements to make room
    if (list->al_size >= list->al_capacity) {
        list->al_capacity *= 2;
        list->al_elements =
            resize(list->al_elements, list->al_capacity * sizeof(void *));
    }
    // slide elements right by one to make room for new value
    for (ii = list->al_size; ii > index; ii--) {
        list->al_elements[ii] = list->al_elements[ii - 1];
    }

    // insert new value
    list->al_elements[index] = val;
    list->al_size++;

    AL_UNLOCK();
    return val;
}



/*
 *  al_insertSorted(list,val) -- add an element at the given index of the list.
 */
void *al_insertSorted(ArrayList *list, void *val) {
    int ii, size;

    assert(list);

    // duplicate check if list is a set
    if (list->al_type == AL_LIST_SET && al_contains(list, val)) {
        return val;
    }

    AL_LOCK();

    // iterate through to find place to insert this new link
    // (insert it after all nodes that are <= it
    size = al_size(list);
    for (ii = 0; ii<size && list->al_compare(al_get(list,ii),val)<=0; ii++) {
    }

    AL_UNLOCK();

    // put the new node here
    al_insertAt(list, val, ii);
    return val;
}



/*
 *  al_isEmpty(list) -- returns 1 if the list has no elements.
 */
int al_isEmpty(ArrayList *list) {
    return al_size(list) == 0;
}



/*
 *  al_iterator(list) --
 */
Iterator *al_iterator(ArrayList *list) {
    Iterator *itr;
    assert(list);

    itr = zalloc(sizeof(Iterator));
    itr->it_dataStructure = list;
    itr->it_next = al_itrNext;
    itr->it_hasNext = al_itrHasNext;
    itr->it_remove = al_itrRemove;
    itr->it_opaque1 = (void *) 0;

    return itr;
}



/*
 *  al_last(list) -- returns the last element of the list.
 */
void *al_last(ArrayList *list) {
    return al_get(list, al_size(list) - 1);
}



/*
 *  al_map(list,func) --
 *
 *
 *  This function applies the function func on every element
 *  in the list.
 */
void al_map(ArrayList *list, void (*func) (void **)) {
    int ii, size;

    assert(list && func);

    // check for invalid arguments
    if (!list || !func) {
        return;
    }

    // iterate through list, applying func to each element
    size = al_size(list);
    for (ii = 0; ii < size; ii++) {
        func(al_get(list, ii));
    }
}



/*
 *  al_print(list) --
 *
 *
 *  Prints this linked list, using the given function pointer to print
 *  each argument.
 */
void al_print(ArrayList *list) {
    int i = 0;

    if (!list) {
        printf("NULL\n");
    } else {
        printf("LIST: [");

        if (al_size(list) > 0) {
            list->al_print(al_get(list, 0));

            for (i = 1; i < al_size(list); i++) {
                printf(", ");
                list->al_print(al_get(list, i));
            }
        }

        printf("]\n");
    }
}



/*
 *  al_remove(list,val) --
 *
 *
 *  Remove an element at the specified index
 *  Must check that index < numElements head
 */
void *al_remove(ArrayList *list, void *val) {
    return al_removeAt(list, al_indexOf(list, val));
}



/*
 *  al_removeAt(list,index) --
 *
 *
 *  Remove an element at the specified index
 *  Must check that index < numElements head
 */
void *al_removeAt(ArrayList *list, int index) {
    int ii, size;
    void *result;

    size = al_size(list);
    assert(list && 0 <= index && index < size);

    //set return val
    result = list->al_elements[index];

    // slide remaining elements left by one
    AL_LOCK();
    for (ii = index; ii < size - 1; ii++) {
        list->al_elements[ii] = list->al_elements[ii + 1];
    }
    list->al_elements[size - 1] = NULL;
    list->al_size--;
    AL_UNLOCK();
    return result;
}



/*
 *  al_removeFirst(list) -- removes and returns first element of list.
 */
void *al_removeFirst(ArrayList *list) {
    return al_removeAt(list, 0);
}



/*
 *  al_removeLast(list) -- removes and returns last element of list.
 */
void *al_removeLast(ArrayList *list) {
    return al_removeAt(list, al_size(list) - 1);
}



/*
 *  al_size(list) -- returns the number of elements in the list.
 */
int al_size(ArrayList *list) {
    if(list == NULL) {
        assert(list);
    }
    return list->al_size;
}



/*
 *  al_sort(list) --
 *
 *
 *  Sorts the linked list using its compare function for equality test.
 *  Uses a crappy selection sort because I do not wish to implement
 *  anything tougher!
 */
void al_sort(ArrayList *list) {
    int ii, jj, size, smallestIndex;
    void *el_ii;
    void *el_jj;
    void *temp;
    void *el_smallest;

    assert(list && list->al_compare);

    AL_LOCK();
    size = al_size(list);
    for (ii = 0; ii < size; ii++) {
        // find smallest element
        el_ii = al_get(list, ii);
        el_smallest = el_ii;
        smallestIndex = ii;

        for (jj = ii + 1; jj < size; jj++) {
            el_jj = al_get(list, jj);

            if (list->al_compare(el_jj, el_smallest) < 0) {
                el_smallest = el_jj;
                smallestIndex = jj;
            }
        }

        // swap
        if (smallestIndex != ii) {
            temp = el_ii;
            list->al_elements[ii] = el_smallest;
            list->al_elements[smallestIndex] = el_ii;
        }
    }

    AL_UNLOCK();
}



/*
 *  al_difference(list1,list2) --
 *
 *
 *  Returns the set difference of the two lists (which represent sets).
 *  The difference is the elements in list1 that are not in list2.
 */
ArrayList *al_difference(ArrayList *list1, ArrayList *list2) {
    int ii, size;
    void *curr;
    ArrayList *result = NULL;

    assert(list1 && list2);

    // make new empty list using list1's type and function ptrs
    result = al_newGeneric(
        list1->al_type, list1->al_compare, list1->al_print, list1->al_free);

    // add all elements of list1 that are not in list2
    size = al_size(list1);
    for (ii = 0; ii < size; ii++) {
        curr = al_get(list1, ii);
        if (!al_contains(list2, curr)) {
            al_add(result, curr);
        }
    }

    return result;
}



/*
 *  al_intersection(list1,list2) --
 *
 *
 *  Returns the set intersection of the two lists (which represent sets).
 *  The intersection is all elements in BOTH list1 and list2.
 */
ArrayList *al_intersection(ArrayList *list1, ArrayList *list2) {
    int ii, size;
    void *curr;
    ArrayList *result = NULL;

    assert(list1 && list2);

    // make new empty list using list1's type and function ptrs
    result = al_newGeneric(
        list1->al_type, list1->al_compare, list1->al_print, list1->al_free);

    // add all elements of list1 that are also in list2
    size = al_size(list1);
    for (ii = 0; ii < size; ii++) {
        curr = al_get(list1, ii);
        if (al_contains(list2, curr)) {
            al_add(result, curr);
        }
    }

    // add all elements of list2 that are also in list1
    size = al_size(list2);
    for (ii = 0; ii < size; ii++) {
        curr = al_get(list2, ii);
        if (al_contains(list1, curr)) {
            al_add(result, curr);
        }
    }

    return result;
}



/*
 *  al_setEquals(list1,list2) --
 *
 *  Returns whether the two linked lists, which represent sets,
 *  are memberwise equal.  Note that order is not considered, only contents.
 */
bool al_setEquals(ArrayList *list1, ArrayList *list2) {
    int i, n;

    assert(list1 && list2);

    /* if their sizes differ, they're unequal */
    n = al_size(list1);
    if (n != al_size(list2)) {
        return false;
    }

    /* if one is missing an element from the other, they're unequal */
    for (i = 0; i < n; i++) {
        if (!al_contains(list2, al_get(list1, i))) {
            return false;
        }
        if (!al_contains(list1, al_get(list2, i))) {
            return false;
        }
    }

    return true;
}



/*
 *  al_union(list1,list2) --
 *
 *  Returns the set union of the two linked lists (which represent sets).
 *  The union is all elements in either list1 or list2 or both.
 */
ArrayList *al_union(ArrayList *list1, ArrayList *list2) {
    int ii, size;
    void *curr;
    ArrayList *result = NULL;

    assert(list1 && list2);

    // make new empty list using list1's type and function ptrs
    result = al_newGeneric(
        list1->al_type, list1->al_compare, list1->al_print, list1->al_free);

    // add all elements of list1
    size = al_size(list1);
    for (ii = 0; ii < size; ii++) {
        al_add(result, al_get(list1, ii));
    }

    // add all elements of list2 not already present
    size = al_size(list2);
    for (ii = 0; ii < size; ii++) {
        curr = al_get(list2, ii);
        if (!al_contains(result, curr)) {
            al_add(result, curr);
        }
    }

    return result;
}
