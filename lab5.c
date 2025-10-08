#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct header {
  uint64_t size;
  struct header *next;
  int id;
};

void initialize_block(struct header *block, uint64_t size, struct header *next,
                      int id) {
  block->size = size;
  block->next = next;
  block->id = id;
}

int find_first_fit(struct header *free_list_ptr, uint64_t size) {

  struct header *block = free_list_ptr;

  while (block->next != NULL) {

    if (block->next->size >= size) {

      return block->next->id;
    }

    block = block->next;
  }

  return -1;
}

int find_best_fit(struct header *free_list_ptr, uint64_t size) {
  int best_fit_id = -1;

  struct header *block = free_list_ptr->next;
  struct header *best_fit = NULL;

  while (block != NULL) {

    if (block->size >= size) {

      if ((best_fit_id == -1) || (block->size < best_fit->size)) {
        best_fit_id = block->id;
        best_fit = block;
      }
    }

    block = block->next;
  }
  // smallest block that can accomodate
  return best_fit_id;
}

int find_worst_fit(struct header *free_list_ptr, uint64_t size) {
  int worst_fit_id = -1;

  struct header *block = free_list_ptr->next;
  struct header *worst_fit = NULL;

  while (block != NULL) {

    if (block->size >= size) {

      if ((worst_fit_id == -1) || (block->size > worst_fit->size)) {
        worst_fit_id = block->id;
        worst_fit = block;
      }
    }

    block = block->next;
  }
  return worst_fit_id;
}

int main(void) {

  struct header *free_block1 = (struct header *)malloc(sizeof(struct header));
  struct header *free_block2 = (struct header *)malloc(sizeof(struct header));
  struct header *free_block3 = (struct header *)malloc(sizeof(struct header));
  struct header *free_block4 = (struct header *)malloc(sizeof(struct header));
  struct header *free_block5 = (struct header *)malloc(sizeof(struct header));

  initialize_block(free_block1, 6, free_block2, 1);
  initialize_block(free_block2, 12, free_block3, 2);
  initialize_block(free_block3, 24, free_block4, 3);
  initialize_block(free_block4, 8, free_block5, 4);
  initialize_block(free_block5, 4, NULL, 5);

  struct header *free_list_ptr = free_block1;

  int first_fit_id = find_first_fit(free_list_ptr, 7);
  int best_fit_id = find_best_fit(free_list_ptr, 7);
  int worst_fit_id = find_worst_fit(free_list_ptr, 7);

  printf("The ID for First-Fit algorithm is: %d\n", first_fit_id);
  printf("The ID for the Best-Fit algorithm is: %d\n", best_fit_id);
  printf("The ID for the Worst-Fit algorithm is: %d\n", worst_fit_id);

  return 0;
}

// pseudo-code
// block = head of the linked list
// while block->next != NULL and block != NULL
//   if block->data is free and block->next->data is free
//     merge(block, block->next)
//     block->next=block->next->next
//   else
//     block=block->next
