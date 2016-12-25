#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "gui.h"
#include "beagleboard.h"
#include "besControls.h"

#define BUF_SIZE 1024

#if defined(BEAGLEBONE_BLACK)
#define BIN_NAME "/home/ubuntu/bes/snes9x-sdl"
#define CONF_NAME "/home/ubuntu/bes/snes9x.conf"
#else
#define BIN_NAME "./snes9x-sdl"
#define CONF_NAME "./snes9x.conf"
#endif

extern void reinitVideo(void);
extern void shutdownVideo(void);

int snes_main(const char *romname)
{
  //pid_t child, retVal;
  int status; 
  char buffer[BUF_SIZE];

  /* nice-ing this to -20 causes audio buffer underrun driver bug... */
  snprintf(buffer, BUF_SIZE - 1, "%s %s -conf %s \"%s\"\n", BIN_NAME, (audioAvailable ? "" : "-nosound"), CONF_NAME, romname);
  fprintf(stderr, "launch: '%s'\n", buffer);
  BESControlShutdown();
  shutdownVideo();
  status = system(buffer);
  reinitVideo();
  BESControlSetup();
  if (status == -1)
    fprintf(stderr, "system() failed!\n"); 
  return 0;
}

