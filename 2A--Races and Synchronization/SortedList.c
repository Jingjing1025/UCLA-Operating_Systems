// Name: Jingjing 
// Email: 
// ID:

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    int found = 0;
    
    if (list==NULL || element==NULL)
        return;
    
    SortedListElement_t *curr = list;
    
    if (opt_yield & INSERT_YIELD)
        sched_yield();
    
    while (curr != list)
    {
        if (strcmp(curr->key, element->key) >= 0)
        {
            curr->prev->next = element;
            element->prev = curr->prev;
            element->next = curr;
            curr->prev = element;
            found = 1;
            break;
        }
        curr = curr->next;
    }
    
    if (found == 0)
    {
        curr->prev->next = element;
        element->prev = curr->prev;
        element->next = curr;
        curr->prev = element;
    }
}

int SortedList_delete(SortedListElement_t *element)
{
    if (element->next->prev != element || element->prev->next != element)
        return 1;
    if (opt_yield & DELETE_YIELD)
        sched_yield();
    element->prev->next = element->next;
    element->next->prev = element->prev;
    
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    if (list==NULL || key==NULL)
        return NULL;
    SortedListElement_t *curr = list->next;
    
    if (opt_yield & LOOKUP_YIELD)
        sched_yield();
    
    while (curr != list)
    {
        if (strcmp(curr->key,key) == 0)
            return curr;
        else
            curr = curr->next;
    }
    
    return NULL;
}

int SortedList_length(SortedList_t *list)
{
    int count = 0;
    
    if (list==NULL)
        return 0;
    SortedListElement_t *curr = list->next;
    
    if (opt_yield & LOOKUP_YIELD)
        sched_yield();
    
    while (curr != list)
    {
        if (curr->prev->next != curr || curr->next->prev != curr)
            return -1;
        count += 1;
        curr = curr->next;
    }
    
    return count;
}
