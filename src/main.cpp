#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <SDL.h>
#include "gui.h"
#include "besControls.h"
#include "besCartDisplay.h"

char ipAddress[20];

int main (int argc, char **argv)
{
	int guiReturn;
	SDL_Event event;

	printf("Beagle Entertainment System\n");

	memset(ipAddress, 0, sizeof(ipAddress));	
	printf("argc: %d\n", argc);
	if (argc == 2)
		strncpy(ipAddress,argv[1], sizeof(ipAddress)-1);

	if (!strlen(ipAddress))
		strcpy(ipAddress, "[No IP assigned]");

	printf("IP Address: %s\n", ipAddress);
  
	if (doGuiSetup())
	{
		fprintf(stderr, "Failure on init, exiting...\n");
		return(0);
	}
	
        enableGuiAudio(); /* Turn on the audio subsystem */
        doGuiLoadingThread(); /* Start the thread loading in the background */
        doSplashScreen(); /* Splash screen */
        disableGuiAudio();

        /* Set up GPIO and PRU inputs */
        BESControlSetup();
	BESCartOpenDisplay(); /* Enable the cartridge TFT display */

	while(1)
	{
		system("../bes-config-node/run.sh");
		printf("Entering GUI...\n");
		enableGuiAudio();
		BESCartDisplayMenu();
		guiReturn = doGui();
		BESCartDisplayGame(vGameInfo[guiReturn].platform, vGameInfo[guiReturn].gameTitle); /* TFT */
		disableGuiAudio();

		/* Kill web interface */
		system("../bes-config-node/kill.sh");
		if (guiQuit) break;
		printf("Done with GUI...\n");
		fprintf(stderr, "rom_filename: %s\n", vGameInfo[guiReturn].romFile.c_str());

		/* Clean out any events still in the queue */
		while( SDL_PollEvent(&event) );
		/* Launch the emulator */
		BESPauseState = PAUSE_NONE;
		BESPauseComboCurrent = 0;
		
		switch(vGameInfo[guiReturn].platform)
		{
			case PLATFORM_SNES:
				snes_main(vGameInfo[guiReturn].romFile.c_str());
				break;
			case PLATFORM_GBA:
				if (audioAvailable)
					/*doGBAGui();*/
					gbaForceSettings = GBA_FORCE_FS_SYNC_AUDIO;
				else
					gbaForceSettings = GBA_FORCE_NO_FS_NO_AUDIO;
				gba_main(vGameInfo[guiReturn].romFile.c_str());
				break;
			case PLATFORM_GBC:
				gbaForceSettings = GBC_FORCE;
				gba_main(vGameInfo[guiReturn].romFile.c_str());
				break;
			case PLATFORM_NES:
				nes_main(vGameInfo[guiReturn].romFile.c_str());
				break;
			default:
				fprintf(stderr, "Unknown platform\n");
				break;
		}
		/* Clean out any events still in the queue */
		while( SDL_PollEvent(&event) );
		BESPauseComboCurrent = 0;
	}
	EGLShutdown();
	BESControlShutdown(); /* Shutdown GPIO/PRU inputs */
	BESCartCloseDisplay(); /* Shutdown catridge TFT display */

	return (0);
}

