#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "gui.h"
#include "beagleboard.h"

#if defined(BEAGLEBONE_BLACK)
#define BIN_NAME "/home/ubuntu/bes/snes9x-sdl"
#define CONF_NAME "/home/ubuntu/bes/snes9x.conf"
#else
#define BIN_NAME "./snes9x-sdl"
#define CONF_NAME "./snes9x.conf"
#endif

extern void reinitVideo(void);
extern void shutdownVideo(void);

/* Args for execve */
char *envp[] = { NULL };
char *argv[] = { "/usr/bin/nice", "-n", "-20", BIN_NAME, "-conf", CONF_NAME, NULL };   

int snes_main(const char *romname)
{
  pid_t child, retVal;
  int status; 
  char buffer[1024];
#if 1 /* AWH */
  /* AWH - nice-ing this to -20 causes buffer underrun driver bug */
  sprintf(buffer, "%s -conf %s %s\n", BIN_NAME, CONF_NAME, romname);

  shutdownVideo();
  status = system(buffer);
  reinitVideo();
  if (status == -1)
    fprintf(stderr, "system() failed!\n"); 
#else 
  child = fork();

  if (child >= 0)
  {
    if (!child) /* Child */
    {
      if( !getcwd(buffer, 1024 - strlen(BIN_NAME)) )
      {
        fprintf(stderr, "getcwd() failed! (errno: %d)\n", errno);
        exit(errno);
      }
      strcat(buffer, BIN_NAME);
      argv[3] = buffer;
      fprintf(stderr, "Launching '%s'\n", buffer);
      status = execve(argv[0], &argv[0], envp);
      fprintf(stderr, "execve() failed! (Return: %d)\n", status);
      exit(status);
    } 
    else /* Parent */
    {
      retVal = child;
      while (retVal == child)
      {
        /* Wait for the child to terminate */
        retVal = waitpid(child, &status, 0);
      } 
      fprintf(stderr, "Main GUI returning from SNES fork()\n");   
    } /* End child process check */
  }
  else
  {
    fprintf(stderr, "fork() failed, returning...\n");
    return -1;
  } /* Check if fork() successful */
#endif /* AWH */  
  return 0;
}

