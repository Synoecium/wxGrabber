#include "../include/log.h"
#include <mutex>

// Logger implementation
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

bool Logger::init(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);
    logFile.open(filename, std::ios::out | std::ios::trunc);
    return logFile.is_open();
}

void Logger::setLevel(Level level) {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLevel = level;
}

Logger::LogBuf::LogBuf(Logger& logger, Level level) 
    : logger_(logger), level_(level) {}

int Logger::LogBuf::overflow(int c) {
    if (c != EOF) {
        if (c == '\n') {
            // Log the accumulated line
            logger_.log(level_, buffer_);
            buffer_.clear();
        } else {
            buffer_ += static_cast<char>(c);
        }
    }
    return c;
}

int Logger::LogBuf::sync() {
    if (!buffer_.empty()) {
        logger_.log(level_, buffer_);
        buffer_.clear();
    }
    return 0;
}

Logger::LogStream::LogStream(Logger& logger, Level level)
    : std::ostream(&buffer_), buffer_(logger, level) {}

Logger::LogStream& Logger::getErrorStream() { 
    return errorStream_; 
}

Logger::LogStream& Logger::getInfoStream() { 
    return infoStream_; 
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

Logger::Logger() 
    : currentLevel(Level::Info)
    , errorStream_(*this, Level::Error)
    , infoStream_(*this, Level::Info) {}

const char* Logger::getLevelString(Level level) const {
    switch (level) {
        case Level::Error: return "ERROR";
        case Level::Warning: return "WARNING";
        case Level::Info: return "INFO";
        case Level::Verbose: return "VERBOSE";
        case Level::Debug: return "DEBUG";
        default: return "UNKNOWN";
    }
}

// Global functions implementation
bool initLog(const std::string& filename) {
    return Logger::getInstance().init(filename);
}

void setLogLevel(Logger::Level level) {
    Logger::getInstance().setLevel(level);
}

void redirectStdStreams() {
    std::cout.rdbuf(Logger::getInstance().getInfoStream().rdbuf());
    std::cerr.rdbuf(Logger::getInstance().getErrorStream().rdbuf());
} 