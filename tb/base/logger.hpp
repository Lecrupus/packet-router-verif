// logger.hpp -- INFO / WARN / ERROR / FATAL logging with +VERBOSITY (R-TB-09).
// Any ERROR fails the test; FATAL throws and ends the run.
#pragma once
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>

enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4 };

struct FatalError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Logger {
public:
    static Logger& get() {
        static Logger inst;
        return inst;
    }

    void set_verbosity(Level l) { verbosity_ = l; }
    static Level parse_level(const std::string& s) {
        if (s == "DEBUG" || s == "HIGH") return Level::DEBUG;
        if (s == "WARN" || s == "LOW") return Level::WARN;
        if (s == "ERROR" || s == "NONE") return Level::ERROR;
        return Level::INFO;
    }
    void set_cycle_source(const uint64_t* c) { cycle_ = c; }
    void set_max_printed_errors(int n) { max_printed_errors_ = n; }

    void log(Level lvl, const std::string& who, const std::string& msg) {
        if (lvl == Level::WARN) ++warnings_;
        if (lvl >= Level::ERROR) {
            ++errors_;
            if (first_error_.empty()) first_error_ = who + ": " + msg;
        }
        bool print = lvl >= verbosity_ || lvl >= Level::ERROR;
        if (lvl == Level::ERROR && errors_ > max_printed_errors_) {
            if (errors_ == max_printed_errors_ + 1)
                std::printf("[%10llu] WARN  logger: further ERROR lines suppressed\n",
                            static_cast<unsigned long long>(cycle_ ? *cycle_ : 0));
            print = false;
        }
        if (print) {
            std::printf("[%10llu] %-5s %s: %s\n", static_cast<unsigned long long>(cycle_ ? *cycle_ : 0),
                        name(lvl), who.c_str(), msg.c_str());
            std::fflush(stdout);
        }
        if (lvl == Level::FATAL) throw FatalError(who + ": " + msg);
    }

    int errors() const { return errors_; }
    int warnings() const { return warnings_; }
    const std::string& first_error() const { return first_error_; }

    static const char* name(Level l) {
        switch (l) {
            case Level::DEBUG: return "DEBUG";
            case Level::INFO: return "INFO";
            case Level::WARN: return "WARN";
            case Level::ERROR: return "ERROR";
            default: return "FATAL";
        }
    }

private:
    Logger() = default;
    Level verbosity_ = Level::INFO;
    const uint64_t* cycle_ = nullptr;
    int errors_ = 0, warnings_ = 0;
    int max_printed_errors_ = 50;
    std::string first_error_;
};

// Concatenate anything streamable: cat("port ", 3, " len=", 7)
template <class... Args>
std::string cat(Args&&... args) {
    std::ostringstream os;
    (os << ... << args);
    return os.str();
}

#define LOG_DEBUG(who, ...) Logger::get().log(Level::DEBUG, who, cat(__VA_ARGS__))
#define LOG_INFO(who, ...) Logger::get().log(Level::INFO, who, cat(__VA_ARGS__))
#define LOG_WARN(who, ...) Logger::get().log(Level::WARN, who, cat(__VA_ARGS__))
#define LOG_ERROR(who, ...) Logger::get().log(Level::ERROR, who, cat(__VA_ARGS__))
#define LOG_FATAL(who, ...) Logger::get().log(Level::FATAL, who, cat(__VA_ARGS__))
