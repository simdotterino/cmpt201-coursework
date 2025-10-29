#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {

  pid_t pid = fork();

  if (pid == 0) {
    // child
    execl("/usr/bin/ls", "ls", "-a", "-l", (char *)NULL);

  }

  else if (pid > 0) {
    // parent
    int wstatus;

    waitpid(pid, &wstatus, 0);
    if (WIFEXITED(wstatus)){
      printf("Child done.");
    }
    else{
      printf("%d", wstatus);
    }

  }

  else {
    printf("Fork failure");
  }
}
