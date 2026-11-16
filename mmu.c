#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "list.h"
#include "util.h"

void TOUPPER(char * arr){
  
    for(int i=0;i<strlen(arr);i++){
        arr[i] = (char)toupper((unsigned char)arr[i]);
    }
}

void get_input(char *args[], int input[][2], int *n, int *size, int *policy) 
{
  	FILE *input_file = fopen(args[1], "r");
	  if (!input_file) {
		    fprintf(stderr, "Error: Invalid filepath\n");
		    fflush(stdout);
		    exit(0);
	  }

    parse_file(input_file, input, n, size);
  
    fclose(input_file);
  
    if (*size <= 0) {
        fprintf(stderr, "Error: Initial partition size must be positive\\n");
        exit(EXIT_FAILURE);
    }

    TOUPPER(args[2]);
  
    if((strcmp(args[2],"-F") == 0) || (strcmp(args[2],"-FIFO") == 0))
        *policy = 1;
    else if((strcmp(args[2],"-B") == 0) || (strcmp(args[2],"-BESTFIT") == 0))
        *policy = 2;
    else if((strcmp(args[2],"-W") == 0) || (strcmp(args[2],"-WORSTFIT") == 0))
        *policy = 3;
    else {
       printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
       exit(1);
    }
        
}

static void insert_free_block(list_t *freelist, block_t *blk, int policy)
{
    switch (policy) {
    case 2:
        list_add_ascending_by_blocksize(freelist, blk);
        break;
    case 3:
        list_add_descending_by_blocksize(freelist, blk);
        break;
    default:
        list_add_to_back(freelist, blk);
        break;
    }
}

void allocate_memory(list_t *freelist, list_t *alloclist,
                     int pid, int blocksize, int policy)
{
    int index = -1;
    int selected_size = 0;
    int current_index = 0;

    // Coalescing leaves the free list in address order. Check every
    // candidate for Best Fit and Worst Fit, keeping the first on a tie.
    for (node_t *node = freelist->head; blocksize > 0 && node != NULL;
         node = node->next, current_index++) {
        int size = node->blk->end - node->blk->start + 1;

        if (size < blocksize)
            continue;

        if (index < 0 || (policy == 2 && size < selected_size) ||
            (policy == 3 && size > selected_size)) {
            index = current_index;
            selected_size = size;
        }

        if (policy == 1)
            break;
    }

    if (index < 0) {
        printf("Error: Memory Allocation %d blocks\n", blocksize);
        return;
    }

    block_t *blk = list_get_elem_at(freelist, index);
    int original_end = blk->end;
    int allocated_end = blk->start + (blocksize - 1);
    block_t *fragment = NULL;

    if (allocated_end < original_end) {
        fragment = malloc(sizeof(*fragment));
        if (fragment == NULL) {
            fprintf(stderr, "Error: Could not allocate block metadata\n");
            exit(EXIT_FAILURE);
        }

        fragment->pid = 0;
        fragment->start = allocated_end + 1;
        fragment->end = original_end;
    }

    blk = list_remove_at_index(freelist, index);
    blk->pid = pid;
    blk->end = allocated_end;
    list_add_ascending_by_address(alloclist, blk);

    if (fragment != NULL)
        insert_free_block(freelist, fragment, policy);
}

void deallocate_memory(list_t *alloclist, list_t *freelist, int pid, int policy)
{
    int index = list_get_index_of_by_Pid(alloclist, pid);

    if (index < 0) {
        printf("Error: Can't locate Memory Used by PID: %d\n", pid);
        return;
    }

    block_t *blk = list_remove_at_index(alloclist, index);
    blk->pid = 0;
    insert_free_block(freelist, blk, policy);
}

list_t *coalese_memory(list_t *list)
{
    node_t *sorted = NULL;

    // Relink existing free nodes by address so adjacent blocks are neighbors.
    while (list->head != NULL) {
        node_t *node = list->head;
        list->head = node->next;

        node_t **position = &sorted;
        while (*position != NULL && (*position)->blk->start <= node->blk->start)
            position = &(*position)->next;

        node->next = *position;
        *position = node;
    }

    list->head = sorted;
    list_coalese_nodes(list);
    return list;
}

void print_list(list_t * list, char * message){
    node_t *current = list->head;
    block_t *blk;
    int i = 0;
  
    printf("%s:\n", message);
  
    while(current != NULL){
        blk = current->blk;
        printf("Block %d:\t START: %d\t END: %d", i, blk->start, blk->end);
      
        if(blk->pid != 0)
            printf("\t PID: %d\n", blk->pid);
        else  
            printf("\n");
      
        current = current->next;
        i += 1;
    }
}

/* DO NOT MODIFY */
int main(int argc, char *argv[]) 
{
   int PARTITION_SIZE, inputdata[200][2], N = 0, Memory_Mgt_Policy;
  
   list_t *FREE_LIST = list_alloc();   // list that holds all free blocks (PID is always zero)
   list_t *ALLOC_LIST = list_alloc();  // list that holds all allocated blocks
   int i;
  
   if(argc != 3) {
       printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
       exit(1);
   }
  
   get_input(argv, inputdata, &N, &PARTITION_SIZE, &Memory_Mgt_Policy);
  
   // Allocated the initial partition of size PARTITION_SIZE
   
   block_t * partition = malloc(sizeof(block_t));   // create the partition meta data
   partition->start = 0;
   partition->end = PARTITION_SIZE + partition->start - 1;
                                   
   list_add_to_front(FREE_LIST, partition);          // add partition to free list
                                   
   for(i = 0; i < N; i++) // loop through all the input data and simulate a memory management policy
   {
       printf("************************\n");
       if(inputdata[i][0] != -99999 && inputdata[i][0] > 0) {
             printf("ALLOCATE: %d FROM PID: %d\n", inputdata[i][1], inputdata[i][0]);
             allocate_memory(FREE_LIST, ALLOC_LIST, inputdata[i][0], inputdata[i][1], Memory_Mgt_Policy);
       }
       else if (inputdata[i][0] != -99999 && inputdata[i][0] < 0) {
             printf("DEALLOCATE MEM: PID %d\n", abs(inputdata[i][0]));
             deallocate_memory(ALLOC_LIST, FREE_LIST, abs(inputdata[i][0]), Memory_Mgt_Policy);
       }
       else {
             printf("COALESCE/COMPACT\n");
             FREE_LIST = coalese_memory(FREE_LIST);
       }   
     
       printf("************************\n");
       print_list(FREE_LIST, "Free Memory");
       print_list(ALLOC_LIST,"\nAllocated Memory");
       printf("\n\n");
   }
  
   list_free(FREE_LIST);
   list_free(ALLOC_LIST);
  
   return 0;
}