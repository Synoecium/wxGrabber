#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <mutex>

class Logger {
public:
    enum class Level {
        Error,      // Critical errors that prevent program execution
        Warning,    // Non-critical issues that might affect program behavior
        Info,       // General information about program execution
        Verbose,    // Detailed information for debugging
        Debug       // Very detailed information for development
    };

    static Logger& getInstance();
    bool init(const std::string& filename = "log.txt");
    void setLevel(Level level);

    template<typename T>
    void log(Level level, const T& message) {
        if (level <= currentLevel) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            std::tm timeinfo;
#ifdef _WIN32
            localtime_s(&timeinfo, &time);
#else
            localtime_r(&time, &timeinfo);
#endif

            std::stringstream ss;
            ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S")
               << '.' << std::setfill('0') << std::setw(3) << ms.count()
               << " [" << getLevelString(level) << "] "
               << message << std::endl;

            std::lock_guard<std::mutex> lock(logMutex);
            if (logFile.is_open()) {
                logFile << ss.str();
                logFile.flush();
            }
        }
    }

    // Custom stream buffer for redirecting cout/cerr
    class LogBuf : public std::streambuf {
    public:
        LogBuf(Logger& logger, Level level);
    protected:
        virtual int overflow(int c);
        virtual int sync();
    private:
        Logger& logger_;
        Level level_;
        std::string buffer_;
    };

    // Custom streams for cout/cerr replacement
    class LogStream : public std::ostream {
    public:
        LogStream(Logger& logger, Level level);
    private:
        LogBuf buffer_;
    };

    LogStream& getErrorStream();
    LogStream& getInfoStream();
    ~Logger();

private:
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    const char* getLevelString(Level level) const;

    std::ofstream logFile;
    Level currentLevel;
    LogStream errorStream_;
    LogStream infoStream_;
    mutable std::mutex logMutex;  // Mutex for thread-safe logging
};

// Global functions for easy access
bool initLog(const std::string& filename = "log.txt");
void setLogLevel(Logger::Level level);

template<typename T>
void logError(const T& message) {
    Logger::getInstance().log(Logger::Level::Error, message);
}

template<typename T>
void logWarning(const T& message) {
    Logger::getInstance().log(Logger::Level::Warning, message);
}

template<typename T>
void logInfo(const T& message) {
    Logger::getInstance().log(Logger::Level::Info, message);
}

template<typename T>
void logVerbose(const T& message) {
    Logger::getInstance().log(Logger::Level::Verbose, message);
}

template<typename T>
void logDebug(const T& message) {
    Logger::getInstance().log(Logger::Level::Debug, message);
}

void redirectStdStreams();

#endif // LOG_H 