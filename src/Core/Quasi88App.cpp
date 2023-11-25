/* SPDX-FileCopyrightText: Copyright 2023 Azamat H. Hackimov <azamat.hackimov@gmail.com> */
/* SPDX-License-Identifier: BSD-3-Clause */

#include "Quasi88App.h"
#include "quasi88.h"

namespace QUASI88 {

ExitStatus Quasi88App::run() {
  quasi88();
  return ExitStatus::SUCCESS;
}

} // namespace QUASI88
