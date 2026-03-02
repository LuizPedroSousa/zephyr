#pragma once

#include "event.hpp"
#include "keyboard.hpp"
#include "listener.hpp"
#include <functional>
#define BASE_FIELDS KeyCode key_code;

namespace zephyr::input {
class KeyPressedEvent : public Event {
public:
  KeyPressedEvent(KeyCode key_code) : key_code(key_code), Event() {}

  BASE_FIELDS
  EVENT_TYPE(KeyPressed)
};

class KeyReleasedEvent : public Event {
public:
  KeyReleasedEvent(KeyCode key_code) : key_code(key_code) {}

  BASE_FIELDS
  EVENT_TYPE(KeyReleased)
};

class KeyboardListener : public BaseListener {
public:
  KeyboardListener(const std::function<void(Event *)> &callback)
      : m_callback(callback) {}

  void dispatch(Event *event) override { m_callback(event); }

  LISTENER_CLASS_TYPE(Keyboard)

private:
  std::function<void(Event *)> m_callback;
};

} // namespace zephyr::input
