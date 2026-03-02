#pragma once

#include "event-scheduler.hpp"
#include "keyboard-event.hpp"
#include "keyboard.hpp"
#include "log.hpp"
#include "mouse-event.hpp"
#include "mouse.hpp"
#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"
#include <GLFW/glfw3.h>

#include <string>

namespace zephyr {

static bool has_pressed = false;

class Window {
public:
  static Window *get() { return m_instance; }
  static void init();

  void open(std::string title, int width, int height) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_width = width;
    m_height = height;
    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    auto initial_mouse_x = static_cast<double>(width) / 2;
    auto initial_mouse_y = static_cast<double>(height) / 2;

    mouse = create_ref<input::Mouse>(initial_mouse_x, initial_mouse_y);

    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, resize_callback);
    EventDispatcher::get()
        ->attach<input::KeyboardListener, input::KeyReleasedEvent>(
            ZEPH_BIND_EVENT_FN(Window::toggle_view_mouse));

    glfwSetCursorPosCallback(m_handle, mouse_callback);
    glfwSetKeyCallback(m_handle, key_callback);
  }

  inline GLFWwindow *handle() const { return m_handle; }

  bool is_resized() const { return m_is_resized; }
  void set_is_resized(bool value) { m_is_resized = value; }

  static void toggle_view_mouse(input::KeyReleasedEvent *event) {
    if (event->key_code == input::KeyCode::Escape) {
      has_pressed = !has_pressed;

      auto window = Window::get();

      if (!has_pressed)
        window->mouse->reset_last();

      glfwSetCursorPos(window->handle(), window->width() / 2.0f,
                       window->height() / 2.0f);

      glfwSetInputMode(window->handle(), GLFW_CURSOR,
                       has_pressed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
  }

  static void resize_callback(GLFWwindow *handle, int width, int height) {
    auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(handle));

    window->m_is_resized = true;
    window->m_width = width;
    window->m_height = height;
  }

  bool is_open() { return !glfwWindowShouldClose(m_handle); }

  void update() {
    glfwPollEvents();
    keyboard.release_keys(this);

    while (m_width == 0 || m_height == 0) {
      glfwWaitEvents();
    }
  }

  void swap() {
    keyboard.destroy_release_keys();
    mouse->reset_delta();
  }

  void cleanup() {
    glfwDestroyWindow(m_handle);
    glfwTerminate();
  }

  static void key_callback(GLFWwindow *window, int key, int scancode,
                           int action, int mods) {
    auto scheduler = EventScheduler::get();

    switch (action) {
    case GLFW_PRESS: {
      auto event = input::KeyCode(key);

      auto self = static_cast<Window *>(glfwGetWindowUserPointer(window));

      auto scheduler_id = scheduler->schedule<input::KeyPressedEvent>(
          SchedulerType::POST_FRAME, event);

      self->keyboard.attach_key(
          event,
          input::Keyboard::KeyState{.event = input::Keyboard::KeyEvent::KeyDown,
                                    .scheduler_id = scheduler_id});
      break;
    }
    default:
      break;
    }
  }

  static void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (!has_pressed) {
      auto self = static_cast<Window *>(glfwGetWindowUserPointer(window));

      self->mouse->set_position(input::Mouse::Position{.x = xpos, .y = ypos});

      EventDispatcher::get()->dispatch(new MouseEvent(xpos, ypos));
    }
  }

  input::Keyboard keyboard;
  Ref<input::Mouse> mouse;

  inline constexpr uint32_t height() { return m_height; }
  inline constexpr uint32_t width() { return m_width; }

private:
  static Window *m_instance;
  GLFWwindow *m_handle;
  bool m_is_resized = false;

  uint32_t m_width;
  uint32_t m_height;
};

namespace input {

static inline bool IS_KEY_DOWN(input::KeyCode keycode) {
  return Window::get()->keyboard.is_key_down(keycode);
}
static inline bool IS_KEY_RELEASED(input::KeyCode keycode) {
  return Window::get()->keyboard.is_key_released(keycode);
}

[[nodiscard]] static inline const input::Mouse::Position MOUSE_DELTA() {
  return Window::get()->mouse->delta();
}

[[nodiscard]] static inline bool HAS_MOUSE_MOVED() {
  return Window::get()->mouse->has_moved();
}

inline void SET_MOUSE_POSITION(input::Mouse::Position &position) {
  return Window::get()->mouse->set_position(position);
}

} // namespace input

} // namespace zephyr
