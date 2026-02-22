#pragma once

#include <chrono>
#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <vector>
#define RESET "\033[0m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define CYAN "\033[36m"
#define BOLD "\033[1m"

#include <ctime>
#include <sstream>
#include <string>

namespace zephyr {

enum class LogLevel { INFO = 0, WARNING = 1, ERROR = 2, DEBUG = 3 };

class Logger {
public:
  static Logger &get() {
    static Logger instance;
    return instance;
  }

  struct Log {
    std::string message;
    std::string timestamp;
    LogLevel level;
    const char *caller;
    std::string file;
    int line;
  };

  template <typename... Args>
  void log(LogLevel level, const char *caller, const char *file, int line,
           Args &&...args) {
    std::ostringstream log_stream;

    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm *local_time = std::localtime(&current_time);
    std::ostringstream timestamp_stream;
    timestamp_stream << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");

    std::string message = build_message(std::forward<Args>(args)...);
    std::size_t msg_hash = std::hash<std::string>{}(message);

    Log *log = nullptr;

    if (m_log_map.find(msg_hash) != m_log_map.end()) {
      log = m_log_map[msg_hash];
      log->timestamp = timestamp_stream.str();
    } else {
      log = new Log{.message = message,
                    .timestamp = timestamp_stream.str(),
                    .level = level,
                    .caller = caller,
                    .file = extract_filename(file),
                    .line = line};
      m_log_map[msg_hash] = log;
    }

    if (m_logs.size() > 100) {
      m_logs.clear();
    }

    m_logs.push_back(log);

    render_log(log);
  }

  std::vector<Log *> logs() const { return m_logs; }

  template <typename... Args> std::string build_message(Args &&...args) {
    std::ostringstream message_stream;
    ((message_stream << std::forward<Args>(args) << " "), ...);
    std::string result = message_stream.str();

    if (!result.empty()) {
      result.pop_back();
    }

    return result;
  }

private:
  std::unordered_map<std::size_t, Log *> m_log_map;

  void render_log(Log *log) {
    std::ostringstream log_stream;

    log_stream << BOLD << "[" << log->timestamp << "] :: ";

    switch (log->level) {
    case LogLevel::INFO:
      log_stream << CYAN << "[INFO] ";
      break;
    case LogLevel::WARNING:
      log_stream << YELLOW << "[WARNING] ";
      break;
    case LogLevel::ERROR:
      log_stream << RED << "[ERROR] ";
      break;
    case LogLevel::DEBUG:
      log_stream << CYAN << "[DEBUG] ";
      break;
    }

    log_stream << RESET << BOLD << "[" << log->file << "::" << log->line << "]";
    log_stream << " [" << log->caller << "]";

    log_stream << RESET << " :: " << log->message << "\n";

    std::cout << log_stream.str();
  }

private:
  std::string extract_filename(const char *file) {
    std::string file_path(file);
    size_t pos = file_path.find_last_of("/\\");
    if (pos != std::string::npos) {
      return file_path.substr(pos + 1);
    }
    return file_path;
  }

  std::vector<Log *> m_logs;
};

enum class MetricVisualization {
  SPARKLINE,
  GAUGE,
  TABLE,
  CHART,
  BAR,
  TEXT,
  AUTO
};

inline const char *metric_visualization_to_string(MetricVisualization viz) {
  switch (viz) {
  case MetricVisualization::SPARKLINE:
    return "sparkline";
  case MetricVisualization::GAUGE:
    return "gauge";
  case MetricVisualization::TABLE:
    return "table";
  case MetricVisualization::CHART:
    return "chart";
  case MetricVisualization::BAR:
    return "bar";
  case MetricVisualization::TEXT:
    return "text";
  case MetricVisualization::AUTO:
  default:
    return "auto";
  }
}

class MetricReporter {
public:
  static MetricReporter &get() {
    static MetricReporter instance;
    return instance;
  }

  template <typename T>
  void send(const std::string &category, const std::string &key, T value) {
#ifdef LOG_IGNIS_METRICS
    std::cout << "[IGNIS_METRIC] " << category << ":" << key << "=" << value
              << std::endl;
#endif
  }

  template <typename T>
  void send_typed(const std::string &category, const std::string &key, T value,
                  MetricVisualization viz) {
#ifdef LOG_IGNIS_METRICS
    std::cout << "[IGNIS_METRIC] " << category << ":" << key << "=" << value
              << ":" << metric_visualization_to_string(viz) << std::endl;
#endif
  }

private:
  MetricReporter() = default;
};

#define LOG_INFO(...)                                                          \
  Logger::get().log(LogLevel::INFO, __FUNCTION__, __FILE__, __LINE__,          \
                    __VA_ARGS__)
#define LOG_WARN(...)                                                          \
  Logger::get().log(LogLevel::WARNING, __FUNCTION__, __FILE__, __LINE__,       \
                    __VA_ARGS__)
#define LOG_ERROR(...)                                                         \
  Logger::get().log(LogLevel::ERROR, __FUNCTION__, __FILE__, __LINE__,         \
                    __VA_ARGS__)
#define LOG_DEBUG(...)                                                         \
  Logger::get().log(LogLevel::DEBUG, __FUNCTION__, __FILE__, __LINE__,         \
                    __VA_ARGS__)

#ifdef LOG_IGNIS_METRICS
#define REPORT_METRIC(category, key, value)                                    \
  MetricReporter::get().send(category, key, value)
#define REPORT_METRIC_VIZ(category, key, value, viz)                           \
  MetricReporter::get().send_typed(category, key, value, viz)
#else
#define REPORT_METRIC(category, key, value)
#define REPORT_METRIC_VIZ(category, key, value, viz)
#endif

} // namespace zephyr
