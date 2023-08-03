#pragma once

#include <ugine/Delegate.h>
#include <ugine/Span.h>
#include <ugine/StringUtils.h>

#include <optional>

#ifdef _DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

#include <spdlog/spdlog.h>

#define UGINE_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define UGINE_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define UGINE_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define UGINE_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define UGINE_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)

namespace ugine {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
};

using LoggerCallback = Delegate<void(LogLevel /* level */, u64 /* millis */, Span<const char> /* message */)>;

void InitLogger(std::optional<LoggerCallback> logger = std::nullopt);

} // namespace ugine
