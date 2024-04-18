//
// test program for the shell lab.
// $ testsh simple_shell
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

unsigned int seed = 123456789;

// return a random integer.
// from Wikipedia, linear congruential generator, glibc's constants.
unsigned int new_rand()
{
  unsigned int a = 1103515245;
  unsigned int c = 12345;
  unsigned int m = (1 << 31);
  seed = (a * seed + c) % m;
  return seed;
}

// generate a random string of the indicated length.
char *
randstring(char *buf, int n)
{
  for(int i = 0; i < n-1; i++)
    buf[i] = "abcdefghijklmnopqrstuvwxyz"[new_rand() % 26];
  buf[n-1] = '\0';
  return buf;
}

// create a file with the indicated content.
void
writefile(char *name, char *data)
{
  unlink(name); // since no truncation
  int fd = open(name, O_CREAT|O_WRONLY,0755);
  if(fd < 0){
    printf("testsh: could not write %s\n", name);
    exit(-1);
  }
  if(write(fd, data, strlen(data)) != strlen(data)){
    printf("testsh: write failed\n");
    exit(-1);
  }
  write(fd, "exit\n", strlen("exit\n"));
  close(fd);
}


void
truncatefile(char *name, char *data)
{
  int fd = open(name, O_CREAT|O_WRONLY,0755);
  if(fd < 0){
    printf("testsh: could not write %s\n", name);
    exit(-1);
  }
  if(write(fd, data, strlen(data)) != strlen(data)){
    printf("testsh: write failed\n");
    exit(-1);
  }
  close(fd);
}
// return the content of a file.
void
readfile(char *name, char *data, int max)
{
  data[0] = '\0';
  int fd = open(name, O_RDONLY,0755);
  if(fd < 0){
    printf("testsh: open %s failed\n", name);
    return;
  }
  int n = read(fd, data, max-1);
  close(fd);
  if(n < 0){
    printf("testsh: read %s failed\n", name);
    return;
  }
  data[n] = '\0';
}

// look for the small string in the big string;
// return the address in the big string, or 0.
char * new_strstr(char *big, char *small)
{
  if(small[0] == '\0')
    return big;
  for(int i = 0; big[i]; i++){
    int j;
    for(j = 0; small[j]; j++){
      if(big[i+j] != small[j]){
        break;
      }
    }
    if(small[j] == '\0'){
      return big + i;
    }
  }
  return 0;
}
// convert integer to string
char *new_itoa(int num,char* str){
  if(str == NULL){
    return NULL;
  }
  sprintf(str,"%d",num);
  return str;
}

// argv[1] -- the shell to be tested.
char *shname;

// fire up the shell to be tested, send it cmd on
// its input, collect the output, check that the
// output includes the expect argument.
// if tight = 1, don't allow much extraneous output.
int
one(char *cmd, char *expect, int tight , int cmd_num)
{
  char infile[12], outfile[12];

  randstring(infile, sizeof(infile));
  randstring(outfile, sizeof(outfile));

  writefile(infile, cmd);
  unlink(outfile);

  int pid = fork();
  if(pid < 0){
    printf("testsh: fork() failed\n");
    exit(-1);
  }

  if(pid == 0){
    close(0);
    if(open(infile, O_RDONLY,0755) != 0){
      printf("testsh: child open != 0\n");
      exit(-1);
    }
    close(1);
    if(open(outfile, O_CREAT|O_WRONLY,0755) != 1){
      printf("testsh: child open != 1\n");
      exit(-1);
    }
    char *argv[2];
    argv[0] = shname;
    argv[1] = 0;
    execv(shname, argv);
    printf("testsh: exec %s failed\n", shname);
    exit(-1);
  }

  if(waitpid(pid,NULL,0) != pid){
    printf("testsh: unexpected wait() return\n");
    exit(-1);
  }
  unlink(infile);

  char out[256];
  readfile(outfile, out, sizeof(out));
  unlink(outfile);
  char wd[4096];
  getcwd(wd,4096);

  if(new_strstr(out, expect) != 0){
    if(tight && strlen(out) > strlen(expect) + (cmd_num + 1)*(11 + strlen(wd)) + 20){
      printf("testsh: saw expected output, but too much else as well\n");
      return 0; // fail
    }
    return 1; // pass
  }
  return 0; // fail
}

