#include "window.hpp"

namespace zephyr {
Window *Window::m_instance = nullptr;

void Window::init() { m_instance = new Window; }
} // namespace zephyr
