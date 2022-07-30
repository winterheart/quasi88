
#include <map>

#include "keymap.h"

std::map<SDL_Keycode, int> keymap_table;
std::map<SDL_Keycode, int> keymap_numlock_table;

void keymap_init() {
  keymap_table = {

      /* Mapping keycodes to key88 codes */
      {SDLK_BACKSPACE, KEY88_INS_DEL},
      {SDLK_TAB, KEY88_TAB},
      {SDLK_HOME, KEY88_HOME},
      {SDLK_RETURN, KEY88_RETURNL},
      {SDLK_PAUSE, KEY88_STOP},
      {SDLK_ESCAPE, KEY88_ESC},
      {SDLK_SPACE, KEY88_SPACE},
      {SDLK_EXCLAIM, KEY88_EXCLAM},
      {SDLK_QUOTEDBL, KEY88_QUOTEDBL},
      {SDLK_HASH, KEY88_NUMBERSIGN},
      {SDLK_DOLLAR, KEY88_DOLLAR},
      {SDLK_PERCENT, KEY88_PERCENT},
      {SDLK_AMPERSAND, KEY88_AMPERSAND},
      {SDLK_QUOTE, KEY88_APOSTROPHE}, // ???
      {SDLK_LEFTPAREN, KEY88_PARENLEFT},
      {SDLK_RIGHTPAREN, KEY88_PARENRIGHT},
      {SDLK_PLUS, KEY88_PLUS},
      {SDLK_COMMA, KEY88_COMMA},
      {SDLK_MINUS, KEY88_MINUS},
      {SDLK_PERIOD, KEY88_PERIOD},
      {SDLK_SLASH, KEY88_SLASH},
      {SDLK_0, KEY88_0},
      {SDLK_1, KEY88_1},
      {SDLK_2, KEY88_2},
      {SDLK_3, KEY88_3},
      {SDLK_4, KEY88_4},
      {SDLK_5, KEY88_5},
      {SDLK_6, KEY88_6},
      {SDLK_7, KEY88_7},
      {SDLK_8, KEY88_8},
      {SDLK_9, KEY88_9},
      {SDLK_COLON, KEY88_COLON},
      {SDLK_SEMICOLON, KEY88_SEMICOLON},
      {SDLK_LESS, KEY88_LESS},
      {SDLK_EQUALS, KEY88_EQUAL},
      {SDLK_GREATER, KEY88_GREATER},
      {SDLK_QUESTION, KEY88_QUESTION},
      {SDLK_AT, KEY88_AT},
      {SDLK_a, KEY88_a},
      {SDLK_b, KEY88_b},
      {SDLK_c, KEY88_c},
      {SDLK_d, KEY88_d},
      {SDLK_e, KEY88_e},
      {SDLK_f, KEY88_f},
      {SDLK_g, KEY88_g},
      {SDLK_h, KEY88_h},
      {SDLK_i, KEY88_i},
      {SDLK_j, KEY88_j},
      {SDLK_k, KEY88_k},
      {SDLK_l, KEY88_l},
      {SDLK_m, KEY88_m},
      {SDLK_n, KEY88_n},
      {SDLK_o, KEY88_o},
      {SDLK_p, KEY88_p},
      {SDLK_q, KEY88_q},
      {SDLK_r, KEY88_r},
      {SDLK_s, KEY88_s},
      {SDLK_t, KEY88_t},
      {SDLK_u, KEY88_u},
      {SDLK_v, KEY88_v},
      {SDLK_w, KEY88_w},
      {SDLK_x, KEY88_x},
      {SDLK_y, KEY88_y},
      {SDLK_z, KEY88_z},
      {SDLK_LEFTBRACKET, KEY88_BRACKETLEFT},
      {SDLK_BACKSLASH, KEY88_YEN}, // Yen code (Shift-JIS) == Backslash code (ASCII)
      {SDLK_RIGHTBRACKET, KEY88_BRACKETRIGHT},
      {SDLK_CARET, KEY88_CARET},
      {SDLK_UNDERSCORE, KEY88_UNDERSCORE},
      {SDLK_BACKQUOTE, KEY88_BACKQUOTE},
      /* no KEY88_BRACELEFT, KEY88_BAR, KEY88_BRACERIGHT, KEY88_TILDE equivalents */
      {SDLK_DELETE, KEY88_DEL},
      {SDLK_KP_0, KEY88_KP_0},
      {SDLK_KP_1, KEY88_KP_1},
      {SDLK_KP_2, KEY88_KP_2},
      {SDLK_KP_3, KEY88_KP_3},
      {SDLK_KP_4, KEY88_KP_4},
      {SDLK_KP_5, KEY88_KP_5},
      {SDLK_KP_6, KEY88_KP_6},
      {SDLK_KP_7, KEY88_KP_7},
      {SDLK_KP_8, KEY88_KP_8},
      {SDLK_KP_9, KEY88_KP_9},
      {SDLK_KP_PERIOD, KEY88_KP_PERIOD},
      {SDLK_KP_DIVIDE, KEY88_KP_DIVIDE},
      {SDLK_KP_MULTIPLY, KEY88_KP_MULTIPLY},
      {SDLK_KP_MINUS, KEY88_KP_SUB},
      {SDLK_KP_PLUS, KEY88_KP_ADD},
      {SDLK_KP_ENTER, KEY88_RETURNR},
      {SDLK_KP_EQUALS, KEY88_KP_EQUAL},
      {SDLK_UP, KEY88_UP},
      {SDLK_DOWN, KEY88_DOWN},
      {SDLK_RIGHT, KEY88_RIGHT},
      {SDLK_LEFT, KEY88_LEFT},
      {SDLK_INSERT, KEY88_INS},
      {SDLK_HOME, KEY88_HOME},
      {SDLK_END, KEY88_HELP},
      {SDLK_PAGEUP, KEY88_ROLLDOWN},
      {SDLK_PAGEDOWN, KEY88_ROLLUP},
      {SDLK_F1, KEY88_F1},
      {SDLK_F2, KEY88_F2},
      {SDLK_F3, KEY88_F3},
      {SDLK_F4, KEY88_F4},
      {SDLK_F5, KEY88_F5},
      {SDLK_F6, KEY88_F6},
      {SDLK_F7, KEY88_F7},
      {SDLK_F8, KEY88_F8},
      {SDLK_F9, KEY88_F9},
      {SDLK_F10, KEY88_F10},
      {SDLK_F11, KEY88_F11},
      {SDLK_F12, KEY88_F12},
      {SDLK_F13, KEY88_F13},
      {SDLK_F14, KEY88_F14},
      {SDLK_F15, KEY88_F15},
      {SDLK_CAPSLOCK, KEY88_CAPS},
      {SDLK_SCROLLLOCK, KEY88_KANA},
      {SDLK_RSHIFT, KEY88_SHIFTR},
      {SDLK_LSHIFT, KEY88_SHIFTL},
      {SDLK_RCTRL, KEY88_CTRL},
      {SDLK_LCTRL, KEY88_CTRL},
      {SDLK_RALT, KEY88_GRAPH},
      {SDLK_LALT, KEY88_GRAPH},
      {SDLK_RGUI, KEY88_GRAPH},
      {SDLK_LGUI, KEY88_GRAPH},
      {SDLK_HELP, KEY88_HELP},
      {SDLK_PRINTSCREEN, KEY88_COPY},
      {SDLK_STOP, KEY88_STOP}

  };
}

