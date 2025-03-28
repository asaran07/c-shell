#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_pid_t.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  char **arr = malloc(5 * sizeof(char *));
  char *line = malloc(50 * sizeof(char));

  while (1) {
    printf(">");
    if (!fgets(line, 50, stdin)) {
      break;
    }
    printf("Input:%s", line);
    char *token = strtok(line, " \n");

    int n = 0;
    while (token != NULL) {
      printf("Parsing input:%s\n", token);
      *(arr + n) = token;
      token = strtok(NULL, " \n");
      n++;
    }
    *(arr + n) = NULL;
    if (n == 0) {
      continue;
    }
    if (strcmp(*(arr + 0), "exit") == 0) {
      break;
    }

    printf("\nExecuting command:'%s %s %s'\n\n", *arr, *(arr + 1), *(arr + 2));

    pid_t pid = fork();
    if (pid < 0) {
      perror("Fork failed");
      free(arr);
      free(line);
      return 1;
    } else if (pid == 0) {
      execvp(*(arr + 0), arr);
      perror("execvp failed");
      free(arr);
      free(line);
      return 1;
    } else {
      int status;
      wait(&status);
      if (WIFEXITED(status)) {
        printf("\ncommand completed with exit status:%d\n",
               WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
        printf("command terminated by signal:%d\n", WTERMSIG(status));
      }
      printf("command completed\n");
      free(arr);
      free(line);
    }
  }
  return 0;
}
