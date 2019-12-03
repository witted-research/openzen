#include <gtest/gtest.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    const std::string loggerName = "OpenZenTest_console";
    auto console_logger = spdlog::stdout_logger_mt(loggerName);
    spdlog::set_default_logger(console_logger);
    spdlog::set_level(spdlog::level::level_enum::debug);

    return RUN_ALL_TESTS();
}
