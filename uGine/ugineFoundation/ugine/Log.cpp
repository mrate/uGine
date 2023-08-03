#include "Log.h"

#include <ugine/String.h>

#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace ugine {

void InitLogger(std::optional<LoggerCallback> delegate) {
    static std::chrono::system_clock::time_point start{ std::chrono::system_clock::now() };

    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };

    if (delegate) {
        auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>([delegate = delegate.value()](const spdlog::details::log_msg& msg) {
            const auto level{ [&] {
                switch (msg.level) {
                case spdlog::level::trace: return LogLevel::Trace;
                case spdlog::level::debug: return LogLevel::Debug;
                case spdlog::level::info: return LogLevel::Info;
                case spdlog::level::warn: return LogLevel::Warn;
                default: return LogLevel::Error;
                }
            }() };

            auto millis{ u64(std::chrono::duration_cast<std::chrono::milliseconds>(msg.time - start).count()) };

            delegate(level, millis, Span<const char>{ msg.payload.data(), msg.payload.size() });
        });
        sinks.push_back(callback_sink);
    }

    auto logger{ std::make_shared<spdlog::logger>("ugine", sinks.begin(), sinks.end()) };

#ifdef _DEBUG
    logger->set_level(spdlog::level::debug);
#endif

    spdlog::set_default_logger(logger);
}

} // namespace ugine
