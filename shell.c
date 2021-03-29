#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int find_pipe(char** cargs);

char** parse(char* s, char* cargs[]) {
  static char* words[500];
  memset(words, 0, sizeof(words));
  
  char break_chars[] = " \t";

  int i = 0;
  char* p = strtok(s, break_chars);

  char** redirect = malloc(2 * sizeof(char*));
  for(int i = 0; i < 2; ++i)
  {
    redirect[i] = malloc(BUFSIZ * sizeof(char));
    redirect[0] = "";
    redirect[1] = "";
  }
  words[i++] = p;
  while (p != NULL) {
    p = strtok(NULL, break_chars);
    if(p == NULL)
    {
      break;
    }
    if(!strncmp(p, ">", 1))
    {
      p = strtok(NULL, break_chars);
      redirect[0] = "o";
      redirect[1] = p;

      return redirect;
    }
    else if(!strncmp(p, "<", 1))
    {
      p = strtok(NULL, break_chars);
      redirect[0] = "i";
      redirect[1] = p;
      return redirect;

    }
    else if(!strncmp(p, "|", 1))
    {
      redirect[0] = "p";
    }
    words[i++] = p;
  }
  return words;
}


int main(int argc, const char * argv[]) 
{  
  char input[BUFSIZ];
  char last_command[BUFSIZ];
  char* cargs[BUFSIZ];
  char* rhs[BUFSIZ];
  char* lhs[BUFSIZ];
  int pipedescriptor[2];

  memset(input, 0, BUFSIZ * sizeof(char));
  memset(last_command, 0, BUFSIZ * sizeof(char));
  memset(lhs, 0, BUFSIZ * sizeof(char));
  memset(rhs, 0, BUFSIZ * sizeof(char));
  bool finished = false;
  
  while (!finished) {
    printf("osh> ");
    fflush(stdout);
    bool hold = true;
    char* waitoffset = strstr(input, "&");
      if(waitoffset != NULL)
      {
        *waitoffset =  ' ';
        hold = false;
      }
    if ((fgets(input, BUFSIZ, stdin)) == NULL) {   // or gets(input, BUFSIZ);
      fprintf(stderr, "no command entered\n");
      exit(1);
    }
    input[strlen(input) - 1] = '\0';          // wipe out newline at end of string
    printf("input was: \n'%s'\n", input);

    // check for history (!!) command
    if (strncmp(input, "!!", 2) == 0) 
    {
      if (strlen(last_command) == 0) 
      {
      fprintf(stderr, "no last command to execute\n");
      }
      printf("last command was: %s\n", last_command);
      
    }
    
     else if (strncmp(input, "exit()", 6) == 0) {   // only compare first 4 letters
      finished = true;
    } else {
      strcpy(last_command, input);
      printf("You entered: %s\n", input);   // you will call fork/exec
      
      pid_t pid = fork();

      if(pid < 0)
      {
          fprintf(stderr, "fork failed\n");
          exit(1);
      }
     else if(pid == 0) 
      {
        printf("I am the child. \n");
        memset(cargs, 0, BUFSIZ * sizeof(char));
        int hist = 0;
        char** redirect = parse((hist ? last_command : input), cargs);
        int history = 0;
            // if we use '!!' we want to read from MRU cache
        if(!strncmp(input, "!!", 2)) hist = 1;
        if(!strncmp(redirect[0], "o", 1))
        {
          printf("output saved to ./%s\n", redirect[1]);
          int fd = open(redirect[1], O_TRUNC | O_CREAT | O_RDWR);
          dup2(fd, STDOUT_FILENO);
        }
        else if(!strncmp(redirect[0], "i", 1))
        {
          printf("reading from file: ./%s\n", redirect[1]);
          int fd = open(redirect[1], O_RDONLY);
          memset(input, 0, BUFSIZ * sizeof(char));
          read(fd, input, BUFSIZ * sizeof(char));
          memset(cargs, 0, BUFSIZ * sizeof(char));
          input[strlen(input) - 1] = '\0';
          parse(input, cargs);
        }
        else if(!strncmp(redirect[0], "p", 1))
        {
          pid_t pidchild;
          int piperhs = find_pipe(cargs);
          cargs[piperhs] = "\0";
          int fail = pipe(pipedescriptor);
          if(fail < 0)
          {
            fprintf(stderr, "pipe creation failed.\n");
            return 1;
          }
          for(int i = 0; i < BUFSIZ; ++i)
          {
            lhs[i] = cargs[i];
          }

          for(int i = 0; i < BUFSIZ; ++i)
          {
            int index = i + piperhs + 1;
            if(cargs[index] == 0)
            break;
            rhs[i] = cargs[index];
          }

          pidchild = fork();
          if(pidchild < 0)
          {
            fprintf(stderr, "fork failed.\n");
            return 1;
          }

          if(pidchild != 0)
          {
            dup2(pipedescriptor[1], STDOUT_FILENO);
            close(pipedescriptor[1]);
            execvp(lhs[0], lhs);
            exit(0);
          }

          else
          {
            dup2(pipedescriptor[0], STDIN_FILENO);
            close(pipedescriptor[0]);
            execvp(rhs[0], rhs);
            exit(0);
          }
          wait(NULL);
        }

        execvp(input, (char *[]){input, NULL});
        execvp(cargs[0], cargs);
        exit(0);
      }
      
     else
      {
          printf("I am the parent, the child is %d.\n", pid);
          if(hold)
          {
          wait(NULL);
          wait(NULL);
          }
      }
    }
  }
  printf("osh exited\n");
  printf("program finished\n");
  
  return 0;
  }



  int find_pipe(char** cargs)
  {
    int index = 0;
    while(cargs[index] != NULL)
    {
        if(!strncmp(cargs[index], "|", 1))
        {
            return index;
        }
        ++index;
    }
    return -1;
  }

