/* SPDX-FileCopyrightText: Copyright 2023 Azamat H. Hackimov <azamat.hackimov@gmail.com> */
/* SPDX-License-Identifier: BSD-3-Clause */
#pragma once

#include <stdexcept>

namespace QUASI88 {

/// Basic Exception class for Quasi88 related exceptions
class Exception : public std::runtime_error {
public:
  explicit Exception(const std::string &msg) : std::runtime_error(msg) {};
};

} // namespace QUASI88
