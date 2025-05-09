#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    int status;
    pid_t pid, res;
   
  pid = fork();
    switch(pid) {
      case -1:
        perror("fork");
        _exit(EXIT_FAILURE);
      case 0:
        printf("Child with pid is %jd: Hello, world!\n", (intmax_t) getpid());
        exit(EXIT_SUCCESS);
      default:
        printf("Parent: Hello, world\n");
        res = waitpid(pid, &status, 0);

        if(res == -1) {
          perror("waitpid");
          exit(EXIT_FAILURE);
        }
        
        if (WIFEXITED(status)) {
          printf("child %d exited with %d\n",
                res, WEXITSTATUS(status));
        }
    }

    return 0;
}