/* osc@ubuntu:~/final-src-osc10e/ch3$ ./shell
osh> ls
input was: 
'ls'
You entered: ls
I am the parent, the child is 13586.
I am the child. 
DateClient.java  fig3-31.c  fig3-33.c  multi-fork       newproc-win32.c  proj2.c  shm-posix-consumer.c  unix_pipe.c
DateServer.java  fig3-32.c  fig3-34.c  multi-fork.c     out.txt          shell    shm-posix-producer.c  win32-pipe-child.c
fig3-30.c        fig3-33    fig3-35.c  newproc-posix.c  pid.c            shell.c  simple-shell.c        win32-pipe-parent.c
osh> exit9)
input was: 
'exit9)'
You entered: exit9)
I am the parent, the child is 13992.
I am the child. 
osh> exit()
input was: 
'exit()'
osh exited
program finished
osc@ubuntu:~/final-src-osc10e/ch3$ ./shell
osh> ls
input was: 
'ls'
You entered: ls
I am the parent, the child is 14010.
I am the child. 
DateClient.java  fig3-31.c  fig3-33.c  multi-fork       newproc-win32.c  proj2.c  shm-posix-consumer.c  unix_pipe.c
DateServer.java  fig3-32.c  fig3-34.c  multi-fork.c     out.txt          shell    shm-posix-producer.c  win32-pipe-child.c
fig3-30.c        fig3-33    fig3-35.c  newproc-posix.c  pid.c            shell.c  simple-shell.c        win32-pipe-parent.c
osh> ls > out.txt
input was: 
'ls > out.txt'
You entered: ls > out.txt
I am the parent, the child is 14164.
I am the child. 
output saved to ./out.txt
osh> ls -l | less
input was: 
'ls -l | less'
You entered: ls -l | less
I am the parent, the child is 14522.
I am the child. 
DateClient.java  fig3-31.c  fig3-33.c  multi-fork       newproc-win32.c  proj2.c  shm-posix-consumer.c  unix_pipe.c
DateServer.java  fig3-32.c  fig3-34.c  multi-fork.c     out.txt          shell    shm-posix-producer.c  win32-pipe-child.c
fig3-30.c        fig3-33    fig3-35.c  newproc-posix.c  pid.c            shell.c  simple-shell.c        win32-pipe-parent.c
osh> !!    
input was: 
'!!'
last command was: ls -l | less
osh> exit()
input was: 
'exit()'
osh exited
program finished
*/
