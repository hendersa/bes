#ifndef __BESKEYS_H__
#define __BESKEYS_H__

#include <SDL.h>

/* This is the "master list" of the keys that all BES input methods
  eventually map to. The emulators are configured to use these keys per
  their various configuration files. As events come in from various 
  sources (USB gamepads, GPIOs, PRU gamepads), those events are 
  consumed and the corresponding key events are created and placed into
  SDL's event queue for consumption by the emulators. */

/* Player 1 */
#define BES_P1_DU	SDLK_UP
#define BES_P1_DD	SDLK_DOWN
#define BES_P1_DL	SDLK_LEFT
#define BES_P1_DR	SDLK_RIGHT
#define BES_P1_BL	SDLK_a
#define BES_P1_BR	SDLK_s
#define BES_P1_BA	SDLK_z
#define BES_P1_BB	SDLK_x
#define BES_P1_BX	SDLK_d
#define BES_P1_BY	SDLK_c
#define BES_P1_SE	SDLK_RETURN
#define BES_P1_ST	SDLK_BACKSPACE

/* Player 2 */
#define BES_P2_DU	SDLK_g
#define BES_P2_DD	SDLK_b
#define BES_P2_DL	SDLK_v
#define BES_P2_DR	SDLK_h
#define BES_P2_BL	SDLK_j
#define BES_P2_BR	SDLK_k
#define BES_P2_BA	SDLK_m
#define BES_P2_BB	SDLK_COMMA
#define BES_P2_BX	SDLK_l
#define BES_P2_BY	SDLK_SEMICOLON
#define BES_P2_SE	SDLK_SLASH
#define BES_P2_ST	SDLK_PERIOD

/* Pause */
#define BES_PAUSE	SDLK_n	

/* Debug */
#define BES_DEBUG	SDLK_r
#define BES_QUIT	SDLK_ESCAPE

#endif /* __BESKEYS_H__ */

