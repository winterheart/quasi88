/* SPDX-FileCopyrightText: Copyright 2023 Azamat H. Hackimov <azamat.hackimov@gmail.com> */
/* SPDX-License-Identifier: BSD-3-Clause */
#pragma once

#include <cstdint>

namespace QUASI88 {

enum class ExitStatus : int32_t {
  SUCCESS = 0,
  FAILURE = 1,
};

/// Abstract class for implementing various back-ends.
class Quasi88App {
public:
  Quasi88App() = default;
  // Quasi88App(std::shared<Quasi88Config> config);
  static ExitStatus run();
private:
  // Quasi88Config m_config;
};

} // namespace QUASI88
