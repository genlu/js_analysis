#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct Iterator {
    void *it_dataStructure;
    void *it_current;
    void *it_opaque1;
    void *it_opaque2;
    void *(*it_next) (struct Iterator *);
    int (*it_hasNext) (struct Iterator *);
    void *(*it_remove) (struct Iterator *);
} Iterator;

typedef enum { AL_LIST_UNSORTED, AL_LIST_SORTED, AL_LIST_SET } ATYPE;

typedef struct ArrayList {
    void **al_elements;
    int al_size;
    int al_capacity;
    ATYPE al_type;
    int al_lock;
    uint32_t flags;
    int (*al_compare) (void *, void *);
    void (*al_print) (void *);
    void (*al_free) (void *);
} ArrayList;

#define AL_FLAG_TMP1				(1<<31)

#define	AL_SET_FLAG(al, flag)		(al->flags |= flag)
#define	AL_CLR_FLAG(al, flag)		(al->flags &= ~flag)
#define	AL_HAS_FLAG(al, flag)		(al->flags & flag)


ArrayList *al_new(void);
ArrayList *al_newT(ATYPE type);
ArrayList *al_newInt(ATYPE type);
ArrayList *al_newPtr(ATYPE type);
ArrayList *al_newStr(ATYPE type);
ArrayList *al_newGeneric(ATYPE type, int (*compareFunc) (void *, void *),
    void (*printFunc) (void *), void (*freeFunc) (void *));
void al_free(ArrayList *list);
void al_freeWithElements(ArrayList *list);

void *al_add(ArrayList *list, void *val);
void al_addAll(ArrayList *list1, ArrayList *list2);
void al_backmap(ArrayList *list, void (*func) (void **));
void al_clear(ArrayList *list);
void al_clearAndFreeElements(ArrayList *list);
ArrayList *al_clone(ArrayList *list);
int al_contains(ArrayList *list, void *val);
int al_equals(ArrayList *list1, ArrayList *list2);
void *al_first(ArrayList *list);
void *al_get(ArrayList *list, int index);
int al_indexOf(ArrayList *list, void *val);
void *al_insert(ArrayList *list, void *val);
void *al_insertAt(ArrayList *list, void *val, int index);
void *al_insertSorted(ArrayList *list, void *val);
int al_isEmpty(ArrayList *list);
Iterator *al_iterator(ArrayList *list);
void *al_last(ArrayList *list);
void al_map(ArrayList *list, void (*func) (void **));
void al_print(ArrayList *list);
void *al_remove(ArrayList *list, void *val);
void *al_removeAt(ArrayList *list, int index);
void *al_removeFirst(ArrayList *list);
void *al_removeLast(ArrayList *list);
int al_size(ArrayList *list);
void al_sort(ArrayList *list);

ArrayList *al_difference(ArrayList *list1, ArrayList *list2);
ArrayList *al_intersection(ArrayList *list1, ArrayList *list2);
ArrayList *al_union(ArrayList *list1, ArrayList *list2);
bool al_setEquals(ArrayList *list1, ArrayList *list2);




#endif /*ARRAY_LIST_H_*/
