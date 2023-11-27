/* SPDX-FileCopyrightText: Copyright 2023 Azamat H. Hackimov <azamat.hackimov@gmail.com> */
/* SPDX-License-Identifier: BSD-3-Clause */
#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace QUASI88 {

inline std::shared_ptr<spdlog::logger> make_log(const std::string& name) {
  std::vector<spdlog::sink_ptr> log_sinks {
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("quasi88.log", true),
  };
  const spdlog::level::level_enum level{spdlog::level::info};
  auto log = spdlog::get(name);
  if (log == nullptr) {
    auto new_log = std::make_shared<spdlog::logger>(name, begin(log_sinks), end(log_sinks));
    spdlog::register_logger(new_log);
    new_log->set_level(level);
    new_log->flush_on(level);
    return new_log;
  }
  return log;
}

} // namespace QUASI88

#define QLOG_TRACE(name, ...) spdlog::get(name)->trace(__VA_ARGS__)
#define QLOG_DEBUG(name, ...) spdlog::get(name)->debug(__VA_ARGS__)
#define QLOG_INFO(name, ...) spdlog::get(name)->info(__VA_ARGS__)
#define QLOG_WARN(name, ...) spdlog::get(name)->warn(__VA_ARGS__)
#define QLOG_ERROR(name, ...) spdlog::get(name)->error(__VA_ARGS__)
#define QLOG_CRITICAL(name, ...) spdlog::get(name)->critical(__VA_ARGS__)