// test kill
void t0(int *ok){
  // printf("I am %d, printing: ", getpid());
  printf("kill: ");
  int pid = fork();
  if(pid == 0){
    sleep(10);
    // printf("I am %d, going to exit\n", getpid());
    exit(EXIT_SUCCESS);
  }
  else{
    char str_pid[10];
    char cmd[64];
    strcpy(cmd, "kill ");
    strcpy(cmd + strlen(cmd), new_itoa(pid,str_pid));
    strcpy(cmd + strlen(cmd), " \n");
    if(one(cmd, "", 1 , 1) == 0){
      printf("FAIL\n");
      *ok = 0;
    } else {
      int status = -1;
      sleep(1);
      pid_t result = waitpid(pid,&status,0);
      if(result == -1){
        printf("waitpid() fail\n");
      }else if(result != pid){
        printf("Unexpected wait() return\n");
        printf("FAIL\n");
        *ok = 0;
        return;
      }
      // int sig = WTERMSIG(status);
      // printf("status = %d", status);
      if(status == SIGTERM){
        printf("PASS\n");
      }
      else{
        printf("FAIL\n");
        *ok = 0;
      }
    }
  }
}
//test if cd can work successfully
void
t1(int *ok) 
{
  printf("cd: ");
  
  if(one("cd /sys/class/net\n", "/sys/class/net", 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    printf("PASS\n");
  }
}
// test if shell can print current path
void
t2(int *ok)
{
  fprintf(stdout,"current path: ");
  char wd[4096];
  getcwd(wd,4096);
  if(one("echo\n", wd, 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    printf("PASS\n");
  }
}

// test a command with arguments.
void
t3(int *ok)
{
  fprintf(stdout,"simple echo: ");
  if(one("echo hello goodbye\n", "hello goodbye", 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    printf("PASS\n");
  }
}

// test a command with arguments.
void
t4(int *ok)
{
  printf("simple grep: ");
  if(one("grep professor testdata\n", "Yongkun Li is a professor in School of Computer Science and Technology", 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    printf("PASS\n");
  }
}

// test a command, then a newline, then another command.
void
t5(int *ok)
{
  printf("two commands: ");
  if(one("echo x\necho goodbye\n", "goodbye", 1 , 2) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    printf("PASS\n");
  }
}

// test a pipe with cat filename | cat.
void
t6(int *ok)
{
  printf("simple pipe: ");

  char name[32], data[32];
  randstring(name, 12);
  randstring(data, 12);
  truncatefile(name, data);

  char cmd[64];
  strcpy(cmd, "cat ");
  strcpy(cmd + strlen(cmd), name);
  strcpy(cmd + strlen(cmd), " | cat\n");
  
  if(one(cmd, data, 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    printf("PASS\n");
  }

  unlink(name);
}

// test output redirection: echo xxx > file
void
t7(int *ok)
{
  printf("output redirection(use >): ");

  char file[16];
  randstring(file, 12);

  char data[16];
  randstring(data, 12);

  char cmd[64];
  strcpy(cmd, "echo ");
  strcpy(cmd+strlen(cmd), data);
  strcpy(cmd+strlen(cmd), " > ");
  strcpy(cmd+strlen(cmd), file);
  strcpy(cmd+strlen(cmd), "\n");

  if(one(cmd, "", 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    char buf[64];
    readfile(file, buf, sizeof(buf));
    if(new_strstr(buf, data) == 0){
      printf("FAIL\n");
      *ok = 0;
    } else {
      printf("PASS\n");
    }
  }

  unlink(file);
}


// test output redirection: echo xxx >> file
void
t8(int *ok)
{
  printf("output redirection(use >>): ");

  char file[16];
  randstring(file, 12);

  char data[16];
  randstring(data, 16);
  char append_data[16];
  randstring(append_data, 16);
  char cmd[128];
  strcpy(cmd, "echo ");
  strcpy(cmd+strlen(cmd), data);
  strcpy(cmd+strlen(cmd), " > ");
  strcpy(cmd+strlen(cmd), file);
  strcpy(cmd+strlen(cmd), " ; ");
  strcpy(cmd+strlen(cmd), "echo ");
  strcpy(cmd+strlen(cmd), append_data);
  strcpy(cmd+strlen(cmd), " >> ");
  strcpy(cmd+strlen(cmd), file);
  strcpy(cmd+strlen(cmd), "\n");

  if(one(cmd, "", 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    char buf[64];
    readfile(file, buf, sizeof(buf));
    if(new_strstr(buf, data) == 0){
      printf("FAIL\n");
      *ok = 0;
    } else {
      printf("PASS\n");
    }
  }

  unlink(file);
}


// test input redirection: cat < file
void
t9(int *ok)
{
  printf("input redirection: ");

  char file[32];
  randstring(file, 12);

  char data[32];
  randstring(data, 12);
  writefile(file, data);

  char cmd[32];
  strcpy(cmd, "cat < ");
  strcpy(cmd+strlen(cmd), file);
  strcpy(cmd+strlen(cmd), "\n");

  if(one(cmd, data, 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    printf("PASS\n");
  }

  unlink(file);
}

// test a command with both input and output redirection.
void
t10(int *ok)
{
  printf("both redirections: ");
  unlink("testsh.out");
  if(one("grep USTC < testdata > testsh.out\n", "", 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    char buf[128];
    readfile("testsh.out", buf, sizeof(buf));
    if(new_strstr(buf, "Office:	Rm 601, High Performance Computing Center USTC (East Campus)") == 0){
      printf("FAIL\n");
      *ok = 0;
    } else {
      printf("PASS\n");
    }
  }
  unlink("testsh.out");
}

// test a pipeline that has both redirection and a pipe.
void
t11(int *ok)
{
  printf("pipe and redirects: ");
  
  if(one("grep system < testdata | wc > testsh.out\n", "", 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    char buf[64];
    readfile("testsh.out", buf, sizeof(buf));
    if(new_strstr(buf, "      4      49     379") == 0){
      printf("FAIL\n");
      *ok = 0;
    } else {
      printf("PASS\n");
    }
  }

  unlink("testsh.out");
}

// use more than one pipe
void
t12(int *ok)
{
  printf("multipipe: ");
  
  if(one("cat testdata | grep system | wc -l\n", "4", 1 , 1) == 0){
    printf("FAIL\n");
    *ok = 0;
  } else {
    printf("PASS\n");
  }
}
int
main(int argc, char *argv[])
{
  if(argc != 2){
    printf("Usage: testsh nsh\n");
    exit(-1);
  }
  shname = argv[1];
  
  seed += getpid();

  int ok = 1;
  t0(&ok);
  t1(&ok);
  t2(&ok);
  t3(&ok);
  t4(&ok);
  t5(&ok);
  t6(&ok);
  t7(&ok);
  t8(&ok);
  t9(&ok);
  t10(&ok);
  t11(&ok);
  t12(&ok);
  if(ok){
    printf("passed all tests\n");
  } else {
    printf("failed some tests\n");
  }
  
  exit(0);
}