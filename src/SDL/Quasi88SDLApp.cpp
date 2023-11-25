/* SPDX-FileCopyrightText: Copyright 2023 Azamat H. Hackimov <azamat.hackimov@gmail.com> */
/* SPDX-License-Identifier: BSD-3-Clause */

#include <sstream>
#include <SDL2/SDL.h>

#include "Core/Exception.h"
#include "Quasi88SDLApp.h"

namespace QUASI88 {

Quasi88SDLApp::Quasi88SDLApp() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
//    if (verbose_proc)
//      printf("Failed\n");
//    fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
    std::ostringstream ss;
    ss << "SDL Error: " << SDL_GetError();
    throw Exception(ss.str());
//  } else {
//    if (verbose_proc)
//      printf("OK\n");
  }
}

Quasi88SDLApp::~Quasi88SDLApp() {
  SDL_Quit();
}

} // namespace QUASI88
