#pragma once
#include "functional"
#include "listener.hpp"

namespace zephyr {

struct MouseEvent : public Event {
public:
  MouseEvent(double x, double y) : x(x), y(y) {}
  double x;
  double y;
  EVENT_TYPE(MouseMovement)
};

class MouseListener : public BaseListener {
public:
  MouseListener(const std::function<void(Event *event)> &callback)
      : m_callback(callback) {}

  void dispatch(Event *event) override { m_callback(event); }

  LISTENER_CLASS_TYPE(Mouse)

private:
  std::function<void(Event *event)> m_callback;
};

} // namespace zephyr
