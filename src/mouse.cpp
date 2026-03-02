#include "mouse.hpp"

namespace zephyr::input {

Mouse::Mouse(double initial_mouse_x, double initial_mouse_y) {
  m_last.x = initial_mouse_x;
  m_last.y = initial_mouse_y;
}

Mouse::~Mouse() {}

} // namespace zephyr::input
