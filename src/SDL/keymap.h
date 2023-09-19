#ifndef KEY_LOOKUP_H_INCLUDED
#define KEY_LOOKUP_H_INCLUDED

#include <SDL2/SDL.h>

#include "keyboard.h"

#ifdef __cplusplus
extern "C" {
#endif

void keymap_init();

void keymap_merge();

void keymap_add(SDL_Keycode keycode, int key88);

int keymap2key88(SDL_Keycode keycode);

void keymap_numlock_init();

void keymap_numlock_add(SDL_Keycode keycode, int key88);

#ifdef __cplusplus
}
#endif

#endif // KEY_LOOKUP_H_INCLUDED
