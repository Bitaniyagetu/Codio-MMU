// list/list.c
// 
// Implementation for linked list.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

list_t *list_alloc() { 
  list_t* list = (list_t*)malloc(sizeof(list_t));
  list->head = NULL;
  return list; 
}

node_t *node_alloc(block_t *blk) {   
  node_t* node = malloc(sizeof(node_t));
  node->next = NULL;
  node->blk = blk;
  return node; 
}

void list_free(list_t *l)
{
    if (l == NULL)
        return;

    // Removing a node transfers its block to the caller.
    block_t *blk;
    while ((blk = list_remove_from_front(l)) != NULL)
        free(blk);

    free(l);
}

void node_free(node_t *node){
  free(node);
  return;
}

void list_print(list_t *l) {
  node_t *current = l->head;
  block_t *b;
  
  if (current == NULL){
    printf("list is empty\n");
  }
  while (current != NULL){
    b = current->blk;
    printf("PID=%d START:%d END:%d", b->pid, b->start, b->end);
    current = current->next;
  }
}

int list_length(list_t *l) { 
  node_t *current = l->head;
  int i = 0;
  while (current != NULL){
    i++;
    current = current->next;
  }
  
  return i; 
}

void list_add_to_back(list_t *l, block_t *blk){  
  node_t* newNode = node_alloc(blk);
  newNode->next = NULL;
  if(l->head == NULL){
    l->head = newNode;
  }
  else{
    node_t *current = l->head;
    while(current->next != NULL){
      current = current->next;
    }
    current->next = newNode;
  }
}

void list_add_to_front(list_t *l, block_t *blk){  
  node_t* newNode = node_alloc(blk);
 
  newNode->next = l->head;
  l->head = newNode;
}

void list_add_at_index(list_t *l, block_t *blk, int index)
{
    if (index < 0)
        return;

    node_t **position = &l->head;
    while (index > 0 && *position != NULL) {
        position = &(*position)->next;
        index--;
    }

    // Index zero inserts at the head. Index length inserts at the tail.
    if (index != 0)
        return;

    node_t *node = node_alloc(blk);
    node->next = *position;
    *position = node;
}

void list_add_ascending_by_address(list_t *l, block_t *blk)
{
    node_t **position = &l->head;

    while (*position != NULL && (*position)->blk->start <= blk->start)
        position = &(*position)->next;

    node_t *node = node_alloc(blk);
    node->next = *position;
    *position = node;
}

void list_add_ascending_by_blocksize(list_t *l, block_t *blk)
{
    node_t **position = &l->head;
    int size = blk->end - blk->start + 1;

    while (*position != NULL &&
           (*position)->blk->end - (*position)->blk->start + 1 <= size)
        position = &(*position)->next;

    node_t *node = node_alloc(blk);
    node->next = *position;
    *position = node;
}

void list_add_descending_by_blocksize(list_t *l, block_t *blk)
{
    node_t **position = &l->head;
    int size = blk->end - blk->start + 1;

    while (*position != NULL &&
           (*position)->blk->end - (*position)->blk->start + 1 >= size)
        position = &(*position)->next;

    node_t *node = node_alloc(blk);
    node->next = *position;
    *position = node;
}

void list_coalese_nodes(list_t *l)
{
    node_t *current = l->head;

    // The caller has already sorted the free list by address.
    while (current != NULL && current->next != NULL) {
        node_t *next = current->next;

        if ((long long)current->blk->end + 1 == next->blk->start) {
            current->blk->end = next->blk->end;
            current->blk->pid = 0;
            current->next = next->next;
            free(next->blk);
            node_free(next);
        } else {
            current = next;
        }
    }
}

block_t *list_remove_from_back(list_t *l)
{
    if (l->head == NULL)
        return NULL;

    node_t **position = &l->head;
    while ((*position)->next != NULL)
        position = &(*position)->next;

    node_t *last = *position;
    block_t *blk = last->blk;
    *position = NULL;
    node_free(last);
    return blk;
}

