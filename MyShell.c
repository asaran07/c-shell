/*
 * File: MyShell.c
 * Author: Arshdeep Saran
 * Date: February 2, 2025
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
  @brief Fork a child to execute the command using execvp. The parent should
  wait for the child to terminate
  @param args Null terminated list of arguments (including program).
  @return returns 1, to continue execution and 0 to terminate the MyShell
  prompt.
 */
int execute(char **args) {
  if (!args[0]) {
    return 1;
  }

  if (strcmp(args[0], "exit") == 0) {
    return 0;
  }

  // INFO: pid_t is a data type for process IDs.
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    return 1;
  } else if (pid == 0) {
    // child
    execvp(args[0], args);
    perror("execvp failed");
    exit(1);  // kill the child process, but the main program will still run. im
              // guessing this is one of the reasons why using child processes
              // is good?
  } else {
    // waiting parent
    int status;
    // INFO: WAIT vs WAITPID: The difference between wait and waitpid is that
    // wait(&status) waits for any child to exit, and if there's multiple
    // children it might just pick the first one that exits. waitpid(pid,
    // &status, 0) on the other hand tracks a specific child using the pid.
    waitpid(pid, &status, 0);  // INFO: block until child returns
    // TODO: need to add back wifexited and wifsignaled
    // maybe also add debug mode? (kinda like an alias).
  }
  return 1;
}

/**
  @brief gets the input from the prompt and splits it into tokens. Prepares the
  arguments for execvp
  @return returns char** args to be used by execvp
 */
char **parse(void) {
  // temp array of 255 characters (+\0 terminator)
  // NOTE: This is a local array on the stack.
  char buffer[256];
  printf("MyShell> ");
  // make sure the input doesn't excede the buffer size
  // NOTE: fgets won't return null if the user types more than 255 characters,
  // it will read at most sizeof(buffer) - 1 and then put a terminating null
  // byte. If the input excedes the 255 characters + \0, fgets will just store
  // the first 255 characters and leave the rest in the buffer.
  // Fgets only returns NULL if we get a EOF or CTRL + D happens.
  if (!fgets(buffer, sizeof(buffer), stdin)) {
    return NULL;
  }

  // fix the newline and null terminator symbols by replacing the newline
  // NOTE: size_t is a unsigned integer type that C uses for memory sizes and
  // string lengths. strlen() also returns a 'size_t' so we wanna store it in a
  // size_t datatype. Technically we can still use int but it's better to use
  // size_t.
  size_t len = strlen(buffer);
  // NOTE: Basically, if we typed "hello\n" into the terminal we'd have:
  // ['h', 'e', 'l', 'l', 'o', '\n', '\0']
  // strlen(buffer) will see the first six characters, or up until right before
  // the '\0', so the len is 6. And if we do buffer[len - 1], that's gonna be
  // the '\n' because the character at the index 5 is the '\n'.
  if (len > 0 && buffer[len - 1] == '\n') {
    buffer[len - 1] = '\0';
  }

  // make a capacity of 8 pointers for now.
  // NOTE: Keep in mind that 'sizeof(char *)' is like saying 'size of char
  // pointer', which we multiply by 8 so we got space for 8 char pointers.
  // Each pointer will store a token (word).
  int cap = 8;
  // INFO: Also we're doing malloc so this double pointer is on the HEAP.
  char **args = malloc(cap * sizeof(char *));
  if (!args) {  // INFO: !args means 'args == NULL'.
    // make sure allocation doesn't go wrong
    fprintf(stderr, "allocation error\n");
    exit(1);  // NOTE: We terminate here with an error code of 1 which would
              // exit the entire program. One important thing to note is that if
              // we had called execvp and then did exit(1), the program would
              // still keep going because it would only exit the current child
              // process which execvp splits into using parent and child
              // processes.
  }

  // count variable for iterating through tokens
  int count = 0;
  // Then we actually create the first token and tell strtok to use a space and
  // '\t as a delimiter'. Technically we could've done " \t\n" to also handle
  // new lines but we already got rid of them before.
  char *token = strtok(buffer, " \t");
  // Now we keep going until the token is null/ there's no more words left.
  // TODO: maybe move some of these functionalites to their own function cause
  // this is really messy lol.
  while (token != NULL) {
    if (count + 1 >=
        cap) {   // Here we have a single statement to check if the amount of
                 // tokes we've parsed through + 1 is more than the initial 8
                 // pointer capacity, and if it is, we realloc more space.
      cap *= 2;  // Also remember to increase the capacity variable itself too.
      args = realloc(args, cap * sizeof(char *));
      if (!args) {
        fprintf(stderr, "reallocation error\n");
        exit(1);
      }
    }

    // NOTE: Since buffer is a local array on the stack, it will get destroyed
    // once we exit out of this local function. That means that all the pointers
    // to the different tokens/words/commands will now point to already freed
    // storage in the memory which is ofc not good. That's why we need to make a
    // token_copy pointer on the heap and use strcpy to copy the tokens into the
    // new heap storage so everything remains intact even outside of this
    // function.
    // So technically we're just interating and getting some memory from the OS
    // each time for each token, and the token_copy variable is just a way to do
    // that.
    char *token_copy = malloc(strlen(token) + 1);
    if (!token_copy) {
      fprintf(stderr, "allocation error\n");
      exit(1);
    }
    strcpy(token_copy, token);
    args[count] =
        token_copy;  // so if we didn't do this part, would the tokens still be
                     // allocated? Cause its not like java where just because
                     // something doesn't have a connection to something else it
                     // would be garbage collected.
    count++;
    token = strtok(NULL, " \t");  // Move strtok to the next word/token.
  }
  args[count] = NULL;  // Also set the last pointer in the main arr array to
                       // NULL so we know where it ends and when to stop.
  return args;
}

/**
   @brief Main function should run infinitely until terminated manually using
   CTRL+C or typing in the exit command It should call the parse() and execute()
   functions
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv) {
  while (1) {
    char **args = parse();
    if (!args) {
      break;
    }
    int status = execute(args);
    if (args) {
      for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
      }
      free(args);
    }
    if (status == 0) {
      break;
    }
  }
  return EXIT_SUCCESS;
}
