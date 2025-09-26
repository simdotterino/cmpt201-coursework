#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main() {

  pid_t pid = fork();

  if (pid == 0) {

    // child
    execl("/usr/bin/ls", "ls", "-a", "-l", "-h", (char *)NULL);

    if (execl("/usr/bin/ls", "ls", "-a", "-l", "-h", (char *)NULL) == -1)

      printf("Exec failed");
  }

  else if (pid > 0) {
    // parent

    execl("/usr/bin/ls", "ls", "-a", (char *)NULL);

  }

  else {

    perror("Fork failure");
  }
}
