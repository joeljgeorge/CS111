#include "SortedList.h"
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <stdio.h>

int opt_yield = 0;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
  //if list is null or element is null
  if(!list || !element)
    return;
  if(!list->next || !list->prev)
    return;
  //case 1: list is empty
  if((list->next == list)&&(list->prev == list)){
    if(opt_yield & INSERT_YIELD)
      sched_yield();
    list->next = element;
    list->prev = element;
    element->prev = list;
    element->next = list;
  }
  //case 2: element needs to be somewhere else in list
  else{
    SortedListElement_t* current_element = list->next;
    if(!current_element)
      return;
    SortedListElement_t* saved_element;
    SortedListElement_t* temp_prev;
    while(current_element!=list){
      temp_prev = current_element->prev;
      if(!temp_prev)
	return;
      if(!current_element->key || !element->key)
	return;
      if(strcmp(current_element->key,element->key)>=0){
	if(opt_yield & INSERT_YIELD)
	  sched_yield();
	temp_prev->next = element;
	element->prev = temp_prev;
	element->next = current_element;
	current_element->prev = element;
	return;
      }
      saved_element = current_element;
      current_element = current_element->next;
    }
    if(opt_yield & INSERT_YIELD)
      sched_yield();
    saved_element->next = element;
    element->prev = saved_element;
    element->next = list;
    list->prev = element;
    return;
  }
}

int SortedList_delete(SortedListElement_t *element){
  //case 1: pointer is null
  if(!element)
    return 1;
  //case 2: list doesn't have proper prev and next elements
  if(!element->prev || !element->next){
    free(element);
    return 1;
  }

  SortedListElement_t* prev_element = element->prev;
  SortedListElement_t* next_element = element->next;
  if((prev_element->next == element) && (next_element->prev == element)){
    if(opt_yield & DELETE_YIELD)
      sched_yield();
    prev_element->next = next_element;
    next_element->prev = prev_element;
    if(opt_yield & DELETE_YIELD)
      sched_yield();
    element->next = NULL;
    element->prev = NULL;
    free(element);
    return 0;
  }
  else
    return 1;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
  if(!list || !key)
    return NULL;
  if((list->next == list) && (list->prev == list))
    return NULL;

  SortedListElement_t* current_element = list->next;
  while(current_element != list){
    if(current_element->key){
      if(!strcmp(current_element->key, key)){
	if(opt_yield & LOOKUP_YIELD)
	  sched_yield();
	return current_element;
      }
    }
    current_element = current_element->next;
  }
  return NULL;
}

int SortedList_length(SortedList_t *list){
  if(!list)
    return -1;
  if(!list->next || !list->prev){
    fprintf(stderr, "NULL pointers found in circular list.");//TODO: remove from code
    return -1;
  }
  int length = 0;
  SortedListElement_t *current_element = list->next;
  SortedListElement_t *prev_element = list;
  SortedListElement_t *next_element = current_element->next;
  while(current_element!=list){
    if(!current_element || !prev_element || !next_element){
      fprintf(stderr, "NULL pointers found in circular list.");//TODO: remove from code
      return -1;
    }
    if(prev_element->next != current_element || 
       current_element->prev != prev_element ||
       current_element->next != next_element ||
       next_element->prev != current_element){
      return -1;
    }
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();
    length++;
    prev_element = current_element;
    current_element = next_element;
    next_element = current_element->next;
  }
  return length;
}
