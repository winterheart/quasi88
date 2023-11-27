/* SPDX-FileCopyrightText: Copyright 2023 Azamat H. Hackimov <azamat.hackimov@gmail.com> */
/* SPDX-License-Identifier: BSD-3-Clause */

#include <SDL2/SDL.h>
#include <fmt/format.h>

#include "Core/Exception.h"
#include "Quasi88SDLApp.h"

namespace QUASI88 {

Quasi88SDLApp::Quasi88SDLApp() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    throw Exception(fmt::format("SDL Error: {}", SDL_GetError()));
  }
}

Quasi88SDLApp::~Quasi88SDLApp() {
  SDL_Quit();
}

} // namespace QUASI88
