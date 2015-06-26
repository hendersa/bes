/*****************************************************************************
  BeagleSNES - Super Nintendo Entertainment System (TM) emulator for the
  BeagleBoard-xM platform.

  See CREDITS file to find the copyright owners of this file.

  GUI front-end code (c) Copyright 2013 Andrew Henderson (hendersa@icculus.org)

  BeagleSNES homepage: http://www.beaglesnes.org/
  
  BeagleSNES is derived from the Snes9x open source emulator:  
  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute BeagleSNES in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  BeagleSNES is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for BeagleSNES or software derived 
  from BeagleSNES, including BeagleSNES or derivatives in commercial game 
  bundles, and/or using BeagleSNES as a promotion for your commercial 
  product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <SDL.h>
#include "gui.h"

#if defined(BEAGLEBONE_BLACK)
#define GPIO_DATA_IN_REG	0x138
#define GPIO_SETDATAOUT		0x194
#define GPIO_CLEARDATAOUT	0x190
#define NUM_GPIO_BANKS 4
static uint32_t *gpios[NUM_GPIO_BANKS];
static uint32_t *controlModule;
static uint8_t currentGPIOState[GPIO_MAP_SIZE];
static uint8_t lastGPIOState[GPIO_MAP_SIZE];
static uint8_t changeGPIOState[GPIO_MAP_SIZE];
static void gpioUpdate(void);
static int gpioMmap(void);
#endif /* BEAGLEBONE_BLACK */
static uint32_t gpioEnabled;

uint32_t gpioPinSetup(void)
{
#if !defined(BEAGLEBONE_BLACK)
	gpioEnabled = 0;
	return(gpioEnabled);
#else
	int fd, fdexport;
	char buf[128];
	int i;

	memset(currentGPIOState, 0, sizeof(uint8_t) * GPIO_MAP_SIZE);
	memset(lastGPIOState, 0, sizeof(uint8_t) * GPIO_MAP_SIZE);
	memset(changeGPIOState, 0, sizeof(uint8_t) * GPIO_MAP_SIZE);
	gpioEnabled = 1;

	fdexport = open("/sys/class/gpio/export", O_WRONLY);
	if (fdexport < 0)
	{
		fprintf(stderr, "Unable to set up GPIOs, skipping...\n");
		gpioEnabled = 0;
		return 1;
	}
	close(fdexport);

	i = chmod("/sys/class/gpio/export", 0666);
	if (i < 0)
	{
		fprintf(stderr, "Unable to chmod GPIO export, skipping...\n");
		gpioEnabled = 0;
		return 1;
	}

	/* Export and init all of the GPIO pins */
	for (i = 0; i < GPIO_MAP_SIZE; i++)
	{
		if (GPIOButtonMap[i] < 0xFF)
		{
#if 1
			sprintf(buf, "%d", GPIOButtonMap[i]);
			fdexport = open("/sys/class/gpio/export", O_WRONLY);
			if (fdexport < 0)
			{
				fprintf(stderr, "Could not open /sys/class/gpio/export\n");
				gpioEnabled = 0;				
			}

			if (write(fdexport, buf, strlen(buf)) < 0)
			{
				fprintf(stderr, "Could not export pin %d\n", GPIOButtonMap[i]);
			}
			//else
			//	fprintf(stderr, "Exported pin %d\n", GPIOButtonMap[i]);
			close(fdexport);

			/* Set up this pin as an input */
			sprintf(buf, "/sys/class/gpio/gpio%d/direction", GPIOButtonMap[i]);
			fd = open(buf, O_WRONLY);
			write(fd, "in", 2);
			close(fd);
#else
			sprintf(buf, "echo %d > /sys/class/gpio/export > /dev/null", GPIOButtonMap[i]);
			system(buf);
			sprintf(buf, "echo in > /sys/class/gpio/gpio%d/direction > /dev/null", GPIOButtonMap[i]);
			system(buf);
#endif
		} /* End if */
	} /* End for */

	if (gpioEnabled)
		gpioMmap();

	if (gpioEnabled)
	{
		gpioUpdate();
		gpioUpdate();
	}

	return(gpioEnabled);
#endif /* BEAGLEBONE_BLACK */
}
#if defined(BEAGLEBONE_BLACK)
int gpioMmap(void)
{
	static const uint32_t gpioAddrs[NUM_GPIO_BANKS] =
		{0x44E07000, 0x4804C000, 0x481AC000, 0x481AE000 };
	static const uint32_t controlAddr = 0x44E10000;
	int i, memfd;

	if (!gpioEnabled)
	{
		fprintf(stderr, "GPIOs disabled, skipping mmap()\n");
		return(0);
	}

	/* Access /dev/mem */
	memfd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC);
	if (memfd < 0)
	{
		fprintf(stderr, "Unable to mmap() GPIOs\n");
		gpioEnabled = 0;
		return(0);
	}

	/* mmap() the controlModule */
	controlModule = (uint32_t *) mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, memfd, controlAddr);
	fprintf(stderr, "Control module at address 0x%08x mapped\n", controlAddr);

	/* mmap() the four GPIO banks */
	for (i = 0; i < 4; i++)
	{
		gpios[i] = (uint32_t *) mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, memfd, gpioAddrs[i]);
		fprintf(stderr, "gpio[%i] at address 0x%08x mapped\n",i,gpioAddrs[i]);
	}	
	close(memfd);

	return(1);	
}

