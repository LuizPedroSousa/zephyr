#pragma once
#pragma once
#include "event.hpp"

namespace zephyr {

enum EventType {
  KeyPressed = 0,
  KeyReleased = 1,
  MouseMovement = 2,
};

class Event {
public:
  Event() = default;
  virtual EventType get_event_type() const = 0;
};

#define EVENT_TYPE(t)                                                          \
  static EventType get_static_type() { return EventType::t; }                  \
  EventType get_event_type() const override { return get_static_type(); }

} // namespace zephyr