block_t* list_get_from_front(list_t *l) {
  block_t *value = NULL;
  if(l->head == NULL){
    return value;
  }
  else{
    node_t *current = l->head;
    value = current->blk;
  }
  return value; 
}


block_t* list_remove_from_front(list_t *l) { 
  block_t *value = NULL;
  if(l->head == NULL){
    return value;
  }
  else{
    node_t *current = l->head;
    value = current->blk;
    l->head = l->head->next;
    node_free(current);
  }
  return value; 
}

block_t* list_remove_at_index(list_t *l, int index) { 
  int i;
  block_t* value = NULL;
  
  bool found = false;
  
  if(l->head == NULL){
    return value;
  }
  else if (index == 0){
    return list_remove_from_front(l);
  }
  else if (index > 0){
    node_t *current = l->head;
    node_t *prev = current;
    
    i = 0;
    while(current != NULL && !found){
      if(i == index)
          found = true;
      else {
         prev = current;
         current = current->next;
         i++;
      }
    }
    
    if(found) {
      value = current->blk; 
      prev->next = current->next;
      node_free(current);
    }
  }
  return value; 
}

bool compareBlks(block_t* a, block_t *b) {
  
  if(a->pid == b->pid && a->start == b->start && a->end == b->end)
     return true;
  
  return false;
}

bool compareSize(int a, block_t *b) {  // greater or equal
  
  if(a <= (b->end - b->start) + 1)
     return true;
  
  return false;
}

bool comparePid(int a, block_t *b) {
  
  if(a == b->pid)
     return true;
  
  return false;
}


bool list_is_in(list_t *l, block_t* value) { 
  node_t *current = l->head;
  while(current != NULL){
    if(compareBlks(value, current->blk)){
      return true;
    }
    current = current->next;
  }
return false; 
}

block_t* list_get_elem_at(list_t *l, int index) { 
 int i;
  block_t* value = NULL;
  if(l->head == NULL){
    return value;
  }
  else if (index == 0){
    return list_get_from_front(l);
  }
  else if (index > 0){
    node_t *current = l->head;
    
    i = 0;
    while(current != NULL){
      if(i == index)
          return(current->blk);
      else {
         current = current->next;
         i++;
      }
    }
  }
  return value; 
}

int list_get_index_of(list_t *l, block_t* value){
 int i = 0;
 node_t *current = l->head;
 if(l->head == NULL){
    return -1;
  }
  
  while (current != NULL){
   if (compareBlks(value,current->blk)){
     return i;
    }
    current = current->next;
    i++;
  }
  return -1; 
}

/* Checks to see if block of Size or greater exists in the list. */
bool list_is_in_by_size(list_t *l, int Size){ 
  node_t *current = l->head;
  while(current != NULL){
    if(compareSize(Size, current->blk)){
      return true;
    }
    current = current->next;
  }
return false; 
}

/* Checks to see if pid of block exists in the list. */
bool list_is_in_by_pid(list_t *l, int pid)
{
    return list_get_index_of_by_Pid(l, pid) >= 0;
}

/* Returns the index at which the given block of Size or greater appears. */
int list_get_index_of_by_Size(list_t *l, int Size){
 int i = 0;
 node_t *current = l->head;
 if(l->head == NULL){
    return -1;
  }
  
  while (current != NULL){
   if (compareSize(Size,current->blk)){
     return i;
    }
    current = current->next;
    i++;
  }

  return -1; 
}
                   
/* Returns the index at which the pid appears. */
int list_get_index_of_by_Pid(list_t *l, int pid){
 int i = 0;
 node_t *current = l->head;
 if(l->head == NULL){
    return -1;
  }
  
  while (current != NULL){
   if (comparePid(pid,current->blk)){
     return i;
    }
    current = current->next;
    i++;
  }

  return -1; 
}