void gpioUpdate(void)
{
	int i;
	uint32_t reg, mask;
	uint8_t bank;

	if (!gpioEnabled) return;

	/* Make our "current" GPIO states the "last" states */
	memcpy(lastGPIOState, currentGPIOState, sizeof(uint8_t) * GPIO_MAP_SIZE);

	/* Sweep the GPIOs and see what has changed */
	for (i = 0; i < GPIO_MAP_SIZE; i++)
	{
		if (GPIOButtonMap[i] != 0xFF)
		{
			bank = GPIOButtonMap[i] / 32;
			mask = 1 << (GPIOButtonMap[i] % 32);
			reg = gpios[bank][GPIO_DATA_IN_REG / 4];
			currentGPIOState[i] = ((reg & mask) >> (GPIOButtonMap[i] % 32));
			if (currentGPIOState[i] == lastGPIOState[i])
				changeGPIOState[i] = 0;
			else {
				changeGPIOState[i] = 1;
				//fprintf(stderr, "gpio[%d] changed state\n", i);
			}
		} else
			changeGPIOState[i] = 0;
	}
}

void gpioEvents(void)
{
	SDL_Event event;
	int i;

	gpioUpdate();

	event.key.keysym.mod = KMOD_NONE;
	/* Generate keyboard events if needed */
	for (i=0; i < GPIO_MAP_SIZE; i++)
	{
		if (changeGPIOState[i])
		{
			if (currentGPIOState[i])
				event.type = SDL_KEYDOWN;
			else
				event.type = SDL_KEYUP;

			switch (i) 
			{
				case 0:
					event.key.keysym.sym = SDLK_LEFT; break;
				case 1:
					event.key.keysym.sym = SDLK_RIGHT; break;
				case 2:
					event.key.keysym.sym = SDLK_UP; break;
				case 3:
					event.key.keysym.sym = SDLK_DOWN; break;
				case 4:  /* L button */
					event.key.keysym.sym = SDLK_a; break;
				case 5:  /* R button */
					event.key.keysym.sym = SDLK_s; break;
				case 6:  /* A button */
					event.key.keysym.sym = SDLK_z; break;
				case 7:  /* B button */
					event.key.keysym.sym = SDLK_b; break;
				case 8:  /* X button */
					event.key.keysym.sym = SDLK_d; break;
				case 9:  /* Y button */
					event.key.keysym.sym = SDLK_c; break;
				case 10: /* select button */
					event.key.keysym.sym = SDLK_BACKSPACE; break;
				case 11: /* start button */
					event.key.keysym.sym = SDLK_RETURN; break;
				case 12: /* pause button */
					event.key.keysym.sym = SDLK_n; break;
				default:
					break;
			} /* end switch */
			
			/* Filter out pause menu button up event */
			if (!((i == 12) && (event.type == SDL_KEYUP)))
				/* Insert event into event queue */
				SDL_PushEvent(&event);

		}	/* end changeState */	
	} /* end for loop */
}
#else
void gpioEvents(void) {}
#endif /* BEAGLEBONE_BLACK */

