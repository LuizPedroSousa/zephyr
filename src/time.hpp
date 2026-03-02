#pragma once
#include "GLFW/glfw3.h"
#include <chrono>

namespace zephyr {

class Time {
public:
  static void init();

  static Time *get() { return m_instance; }

  void end() { delete m_instance; }

  void update(float time = glfwGetTime()) {
    m_current_frame = time;
    m_deltatime = m_current_frame - m_last_frame;
    m_last_frame = m_current_frame;
  };

  float deltatime() { return m_deltatime; }
  float current() { return m_current_frame; }

private:
  float m_deltatime;
  float m_current_frame;
  float m_last_frame;

  static Time *m_instance;
};

struct Timer {
  Timer() : m_start(std::chrono::high_resolution_clock::now()) {}

  float elapsed() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::chrono::seconds::period>(
               now - m_start)
        .count();
  }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

} // namespace zephyr
