#pragma once

#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"
#include <GLFW/glfw3.h>

#include <string>

namespace zephyr {

class Window {
public:
  void open(std::string title, int width, int height) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_width = width;
    m_height = height;
    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, resize_callback);
  }

  inline GLFWwindow *handle() const { return m_handle; }

  bool is_resized() const { return m_is_resized; }
  void set_is_resized(bool value) { m_is_resized = value; }

  static void resize_callback(GLFWwindow *handle, int width, int height) {
    auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(handle));

    window->m_is_resized = true;
    window->m_width = width;
    window->m_height = height;
  }

  bool is_open() { return !glfwWindowShouldClose(m_handle); }

  void update() {
    glfwPollEvents();

    while (m_width == 0 || m_height == 0) {
      glfwWaitEvents();
    }
  }

  void cleanup() {
    glfwDestroyWindow(m_handle);
    glfwTerminate();
  }

private:
  GLFWwindow *m_handle;
  bool m_is_resized = false;

  uint32_t m_width;
  uint32_t m_height;
};

} // namespace zephyr
