#pragma once
#include "exception.hpp"

namespace zephyr {
#define ZEPH_ASSERT(EXPRESSION, MESSAGE)                                       \
  if (EXPRESSION)                                                              \
  return BaseException(__FILE__, __LINE__, MESSAGE)

#define ZEPH_EXCEPTION(...)                                                    \
  throw BaseException(__FILE__, __FUNCTION__, __LINE__,                        \
                      build_exception_message(__VA_ARGS__));

#define ZEPH_ENSURE(EXPRESSION, ...)                                           \
  if (EXPRESSION) {                                                            \
    throw BaseException(__FILE__, __FUNCTION__, __LINE__,                      \
                        build_exception_message(__VA_ARGS__));                 \
  }

#define ZEPH_ASSERT_EITHER(EXPRESSION)                                         \
  if (EXPRESSION.isLeft())                                                     \
  return EXPRESSION.left()

#define ZEPH_ASSERT_EITHER_THROW(EXPRESSION)                                   \
  if (EXPRESSION.isLeft())                                                     \
  throw EXPRESSION.left()
} // namespace zephyr
