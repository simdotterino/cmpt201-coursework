#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {

  // write a prompt
  // receive input with getline
  // loop over the tokens in the input
  // output the tokens

  char *line = NULL;
  size_t n = 0;

  printf("Please enter some text: ");

  if (getline(&line, &n, stdin) != -1) {

    printf("Tokens:\n");

    char *token = NULL;
    char *saveptr = NULL;
    char *str = line;
    char *delim = " ";

    token = strtok_r(str, delim, &saveptr);

    while (token != NULL) {

      printf("%s\n", token);
      str = NULL; // need to set str to NULL in order to continue parsing
      token = strtok_r(str, delim, &saveptr);
    }

  } else {
    printf("The line could not be read.\n");
  }
  free(line);
  return 0;
}