/**
 * Update keymap_table with keymap_numlock_table entries
 */
void keymap_merge() {
  for (auto &it : keymap_numlock_table) {
    keymap_table[it.first] = it.second;
  }
}

void keymap_add(SDL_Keycode keycode, int key88) { keymap_table.insert_or_assign(keycode, key88); }

int keymap2key88(SDL_Keycode keycode) {
  if (keymap_table.find(keycode) != keymap_table.end()) {
    return keymap_table[keycode];
  }

  return 0;
}

void keymap_numlock_init() {

  keymap_numlock_table = {

      /* Mapping keycodes to key88 codes */
      {SDLK_5, KEY88_HOME},
      {SDLK_6, KEY88_HELP},
      {SDLK_7, KEY88_KP_7},
      {SDLK_8, KEY88_KP_8},
      {SDLK_9, KEY88_KP_9},
      {SDLK_0, KEY88_KP_MULTIPLY},
      {SDLK_MINUS, KEY88_KP_SUB},
      {SDLK_CARET, KEY88_KP_DIVIDE},
      {SDLK_u, KEY88_KP_4},
      {SDLK_i, KEY88_KP_5},
      {SDLK_o, KEY88_KP_6},
      {SDLK_p, KEY88_KP_ADD},
      {SDLK_j, KEY88_KP_1},
      {SDLK_k, KEY88_KP_2},
      {SDLK_l, KEY88_KP_3},
      {SDLK_SEMICOLON, KEY88_KP_EQUAL},
      {SDLK_m, KEY88_KP_0},
      {SDLK_COMMA, KEY88_KP_COMMA},
      {SDLK_PERIOD, KEY88_KP_PERIOD},
      {SDLK_SLASH, KEY88_RETURNR}

  };
}

void keymap_numlock_add(SDL_Keycode keycode, int key88) { keymap_numlock_table.insert_or_assign(keycode, key88); }
