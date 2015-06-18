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
#include "gui.h" /* AWH: BeagleSNES GUI */

int main (int argc, char **argv)
{
	int guiReturn, i;
	gameInfo_t *currentNode;
	char romFilename[512];
	SDL_Event event;

	printf("Beagle Entertainment System\n");
	if (doGuiSetup())
	{
		fprintf(stderr, "Failure on init, exiting...\n");
		return(0);
	}
        enableGuiAudio(); /* Turn on the audio subsystem */
        doGuiLoadingThread(); /* Start the thread loading in the background */
        doSplashScreen(); /* Splash screen */
        disableGuiAudio();

        /* Set up GPIOs */
        gpioPinSetup();

	while(1)
	{
		printf("Entering GUI...\n");
		enableGuiAudio();
		guiReturn = doGui();
		disableGuiAudio();
		if (guiQuit) break;
		printf("Done with GUI...\n");
		currentNode = gameInfo->next;
		for (i=0; i < guiReturn; i++)
			currentNode = currentNode->next;
		//strcat(romFilename, currentNode->romFile);
		fprintf(stderr, "rom_filename: %s\n", currentNode->romFile);

		/* Clean out any events still in the queue */
		while( SDL_PollEvent(&event) );
		/* Launch the emulator */
		BESPauseState = PAUSE_NONE;
		BESPauseComboCurrent = 0;
		switch(currentNode->platform)
		{
			case PLATFORM_SNES:
				snes_main(currentNode->romFile);
				break;
			case PLATFORM_GBA:
				if (audioAvailable)
					/*doGBAGui();*/
					gbaForceSettings = GBA_FORCE_FS_SYNC_AUDIO;
				else
					gbaForceSettings = GBA_FORCE_NO_FS_NO_AUDIO;
				gba_main(currentNode->romFile);
				break;
			case PLATFORM_GBC:
				gbaForceSettings = GBC_FORCE;
				gba_main(currentNode->romFile);
				break;
			case PLATFORM_NES:
				nes_main(currentNode->romFile);
				break;
			default:
				fprintf(stderr, "Unknown platform\n");
				break;
		}
		/* Clean out any events still in the queue */
		while( SDL_PollEvent(&event) );
		BESPauseComboCurrent = 0;
	}
	return (0);
}

