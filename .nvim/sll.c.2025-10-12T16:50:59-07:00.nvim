#include <stdio.h>
#include <stdlib.h>

struct Node {
  int data;
  struct Node *next;
};

struct Node *createNode(int data) {
  struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
  newNode->data = data;
  newNode->next = NULL;
  return newNode;
}

void append(struct Node **head, int data) {
  struct Node *newNode = createNode(data); // create new node with the given
                                           // data

 // if (*head == NULL) {

  //  *head = newNode; // if there's no head, then create the new one
 // }

                   // otherwise, we have to shift the head over
                   //
                   // else-if not necessary
  newNode->next = *head; // the new node's next is head
  *head = newNode;       // and the new head is now newNode
  
}

void traverse(struct Node *head) {
  struct Node *current = head;
  while (current != NULL) {
    printf("%d -> ", current->data);
    current = current->next;
  }
  printf("NULL\n");
}

int main() {
  struct Node *head = NULL;
  append(&head, 1);
  append(&head, 2);
  append(&head, 3);
  printf("Linked List: ");
  traverse(head);
}
